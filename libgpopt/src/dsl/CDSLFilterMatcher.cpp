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
#include "gpopt/operators/CLogicalInnerJoin.h"
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
//		Assign DSL Filters[ulFilter..ulFilters) to compatible conjuncts,
//		preferring distinct conjuncts before normalized duplicate reuse and
//		backtracking on failure. Search is deliberately side-effect free: writing
//		bindings while exploring poisoned later branches because CDSLModel has no
//		rollback operation. Once a complete compatible assignment is found,
//		FMatch commits every binding exactly once.
//---------------------------------------------------------------------------
namespace
{
BOOL
FDirectEquality(const CDSLRule *prule, EDslConstraintKind edslcon,
				const CDSLSymbol *psymFirst, const CDSLSymbol *psymSecond)
{
	if (psymFirst == psymSecond)
	{
		return true;
	}
	if (nullptr == prule)
	{
		return false;
	}
	CDSLConstraintArray *pdrgpcon = prule->Pdrgpcon();
	for (ULONG ul = 0; ul < pdrgpcon->Size(); ul++)
	{
		const CDSLConstraint *pcon = (*pdrgpcon)[ul];
		if (edslcon != pcon->Edslcon())
		{
			continue;
		}
		CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
		if (2 == pdrgpsym->Size() &&
			((psymFirst == (*pdrgpsym)[0] && psymSecond == (*pdrgpsym)[1]) ||
			 (psymFirst == (*pdrgpsym)[1] && psymSecond == (*pdrgpsym)[0])))
		{
			return true;
		}
	}
	return false;
}

BOOL
FUsedColumnsEqual(CExpression *pexprFirst, CExpression *pexprSecond)
{
	CColRefSet *pcrsFirst = pexprFirst->DeriveUsedColumns();
	CColRefSet *pcrsSecond = pexprSecond->DeriveUsedColumns();
	return pcrsFirst->Size() == pcrsSecond->Size() &&
		   pcrsFirst->ContainsAll(pcrsSecond);
}
}  // namespace

