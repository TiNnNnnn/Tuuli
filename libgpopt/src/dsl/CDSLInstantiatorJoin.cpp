//---------------------------------------------------------------------------
// MONSOON DSL rule engine — CDSLInstantiatorJoin.cpp
// Join/Apply builders and their captured predicate/subquery metadata.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLInstantiator.h"
#include "CDSLInstantiatorUtils.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLExpressionDefinitions.h"
#include "gpopt/dsl/CDSLJoinMatcher.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CLogicalInnerApply.h"
#include "gpopt/operators/CLogicalInnerCorrelatedApply.h"
#include "gpopt/operators/CLogicalFullOuterJoin.h"
#include "gpopt/operators/CLogicalLeftOuterApply.h"
#include "gpopt/operators/CLogicalLeftOuterCorrelatedApply.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CLogicalLeftAntiSemiApply.h"
#include "gpopt/operators/CLogicalLeftAntiSemiJoin.h"
#include "gpopt/operators/CLogicalLeftAntiSemiJoinNotIn.h"
#include "gpopt/operators/CLogicalLeftAntiSemiApplyNotIn.h"
#include "gpopt/operators/CLogicalLeftSemiApply.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"
#include "gpopt/operators/CLogicalMaxOneRow.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;
using namespace gpopt::dslinstantiator;

namespace
{
const CDSLExpressionDefinitions::CDefinition *
PdefScalarApply(const CDSLExpressionDefinitions *pexprdefs,
				const CDSLOp *popApply)
{
	if (nullptr == pexprdefs || nullptr == popApply ||
		EdslopInnerApply != popApply->Edslop() ||
		2 != popApply->UlChildren() || nullptr == popApply->Pdrgpsym() ||
		4 != popApply->Pdrgpsym()->Size())
	{
		return nullptr;
	}
	const CDSLOp *popRight = (*popApply)[1];
	if (EdslopInput != popRight->Edslop() ||
		nullptr == popRight->Pdrgpsym() || 1 != popRight->Pdrgpsym()->Size())
	{
		return nullptr;
	}
	const CDSLSymbolArray *pdrgpsymApply = popApply->Pdrgpsym();
	for (ULONG ul = 0; ul < pexprdefs->UlDefinitions(); ul++)
	{
		const CDSLExpressionDefinitions::CDefinition *pdef =
			pexprdefs->PdefAt(ul);
		if (EdslexprScalarSubquery == pdef->Edslexpr() &&
			5 == pdef->Arity() &&
			pdef->PsymOperand(0) == (*pdrgpsymApply)[0] &&
			pdef->PsymOperand(1) == (*pdrgpsymApply)[1] &&
			pdef->PsymOperand(2) == (*pdrgpsymApply)[2] &&
			pdef->PsymOperand(3) == (*pdrgpsymApply)[3] &&
			pdef->PsymOperand(4) == (*popRight->Pdrgpsym())[0])
		{
			return pdef;
		}
	}
	return nullptr;
}

const CDSLExpressionDefinitions::CDefinition *
PdefExprListApply(const CDSLExpressionDefinitions *pexprdefs,
				   const CDSLOp *popApply)
{
	if (nullptr == pexprdefs || nullptr == popApply ||
		EdslopLeftOuterApply != popApply->Edslop() ||
		2 != popApply->UlChildren() || nullptr == popApply->Pdrgpsym() ||
		4 != popApply->Pdrgpsym()->Size())
	{
		return nullptr;
	}

	const CDSLSymbolArray *pdrgpsymApply = popApply->Pdrgpsym();
	const CDSLOp *popRight = (*popApply)[1];
	const CDSLExpressionDefinitions::CDefinition *pdefMatch = nullptr;
	for (ULONG ul = 0; ul < pexprdefs->UlDefinitions(); ul++)
	{
		const CDSLExpressionDefinitions::CDefinition *pdef =
			pexprdefs->PdefAt(ul);
		BOOL fMatches = false;
		if (EdslexprExprListScalarSubquery == pdef->Edslexpr() &&
			7 == pdef->Arity() && EdslopInput == popRight->Edslop() &&
			nullptr != popRight->Pdrgpsym() &&
			1 == popRight->Pdrgpsym()->Size())
		{
			fMatches = pdef->PsymOperand(1) == (*pdrgpsymApply)[0] &&
				pdef->PsymOperand(2) == (*pdrgpsymApply)[1] &&
				pdef->PsymOperand(3) == (*pdrgpsymApply)[2] &&
				pdef->PsymOperand(4) == (*pdrgpsymApply)[3] &&
				pdef->PsymOperand(6) == (*popRight->Pdrgpsym())[0];
		}
		else
		{
			const BOOL fExistential =
				EdslexprExprListExists == pdef->Edslexpr() ||
				EdslexprExprListNotExists == pdef->Edslexpr();
			const BOOL fQuantified =
				EdslexprExprListAny == pdef->Edslexpr() ||
				EdslexprExprListAll == pdef->Edslexpr();
			const CDSLOp *popMarker = fExistential &&
					EdslopLimit == popRight->Edslop() &&
					1 == popRight->UlChildren()
				? (*popRight)[0]
				: (fQuantified ? popRight : nullptr);
			const CDSLOp *popInput = nullptr != popMarker &&
					EdslopCompute == popMarker->Edslop() &&
					1 == popMarker->UlChildren()
				? (*popMarker)[0]
				: nullptr;
			if ((fExistential || fQuantified) && 10 == pdef->Arity() &&
				nullptr != popMarker && nullptr != popMarker->Pdrgpsym() &&
				3 == popMarker->Pdrgpsym()->Size() && nullptr != popInput &&
				EdslopInput == popInput->Edslop() &&
				nullptr != popInput->Pdrgpsym() &&
				1 == popInput->Pdrgpsym()->Size())
			{
				fMatches =
					pdef->PsymOperand(1) == (*popMarker->Pdrgpsym())[0] &&
					pdef->PsymOperand(2) == (*popMarker->Pdrgpsym())[1] &&
					pdef->PsymOperand(3) == (*popMarker->Pdrgpsym())[2] &&
					pdef->PsymOperand(4) == (*pdrgpsymApply)[0] &&
					pdef->PsymOperand(5) == (*pdrgpsymApply)[1] &&
					pdef->PsymOperand(6) == (*pdrgpsymApply)[2] &&
					pdef->PsymOperand(7) == (*pdrgpsymApply)[3] &&
					pdef->PsymOperand(9) == (*popInput->Pdrgpsym())[0];
			}
		}
		if (fMatches)
		{
			if (nullptr != pdefMatch)
			{
				return nullptr;
			}
			pdefMatch = pdef;
		}
	}
	return pdefMatch;
}
}  // namespace

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildJoin
//
//	@doc:
//		InnerJoin/LeftJoin/SemiJoin/SemiApply rebuild both relational children and
//		graft the
//		SOURCE-matched predicate, building the join operator the TARGET op names.
	//		For every Join, <p a a> carries the complete predicate, including
	//		equality. Keyed forms bind the join predicate directly or obtain it from a
