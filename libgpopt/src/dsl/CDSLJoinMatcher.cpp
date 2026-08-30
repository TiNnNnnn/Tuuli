//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLJoinMatcher.cpp
//
//	@doc:
//		Implementation of the join symbol binder (see CDSLJoinMatcher.h). Migrates
//		the SEMANTICS of WeTune's join match: bind the equi-join key columns to the
//		two <a> symbols, keep non-equi conjuncts as residual, recurse both children.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLJoinMatcher.h"

#include <vector>

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CPropConstraint.h"
#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalInnerApply.h"
#include "gpopt/operators/CLogicalInnerCorrelatedApply.h"
#include "gpopt/operators/CLogicalLeftOuterApply.h"
#include "gpopt/operators/CLogicalLeftOuterCorrelatedApply.h"
#include "gpopt/operators/CLogicalLeftAntiSemiApply.h"
#include "gpopt/operators/CLogicalLeftAntiSemiJoin.h"
#include "gpopt/operators/CLogicalLeftSemiApply.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CPredicateUtils.h"

using namespace gpopt;

namespace
{
// Return the already-bound source attrs connected to psymJoin by a direct
// AttrsEq declaration. Target symbols and not-yet-bound source symbols do not
// constrain this match. Multiple declarations are accepted only when they
// agree on the same ordered column vector.
const CColRefArray *
PdrgpcrEqualityPeer(const CDSLRule *prule, const CDSLSymbol *psymJoin,
					const CDSLModel *pmodel)
{
	if (nullptr == prule)
	{
		return nullptr;
	}

	const CColRefArray *pdrgpcrExpected = nullptr;
	CDSLConstraintArray *pdrgpcon = prule->Pdrgpcon();
	for (ULONG ul = 0; ul < pdrgpcon->Size(); ul++)
	{
		const CDSLConstraint *pcon = (*pdrgpcon)[ul];
		if (EdslconAttrsEq != pcon->Edslcon() ||
			2 != pcon->Pdrgpsym()->Size())
		{
			continue;
		}
		const CDSLSymbol *psymFirst = (*pcon->Pdrgpsym())[0];
		const CDSLSymbol *psymSecond = (*pcon->Pdrgpsym())[1];
		const CDSLSymbol *psymPeer = nullptr;
		if (psymJoin == psymFirst)
		{
			psymPeer = psymSecond;
		}
		else if (psymJoin == psymSecond)
		{
			psymPeer = psymFirst;
		}
		if (nullptr == psymPeer || EdslsideSource != psymPeer->Eside())
		{
			continue;
		}

		CColRefArray *pdrgpcrPeer = pmodel->PdrgpcrAttrs(psymPeer);
		if (nullptr == pdrgpcrPeer)
		{
			continue;
		}
		if (nullptr == pdrgpcrExpected)
		{
			pdrgpcrExpected = pdrgpcrPeer;
			continue;
		}
		if (pdrgpcrExpected->Size() != pdrgpcrPeer->Size())
		{
			return nullptr;
		}
		for (ULONG col = 0; col < pdrgpcrExpected->Size(); col++)
		{
			if ((*pdrgpcrExpected)[col] != (*pdrgpcrPeer)[col])
			{
				return nullptr;
			}
		}
	}
	return pdrgpcrExpected;
}

BOOL
FColArraysEqual(const CColRefArray *pdrgpcrFirst,
				const CColRefArray *pdrgpcrSecond)
{
	if (nullptr == pdrgpcrFirst || nullptr == pdrgpcrSecond ||
		pdrgpcrFirst->Size() != pdrgpcrSecond->Size())
	{
		return false;
	}
	for (ULONG ul = 0; ul < pdrgpcrFirst->Size(); ul++)
	{
		if ((*pdrgpcrFirst)[ul] != (*pdrgpcrSecond)[ul])
		{
			return false;
		}
	}
	return true;
}

// An inner join is commutative, but a BottomUp RBO sees only the translator's
// chosen child order. An already-bound AttrsEq peer (typically a surrounding
// Filter or Proj) tells us which child order the DSL source template expects.
// Prefer the native order when both are possible; expose the commuted read-only
// view only when the native orientation is incompatible and the reverse is
// fully determined by those bindings.
BOOL
FJoinKeyOrientationCompatible(const CColRefArray *pdrgpcrExpectedLeft,
							  const CColRefArray *pdrgpcrExpectedRight,
							  const CColRefArray *pdrgpcrLeft,
							  const CColRefArray *pdrgpcrRight)
{
	return (nullptr == pdrgpcrExpectedLeft ||
			FColArraysEqual(pdrgpcrExpectedLeft, pdrgpcrLeft)) &&
		   (nullptr == pdrgpcrExpectedRight ||
			FColArraysEqual(pdrgpcrExpectedRight, pdrgpcrRight));
}

// WeTune may match one equality edge of a join whose physical predicate has
// several cross-child equalities. If a surrounding source operator has already
// bound an AttrsEq peer, select the unique ordered edge subset compatible with
// that peer. The complete predicate is still retained on the model, so
// unselected equalities are never dropped by target construction.
void
NarrowJoinKeys(CMemoryPool *mp, const CDSLRule *prule,
			   const CDSLSymbol *psymLeft, const CDSLSymbol *psymRight,
			   const CDSLModel *pmodel, CColRefArray **ppdrgpcrLeft,
			   CColRefArray **ppdrgpcrRight)
{
	const CColRefArray *pdrgpcrExpectedLeft =
		PdrgpcrEqualityPeer(prule, psymLeft, pmodel);
	const CColRefArray *pdrgpcrExpectedRight =
		PdrgpcrEqualityPeer(prule, psymRight, pmodel);
	if (nullptr == pdrgpcrExpectedLeft && nullptr == pdrgpcrExpectedRight)
	{
		return;
	}
	if (nullptr != pdrgpcrExpectedLeft && nullptr != pdrgpcrExpectedRight &&
		pdrgpcrExpectedLeft->Size() != pdrgpcrExpectedRight->Size())
	{
		return;
	}

	const CColRefArray *pdrgpcrSelector = nullptr != pdrgpcrExpectedLeft
		? pdrgpcrExpectedLeft
		: pdrgpcrExpectedRight;
	CColRefArray *pdrgpcrNarrowLeft = GPOS_NEW(mp) CColRefArray(mp);
	CColRefArray *pdrgpcrNarrowRight = GPOS_NEW(mp) CColRefArray(mp);
	std::vector<BOOL> used((*ppdrgpcrLeft)->Size(), false);
	for (ULONG expected = 0; expected < pdrgpcrSelector->Size(); expected++)
	{
		ULONG match = gpos::ulong_max;
		ULONG matches = 0;
		for (ULONG candidate = 0; candidate < (*ppdrgpcrLeft)->Size();
			 candidate++)
		{
			if (used[candidate])
			{
				continue;
			}
			BOOL fMatches = nullptr != pdrgpcrExpectedLeft
				? (*(*ppdrgpcrLeft))[candidate] ==
					  (*pdrgpcrExpectedLeft)[expected]
				: (*(*ppdrgpcrRight))[candidate] ==
					  (*pdrgpcrExpectedRight)[expected];
			if (fMatches && nullptr != pdrgpcrExpectedLeft &&
				nullptr != pdrgpcrExpectedRight)
			{
				fMatches = (*(*ppdrgpcrRight))[candidate] ==
					(*pdrgpcrExpectedRight)[expected];
			}
			if (fMatches)
			{
				match = candidate;
				matches++;
			}
		}
		// An ambiguous subset must be left for the ordinary constraint checker;
		// choosing an arbitrary equality would make rule order affect semantics.
		if (1 != matches)
		{
			pdrgpcrNarrowLeft->Release();
			pdrgpcrNarrowRight->Release();
			return;
		}
		used[match] = true;
		pdrgpcrNarrowLeft->Append((*(*ppdrgpcrLeft))[match]);
		pdrgpcrNarrowRight->Append((*(*ppdrgpcrRight))[match]);
	}

	(*ppdrgpcrLeft)->Release();
	(*ppdrgpcrRight)->Release();
	*ppdrgpcrLeft = pdrgpcrNarrowLeft;
	*ppdrgpcrRight = pdrgpcrNarrowRight;
}

BOOL
FColumnsEquivalent(CExpression *pexpr, const CColRef *pcrFirst,
				   const CColRef *pcrSecond)
{
	if (pcrFirst == pcrSecond)
	{
		return true;
	}
	CPropConstraint *ppc = pexpr->DerivePropertyConstraint();
	CColRefSet *pcrs = ppc->PcrsEquivClass(pcrFirst);
	return nullptr != pcrs && pcrs->FMember(pcrSecond);
}

BOOL
FEquivalentJoinEdges(CExpression *pexprLeft, CExpression *pexprRight,
					 const CColRefArray *pdrgpcrLeft,
					 const CColRefArray *pdrgpcrRight, ULONG ulFirst,
					 ULONG ulSecond)
{
	return FColumnsEquivalent(pexprLeft, (*pdrgpcrLeft)[ulFirst],
						  (*pdrgpcrLeft)[ulSecond]) &&
		   FColumnsEquivalent(pexprRight, (*pdrgpcrRight)[ulFirst],
						  (*pdrgpcrRight)[ulSecond]);
}

// Candidate validation is traversal-order independent only after all other
// source symbols have concrete bindings. A nested join whose sibling has not
// been visited yet simply retains its complete key vector.
BOOL
FAllOtherSourceSymbolsBound(const CDSLRule *prule,
						 const CDSLSymbol *psymLeft,
						 const CDSLSymbol *psymRight,
						 const CDSLModel *pmodel)
{
	CDSLSymbolArray *pdrgpsym = prule->PfragSrc()->Pdrgpsym();
	for (ULONG ul = 0; ul < pdrgpsym->Size(); ul++)
	{
		const CDSLSymbol *psym = (*pdrgpsym)[ul];
		if (psym != psymLeft && psym != psymRight &&
			nullptr == pmodel->PvalLookup(psym))
		{
			return false;
		}
	}
	return true;
}

BOOL
FConstraintsHoldWithJoinKeys(CMemoryPool *mp, const CDSLRule *prule,
						 const CDSLSymbol *psymLeft,
						 const CDSLSymbol *psymRight,
						 const CDSLModel *pmodel,
						 CColRefArray *pdrgpcrLeft,
						 CColRefArray *pdrgpcrRight)
{
	CDSLModel *pmodelProbe = GPOS_NEW(mp) CDSLModel(mp);
	CDSLSymbolArray *pdrgpsym = prule->PfragSrc()->Pdrgpsym();
	BOOL fBound = true;
	for (ULONG ul = 0; fBound && ul < pdrgpsym->Size(); ul++)
	{
		const CDSLSymbol *psym = (*pdrgpsym)[ul];
		CRefCount *pval = psym == psymLeft
			? static_cast<CRefCount *>(pdrgpcrLeft)
			: (psym == psymRight
				   ? static_cast<CRefCount *>(pdrgpcrRight)
				   : pmodel->PvalLookup(psym));
		fBound = nullptr != pval && pmodelProbe->FBind(psym, pval);
	}

	CDSLConstraintChecker checker(mp);
	BOOL fHolds = fBound && checker.FCheck(prule, pmodelProbe);
	pmodelProbe->Release();
	return fHolds;
}

// Evaluate one commutative orientation in an isolated model. This is used only
// when the native and reversed key vectors are both compatible with already
// bound AttrsEq peers. Matching the child templates in the probe supplies the
// table bindings needed by Unique/Reference/NotNull, without mutating the real
// match attempt. *pfComplete is false when an outer source symbol is not bound
// yet, in which case orientation remains deliberately undecided.
BOOL
FJoinOrientationConstraintsHold(
	CMemoryPool *mp, const CDSLRule *prule, const CDSLOp *popJoin,
	const CDSLSymbol *psymLeft, const CDSLSymbol *psymRight,
	const CDSLModel *pmodel, CExpression *pexprLeft,
	CExpression *pexprRight, CColRefArray *pdrgpcrLeft,
	CColRefArray *pdrgpcrRight, BOOL *pfComplete)
{
	GPOS_ASSERT(nullptr != pfComplete);
	*pfComplete = false;
	if (nullptr == prule)
	{
		return false;
	}

	CDSLModel *pmodelProbe = GPOS_NEW(mp) CDSLModel(mp);
	CDSLSymbolArray *pdrgpsym = prule->PfragSrc()->Pdrgpsym();
	BOOL fBound = true;
	for (ULONG ul = 0; fBound && ul < pdrgpsym->Size(); ul++)
	{
		const CDSLSymbol *psym = (*pdrgpsym)[ul];
		if (psym == psymLeft || psym == psymRight)
		{
			continue;
		}
		CRefCount *pval = pmodel->PvalLookup(psym);
		if (nullptr != pval)
		{
			fBound = pmodelProbe->FBind(psym, pval);
		}
	}

	CDSLMatcher matcherProbe(mp, prule);
	fBound = fBound &&
		matcherProbe.FMatch((*popJoin)[0], pexprLeft, pmodelProbe) &&
		matcherProbe.FMatch((*popJoin)[1], pexprRight, pmodelProbe);
	if (fBound)
	{
		*pfComplete = FAllOtherSourceSymbolsBound(
			prule, psymLeft, psymRight, pmodelProbe);
	}
	const BOOL fHolds = fBound && *pfComplete &&
		FConstraintsHoldWithJoinKeys(mp, prule, psymLeft, psymRight,
								 pmodelProbe, pdrgpcrLeft, pdrgpcrRight);
	pmodelProbe->Release();
	return fHolds;
}

BOOL
FTryJoinKeyRepresentatives(
	CMemoryPool *mp, const CDSLRule *prule, const CDSLSymbol *psymLeft,
	const CDSLSymbol *psymRight, const CDSLModel *pmodel,
	const CColRefArray *pdrgpcrAllLeft,
	const CColRefArray *pdrgpcrAllRight,
	const std::vector<std::vector<ULONG>> &groups, ULONG ulGroup,
	std::vector<ULONG> *pselected, CColRefArray **ppdrgpcrLeft,
	CColRefArray **ppdrgpcrRight)
{
	if (ulGroup < groups.size())
	{
		for (ULONG ulEdge : groups[ulGroup])
		{
			pselected->push_back(ulEdge);
			if (FTryJoinKeyRepresentatives(
					mp, prule, psymLeft, psymRight, pmodel,
					pdrgpcrAllLeft, pdrgpcrAllRight, groups, ulGroup + 1,
					pselected, ppdrgpcrLeft, ppdrgpcrRight))
			{
				return true;
			}
			pselected->pop_back();
		}
		return false;
	}

	CColRefArray *pdrgpcrCandidateLeft = GPOS_NEW(mp) CColRefArray(mp);
	CColRefArray *pdrgpcrCandidateRight = GPOS_NEW(mp) CColRefArray(mp);
	for (ULONG ulEdge : *pselected)
	{
		pdrgpcrCandidateLeft->Append((*pdrgpcrAllLeft)[ulEdge]);
		pdrgpcrCandidateRight->Append((*pdrgpcrAllRight)[ulEdge]);
	}

	if (!FConstraintsHoldWithJoinKeys(mp, prule, psymLeft, psymRight,
								pmodel, pdrgpcrCandidateLeft,
								pdrgpcrCandidateRight))
	{
		pdrgpcrCandidateLeft->Release();
		pdrgpcrCandidateRight->Release();
		return false;
	}

	(*ppdrgpcrLeft)->Release();
	(*ppdrgpcrRight)->Release();
	*ppdrgpcrLeft = pdrgpcrCandidateLeft;
	*ppdrgpcrRight = pdrgpcrCandidateRight;
	return true;
}

// ORCA's equality closure can add transitively implied edges to a binary Join,
// while WeTune binds the keys written on the original binary join. Collapse
// only edges proven equivalent inside both relational children. Trying one
// representative per equivalence-pair class preserves predicate semantics and
// avoids an exponential arbitrary-subset search.
void
NarrowEquivalentJoinKeysByConstraints(
	CMemoryPool *mp, const CDSLRule *prule, const CDSLSymbol *psymLeft,
	const CDSLSymbol *psymRight, CExpression *pexprLeft,
	CExpression *pexprRight, const CDSLModel *pmodel,
	CColRefArray **ppdrgpcrLeft, CColRefArray **ppdrgpcrRight)
{
	if (nullptr == prule || (*ppdrgpcrLeft)->Size() < 2 ||
		!FAllOtherSourceSymbolsBound(prule, psymLeft, psymRight, pmodel) ||
		FConstraintsHoldWithJoinKeys(mp, prule, psymLeft, psymRight, pmodel,
								  *ppdrgpcrLeft, *ppdrgpcrRight))
	{
		return;
	}

	std::vector<std::vector<ULONG>> groups;
	for (ULONG ulEdge = 0; ulEdge < (*ppdrgpcrLeft)->Size(); ulEdge++)
	{
		BOOL fGrouped = false;
		for (std::vector<ULONG> &group : groups)
		{
			if (FEquivalentJoinEdges(pexprLeft, pexprRight,
								 *ppdrgpcrLeft, *ppdrgpcrRight,
								 group.front(), ulEdge))
			{
				group.push_back(ulEdge);
				fGrouped = true;
				break;
			}
		}
		if (!fGrouped)
		{
			groups.push_back(std::vector<ULONG>(1, ulEdge));
		}
	}
	if (groups.size() == (*ppdrgpcrLeft)->Size())
	{
		return;
	}

	std::vector<ULONG> selected;
	(void) FTryJoinKeyRepresentatives(
		mp, prule, psymLeft, psymRight, pmodel, *ppdrgpcrLeft,
		*ppdrgpcrRight, groups, 0, &selected, ppdrgpcrLeft,
		ppdrgpcrRight);
}

}  // namespace

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinMatcher::FSplitPredicate
//---------------------------------------------------------------------------
BOOL
CDSLJoinMatcher::FSplitPredicate(CExpression *pexprPred,
								 CExpression *pexprLeftRel,
								 CColRefArray *pdrgpcrLeft,
								 CColRefArray *pdrgpcrRight,
								 CExpressionArray *pdrgpexprResidual) const
{
	return CDSLMatchView::FSplitJoinPredicate(
		m_mp, pexprPred, pexprLeftRel, pdrgpcrLeft, pdrgpcrRight,
		pdrgpexprResidual);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinMatcher::FMatch
//---------------------------------------------------------------------------
BOOL
CDSLJoinMatcher::FMatch(const CDSLOp *popJoin, CExpression *pexprJoin,
						CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != popJoin);
	GPOS_ASSERT(EdslopInnerJoin == popJoin->Edslop() ||
				EdslopLeftJoin == popJoin->Edslop() ||
				EdslopSemiJoin == popJoin->Edslop() ||
				EdslopSemiApply == popJoin->Edslop() ||
				EdslopAntiJoin == popJoin->Edslop() ||
				EdslopAntiApply == popJoin->Edslop() ||
				EdslopInnerApply == popJoin->Edslop() ||
				EdslopLeftOuterApply == popJoin->Edslop());
	GPOS_ASSERT(nullptr != pexprJoin);

	// identity + arity gate: the live node must be the matching join carrying
	// (left rel, right rel, scalar predicate).
	const COperator::EOperatorId eopid = pexprJoin->Pop()->Eopid();
	const BOOL fInner = (EdslopInnerJoin == popJoin->Edslop());
	const BOOL fSemi = (EdslopSemiJoin == popJoin->Edslop());
	const BOOL fSemiApply = (EdslopSemiApply == popJoin->Edslop());
	const BOOL fAnti = (EdslopAntiJoin == popJoin->Edslop());
	const BOOL fAntiApply = (EdslopAntiApply == popJoin->Edslop());
	const BOOL fInnerApply = (EdslopInnerApply == popJoin->Edslop());
	const BOOL fLeftOuterApply =
		(EdslopLeftOuterApply == popJoin->Edslop());
	const BOOL fPredicateJoin = fSemi || fAnti;
	const BOOL fPredicateApply =
		fSemiApply || fAntiApply || fInnerApply || fLeftOuterApply;
	if (fInnerApply &&
		COperator::EopLogicalSelect == pexprJoin->Pop()->Eopid())
	{
		return FMatchScalarSubquerySelect(popJoin, pexprJoin, pmodel);
	}
	const COperator::EOperatorId eopidExpected = fInner
		? COperator::EopLogicalInnerJoin
		: (fSemi ? COperator::EopLogicalLeftSemiJoin
				 : (fSemiApply ? COperator::EopLogicalLeftSemiApply
					: (fAnti ? COperator::EopLogicalLeftAntiSemiJoin
						: (fAntiApply ? COperator::EopLogicalLeftAntiSemiApply
									  : (fInnerApply
										 ? COperator::EopLogicalInnerApply
										 : (fLeftOuterApply
											? COperator::EopLogicalLeftOuterApply
											: COperator::EopLogicalLeftOuterJoin))))));
	const BOOL fExpectedSemiApply =
		fSemiApply &&
		(COperator::EopLogicalLeftSemiApply == eopid ||
			 COperator::EopLogicalLeftSemiApplyIn == eopid);
	const BOOL fExpectedInnerApply =
		fInnerApply &&
		(COperator::EopLogicalInnerApply == eopid ||
		 COperator::EopLogicalInnerCorrelatedApply == eopid);
	const BOOL fExpectedLeftOuterApply =
		fLeftOuterApply &&
		(COperator::EopLogicalLeftOuterApply == eopid ||
		 COperator::EopLogicalLeftOuterCorrelatedApply == eopid);
	if ((!fExpectedSemiApply && !fExpectedInnerApply &&
		 !fExpectedLeftOuterApply &&
		 eopid != eopidExpected) ||
		3 != pexprJoin->Arity())
	{
		return false;
	}

	// Join symbols are equality keys followed by optional output/schema and/or
	// residual predicate plus its left/right dependency vectors.
	CDSLSymbolArray *pdrgpsym = popJoin->Pdrgpsym();
	const ULONG ulSymbols = nullptr == pdrgpsym ? 0 : pdrgpsym->Size();
	if (nullptr == pdrgpsym ||
		(fPredicateJoin ? 3 != ulSymbols
			   : (fPredicateApply ? 4 != ulSymbols
			   : (2 != ulSymbols && 3 != ulSymbols && 4 != ulSymbols &&
				  5 != ulSymbols && 7 != ulSymbols))) ||
		2 != popJoin->UlChildren())
	{
		return false;
	}

	// InnerJoin/LeftJoin<p a a> binds a complete predicate that has no
	// extractable equality keys. This is deliberately disjoint from the keyed
	// two/five/seven-symbol forms. The predicate dependencies give the generic
	// remapper all information needed to rebuild the same scalar expression over
	// target children.
	if ((fPredicateJoin && 3 == ulSymbols) ||
		(fPredicateApply && 4 == ulSymbols) ||
		(!fPredicateJoin && !fPredicateApply && 3 == ulSymbols))
	{
		CExpression *pexprPred = nullptr;
		if (fPredicateJoin || fPredicateApply)
		{
			// SemiJoin<p a a> always names the complete predicate. Unlike the
			// legacy three-symbol Join form, equality conjuncts are not split out.
			pexprPred = (*pexprJoin)[2];
			pexprPred->AddRef();
		}
		else
		{
			CColRefArray *pdrgpcrLeftKeys =
				GPOS_NEW(m_mp) CColRefArray(m_mp);
			CColRefArray *pdrgpcrRightKeys =
				GPOS_NEW(m_mp) CColRefArray(m_mp);
			CExpressionArray *pdrgpexprPred =
				GPOS_NEW(m_mp) CExpressionArray(m_mp);
			FSplitPredicate((*pexprJoin)[2], (*pexprJoin)[0],
							pdrgpcrLeftKeys, pdrgpcrRightKeys,
							pdrgpexprPred);
			const BOOL fPredicateOnly = 0 == pdrgpcrLeftKeys->Size() &&
				0 == pdrgpcrRightKeys->Size() &&
				0 < pdrgpexprPred->Size();
			pdrgpcrLeftKeys->Release();
			pdrgpcrRightKeys->Release();
			if (!fPredicateOnly)
			{
				pdrgpexprPred->Release();
				return false;
			}

			pexprPred =
				CPredicateUtils::PexprConjunction(m_mp, pdrgpexprPred);
		}
		CColRefSet *pcrsUsed = pexprPred->DeriveUsedColumns();
		CColRefSet *pcrsLeftDeps =
			GPOS_NEW(m_mp) CColRefSet(m_mp, *pcrsUsed);
		pcrsLeftDeps->Intersection((*pexprJoin)[0]->DeriveOutputColumns());
		CColRefSet *pcrsRightDeps =
			GPOS_NEW(m_mp) CColRefSet(m_mp, *pcrsUsed);
		pcrsRightDeps->Intersection((*pexprJoin)[1]->DeriveOutputColumns());
		CColRefSet *pcrsDeclared =
			GPOS_NEW(m_mp) CColRefSet(m_mp, *pcrsLeftDeps);
		pcrsDeclared->Union(pcrsRightDeps);
		const BOOL fDependenciesExact = pcrsDeclared->Equals(pcrsUsed);
		CColRefArray *pdrgpcrLeftDeps = pcrsLeftDeps->Pdrgpcr(m_mp);
		CColRefArray *pdrgpcrRightDeps = pcrsRightDeps->Pdrgpcr(m_mp);
		pcrsDeclared->Release();
		pcrsLeftDeps->Release();
		pcrsRightDeps->Release();

		CColRefArray *pdrgpcrCorrelations = nullptr;
		if (fPredicateApply)
		{
			CColRefSet *pcrsCorrelations = GPOS_NEW(m_mp) CColRefSet(
				m_mp, *(*pexprJoin)[1]->DeriveOuterReferences());
			pcrsCorrelations->Intersection(
				(*pexprJoin)[0]->DeriveOutputColumns());
			pdrgpcrCorrelations = pcrsCorrelations->Pdrgpcr(m_mp);
			pcrsCorrelations->Release();
		}

		BOOL fMatched = fDependenciesExact &&
			m_pmatcher->FMatch((*popJoin)[0], (*pexprJoin)[0], pmodel) &&
			m_pmatcher->FMatch((*popJoin)[1], (*pexprJoin)[1], pmodel) &&
			pmodel->FBind((*pdrgpsym)[0], pexprPred) &&
			pmodel->FBind((*pdrgpsym)[1], pdrgpcrLeftDeps) &&
			pmodel->FBind((*pdrgpsym)[2], pdrgpcrRightDeps) &&
			(!fPredicateApply ||
			 pmodel->FBind((*pdrgpsym)[3], pdrgpcrCorrelations));
		if (fMatched && (fInnerApply || fLeftOuterApply))
		{
			pexprJoin->AddRef();
			fMatched =
				pmodel->FSetApplyCarrier((*pdrgpsym)[0], pexprJoin);
		}
		pexprPred->Release();
		pdrgpcrLeftDeps->Release();
		pdrgpcrRightDeps->Release();
		CRefCount::SafeRelease(pdrgpcrCorrelations);
		return fMatched;
	}
	const CDSLSymbol *psymLeft = (*pdrgpsym)[0];
	const CDSLSymbol *psymRight = (*pdrgpsym)[1];
	CExpression *pexprLeftRel = (*pexprJoin)[0];
	CExpression *pexprRightRel = (*pexprJoin)[1];

	// split the predicate into left/right equi-key columns + residual.
	CColRefArray *pdrgpcrLeft = GPOS_NEW(m_mp) CColRefArray(m_mp);
	CColRefArray *pdrgpcrRight = GPOS_NEW(m_mp) CColRefArray(m_mp);
	CExpressionArray *pdrgpexprResidual = GPOS_NEW(m_mp) CExpressionArray(m_mp);

	if (!FSplitPredicate((*pexprJoin)[2], pexprLeftRel, pdrgpcrLeft,
						 pdrgpcrRight, pdrgpexprResidual))
	{
		pdrgpcrLeft->Release();
		pdrgpcrRight->Release();
		pdrgpexprResidual->Release();
		return false;
	}

	if (fInner)
	{
		const CColRefArray *pdrgpcrExpectedLeft =
			PdrgpcrEqualityPeer(m_prule, psymLeft, pmodel);
		const CColRefArray *pdrgpcrExpectedRight =
			PdrgpcrEqualityPeer(m_prule, psymRight, pmodel);
		const BOOL fDirect = FJoinKeyOrientationCompatible(
			pdrgpcrExpectedLeft, pdrgpcrExpectedRight, pdrgpcrLeft,
			pdrgpcrRight);
		const BOOL fReverse = FJoinKeyOrientationCompatible(
			pdrgpcrExpectedLeft, pdrgpcrExpectedRight, pdrgpcrRight,
			pdrgpcrLeft);
		BOOL fUseReverse = !fDirect && fReverse;
		if (fDirect && fReverse)
		{
			BOOL fDirectComplete = false;
			BOOL fReverseComplete = false;
			const BOOL fDirectConstraints = FJoinOrientationConstraintsHold(
				m_mp, m_prule, popJoin, psymLeft, psymRight, pmodel,
				pexprLeftRel, pexprRightRel, pdrgpcrLeft, pdrgpcrRight,
				&fDirectComplete);
			const BOOL fReverseConstraints = FJoinOrientationConstraintsHold(
				m_mp, m_prule, popJoin, psymLeft, psymRight, pmodel,
				pexprRightRel, pexprLeftRel, pdrgpcrRight, pdrgpcrLeft,
				&fReverseComplete);
			fUseReverse = fDirectComplete && fReverseComplete &&
				!fDirectConstraints && fReverseConstraints;
		}
		if (fUseReverse)
		{
			pexprLeftRel = (*pexprJoin)[1];
			pexprRightRel = (*pexprJoin)[0];
			pdrgpcrLeft->Release();
			pdrgpcrRight->Release();
			pdrgpexprResidual->Release();
			pdrgpcrLeft = GPOS_NEW(m_mp) CColRefArray(m_mp);
			pdrgpcrRight = GPOS_NEW(m_mp) CColRefArray(m_mp);
			pdrgpexprResidual = GPOS_NEW(m_mp) CExpressionArray(m_mp);
			if (!FSplitPredicate((*pexprJoin)[2], pexprLeftRel,
							 pdrgpcrLeft, pdrgpcrRight,
							 pdrgpexprResidual))
			{
				pdrgpcrLeft->Release();
				pdrgpcrRight->Release();
				pdrgpexprResidual->Release();
				return false;
			}
		}
	}
	NarrowJoinKeys(m_mp, m_prule, psymLeft, psymRight, pmodel,
				   &pdrgpcrLeft, &pdrgpcrRight);

	// recurse both relational children through the generic matcher.
	if (!m_pmatcher->FMatch((*popJoin)[0], pexprLeftRel, pmodel) ||
		!m_pmatcher->FMatch((*popJoin)[1], pexprRightRel, pmodel))
	{
		pdrgpcrLeft->Release();
		pdrgpcrRight->Release();
		pdrgpexprResidual->Release();
		return false;
	}

	// Equality-closure selection needs child bindings and derived equivalence
	// classes, so it runs after recursion but before the attrs become immutable
	// model bindings. A non-equality residual makes narrowing unsafe.
	if (0 == pdrgpexprResidual->Size())
	{
		NarrowEquivalentJoinKeysByConstraints(
			m_mp, m_prule, psymLeft, psymRight, pexprLeftRel,
			pexprRightRel, pmodel, &pdrgpcrLeft, &pdrgpcrRight);
	}

	// bind the two <a> symbols (FBind AddRefs; release our local refs after).
	BOOL fBound = pmodel->FBind(psymLeft, pdrgpcrLeft) &&
				  pmodel->FBind(psymRight, pdrgpcrRight);
	pdrgpcrLeft->Release();
	pdrgpcrRight->Release();
	const BOOL fBindsOutput = 4 == ulSymbols || 7 == ulSymbols;
	const BOOL fBindsResidual = 5 == ulSymbols || 7 == ulSymbols;
	if (fBound && fBindsOutput)
	{
		// ORCA exposes relational outputs by stable CColRef identity. Sorting the
		// complete set by CColRef id gives a deterministic ordered binding that is
		// unchanged when target construction merely reorders Join children.
		CColRefArray *pdrgpcrOutput =
			pexprJoin->DeriveOutputColumns()->Pdrgpcr(m_mp);
		fBound = pmodel->FBind((*pdrgpsym)[2], pdrgpcrOutput) &&
			pmodel->FBind((*pdrgpsym)[3], pdrgpcrOutput);
		pdrgpcrOutput->Release();
	}
	if (!fBound)
	{
		pdrgpexprResidual->Release();
		return false;
	}

	if (fBindsResidual)
	{
		// An explicit Join predicate binding denotes the non-equality remainder,
		// not the key equalities already represented by the first two symbols.
		// Requiring at least one residual keeps this form shape-sensitive; an
		// equality-only Join continues to use the two/four-symbol forms.
		if (0 == pdrgpexprResidual->Size())
		{
			pdrgpexprResidual->Release();
			return false;
		}
		CExpression *pexprResidual =
			CPredicateUtils::PexprConjunction(m_mp, pdrgpexprResidual);
		const ULONG ulPredOffset = 5 == ulSymbols ? 2 : 4;

		CColRefSet *pcrsLeftDeps = GPOS_NEW(m_mp) CColRefSet(
			m_mp, *pexprResidual->DeriveUsedColumns());
		pcrsLeftDeps->Intersection(pexprLeftRel->DeriveOutputColumns());
		CColRefSet *pcrsRightDeps = GPOS_NEW(m_mp) CColRefSet(
			m_mp, *pexprResidual->DeriveUsedColumns());
		pcrsRightDeps->Intersection(pexprRightRel->DeriveOutputColumns());
		CColRefArray *pdrgpcrLeftDeps = pcrsLeftDeps->Pdrgpcr(m_mp);
		CColRefArray *pdrgpcrRightDeps = pcrsRightDeps->Pdrgpcr(m_mp);
		pcrsLeftDeps->Release();
		pcrsRightDeps->Release();

		fBound = pmodel->FBind((*pdrgpsym)[ulPredOffset], pexprResidual) &&
			pmodel->FBind((*pdrgpsym)[ulPredOffset + 1],
						  pdrgpcrLeftDeps) &&
			pmodel->FBind((*pdrgpsym)[ulPredOffset + 2],
						  pdrgpcrRightDeps);
		pexprResidual->Release();
		pdrgpcrLeftDeps->Release();
		pdrgpcrRightDeps->Release();
		if (!fBound)
		{
			return false;
		}
	}
	else
	{
		// The complete predicate retained below already carries every residual.
		pdrgpexprResidual->Release();
	}

	// Record by this Join node's attrs pair, so nested joins retain independent
	// predicates and target-side AttrsEq aliases can find the right one.
	CExpression *pexprPred = (*pexprJoin)[2];
	BOOL fMatched = pmodel->FSetJoinPred(psymLeft, psymRight, pexprPred);
	if (fMatched && (fInnerApply || fLeftOuterApply))
	{
		// Apply operators carry optimizer-owned required-inner-column and
		// origin-subquery metadata that is not part of the DSL surface. Keep the
		// exact matched carrier for any Apply-rooted target that preserves it.
		pexprJoin->AddRef();
		fMatched = pmodel->FSetApplyCarrier((*pdrgpsym)[0], pexprJoin);
	}
	return fMatched;
}

BOOL
CDSLJoinMatcher::FMatchScalarSubquerySelect(const CDSLOp *popJoin,
									 CExpression *pexprSelect,
									 CDSLModel *pmodel) const
{
	if (2 != pexprSelect->Arity() || nullptr == popJoin->Pdrgpsym() ||
		4 != popJoin->Pdrgpsym()->Size())
	{
		return false;
	}

	CExpression *pexprCanonical =
		CDSLMatchView::PexprLowerSingleScalarSubquery(m_mp, pexprSelect);
	if (nullptr == pexprCanonical)
	{
		return false;
	}

	const COperator::EOperatorId eopid = pexprCanonical->Pop()->Eopid();
	if ((COperator::EopLogicalInnerApply != eopid &&
		 COperator::EopLogicalInnerCorrelatedApply != eopid) ||
		3 != pexprCanonical->Arity())
	{
		pexprCanonical->Release();
		return false;
	}

	BOOL fMatched = FMatch(popJoin, pexprCanonical, pmodel);
	pexprCanonical->Release();
	return fMatched;
}

// EOF