//---------------------------------------------------------------------------
//	@function:
//		CDSLFilterMatcher::FMatchPushedDownInnerJoin
//
//	@doc:
//		ORCA pushes a predicate that references one inner-join input below the
//		join before exploration. Reconstruct a temporary, equivalent
//		Select(InnerJoin) view so an ordinary Filter-rooted DSL rule can still be
//		matched. The selected input's Select is peeled as a whole; unconsumed
//		conjuncts become ordinary Filter residuals and are restored by the target
//		instantiator. This is safe only for inner joins and predicates whose used
//		columns are produced entirely by that input.
//---------------------------------------------------------------------------
BOOL
CDSLFilterMatcher::FMatchPushedDownInnerJoin(
	const CDSLOp *popFilterRoot, CExpression *pexprJoin,
	CDSLModel *pmodel) const
{
	if (nullptr == m_prule ||
		COperator::EopLogicalInnerJoin != pexprJoin->Pop()->Eopid() ||
		3 != pexprJoin->Arity())
	{
		return false;
	}

	const CDSLOp *rgpopFilters[GPOPT_DSL_MAX_FILTER_CHAIN];
	ULONG ulFilters = 0;
	const CDSLOp *popBase = PopCollectChain(
		popFilterRoot, rgpopFilters, GPOPT_DSL_MAX_FILTER_CHAIN, &ulFilters);
	if (nullptr == popBase || EdslopInnerJoin != popBase->Edslop() ||
		2 != popBase->UlChildren() || nullptr == popBase->Pdrgpsym() ||
		2 != popBase->Pdrgpsym()->Size())
	{
		return false;
	}

	// An explicit source AttrsEq connects every pulled Filter's referenced attrs
	// to the join-key side from which it may have been pushed. This avoids
	// guessing from rule ids or SQL text and rejects unconstrained movement.
	BOOL rgfEligible[2] = {true, true};
	for (ULONG ulSide = 0; ulSide < 2; ulSide++)
	{
		const CDSLSymbol *psymJoinAttrs = (*popBase->Pdrgpsym())[ulSide];
		for (ULONG ulFilter = 0; ulFilter < ulFilters; ulFilter++)
		{
			CDSLSymbolArray *pdrgpsymFilter =
				rgpopFilters[ulFilter]->Pdrgpsym();
			if (nullptr == pdrgpsymFilter || 2 != pdrgpsymFilter->Size() ||
				!FDirectEquality(m_prule, EdslconAttrsEq,
							 (*pdrgpsymFilter)[1], psymJoinAttrs))
			{
				rgfEligible[ulSide] = false;
				break;
			}
		}
	}

	ULONG ulSelected = 2;
	for (ULONG ulSide = 0; ulSide < 2; ulSide++)
	{
		CExpression *pexprInput = (*pexprJoin)[ulSide];
		if (!rgfEligible[ulSide] ||
			COperator::EopLogicalSelect != pexprInput->Pop()->Eopid() ||
			2 != pexprInput->Arity() ||
			!(*pexprInput)[0]->DeriveOutputColumns()->ContainsAll(
				(*pexprInput)[1]->DeriveUsedColumns()))
		{
			continue;
		}
		ulSelected = ulSide;
		break;
	}
	if (2 == ulSelected)
	{
		return false;
	}

	CExpression *pexprPushedSelect = (*pexprJoin)[ulSelected];
	CExpression *pexprLeft =
		(0 == ulSelected) ? (*pexprPushedSelect)[0] : (*pexprJoin)[0];
	CExpression *pexprRight =
		(1 == ulSelected) ? (*pexprPushedSelect)[0] : (*pexprJoin)[1];
	pexprJoin->Pop()->AddRef();
	pexprLeft->AddRef();
	pexprRight->AddRef();
	(*pexprJoin)[2]->AddRef();
	CExpression *pexprVirtualJoin = GPOS_NEW(m_mp) CExpression(
		m_mp, pexprJoin->Pop(), pexprLeft, pexprRight, (*pexprJoin)[2]);

	(*pexprPushedSelect)[1]->AddRef();
	CExpression *pexprVirtualSelect = GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprVirtualJoin,
		(*pexprPushedSelect)[1]);
	BOOL fMatched = FMatch(popFilterRoot, pexprVirtualSelect, pmodel);
	pexprVirtualSelect->Release();
	return fMatched;
}

BOOL
CDSLFilterMatcher::FAssignmentCompatible(
	const CDSLOp **rgpopFilters, ULONG ulFilter,
	CExpressionArray *pdrgpexprConj, const ULONG *rgulAssigned,
	CExpression *pexprCandidate) const
{
	CDSLSymbolArray *pdrgpsymCandidate =
		rgpopFilters[ulFilter]->Pdrgpsym();
	GPOS_ASSERT(nullptr != pdrgpsymCandidate &&
				2 == pdrgpsymCandidate->Size());
	const CDSLSymbol *psymPredCandidate = (*pdrgpsymCandidate)[0];
	const CDSLSymbol *psymAttrsCandidate = (*pdrgpsymCandidate)[1];

	for (ULONG ulPrevious = 0; ulPrevious < ulFilter; ulPrevious++)
	{
		CDSLSymbolArray *pdrgpsymPrevious =
			rgpopFilters[ulPrevious]->Pdrgpsym();
		GPOS_ASSERT(nullptr != pdrgpsymPrevious &&
					2 == pdrgpsymPrevious->Size());
		CExpression *pexprPrevious =
			(*pdrgpexprConj)[rgulAssigned[ulPrevious]];

		if (FDirectEquality(m_prule, EdslconAttrsEq, psymAttrsCandidate,
						(*pdrgpsymPrevious)[1]) &&
			!FUsedColumnsEqual(pexprCandidate, pexprPrevious))
		{
			return false;
		}
		if (FDirectEquality(m_prule, EdslconPredicateEq,
						psymPredCandidate, (*pdrgpsymPrevious)[0]) &&
			!pexprCandidate->Matches(pexprPrevious))
		{
			return false;
		}
	}
	return true;
}

