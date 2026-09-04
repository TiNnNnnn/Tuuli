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
#include "gpopt/dsl/CDSLExpressionDefinitions.h"

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalLeftSemiApplyIn.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarIdent.h"

using namespace gpopt;

// max DSL Filter-chain depth we handle in one match. Real rules nest at most a
// handful of Filters; a chain longer than this is treated as no-match (rather
// than allocating), which is safe — such a rule simply won't fire here.
#define GPOPT_DSL_MAX_FILTER_CHAIN 16

namespace
{
CColRefArray *PdrgpcrFilterDependencies(CMemoryPool *mp,
										const CDSLOp *popFilter,
										CExpression *pexprPredicate,
										CExpression *pexprBase,
										ULONG ulSymbol);

// ORCA may retain adjacent Select nodes in separate Memo groups or normalize
// them into one AND predicate. Present both physical forms as one Filter chain
// to the DSL matcher and return the relational child left after consuming the
// requested Filter placeholders. A constraint that explicitly partitions a
// complete predicate domain may request the whole adjacent Select chain.
CExpression *
PexprCollectSelectChain(CMemoryPool *mp, CExpression *pexprSelect,
						ULONG ulRequiredFilters, BOOL fCollectAll,
						CExpressionArray *pdrgpexprConj)
{
	CExpression *pexprCurrent = pexprSelect;
	while (COperator::EopLogicalSelect == pexprCurrent->Pop()->Eopid() &&
		   2 == pexprCurrent->Arity())
	{
		CExpressionArray *pdrgpexprLevel =
			CPredicateUtils::PdrgpexprConjuncts(mp, (*pexprCurrent)[1]);
		CUtils::AddRefAppend(pdrgpexprConj, pdrgpexprLevel);
		pdrgpexprLevel->Release();
		pexprCurrent = (*pexprCurrent)[0];
		// A source rule may consume only the Filter nodes it declares. Descend
		// into another physical Select only when the conjuncts collected so far
		// cannot bind the remaining DSL Filter placeholders. This keeps a
		// one-Filter rule from silently absorbing an adjacent inner Filter.
		if (!fCollectAll && pdrgpexprConj->Size() >= ulRequiredFilters)
		{
			break;
		}
	}
	return pexprCurrent;
}
}

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
									  CExpression *pexprBase,
									  CDSLModel *pmodel) const
{
	GPOS_ASSERT(EdslopFilter == popFilter->Edslop());

	CDSLSymbolArray *pdrgpsym = popFilter->Pdrgpsym();
	// Filter schema is <p local> or <p local outer>.
	if (nullptr == pdrgpsym ||
		(2 != pdrgpsym->Size() && 3 != pdrgpsym->Size()) ||
		nullptr == pexprBase)
	{
		return false;
	}
	const CDSLSymbol *psymPred = (*pdrgpsym)[0];

	// <p> -> the conjunct predicate subtree
	if (!pmodel->FBind(psymPred, pexprConj))
	{
		return false;
	}

	// Materialize dependency vectors with the same partitioning helper used by
	// rollback-free candidate selection below. FBind AddRefs arrays, so release
	// each local reference after binding.
	if (2 == pdrgpsym->Size())
	{
		CColRefArray *pdrgpcrDeps = PdrgpcrFilterDependencies(
			m_mp, popFilter, pexprConj, pexprBase, 1);
		BOOL fBound = pmodel->FBind((*pdrgpsym)[1], pdrgpcrDeps);
		pdrgpcrDeps->Release();
		return fBound;
	}

	CColRefArray *pdrgpcrLocal = PdrgpcrFilterDependencies(
		m_mp, popFilter, pexprConj, pexprBase, 1);
	CColRefArray *pdrgpcrOuter = PdrgpcrFilterDependencies(
		m_mp, popFilter, pexprConj, pexprBase, 2);
	BOOL fBound = pmodel->FBind((*pdrgpsym)[1], pdrgpcrLocal) &&
		pmodel->FBind((*pdrgpsym)[2], pdrgpcrOuter);
	pdrgpcrLocal->Release();
	pdrgpcrOuter->Release();
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

// PredicateDomainSplit reasons about one complete source predicate. When that
// predicate is defined by PredicateAnd, a Filter carrying either operand must
// consume adjacent physical Select wrappers: those wrappers are normalization
// artifacts of the same logical conjunction, not independent DSL boundaries.
// This follows symbol definitions rather than a rule or operator identity.
BOOL
FRequiresCompleteSelectChain(const CDSLRule *prule, const CDSLOp *popFilter)
{
	if (nullptr == prule || nullptr == popFilter ||
		nullptr == popFilter->Pdrgpsym() ||
		3 != popFilter->Pdrgpsym()->Size())
	{
		return false;
	}

	const CDSLSymbol *psymPredicate = (*popFilter->Pdrgpsym())[0];
	CDSLConstraintArray *pdrgpcon = prule->Pdrgpcon();
	for (ULONG ul = 0; ul < pdrgpcon->Size(); ul++)
	{
		const CDSLConstraint *pcon = (*pdrgpcon)[ul];
		if (EdslconPredicateDomainSplit != pcon->Edslcon())
		{
			continue;
		}
		CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
		if (9 != pdrgpsym->Size())
		{
			continue;
		}
		const CDSLSymbol *psymComplete = (*pdrgpsym)[0];
		if (prule->Pexprdefs()->FUses(psymComplete, psymPredicate))
		{
			return true;
		}
	}
	return false;
}

BOOL
FUsedColumnsEqual(CMemoryPool *mp, CExpression *pexpr,
				  CColRefArray *pdrgpcr)
{
	CColRefSet *pcrsUsed = pexpr->DeriveUsedColumns();
	CColRefSet *pcrsExpected = GPOS_NEW(mp) CColRefSet(mp);
	pcrsExpected->Include(pdrgpcr);
	BOOL fEqual = pcrsUsed->Size() == pcrsExpected->Size() &&
				  pcrsUsed->ContainsAll(pcrsExpected);
	pcrsExpected->Release();
	return fEqual;
}

// Materialize one Filter dependency vector using the same definition during
// candidate selection and final binding. The legacy two-symbol form has one
// vector containing every used column. The extended form partitions that set
// by whether the column is produced by the Filter's relational child.
CColRefArray *
PdrgpcrFilterDependencies(CMemoryPool *mp, const CDSLOp *popFilter,
						  CExpression *pexprPredicate,
						  CExpression *pexprBase, ULONG ulSymbol)
{
	GPOS_ASSERT(nullptr != popFilter && EdslopFilter == popFilter->Edslop());
	GPOS_ASSERT(nullptr != pexprPredicate && nullptr != pexprBase);
	CDSLSymbolArray *pdrgpsym = popFilter->Pdrgpsym();
	GPOS_ASSERT(nullptr != pdrgpsym &&
				(2 == pdrgpsym->Size() || 3 == pdrgpsym->Size()));
	GPOS_ASSERT(0 < ulSymbol && ulSymbol < pdrgpsym->Size());

	CColRefArray *pdrgpcrUsed =
		pexprPredicate->DeriveUsedColumns()->Pdrgpcr(mp);
	if (2 == pdrgpsym->Size())
	{
		return pdrgpcrUsed;
	}

	CColRefSet *pcrsLocal = pexprBase->DeriveOutputColumns();
	CColRefArray *pdrgpcrDeps = GPOS_NEW(mp) CColRefArray(mp);
	for (ULONG ul = 0; ul < pdrgpcrUsed->Size(); ul++)
	{
		CColRef *pcr = (*pdrgpcrUsed)[ul];
		const BOOL fLocal = pcrsLocal->FMember(pcr);
		if ((1 == ulSymbol && fLocal) || (2 == ulSymbol && !fLocal))
		{
			pdrgpcrDeps->Append(pcr);
		}
	}
	pdrgpcrUsed->Release();
	return pdrgpcrDeps;
}

void
ExtractJoinKeys(CMemoryPool *mp, CExpression *pexprJoin,
				CColRefArray *pdrgpcrLeft, CColRefArray *pdrgpcrRight)
{
	CColRefSet *pcrsLeft = (*pexprJoin)[0]->DeriveOutputColumns();
	CExpressionArray *pdrgpexprConj =
		CPredicateUtils::PdrgpexprConjuncts(mp, (*pexprJoin)[2]);
	for (ULONG ul = 0; ul < pdrgpexprConj->Size(); ul++)
	{
		CExpression *pexprConj = (*pdrgpexprConj)[ul];
		if (2 != pexprConj->Arity() ||
			!CPredicateUtils::FPlainEquality(pexprConj))
		{
			continue;
		}

		CColRef *pcrFirst = const_cast<CColRef *>(
			CScalarIdent::PopConvert((*pexprConj)[0]->Pop())->Pcr());
		CColRef *pcrSecond = const_cast<CColRef *>(
			CScalarIdent::PopConvert((*pexprConj)[1]->Pop())->Pcr());
		BOOL fFirstLeft = pcrsLeft->FMember(pcrFirst);
		BOOL fSecondLeft = pcrsLeft->FMember(pcrSecond);
		if (fFirstLeft && !fSecondLeft)
		{
			pdrgpcrLeft->Append(pcrFirst);
			pdrgpcrRight->Append(pcrSecond);
		}
		else if (fSecondLeft && !fFirstLeft)
		{
			pdrgpcrLeft->Append(pcrSecond);
			pdrgpcrRight->Append(pcrFirst);
		}
	}
	pdrgpexprConj->Release();
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

	CDSLMatchView::SJoinSpineRouteArray *pdrgproute =
		CDSLMatchView::PdrgprouteJoinSpine(
			m_mp, pexprJoin, COperator::EopLogicalSelect);
	BOOL fMatched = false;
	CColRefArray *pdrgpcrLeft = GPOS_NEW(m_mp) CColRefArray(m_mp);
	CColRefArray *pdrgpcrRight = GPOS_NEW(m_mp) CColRefArray(m_mp);
	ExtractJoinKeys(m_mp, pexprJoin, pdrgpcrLeft, pdrgpcrRight);
	for (ULONG ulRoute = 0; ulRoute < pdrgproute->Size() && !fMatched;
		 ulRoute++)
	{
		CDSLMatchView::SJoinSpineRoute *proute = (*pdrgproute)[ulRoute];
		CExpression *pexprPushedSelect = proute->m_pexprCarrier;
		if (2 != pexprPushedSelect->Arity() ||
			!(*pexprPushedSelect)[0]->DeriveOutputColumns()->ContainsAll(
				(*pexprPushedSelect)[1]->DeriveUsedColumns()))
		{
			continue;
		}

		// Cheap rule-guided prefilter: only a conjunct using the source-declared
		// root join-key side can have been pushed from Filter(InnerJoin). This
		// prevents deep join trees from invoking the full matcher for every Select.
		BOOL fEligible = false;
		CExpressionArray *pdrgpexprConj = CPredicateUtils::PdrgpexprConjuncts(
			m_mp, (*pexprPushedSelect)[1]);
		for (ULONG ulConj = 0; ulConj < pdrgpexprConj->Size() && !fEligible;
			 ulConj++)
		{
			CExpression *pexprConj = (*pdrgpexprConj)[ulConj];
			for (ULONG ulSide = 0; ulSide < 2 && !fEligible; ulSide++)
			{
				const CDSLSymbol *psymJoinAttrs =
					(*popBase->Pdrgpsym())[ulSide];
				CColRefArray *pdrgpcrSide =
					(0 == ulSide) ? pdrgpcrLeft : pdrgpcrRight;
				BOOL fLinked = true;
				for (ULONG ulFilter = 0; ulFilter < ulFilters; ulFilter++)
				{
					CDSLSymbolArray *pdrgpsymFilter =
						rgpopFilters[ulFilter]->Pdrgpsym();
					fLinked = fLinked && nullptr != pdrgpsymFilter &&
						2 == pdrgpsymFilter->Size() &&
						FDirectEquality(m_prule, EdslconAttrsEq,
									(*pdrgpsymFilter)[1], psymJoinAttrs);
				}
				fEligible = fLinked && 0 < pdrgpcrSide->Size() &&
					FUsedColumnsEqual(m_mp, pexprConj, pdrgpcrSide);
			}
		}
		pdrgpexprConj->Release();
		if (!fEligible)
		{
			continue;
		}

		proute->m_pexprRel->AddRef();
		(*pexprPushedSelect)[1]->AddRef();
		CExpression *pexprVirtualSelect = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp),
			proute->m_pexprRel, (*pexprPushedSelect)[1]);

		// Probe in a disposable model. Rejected routes must not leave partial
		// symbol bindings in the caller's model.
		CDSLModel *pmodelProbe = GPOS_NEW(m_mp) CDSLModel(m_mp);
		fMatched = FMatch(popFilterRoot, pexprVirtualSelect, pmodelProbe);
		if (fMatched && nullptr != m_prule)
		{
			CDSLConstraintChecker checker(m_mp);
			fMatched = checker.FCheck(m_prule, pmodelProbe);
		}
		pmodelProbe->Release();
		if (fMatched)
		{
			fMatched = FMatch(popFilterRoot, pexprVirtualSelect, pmodel);
		}
		pexprVirtualSelect->Release();
	}
	pdrgpcrRight->Release();
	pdrgpcrLeft->Release();
	pdrgproute->Release();
	return fMatched;
}

