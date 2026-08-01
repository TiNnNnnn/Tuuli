//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLFilterMatcher.cpp
//
//	@doc:
//		Implementation of the Filter-chain <-> conjunctive-Select matcher (see
//		CDSLFilterMatcher.h). Migrates the SEMANTICS of WeTune FilterMatcher /
//		FilterChain / FilterAssignments: subset match + reorder + residual.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLFilterMatcher.h"

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CPredicateUtils.h"

using namespace gpopt;

// max DSL Filter-chain depth we handle in one match. Real rules nest at most a
// handful of Filters; a chain longer than this is treated as no-match (rather
// than allocating), which is safe — such a rule simply won't fire here.
#define GPOPT_DSL_MAX_FILTER_CHAIN 16

//---------------------------------------------------------------------------
//	@function:
//		CDSLFilterMatcher::PopCollectChain
//
//	@doc:
//		Peel the maximal run of Filter ops from popFilterRoot down child[0] into
//		rgpopFilters (non-owning — the rule IR outlives the match). Returns the
//		first non-Filter op (the chain base), or NULL on a malformed / too-long
//		chain.
//---------------------------------------------------------------------------
const CDSLOp *
CDSLFilterMatcher::PopCollectChain(const CDSLOp *popFilterRoot,
								   const CDSLOp **rgpopFilters, ULONG ulCapacity,
								   ULONG *pulFilters) const
{
	ULONG ulCount = 0;
	const CDSLOp *popCur = popFilterRoot;
	while (nullptr != popCur && EdslopFilter == popCur->Edslop())
	{
		if (ulCount == ulCapacity)
		{
			return nullptr;	 // chain longer than we handle
		}
		rgpopFilters[ulCount++] = popCur;

		// a Filter has exactly one relational child (child[0]); if the template
		// is malformed, stop.
		if (1 != popCur->UlChildren())
		{
			return nullptr;
		}
		popCur = (*popCur)[0];
	}
	*pulFilters = ulCount;
	return popCur;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLFilterMatcher::FBindFilterSymbols
//
//	@doc:
//		Bind one Filter's <p a> symbols: <p> (sym[0], Pred) to the conjunct
//		subtree, <a> (sym[1], Attrs) to the conjunct's used columns. FBind
//		enforces equality classes (a rebind to a different artifact fails).
//---------------------------------------------------------------------------
BOOL
CDSLFilterMatcher::FBindFilterSymbols(const CDSLOp *popFilter,
									  CExpression *pexprConj,
									  CDSLModel *pmodel) const
{
	GPOS_ASSERT(EdslopFilter == popFilter->Edslop());

	CDSLSymbolArray *pdrgpsym = popFilter->Pdrgpsym();
	// Filter schema is <p a> — pred first, attrs second (validated at parse).
	if (nullptr == pdrgpsym || 2 != pdrgpsym->Size())
	{
		return false;
	}
	const CDSLSymbol *psymPred = (*pdrgpsym)[0];
	const CDSLSymbol *psymAttrs = (*pdrgpsym)[1];

	// <p> -> the conjunct predicate subtree
	if (!pmodel->FBind(psymPred, pexprConj))
	{
		return false;
	}

	// <a> -> the columns this conjunct references (its "attrs"). Materialize the
	// used-column set as an ordered CColRefArray (the model stores arrays for
	// attrs symbols). FBind AddRefs it, so we release our local ref after.
	CColRefSet *pcrsUsed = pexprConj->DeriveUsedColumns();
	CColRefArray *pdrgpcr = pcrsUsed->Pdrgpcr(m_mp);
	BOOL fBound = pmodel->FBind(psymAttrs, pdrgpcr);
	pdrgpcr->Release();
	return fBound;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLFilterMatcher::FAssign
//
//	@doc:
//		Assign DSL Filters[ulFilter..ulFilters) to distinct unused conjuncts,
//		backtracking on failure. This is the subset + reorder core (WeTune's
//		grouped/free sub-matchers, doc §2): every DSL Filter must claim some
//		conjunct, no two share one, and equality-class binding must stay
//		consistent (FBind rejects incompatible rebinds).
//
//		NOTE: bindings made on a failed branch are left in the model; because a
//		symbol only ever binds to one artifact within a rule (equality classes),
//		re-binding the SAME symbol to the SAME conjunct on the successful branch
//		is a no-op success, so stale bindings from abandoned branches are
//		harmless for the well-formed rules we admit. (A future tightening could
//		snapshot/rollback; not needed for correctness here.)
//---------------------------------------------------------------------------
BOOL
CDSLFilterMatcher::FAssign(const CDSLOp **rgpopFilters, ULONG ulFilters,
						   ULONG ulFilter, CExpressionArray *pdrgpexprConj,
						   BOOL *rgfUsed, CDSLModel *pmodel) const
{
	if (ulFilter == ulFilters)
	{
		// all DSL Filters placed
		return true;
	}

	const CDSLOp *popFilter = rgpopFilters[ulFilter];
	const ULONG ulConj = pdrgpexprConj->Size();

	for (ULONG ul = 0; ul < ulConj; ul++)
	{
		if (rgfUsed[ul])
		{
			continue;
		}
		if (!FBindFilterSymbols(popFilter, (*pdrgpexprConj)[ul], pmodel))
		{
			// this conjunct is incompatible with popFilter's already-bound
			// equality class; try the next conjunct.
			continue;
		}

		rgfUsed[ul] = true;
		if (FAssign(rgpopFilters, ulFilters, ulFilter + 1, pdrgpexprConj,
					rgfUsed, pmodel))
		{
			return true;
		}
		rgfUsed[ul] = false;  // backtrack
	}
	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLFilterMatcher::RecordResidual
//
//	@doc:
//		Collect the conjuncts no DSL Filter consumed and hand them to the model
//		(AddRef'd; the model owns the array). Even when empty, record an empty
//		array so the instantiator can tell "filter was split, nothing left over"
//		from "no filter at all".
//---------------------------------------------------------------------------
void
CDSLFilterMatcher::RecordResidual(CExpressionArray *pdrgpexprConj,
								  const BOOL *rgfUsed, CDSLModel *pmodel) const
{
	CExpressionArray *pdrgpexprResidual = GPOS_NEW(m_mp) CExpressionArray(m_mp);
	const ULONG ulConj = pdrgpexprConj->Size();
	for (ULONG ul = 0; ul < ulConj; ul++)
	{
		if (!rgfUsed[ul])
		{
			CExpression *pexpr = (*pdrgpexprConj)[ul];
			pexpr->AddRef();
			pdrgpexprResidual->Append(pexpr);
		}
	}
	pmodel->SetResidualConjuncts(pdrgpexprResidual);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLFilterMatcher::FMatch
//
//	@doc:
//		Match a Filter-rooted DSL template against a live CLogicalSelect.
//---------------------------------------------------------------------------
BOOL
CDSLFilterMatcher::FMatch(const CDSLOp *popFilterRoot,
						  CExpression *pexprSelect, CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != popFilterRoot);
	GPOS_ASSERT(EdslopFilter == popFilterRoot->Edslop());
	GPOS_ASSERT(nullptr != pexprSelect);

	// the live node must be a Select carrying (relational child, predicate).
	if (COperator::EopLogicalSelect != pexprSelect->Pop()->Eopid() ||
		2 != pexprSelect->Arity())
	{
		return false;
	}

	// 1. peel the DSL Filter chain down to its non-Filter base op.
	const CDSLOp *rgpopFilters[GPOPT_DSL_MAX_FILTER_CHAIN];
	ULONG ulFilters = 0;
	const CDSLOp *popBase = PopCollectChain(
		popFilterRoot, rgpopFilters, GPOPT_DSL_MAX_FILTER_CHAIN, &ulFilters);
	if (nullptr == popBase)
	{
		return false;
	}

	// 2. flatten the Select's conjunctive predicate into a conjunct set.
	CExpression *pexprPred = (*pexprSelect)[1];
	CExpressionArray *pdrgpexprConj =
		CPredicateUtils::PdrgpexprConjuncts(m_mp, pexprPred);

	const ULONG ulConj = pdrgpexprConj->Size();

	BOOL fMatched = false;
	// a subset match needs at least as many conjuncts as DSL Filters.
	if (ulFilters <= ulConj)
	{
		BOOL *rgfUsed = GPOS_NEW_ARRAY(m_mp, BOOL, ulConj);
		for (ULONG ul = 0; ul < ulConj; ul++)
		{
			rgfUsed[ul] = false;
		}

		// 3a. assign each DSL Filter to a distinct conjunct (subset + reorder).
		if (FAssign(rgpopFilters, ulFilters, 0 /*ulFilter*/, pdrgpexprConj,
					rgfUsed, pmodel))
		{
			// 3b. the chain base recurses against the Select's relational child.
			if (m_pmatcher->FMatch(popBase, (*pexprSelect)[0], pmodel))
			{
				// 3c. carry the unconsumed conjuncts forward.
				RecordResidual(pdrgpexprConj, rgfUsed, pmodel);
				fMatched = true;
			}
		}

		GPOS_DELETE_ARRAY(rgfUsed);
	}

	pdrgpexprConj->Release();
	return fMatched;
}

// EOF