BOOL
CDSLFilterMatcher::FAssign(const CDSLOp **rgpopFilters, ULONG ulFilters,
						   ULONG ulFilter, CExpressionArray *pdrgpexprConj,
						   BOOL *rgfUsed, ULONG *rgulAssigned) const
{
	if (ulFilter == ulFilters)
	{
		// all DSL Filters placed
		return true;
	}

	const ULONG ulConj = pdrgpexprConj->Size();

	// Prefer a structural 1:1 assignment. If none works, allow reuse: ORCA's
	// predicate normalizer removes duplicate AND children before xforms run, so
	// Filter(p, Filter(p, child)) has the canonical view Select(child, p). DSL
	// variables are not required to bind different values, making this a valid
	// specialization of the source pattern rather than a weakened constraint.
	for (ULONG ulPass = 0; ulPass < 2; ulPass++)
	{
		for (ULONG ul = 0; ul < ulConj; ul++)
		{
			const BOOL fAlreadyUsed = rgfUsed[ul];
			if ((0 == ulPass && fAlreadyUsed) ||
				(1 == ulPass && !fAlreadyUsed))
			{
				continue;
			}
			CExpression *pexprCandidate = (*pdrgpexprConj)[ul];
			if (!FAssignmentCompatible(rgpopFilters, ulFilter, pdrgpexprConj,
								   rgulAssigned, pexprCandidate))
			{
				continue;
			}

			rgfUsed[ul] = true;
			rgulAssigned[ulFilter] = ul;
			if (FAssign(rgpopFilters, ulFilters, ulFilter + 1,
						pdrgpexprConj, rgfUsed, rgulAssigned))
			{
				return true;
			}
			// A conjunct selected by an earlier Filter remains consumed when this
			// branch releases its virtual duplicate assignment.
			rgfUsed[ul] = fAlreadyUsed;
		}
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

	// A Filter above an inner join may already have been pushed into one input.
	// Expose a temporary pre-pushdown view before applying the ordinary Select
	// matcher below.
	if (COperator::EopLogicalInnerJoin == pexprSelect->Pop()->Eopid())
	{
		return FMatchPushedDownInnerJoin(popFilterRoot, pexprSelect, pmodel);
	}

	// Otherwise the live node must be a Select carrying (relational child,
	// predicate).
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
	// A shorter normalized conjunction may represent a longer Filter chain when
	// duplicate predicates were eliminated before xform exploration.
	if (0 < ulConj)
	{
		BOOL *rgfUsed = GPOS_NEW_ARRAY(m_mp, BOOL, ulConj);
		ULONG *rgulAssigned = GPOS_NEW_ARRAY(m_mp, ULONG, ulFilters);
		for (ULONG ul = 0; ul < ulConj; ul++)
		{
			rgfUsed[ul] = false;
		}

		// 3a. assign each DSL Filter to a distinct conjunct (subset + reorder).
		if (FAssign(rgpopFilters, ulFilters, 0 /*ulFilter*/, pdrgpexprConj,
					rgfUsed, rgulAssigned))
		{
			BOOL fBound = true;
			for (ULONG ul = 0; fBound && ul < ulFilters; ul++)
			{
				fBound = FBindFilterSymbols(
					rgpopFilters[ul], (*pdrgpexprConj)[rgulAssigned[ul]],
					pmodel);
			}
			// 3b. the chain base recurses against the Select's relational child.
			if (fBound &&
				m_pmatcher->FMatch(popBase, (*pexprSelect)[0], pmodel))
			{
				// 3c. carry the unconsumed conjuncts forward.
				RecordResidual(pdrgpexprConj, rgfUsed, pmodel);
				fMatched = true;
			}
		}

		GPOS_DELETE_ARRAY(rgulAssigned);
		GPOS_DELETE_ARRAY(rgfUsed);
	}

	pdrgpexprConj->Release();
	return fMatched;
}

// EOF