BOOL
CDSLFilterMatcher::FMatchSubqueryCarrier(
	const CDSLOp *popFilterRoot, CExpression *pexprCarrier,
	CDSLModel *pmodel) const
{
	const COperator::EOperatorId eopid = pexprCarrier->Pop()->Eopid();
	if ((COperator::EopLogicalLeftSemiApplyIn != eopid &&
		 COperator::EopLogicalLeftSemiJoin != eopid) ||
		3 != pexprCarrier->Arity() ||
		CUtils::HasOuterRefs((*pexprCarrier)[1]))
	{
		return false;
	}

	const CDSLOp *rgpopFilters[GPOPT_DSL_MAX_FILTER_CHAIN];
	ULONG ulFilters = 0;
	const CDSLOp *popBase = PopCollectChain(
		popFilterRoot, rgpopFilters, GPOPT_DSL_MAX_FILTER_CHAIN, &ulFilters);
	if (1 != ulFilters || nullptr == popBase ||
		nullptr == rgpopFilters[0]->Pdrgpsym() ||
		2 != rgpopFilters[0]->Pdrgpsym()->Size())
	{
		return false;
	}

	CExpression *pexprPred = (*pexprCarrier)[2];
	CColRefSet *pcrsOuterUsed =
		GPOS_NEW(m_mp) CColRefSet(m_mp, *pexprPred->DeriveUsedColumns());
	pcrsOuterUsed->Exclude((*pexprCarrier)[1]->DeriveOutputColumns());
	CColRefArray *pdrgpcrOuter = pcrsOuterUsed->Pdrgpcr(m_mp);
	pcrsOuterUsed->Release();
	if (1 != pdrgpcrOuter->Size())
	{
		pdrgpcrOuter->Release();
		return false;
	}

	CDSLSymbolArray *pdrgpsym = rgpopFilters[0]->Pdrgpsym();
	const CDSLSymbol *psymPred = (*pdrgpsym)[0];
	BOOL fMatched = pmodel->FBind(psymPred, pexprPred) &&
		pmodel->FBind((*pdrgpsym)[1], pdrgpcrOuter) &&
		m_pmatcher->FMatch(popBase, (*pexprCarrier)[0], pmodel);
	pdrgpcrOuter->Release();
	if (!fMatched)
	{
		return false;
	}

	pexprCarrier->AddRef();
	return pmodel->FSetFilterCarrier(psymPred, pexprCarrier);
}