//		unique InSub source when a proved rule turns a semi-join view into an inner
//		join.
//		Reusing the exact predicate subtree preserves comparison semantics, while
//		the shared positional remapper adapts columns to rebuilt target children.
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildJoin(const CDSLOp *pop,
								 const CDSLModel *pmodel) const
{
	if (2 != pop->UlChildren())
	{
		return nullptr;
	}

	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	const ULONG ulSymbols = nullptr == pdrgpsym ? 0 : pdrgpsym->Size();
	const BOOL fSemiJoin = EdslopSemiJoin == pop->Edslop();
	const BOOL fSemiApply = EdslopSemiApply == pop->Edslop();
	const BOOL fAntiJoin = EdslopAntiJoin == pop->Edslop();
	const BOOL fAntiApply = EdslopAntiApply == pop->Edslop();
	const BOOL fAntiJoinNotIn = EdslopAntiJoinNotIn == pop->Edslop();
	const BOOL fAntiApplyNotIn = EdslopAntiApplyNotIn == pop->Edslop();
	const BOOL fInnerApply = EdslopInnerApply == pop->Edslop();
	const BOOL fLeftOuterApply = EdslopLeftOuterApply == pop->Edslop();
	const BOOL fFullJoin = EdslopFullJoin == pop->Edslop();
	const BOOL fPredicateJoin =
		fFullJoin || fSemiJoin || fAntiJoin || fAntiJoinNotIn;
	const BOOL fPredicateApply =
		fSemiApply || fAntiApply || fAntiApplyNotIn || fInnerApply ||
		fLeftOuterApply;
	const CDSLExpressionDefinitions::CDefinition *pdefScalarSubquery =
		fInnerApply ? PdefScalarApply(m_prule->Pexprdefs(), pop) : nullptr;
	const CDSLExpressionDefinitions::CDefinition *pdefExprListSubquery =
		fLeftOuterApply
			? PdefExprListApply(m_prule->Pexprdefs(), pop)
			: nullptr;
	const BOOL fQualifiedAntiJoinNotIn = fAntiJoinNotIn && 6 == ulSymbols;
	const BOOL fValidSymbols = fPredicateJoin
		? (3 == ulSymbols || fQualifiedAntiJoinNotIn)
		: (fPredicateApply ? 4 == ulSymbols
					  : (2 == ulSymbols || 3 == ulSymbols ||
						 4 == ulSymbols || 5 == ulSymbols ||
						 7 == ulSymbols));
	if (nullptr == pdrgpsym || !fValidSymbols)
	{
		return nullptr;
	}
	const BOOL fScalarSubquery =
		nullptr != pdefScalarSubquery &&
		EdslexprScalarSubquery == pdefScalarSubquery->Edslexpr() &&
		5 == pdefScalarSubquery->Arity() && 4 == ulSymbols &&
		pdefScalarSubquery->PsymOperand(0) == (*pdrgpsym)[0] &&
		pdefScalarSubquery->PsymOperand(1) == (*pdrgpsym)[1] &&
		pdefScalarSubquery->PsymOperand(2) == (*pdrgpsym)[2] &&
		pdefScalarSubquery->PsymOperand(3) == (*pdrgpsym)[3] &&
		nullptr != (*pop)[1]->Pdrgpsym() &&
		1 == (*pop)[1]->Pdrgpsym()->Size() &&
		pdefScalarSubquery->PsymOperand(4) ==
			(*(*pop)[1]->Pdrgpsym())[0];
	const BOOL fExprListScalarSubquery =
		nullptr != pdefExprListSubquery &&
		EdslexprExprListScalarSubquery ==
			pdefExprListSubquery->Edslexpr();
	const BOOL fExprListExistential =
		nullptr != pdefExprListSubquery &&
		(EdslexprExprListExists == pdefExprListSubquery->Edslexpr() ||
		 EdslexprExprListNotExists == pdefExprListSubquery->Edslexpr());
	const BOOL fExprListQuantified =
		nullptr != pdefExprListSubquery &&
		(EdslexprExprListAny == pdefExprListSubquery->Edslexpr() ||
		 EdslexprExprListAll == pdefExprListSubquery->Edslexpr());
	const BOOL fPredicateOnly = 3 == ulSymbols || fQualifiedAntiJoinNotIn ||
		(fPredicateApply && 4 == ulSymbols);
	const BOOL fBindsPredicate =
		fPredicateOnly || 5 == ulSymbols || 7 == ulSymbols;
	if (fBindsPredicate)
	{
		const ULONG ulPredOffset =
			fPredicateOnly ? 0 : (5 == ulSymbols ? 2 : 4);
		const CDSLSymbol *psymPred = (*pdrgpsym)[ulPredOffset];
		const CDSLSymbol *psymLeftDeps =
			PsymResolve((*pdrgpsym)[ulPredOffset + 1]);
		const CDSLSymbol *psymRightDeps =
			PsymResolve((*pdrgpsym)[ulPredOffset + 2]);
		CExpression *pexprResidual =
			PexprResolvePredicate(psymPred, pmodel);
		CColRefArray *pdrgpcrLeftDeps =
			PdrgpcrResolveCols(psymLeftDeps, pmodel);
		CColRefArray *pdrgpcrRightDeps =
			PdrgpcrResolveCols(psymRightDeps, pmodel);
		if (nullptr == pexprResidual || nullptr == pdrgpcrLeftDeps ||
			nullptr == pdrgpcrRightDeps)
		{
			CRefCount::SafeRelease(pexprResidual);
			return nullptr;
		}
		CColRefSet *pcrsDeclared = GPOS_NEW(m_mp) CColRefSet(m_mp);
		pcrsDeclared->Include(pdrgpcrLeftDeps);
		pcrsDeclared->Include(pdrgpcrRightDeps);
		const BOOL fDependenciesExact =
			pcrsDeclared->Equals(pexprResidual->DeriveUsedColumns());
		pcrsDeclared->Release();
		pexprResidual->Release();
		if (!fDependenciesExact)
		{
			return nullptr;
		}
	}
	if (fQualifiedAntiJoinNotIn)
	{
		const CDSLSymbol *psymQualifier = (*pdrgpsym)[3];
		const CDSLSymbol *psymQualifierLeftDeps =
			PsymResolve((*pdrgpsym)[4]);
		const CDSLSymbol *psymQualifierRightDeps =
			PsymResolve((*pdrgpsym)[5]);
		CExpression *pexprQualifier =
			PexprResolvePredicate(psymQualifier, pmodel);
		CColRefArray *pdrgpcrQualifierLeftDeps =
			PdrgpcrResolveCols(psymQualifierLeftDeps, pmodel);
		CColRefArray *pdrgpcrQualifierRightDeps =
			PdrgpcrResolveCols(psymQualifierRightDeps, pmodel);
		if (nullptr == pexprQualifier ||
			nullptr == pdrgpcrQualifierLeftDeps ||
			nullptr == pdrgpcrQualifierRightDeps)
		{
			CRefCount::SafeRelease(pexprQualifier);
			return nullptr;
		}
		CColRefSet *pcrsDeclared = GPOS_NEW(m_mp) CColRefSet(m_mp);
		pcrsDeclared->Include(pdrgpcrQualifierLeftDeps);
		pcrsDeclared->Include(pdrgpcrQualifierRightDeps);
		const BOOL fDependenciesExact =
			pcrsDeclared->Equals(pexprQualifier->DeriveUsedColumns());
		pcrsDeclared->Release();
		pexprQualifier->Release();
		if (!fDependenciesExact)
		{
			return nullptr;
		}
	}
	const CDSLSymbol *psymLeft =
		fPredicateOnly ? nullptr : PsymResolve((*pdrgpsym)[0]);
	const CDSLSymbol *psymRight =
		fPredicateOnly ? nullptr : PsymResolve((*pdrgpsym)[1]);
	CExpression *pexprOwnedJoinPred = fPredicateOnly
		? PexprResolvePredicate((*pdrgpsym)[0], pmodel)
		: nullptr;
	CExpression *pexprJoinPred =
		fPredicateOnly ? pexprOwnedJoinPred
					   : pmodel->PexprJoinPred(psymLeft, psymRight);
	if (!fPredicateOnly && nullptr == pexprJoinPred)
	{
		ULONG ulInSubMatches = 0;
		const CDSLOp *popSourceInSub = PopOnlyBoundInSub(
			m_prule->PfragSrc()->PopRoot(), pmodel, &ulInSubMatches);
		if (1 == ulInSubMatches && nullptr != popSourceInSub &&
			nullptr != popSourceInSub->Pdrgpsym() &&
			(1 == popSourceInSub->Pdrgpsym()->Size() ||
			 5 == popSourceInSub->Pdrgpsym()->Size()))
		{
			const CDSLSymbol *psymInSubAttrs =
				(*popSourceInSub->Pdrgpsym())[0];
			if (psymLeft == psymInSubAttrs || psymRight == psymInSubAttrs)
			{
				pexprJoinPred = pmodel->PexprInSubPred(psymInSubAttrs);
			}
		}
		// A keyed target Join is itself a complete equality specification. If
		// no source Join/InSub predicate exists, construct that equality from
		// the two ordered key vectors instead of requiring an unrelated source
		// scalar artifact.
		if (nullptr == pexprJoinPred && nullptr != psymLeft &&
			nullptr != psymRight)
		{
			CColRefArray *pdrgpcrLeft = PdrgpcrResolveCols(psymLeft, pmodel);
			CColRefArray *pdrgpcrRight = PdrgpcrResolveCols(psymRight, pmodel);
			if (nullptr != pdrgpcrLeft && nullptr != pdrgpcrRight &&
				0 < pdrgpcrLeft->Size() &&
				pdrgpcrLeft->Size() == pdrgpcrRight->Size())
			{
				CExpressionArray *pdrgpexpr =
					GPOS_NEW(m_mp) CExpressionArray(m_mp);
				for (ULONG ul = 0; ul < pdrgpcrLeft->Size(); ul++)
				{
					pdrgpexpr->Append(CUtils::PexprScalarEqCmp(
						m_mp, (*pdrgpcrLeft)[ul], (*pdrgpcrRight)[ul]));
				}
				pexprOwnedJoinPred =
					CPredicateUtils::PexprConjunction(m_mp, pdrgpexpr);
				pexprJoinPred = pexprOwnedJoinPred;
			}
		}
		if (nullptr == pexprJoinPred)
		{
			return nullptr;
		}
	}

	CExpression *pexprLeft = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprLeft)
	{
		CRefCount::SafeRelease(pexprOwnedJoinPred);
		return nullptr;
	}
	CExpression *pexprRight = PexprBuild((*pop)[1], pmodel);
	if (nullptr == pexprRight)
	{
		CRefCount::SafeRelease(pexprOwnedJoinPred);
		pexprLeft->Release();
		return nullptr;
	}
	if (!pexprLeft->DeriveOutputColumns()->IsDisjoint(
			pexprRight->DeriveOutputColumns()))
	{
		if (GPOS_FTRACE(EopttracePrintDSLRule))
		{
			GPOS_TRACE_FORMAT(
				"DSL_INSTANTIATE_TRACE operator=%s status=rejected "
				"reason=overlapping_child_outputs",
				CDSLOpKindTable::SzName(pop->Edslop()));
		}
		CRefCount::SafeRelease(pexprOwnedJoinPred);
		pexprLeft->Release();
		pexprRight->Release();
		return nullptr;
	}

	CExpression *pexprTargetPred = PexprRemapPredicateToChildren(
		(*pop)[0], pexprLeft, (*pop)[1], pexprRight, pexprJoinPred,
		pmodel);
	CRefCount::SafeRelease(pexprOwnedJoinPred);
	if (nullptr == pexprTargetPred)
	{
		pexprLeft->Release();
		pexprRight->Release();
		return nullptr;
	}
	CExpression *pexprNotInComparison = nullptr;
	if (m_prule->Pexprdefs()->FHasBindings() && fPredicateOnly)
	{
		// Validate each scope after remapping. A correct union does not imply
		// correct left/right partitions, and target inputs can be reordered.
		CColRefArray *actual[2] = {nullptr, nullptr};
		BOOL valid = CDSLJoinMatcher::FDerivePredicateDependencies(
			m_mp, pexprTargetPred, pexprLeft, pexprRight, &actual[0], &actual[1]);
		CExpression *children[2] = {pexprLeft, pexprRight};
		for (ULONG i = 0; valid && i < 2; i++)
		{
			CColRefArray *declared = PdrgpcrResolveCols(
				PsymResolve((*pdrgpsym)[i + 1]), pmodel);
			CColRefArray *mapped = nullptr == declared ? nullptr :
				PdrgpcrMapToTarget((*pop)[i], children[i], declared, pmodel);
			CColRefSet *expected = GPOS_NEW(m_mp) CColRefSet(m_mp);
			if (nullptr != mapped)
				expected->Include(mapped);
			valid = nullptr != mapped && expected->Size() == actual[i]->Size() &&
				FColSetContainsArray(expected, actual[i]);
			expected->Release();
			CRefCount::SafeRelease(mapped);
		}
		actual[0]->Release();
		actual[1]->Release();
		if (!valid)
		{
			pexprTargetPred->Release();
			pexprLeft->Release();
			pexprRight->Release();
			return nullptr;
		}
	}
	if (fAntiJoinNotIn || fAntiApplyNotIn)
	{
		CExpression *pexprInverse =
			CDSLMatchView::PexprInverseComparison(m_mp, pexprTargetPred);
		pexprTargetPred->Release();
		pexprTargetPred = pexprInverse;
		if (nullptr == pexprTargetPred)
		{
			pexprLeft->Release();
			pexprRight->Release();
			return nullptr;
		}
	}
	if (fQualifiedAntiJoinNotIn)
	{
		// Retain the raw violation comparison independently of its position in
		// the conjunction. Scalar AND children are unordered in the memo, so a
		// numeric split point would not survive extraction.
		pexprTargetPred->AddRef();
		pexprNotInComparison = pexprTargetPred;
		CExpression *pexprSourceQualifier =
			PexprResolvePredicate((*pdrgpsym)[3], pmodel);
		CExpression *pexprTargetQualifier = PexprRemapPredicateToChildren(
			(*pop)[0], pexprLeft, (*pop)[1], pexprRight,
			pexprSourceQualifier, pmodel);
		CRefCount::SafeRelease(pexprSourceQualifier);
		if (nullptr == pexprTargetQualifier)
		{
			pexprNotInComparison->Release();
			pexprTargetPred->Release();
			pexprLeft->Release();
			pexprRight->Release();
			return nullptr;
		}
		CExpression *pexprQualified = CPredicateUtils::PexprConjunction(
			m_mp, pexprTargetPred, pexprTargetQualifier);
		pexprTargetPred->Release();
		pexprTargetQualifier->Release();
		pexprTargetPred = pexprQualified;
	}

	// build the join operator the TARGET names.
	COperator *popJoin = nullptr;
	if (fInnerApply || fLeftOuterApply ||
		(m_prule->Pexprdefs()->FHasBindings() && (fSemiApply || fAntiApply)))
	{
		const BOOL bindings = m_prule->Pexprdefs()->FHasBindings();
		CExpression *pexprCarrier = bindings ? nullptr :
			pmodel->PexprApplyCarrier(PsymResolve((*pdrgpsym)[0]));
		const auto findCarrier = [&](const auto &self, const CDSLOp *source) -> BOOL {
			CExpression *candidate =
				(source->Edslop() == pop->Edslop() ||
				 ((fInnerApply || fLeftOuterApply) &&
				  (EdslopInnerApply == source->Edslop() || EdslopLeftOuterApply == source->Edslop())))
					? pmodel->PexprApplyCarrier((*source->Pdrgpsym())[0]) : nullptr;
			if (nullptr != candidate)
			{
				BOOL matches = true;
				// New predicates need not alias their old root. Reuse a complete
				// source metadata binding, not an arbitrary Apply in the tree.
				for (ULONG i = 1; i < 4; i++)
					matches &= PsymResolve((*pdrgpsym)[i]) == (*source->Pdrgpsym())[i];
				if (matches)
				{
					if (nullptr != pexprCarrier &&
						(!pexprCarrier->Pop()->Matches(candidate->Pop()) ||
						 CLogicalApply::PopConvert(pexprCarrier->Pop())->EopidOriginSubq() !=
						 CLogicalApply::PopConvert(candidate->Pop())->EopidOriginSubq()))
						return false;
					pexprCarrier = candidate;
				}
			}
			for (ULONG i = 0; i < source->UlChildren(); i++)
				if (!self(self, (*source)[i]))
					return false;
			return true;
		};
		if (bindings && (!findCarrier(findCarrier, m_prule->PfragSrc()->PopRoot()) ||
						 nullptr == pexprCarrier))
		{
			pexprTargetPred->Release();
			pexprLeft->Release();
			pexprRight->Release();
			return nullptr;
		}
		if (nullptr != pexprCarrier)
		{
			CLogicalApply *popCarrier =
				CLogicalApply::PopConvert(pexprCarrier->Pop());
			CColRefArray *pdrgpcrInner = popCarrier->PdrgPcrInner();
			CColRefArray *pdrgpcrTargetInner =
				nullptr == pdrgpcrInner
					? nullptr
					: PdrgpcrMapToTarget((*pop)[1], pexprRight,
										 pdrgpcrInner, pmodel);
			if (nullptr == pdrgpcrTargetInner ||
				((fInnerApply || fLeftOuterApply) && 0 == pdrgpcrTargetInner->Size()))
			{
				CRefCount::SafeRelease(pdrgpcrTargetInner);
				pexprTargetPred->Release();
				pexprLeft->Release();
				pexprRight->Release();
				return nullptr;
			}
			if (fSemiApply || fAntiApply)
			{
				// Native remapping represents unchanged columns by absent entries.
				// PdrgpcrMapToTarget above has already validated every required column.
				UlongToColRefMap *mapping = GPOS_NEW(m_mp) UlongToColRefMap(m_mp);
				for (ULONG i = 0; i < pdrgpcrInner->Size(); i++)
				{
					ULONG id = (*pdrgpcrInner)[i]->Id();
					if ((*pdrgpcrInner)[i] != (*pdrgpcrTargetInner)[i] && nullptr == mapping->Find(&id))
						mapping->Insert(GPOS_NEW(m_mp) ULONG(id), (*pdrgpcrTargetInner)[i]);
				}
				popJoin = popCarrier->PopCopyWithRemappedColumns(m_mp, mapping, false);
				mapping->Release();
				pdrgpcrTargetInner->Release();
			}
			else if (fInnerApply)
			{
				popJoin = popCarrier->FCorrelated()
					? static_cast<COperator *>(GPOS_NEW(m_mp)
						  CLogicalInnerCorrelatedApply(
							  m_mp, pdrgpcrTargetInner,
							  popCarrier->EopidOriginSubq()))
					: static_cast<COperator *>(GPOS_NEW(m_mp) CLogicalInnerApply(
						  m_mp, pdrgpcrTargetInner,
						  popCarrier->EopidOriginSubq()));
			}
			else
			{
				popJoin = popCarrier->FCorrelated()
					? static_cast<COperator *>(GPOS_NEW(m_mp)
						  CLogicalLeftOuterCorrelatedApply(
							  m_mp, pdrgpcrTargetInner,
							  popCarrier->EopidOriginSubq()))
					: static_cast<COperator *>(GPOS_NEW(m_mp)
						  CLogicalLeftOuterApply(
							  m_mp, pdrgpcrTargetInner,
							  popCarrier->EopidOriginSubq()));
			}
		}
		else if (fScalarSubquery || fExprListScalarSubquery ||
				 fExprListExistential || fExprListQuantified)
		{
			const CDSLSymbol *psymInner = fScalarSubquery
				? (*pdrgpsym)[2]
				: pdefExprListSubquery->PsymOperand(
					  (fExprListExistential || fExprListQuantified) ? 8 : 5);
			CColRefArray *pdrgpcrInner =
				PdrgpcrResolveCols(psymInner, pmodel);
			if (nullptr == pdrgpcrInner ||
				((fExprListExistential || fExprListQuantified) ? 2 : 1) !=
					pdrgpcrInner->Size())
			{
				pexprTargetPred->Release();
				pexprLeft->Release();
				pexprRight->Release();
				return nullptr;
			}
			pdrgpcrInner->AddRef();
			const COperator::EOperatorId eopidOrigin = fExprListQuantified
				? (EdslexprExprListAll == pdefExprListSubquery->Edslexpr()
					   ? COperator::EopScalarSubqueryAll
					   : COperator::EopScalarSubqueryAny)
				: COperator::EopScalarSubquery;
			if (fExprListQuantified)
			{
				// ANY/ALL is evaluated by the correlated subplan carrier.  This
				// preserves empty-set and NULL results without duplicating ORCA's
				// aggregate-based decorrelation implementation in the DSL runtime.
				popJoin = GPOS_NEW(m_mp) CLogicalLeftOuterCorrelatedApply(
					m_mp, pdrgpcrInner, eopidOrigin);
			}
			else if (0 == pexprRight->DeriveOuterReferences()->Size())
			{
				if (fScalarSubquery ||
					(fExprListScalarSubquery &&
					 1 < pexprRight->DeriveMaxCard().Ull()))
				{
					pexprRight = GPOS_NEW(m_mp) CExpression(
						m_mp, GPOS_NEW(m_mp) CLogicalMaxOneRow(m_mp),
						pexprRight);
				}
				popJoin = fLeftOuterApply
					? static_cast<COperator *>(GPOS_NEW(m_mp)
						  CLogicalLeftOuterApply(
							  m_mp, pdrgpcrInner,
							  eopidOrigin))
					: static_cast<COperator *>(GPOS_NEW(m_mp)
						  CLogicalInnerApply(
							  m_mp, pdrgpcrInner,
							  eopidOrigin));
			}
			else
			{
				popJoin = fLeftOuterApply
					? static_cast<COperator *>(GPOS_NEW(m_mp)
						  CLogicalLeftOuterCorrelatedApply(
							  m_mp, pdrgpcrInner,
							  eopidOrigin))
					: static_cast<COperator *>(GPOS_NEW(m_mp)
						  CLogicalInnerCorrelatedApply(
							  m_mp, pdrgpcrInner,
							  eopidOrigin));
			}
		}
	}
	switch (pop->Edslop())
	{
		case EdslopInnerJoin:
			popJoin = GPOS_NEW(m_mp) CLogicalInnerJoin(m_mp);
			break;
		case EdslopLeftJoin:
			popJoin = GPOS_NEW(m_mp) CLogicalLeftOuterJoin(m_mp);
			break;
		case EdslopFullJoin:
			popJoin = GPOS_NEW(m_mp) CLogicalFullOuterJoin(m_mp);
			break;
		case EdslopSemiJoin:
			popJoin = GPOS_NEW(m_mp) CLogicalLeftSemiJoin(m_mp);
			break;
		case EdslopSemiApply:
			if (nullptr == popJoin)
				popJoin = GPOS_NEW(m_mp) CLogicalLeftSemiApply(m_mp);
			break;
		case EdslopAntiJoin:
			popJoin = GPOS_NEW(m_mp) CLogicalLeftAntiSemiJoin(m_mp);
			break;
		case EdslopAntiApply:
			if (nullptr == popJoin)
				popJoin = GPOS_NEW(m_mp) CLogicalLeftAntiSemiApply(m_mp);
			break;
		case EdslopAntiJoinNotIn:
			popJoin = nullptr == pexprNotInComparison
				? static_cast<COperator *>(GPOS_NEW(m_mp)
					  CLogicalLeftAntiSemiJoinNotIn(m_mp))
				: static_cast<COperator *>(GPOS_NEW(m_mp)
					  CLogicalLeftAntiSemiJoinNotIn(
						  m_mp, pexprNotInComparison));
			pexprNotInComparison = nullptr;
			break;
		case EdslopAntiApplyNotIn:
			popJoin = GPOS_NEW(m_mp) CLogicalLeftAntiSemiApplyNotIn(m_mp);
			break;
		case EdslopInnerApply:
			if (nullptr == popJoin)
			{
				popJoin = GPOS_NEW(m_mp) CLogicalInnerApply(m_mp);
			}
			break;
		case EdslopLeftOuterApply:
			if (nullptr == popJoin)
			{
				popJoin = GPOS_NEW(m_mp) CLogicalLeftOuterApply(m_mp);
			}
			break;
		default:
			CRefCount::SafeRelease(pexprNotInComparison);
			pexprTargetPred->Release();
			pexprLeft->Release();
			pexprRight->Release();
			return nullptr;
	}

	CExpression *pexprResult = GPOS_NEW(m_mp)
		CExpression(m_mp, popJoin, pexprLeft, pexprRight, pexprTargetPred);
	if (!fPredicateApply && (4 == ulSymbols || 7 == ulSymbols))
	{
		const CDSLSymbol *psymOutput = PsymResolve((*pdrgpsym)[2]);
		const CDSLSymbol *psymSchema = PsymResolve((*pdrgpsym)[3]);
		CColRefArray *pdrgpcrOutput =
			PdrgpcrResolveCols(psymOutput, pmodel);
		CColRefArray *pdrgpcrSchema =
			PdrgpcrResolveCols(psymSchema, pmodel);
		CColRefArray *pdrgpcrActual =
			pexprResult->DeriveOutputColumns()->Pdrgpcr(m_mp);
		const BOOL fOutputPreserved = nullptr != pdrgpcrOutput &&
			nullptr != pdrgpcrSchema &&
			CColRef::Equals(pdrgpcrOutput, pdrgpcrSchema) &&
			CColRef::Equals(pdrgpcrOutput, pdrgpcrActual);
		pdrgpcrActual->Release();
		if (!fOutputPreserved)
		{
			pexprResult->Release();
			return nullptr;
		}
	}
	if (fPredicateApply)
	{
		const CDSLSymbol *psymCorrelations =
			PsymResolve((*pdrgpsym)[3]);
		CColRefArray *pdrgpcrExpected =
			PdrgpcrResolveCols(psymCorrelations, pmodel);
		CColRefSet *pcrsActual = GPOS_NEW(m_mp) CColRefSet(
			m_mp, *pexprRight->DeriveOuterReferences());
		pcrsActual->Intersection(pexprLeft->DeriveOutputColumns());
		CColRefArray *pdrgpcrActual = pcrsActual->Pdrgpcr(m_mp);
		const BOOL fCorrelationsPreserved = nullptr != pdrgpcrExpected &&
			CColRef::Equals(pdrgpcrExpected, pdrgpcrActual);
		pdrgpcrActual->Release();
		pcrsActual->Release();
		if (!fCorrelationsPreserved)
		{
			pexprResult->Release();
			return nullptr;
		}
	}
	if (fScalarSubquery)
	{
		CExpressionArray *pdrgpexprResidual = pmodel->PdrgpexprResidual();
		if (nullptr != pdrgpexprResidual && 0 < pdrgpexprResidual->Size())
		{
			CExpressionArray *pdrgpexprCopy =
				GPOS_NEW(m_mp) CExpressionArray(m_mp);
			for (ULONG ul = 0; ul < pdrgpexprResidual->Size(); ul++)
			{
				CExpression *pexprConj = (*pdrgpexprResidual)[ul];
				pexprConj->AddRef();
				pdrgpexprCopy->Append(pexprConj);
			}
			CExpression *pexprResidual =
				CPredicateUtils::PexprConjunction(m_mp, pdrgpexprCopy);
			pexprResult = GPOS_NEW(m_mp) CExpression(
				m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprResult,
				pexprResidual);
		}
	}
	return pexprResult;
}