BOOL
CDSLFilterMatcher::FBaseAssignmentCompatible(
	const CDSLOp *popFilter, const CDSLOp *popBase,
	CExpression *pexprBase, CExpression *pexprCandidate) const
{
	if (nullptr == m_prule || nullptr == popBase || nullptr == pexprBase ||
		(EdslopInnerJoin != popBase->Edslop() &&
		 EdslopLeftJoin != popBase->Edslop()) ||
		3 != pexprBase->Arity() || nullptr == popBase->Pdrgpsym() ||
		2 != popBase->Pdrgpsym()->Size() || nullptr == popFilter->Pdrgpsym() ||
		(2 != popFilter->Pdrgpsym()->Size() &&
		 3 != popFilter->Pdrgpsym()->Size()))
	{
		return true;
	}

	const COperator::EOperatorId eopidExpected =
		EdslopInnerJoin == popBase->Edslop()
			? COperator::EopLogicalInnerJoin
			: COperator::EopLogicalLeftOuterJoin;
	if (eopidExpected != pexprBase->Pop()->Eopid())
	{
		return true;
	}

	CColRefArray *pdrgpcrLeft = GPOS_NEW(m_mp) CColRefArray(m_mp);
	CColRefArray *pdrgpcrRight = GPOS_NEW(m_mp) CColRefArray(m_mp);
	ExtractJoinKeys(m_mp, pexprBase, pdrgpcrLeft, pdrgpcrRight);
	BOOL fLinked = false;
	BOOL fCompatible = false;
	CDSLSymbolArray *pdrgpsymFilter = popFilter->Pdrgpsym();
	for (ULONG ulSymbol = 1; ulSymbol < pdrgpsymFilter->Size(); ulSymbol++)
	{
		const CDSLSymbol *psymFilterAttrs = (*pdrgpsymFilter)[ulSymbol];
		const BOOL fLinkedLeft = FDirectEquality(
			m_prule, EdslconAttrsEq, psymFilterAttrs,
			(*popBase->Pdrgpsym())[0]);
		const BOOL fLinkedRight = FDirectEquality(
			m_prule, EdslconAttrsEq, psymFilterAttrs,
			(*popBase->Pdrgpsym())[1]);
		if (!fLinkedLeft && !fLinkedRight)
		{
			continue;
		}

		fLinked = true;
		CColRefArray *pdrgpcrDeps = PdrgpcrFilterDependencies(
			m_mp, popFilter, pexprCandidate, pexprBase, ulSymbol);
		fCompatible =
			(fLinkedLeft && 0 < pdrgpcrLeft->Size() &&
			 CColRef::Equals(pdrgpcrDeps, pdrgpcrLeft)) ||
			(fLinkedRight && 0 < pdrgpcrRight->Size() &&
			 CColRef::Equals(pdrgpcrDeps, pdrgpcrRight));
		// A BottomUp RBO sees only the translator's inner-join orientation. The
		// actual candidate can therefore use the opposite member of the same join
		// equality while a commuted match view binds the source template side named
		// by AttrsEq. This is safe only for InnerJoin; outer-join sides are not
		// interchangeable.
		if (!fCompatible && COperator::EopLogicalInnerJoin == eopidExpected)
		{
			fCompatible =
				(fLinkedLeft && 0 < pdrgpcrRight->Size() &&
				 CColRef::Equals(pdrgpcrDeps, pdrgpcrRight)) ||
				(fLinkedRight && 0 < pdrgpcrLeft->Size() &&
				 CColRef::Equals(pdrgpcrDeps, pdrgpcrLeft));
		}
		pdrgpcrDeps->Release();
		if (!fCompatible)
		{
			break;
		}
	}
	pdrgpcrRight->Release();
	pdrgpcrLeft->Release();
	return !fLinked || fCompatible;
}

BOOL
CDSLFilterMatcher::FAssignmentCompatible(
	const CDSLOp **rgpopFilters, ULONG ulFilter,
	CExpressionArray *pdrgpexprConj, const ULONG *rgulAssigned,
	const CDSLOp *popBase, CExpression *pexprBase,
	CExpression *pexprCandidate) const
{
	CDSLSymbolArray *pdrgpsymCandidate =
		rgpopFilters[ulFilter]->Pdrgpsym();
	GPOS_ASSERT(nullptr != pdrgpsymCandidate &&
				(2 == pdrgpsymCandidate->Size() ||
				 3 == pdrgpsymCandidate->Size()));
	const CDSLSymbol *psymPredCandidate = (*pdrgpsymCandidate)[0];

	for (ULONG ulPrevious = 0; ulPrevious < ulFilter; ulPrevious++)
	{
		CDSLSymbolArray *pdrgpsymPrevious =
			rgpopFilters[ulPrevious]->Pdrgpsym();
		GPOS_ASSERT(nullptr != pdrgpsymPrevious &&
					(2 == pdrgpsymPrevious->Size() ||
					 3 == pdrgpsymPrevious->Size()));
		CExpression *pexprPrevious =
			(*pdrgpexprConj)[rgulAssigned[ulPrevious]];

		// Compare the exact dependency partitions selected by AttrsEq. This must
		// mirror final binding because the model has no rollback after a conjunct
		// assignment is committed.
		for (ULONG ulCandidate = 1; ulCandidate < pdrgpsymCandidate->Size();
			 ulCandidate++)
		{
			for (ULONG ulPreviousSym = 1;
				 ulPreviousSym < pdrgpsymPrevious->Size(); ulPreviousSym++)
			{
				if (FDirectEquality(
						m_prule, EdslconAttrsEq,
						(*pdrgpsymCandidate)[ulCandidate],
						(*pdrgpsymPrevious)[ulPreviousSym]))
				{
					CColRefArray *pdrgpcrCandidate =
						PdrgpcrFilterDependencies(
							m_mp, rgpopFilters[ulFilter], pexprCandidate,
							pexprBase, ulCandidate);
					CColRefArray *pdrgpcrPrevious =
						PdrgpcrFilterDependencies(
							m_mp, rgpopFilters[ulPrevious], pexprPrevious,
							pexprBase, ulPreviousSym);
					const BOOL fEqual = CColRef::Equals(
						pdrgpcrCandidate, pdrgpcrPrevious);
					pdrgpcrPrevious->Release();
					pdrgpcrCandidate->Release();
					if (!fEqual)
					{
						return false;
					}
				}
			}
		}
		if (FDirectEquality(m_prule, EdslconPredicateEq,
						psymPredCandidate, (*pdrgpsymPrevious)[0]) &&
			!pexprCandidate->Matches(pexprPrevious))
		{
			return false;
		}
	}
	return FBaseAssignmentCompatible(rgpopFilters[ulFilter], popBase,
								 pexprBase, pexprCandidate);
}

BOOL
CDSLFilterMatcher::FAssign(const CDSLOp **rgpopFilters, ULONG ulFilters,
						   ULONG ulFilter, CExpressionArray *pdrgpexprConj,
						   const CDSLOp *popBase, CExpression *pexprBase,
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
								   rgulAssigned, popBase, pexprBase,
								   pexprCandidate))
			{
				continue;
			}

			rgfUsed[ul] = true;
			rgulAssigned[ulFilter] = ul;
			if (FAssign(rgpopFilters, ulFilters, ulFilter + 1,
						pdrgpexprConj, popBase, pexprBase, rgfUsed,
						rgulAssigned))
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
		// The explicit three-symbol Filter is the correlation-aware form.  A
		// correlated or subquery InnerJoin ON predicate has an exact Filter view.
		// Keeping it shared lets operator-local DSL rules compose without learning
		// ORCA's decorrelator recursion.
		CDSLSymbolArray *pdrgpsym = popFilterRoot->Pdrgpsym();
		if (3 == pexprSelect->Arity() &&
			((*pexprSelect)[2]->DeriveHasSubquery() ||
			 (nullptr != pdrgpsym && 3 == pdrgpsym->Size())))
		{
			CExpression *pexprView =
				CDSLMatchView::PexprCorrelatedInnerJoinFilter(m_mp, pexprSelect);
			if (nullptr != pexprView)
			{
				BOOL fMatched = FMatch(popFilterRoot, pexprView, pmodel);
				pexprView->Release();
				return fMatched;
			}
		}
		return FMatchPushedDownInnerJoin(popFilterRoot, pexprSelect, pmodel);
	}
	if (COperator::EopLogicalLeftSemiApplyIn == pexprSelect->Pop()->Eopid() ||
		COperator::EopLogicalLeftSemiJoin == pexprSelect->Pop()->Eopid())
	{
		return FMatchSubqueryCarrier(popFilterRoot, pexprSelect, pmodel);
	}

	// A subquery used below OR/NOT or another general boolean expression is
	// not a relational semi/anti filter.  The production subquery handler
	// preserves its three-valued value semantics as a Select over an
	// LeftOuterApply.  Expose that exact tree to an explicitly LeftApply-shaped
	// DSL template; ordinary Filter rules continue to see the
	// untranslated Select and therefore cannot acquire implicit lowering
	// behavior.
	if (COperator::EopLogicalSelect == pexprSelect->Pop()->Eopid() &&
		2 == pexprSelect->Arity() && (*pexprSelect)[1]->DeriveHasSubquery())
	{
		const CDSLOp *rgpopFilters[GPOPT_DSL_MAX_FILTER_CHAIN];
		ULONG ulFilters = 0;
		const CDSLOp *popBase = PopCollectChain(
			popFilterRoot, rgpopFilters, GPOPT_DSL_MAX_FILTER_CHAIN, &ulFilters);
		if (nullptr != popBase &&
			EdslopLeftOuterApply == popBase->Edslop())
		{
			CExpression *pexprLowered = CDSLMatchView::PexprLowerSubqueries(
				m_mp, pexprSelect, false /*fEnforceCorrelatedApply*/,
				false /*fScalarOnly*/);
			CExpression *pexprLoweredBase = pexprLowered;
			while (nullptr != pexprLoweredBase &&
				   COperator::EopLogicalSelect ==
					   pexprLoweredBase->Pop()->Eopid() &&
				   2 == pexprLoweredBase->Arity())
			{
				pexprLoweredBase = (*pexprLoweredBase)[0];
			}
			const COperator::EOperatorId eopidLoweredBase =
				nullptr == pexprLoweredBase
					? COperator::EopSentinel
					: pexprLoweredBase->Pop()->Eopid();
			const BOOL fDirectLeftApply =
				COperator::EopLogicalLeftOuterApply == eopidLoweredBase ||
				COperator::EopLogicalLeftOuterCorrelatedApply ==
					 eopidLoweredBase;
			if (nullptr != pexprLowered && !fDirectLeftApply)
			{
				// The regular value-context alternative may place a compensating
				// GbAgg above LeftApply.  When this rule explicitly names a direct
				// Apply, select the production handler's existing correlated
				// alternative before binding any model symbols.
				pexprLowered->Release();
				pexprLowered = CDSLMatchView::PexprLowerSubqueries(
					m_mp, pexprSelect, true /*fEnforceCorrelatedApply*/,
					false /*fScalarOnly*/);
			}
			if (nullptr != pexprLowered)
			{
				const BOOL fMatched = FMatch(popFilterRoot, pexprLowered, pmodel);
				pexprLowered->Release();
				return fMatched;
			}
		}
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

	// Treat adjacent Select nodes and one conjunctive Select as the same DSL
	// Filter-chain representation. Every collected predicate remains available
	// for assignment/residual preservation; only the physical wrappers disappear
	// from the read-only match view.
	CExpressionArray *pdrgpexprConj = GPOS_NEW(m_mp) CExpressionArray(m_mp);
	const BOOL fCompletePredicate =
		1 == ulFilters && nullptr != rgpopFilters[0]->Pdrgpsym() &&
		3 == rgpopFilters[0]->Pdrgpsym()->Size();
	const BOOL fCollectAllSelects =
		fCompletePredicate &&
		FRequiresCompleteSelectChain(m_prule, rgpopFilters[0]);
	CExpression *pexprBase =
		PexprCollectSelectChain(m_mp, pexprSelect, ulFilters,
							fCollectAllSelects, pdrgpexprConj);

	// A predicate that rejects NULLs from a LeftJoin's nullable side makes the
	// filtered result exactly equivalent to filtering an InnerJoin. Expose that
	// representation to InnerJoin-rooted DSL rules before the ordinary matcher;
	// the view is read-only and CPredicateUtils proves the semantic precondition.
	const COperator::EOperatorId eopidFilterChild =
		pexprBase->Pop()->Eopid();
	if (EdslopInnerJoin == popBase->Edslop() &&
		(COperator::EopLogicalLeftOuterJoin == eopidFilterChild ||
		 COperator::EopLogicalFullOuterJoin == eopidFilterChild))
	{
		CExpressionArray *pdrgpexprPred = GPOS_NEW(m_mp) CExpressionArray(m_mp);
		CUtils::AddRefAppend(pdrgpexprPred, pdrgpexprConj);
		CExpression *pexprCombined =
			CPredicateUtils::PexprConjunction(m_mp, pdrgpexprPred);
		pexprBase->AddRef();
		CExpression *pexprSelectView = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprBase,
			pexprCombined);
		CExpression *pexprInnerView =
			CDSLMatchView::PexprNullRejectedInnerJoin(m_mp, pexprSelectView);
		pexprSelectView->Release();
		if (nullptr == pexprInnerView)
		{
			pdrgpexprConj->Release();
			return false;
		}
		BOOL fMatched = FMatch(popFilterRoot, pexprInnerView, pmodel);
		pexprInnerView->Release();
		pdrgpexprConj->Release();
		return fMatched;
	}

	const ULONG ulConj = pdrgpexprConj->Size();

	BOOL fMatched = false;
	// Filter<p,local,outer> is an explicit complete-predicate binding.  Unlike
	// the historical two-symbol Filter-chain form, it must not silently select
	// one conjunct: a rewrite that absorbs this Filter (for example into a
	// SemiJoin predicate) would otherwise lose the unselected correlations.
	if (fCompletePredicate)
	{
		CExpressionArray *pdrgpexprPred = GPOS_NEW(m_mp) CExpressionArray(m_mp);
		CUtils::AddRefAppend(pdrgpexprPred, pdrgpexprConj);
		CExpression *pexprPred =
			CPredicateUtils::PexprConjunction(m_mp, pdrgpexprPred);
		if (FBindFilterSymbols(rgpopFilters[0], pexprPred,
							   pexprBase, pmodel) &&
			m_pmatcher->FMatch(popBase, pexprBase, pmodel))
		{
			BOOL *rgfUsed = GPOS_NEW_ARRAY(m_mp, BOOL, ulConj);
			for (ULONG ul = 0; ul < ulConj; ul++)
			{
				rgfUsed[ul] = true;
			}
			RecordResidual(pdrgpexprConj, rgfUsed, pmodel);
			GPOS_DELETE_ARRAY(rgfUsed);
			fMatched = true;
		}
		pexprPred->Release();
		pdrgpexprConj->Release();
		return fMatched;
	}
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
					popBase, pexprBase, rgfUsed, rgulAssigned))
		{
			BOOL fBound = true;
			for (ULONG ul = 0; fBound && ul < ulFilters; ul++)
			{
				fBound = FBindFilterSymbols(
					rgpopFilters[ul], (*pdrgpexprConj)[rgulAssigned[ul]],
					pexprBase,
					pmodel);
			}
			// 3b. the chain base recurses against the Select's relational child.
			if (fBound &&
				m_pmatcher->FMatch(popBase, pexprBase, pmodel))
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
