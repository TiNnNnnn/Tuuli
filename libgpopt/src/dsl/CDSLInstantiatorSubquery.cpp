//---------------------------------------------------------------------------
// MONSOON DSL rule engine — CDSLInstantiatorSubquery.cpp
// EXISTS, IN and quantified subquery builders. Exact bindings retain their safety
// gates; legacy carrier paths below those gates are not fallbacks for failed exact builds.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLInstantiator.h"
#include "CDSLInstantiatorUtils.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLExpressionDefinitions.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CLogicalLeftAntiSemiApply.h"
#include "gpopt/operators/CLogicalLeftAntiSemiApplyNotIn.h"
#include "gpopt/operators/CLogicalLeftAntiSemiCorrelatedApplyNotIn.h"
#include "gpopt/operators/CLogicalLeftSemiApply.h"
#include "gpopt/operators/CLogicalLeftSemiApplyIn.h"
#include "gpopt/operators/CLogicalLeftSemiCorrelatedApplyIn.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CNormalizer.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarSubqueryExists.h"
#include "gpopt/operators/CScalarSubqueryAny.h"
#include "gpopt/operators/CScalarSubqueryNotExists.h"

using namespace gpopt;
using namespace gpopt::dslinstantiator;

namespace
{
// Find the source quantified node that declared a bound predicate. Target
// PredicateEq and AttrsEq constraints may deliberately send that predicate to
// a different dependency vector, so target construction must remap from the
// source owner's attrs rather than assuming both vectors are identical.
const CDSLOp *
PopSourceQuantifiedForPredicate(const CDSLOp *pop,
								const CDSLSymbol *psymPred)
{
	if ((EdslopAny == pop->Edslop() || EdslopAll == pop->Edslop()) &&
		nullptr != pop->Pdrgpsym() && 2 == pop->Pdrgpsym()->Size() &&
		(*pop->Pdrgpsym())[0] == psymPred)
	{
		return pop;
	}
	for (ULONG ul = 0; ul < pop->UlChildren(); ul++)
	{
		const CDSLOp *popFound =
			PopSourceQuantifiedForPredicate((*pop)[ul], psymPred);
		if (nullptr != popFound)
		{
			return popFound;
		}
	}
	return nullptr;
}

CExpression *
PexprRemapInSubPredicate(CMemoryPool *mp, CExpression *pexprPred,
						 const CColRefArray *pdrgpcrFrom,
						 const CColRefArray *pdrgpcrTo)
{
	if (nullptr == pdrgpcrFrom || nullptr == pdrgpcrTo ||
		pdrgpcrFrom->Size() != pdrgpcrTo->Size())
	{
		return nullptr;
	}

	UlongToColRefMap *phm = GPOS_NEW(mp) UlongToColRefMap(mp);
	BOOL fRemap = false;
	BOOL fTypeChange = false;
	for (ULONG ul = 0; ul < pdrgpcrFrom->Size(); ul++)
	{
		CColRef *pcrFrom = (*pdrgpcrFrom)[ul];
		CColRef *pcrTo = (*pdrgpcrTo)[ul];
		fTypeChange = fTypeChange ||
			!pcrFrom->RetrieveType()->MDId()->Equals(
				pcrTo->RetrieveType()->MDId());
		if (pcrFrom != pcrTo)
		{
			BOOL fInserted GPOS_ASSERTS_ONLY = phm->Insert(
				GPOS_NEW(mp) ULONG(pcrFrom->Id()), pcrTo);
			GPOS_ASSERT(fInserted);
			fRemap = true;
		}
	}

	if (!fRemap)
	{
		phm->Release();
		pexprPred->AddRef();
		return pexprPred;
	}
	CExpression *pexprRemapped = PexprRemapPredicate(mp, pexprPred, phm);
	phm->Release();
	if (fTypeChange)
	{
		CExpression *pexprTyped =
			PexprRebuildComparisons(mp, pexprRemapped);
		pexprRemapped->Release();
		return pexprTyped;
	}
	return pexprRemapped;
}
}  // namespace

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildExists
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildExists(const CDSLOp *pop,
								   const CDSLModel *pmodel) const
{
	const BOOL fNegated = EdslopNotExists == pop->Edslop();
	if (!fNegated && EdslopExists != pop->Edslop())
	{
		return nullptr;
	}
	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	const ULONG ulSymbols = nullptr == pdrgpsym ? 0 : pdrgpsym->Size();
	if (2 != pop->UlChildren() || nullptr == pdrgpsym ||
		(0 != ulSymbols && 3 != ulSymbols))
	{
		return nullptr;
	}
	if (3 == ulSymbols)
	{
		if (fNegated)
		{
			return nullptr;
		}
		const CDSLSymbol *psymPred = (*pdrgpsym)[0];
		const CDSLSymbol *psymLeftDeps = PsymResolve((*pdrgpsym)[1]);
		const CDSLSymbol *psymRightDeps = PsymResolve((*pdrgpsym)[2]);
		CExpression *pexprPred = PexprResolvePredicate(psymPred, pmodel);
		CColRefArray *pdrgpcrLeftDeps =
			PdrgpcrResolveCols(psymLeftDeps, pmodel);
		CColRefArray *pdrgpcrRightDeps =
			PdrgpcrResolveCols(psymRightDeps, pmodel);
		if (nullptr == pexprPred || nullptr == pdrgpcrLeftDeps ||
			nullptr == pdrgpcrRightDeps)
		{
			CRefCount::SafeRelease(pexprPred);
			return nullptr;
		}
		CColRefSet *pcrsDeclared = GPOS_NEW(m_mp) CColRefSet(m_mp);
		pcrsDeclared->Include(pdrgpcrLeftDeps);
		pcrsDeclared->Include(pdrgpcrRightDeps);
		const BOOL fDependenciesExact =
			pcrsDeclared->Equals(pexprPred->DeriveUsedColumns());
		pcrsDeclared->Release();
		pexprPred->Release();
		if (!fDependenciesExact)
		{
			return nullptr;
		}
	}

	CExpression *pexprOuter = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprOuter)
	{
		return nullptr;
	}
	CExpression *pexprInner = PexprBuild((*pop)[1], pmodel);
	if (nullptr == pexprInner)
	{
		pexprOuter->Release();
		return nullptr;
	}

	if (m_prule->Pexprdefs()->FHasBindings())
	{
		// Build the certified tree verbatim. Unnesting and projection removal
		// are separate rewrites, not implicit expression-binding operations.
		COperator *subquery = fNegated
			? static_cast<COperator *>(GPOS_NEW(m_mp) CScalarSubqueryNotExists(m_mp))
			: static_cast<COperator *>(GPOS_NEW(m_mp) CScalarSubqueryExists(m_mp));
		return GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprOuter,
			GPOS_NEW(m_mp) CExpression(m_mp, subquery, pexprInner));
	}

	// A plain Project in an EXISTS target list is cardinality preserving and
	// its values are semantically unobserved. Keeping SELECT 1 (or an equivalent
	// scalar list) under a correlated Apply can leave the DXL translator looking
	// for pass-through attributes in that Project's explicit list. Canonicalize
	// away only ordinary scalar Projects; an SRF/non-scalar Project can change
	// cardinality and must remain part of the EXISTS input.
	while (COperator::EopLogicalProject == pexprInner->Pop()->Eopid() &&
		   2 == pexprInner->Arity() &&
		   !(*pexprInner)[1]->DeriveHasNonScalarFunction())
	{
		CExpression *pexprChild = (*pexprInner)[0];
		pexprChild->AddRef();
		pexprInner->Release();
		pexprInner = pexprChild;
	}

	if (3 == ulSymbols)
	{
		CExpression *pexprPred =
			PexprResolvePredicate((*pdrgpsym)[0], pmodel);
		CExpression *pexprTargetPred = PexprRemapPredicateToChildren(
			(*pop)[0], pexprOuter, (*pop)[1], pexprInner, pexprPred, pmodel);
		CRefCount::SafeRelease(pexprPred);
		if (nullptr == pexprTargetPred)
		{
			pexprOuter->Release();
			pexprInner->Release();
			return nullptr;
		}
		return CUtils::PexprLogicalJoin<CLogicalLeftSemiJoin>(
			m_mp, pexprOuter, pexprInner, pexprTargetPred);
	}

	CColRefSet *pcrsInnerOutput = pexprInner->DeriveOutputColumns();
	if (0 == pcrsInnerOutput->Size())
	{
		pexprOuter->Release();
		pexprInner->Release();
		return nullptr;
	}
	CColRef *pcrInner = pcrsInnerOutput->PcrFirst();

	// Mirror subquery removal: LIMIT 1 is valid and avoids unnecessary work only
	// for an uncorrelated EXISTS input.
	if (!fNegated && 0 == pexprInner->DeriveOuterReferences()->Size() &&
		1 < pexprInner->DeriveMaxCard().Ull())
	{
		pexprInner = CUtils::PexprLimit(m_mp, pexprInner, 0, 1);
	}

	CExpression *pexprResult = nullptr;
	if (fNegated)
	{
		pexprResult = CUtils::PexprLogicalApply<CLogicalLeftAntiSemiApply>(
			m_mp, pexprOuter, pexprInner, pcrInner,
			COperator::EopScalarSubqueryNotExists);
	}
	else
	{
		pexprResult = CUtils::PexprLogicalApply<CLogicalLeftSemiApply>(
			m_mp, pexprOuter, pexprInner, pcrInner,
			COperator::EopScalarSubqueryExists);
	}

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
		CExpression *pexprPred =
			CPredicateUtils::PexprConjunction(m_mp, pdrgpexprCopy);
		pexprResult = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprResult,
			pexprPred);
	}
	CExpression *pexprNormalized =
		CNormalizer::PexprNormalize(m_mp, pexprResult);
	pexprResult->Release();
	CExpression *pexprCanonical =
		CNormalizer::PexprPullUpProjections(m_mp, pexprNormalized);
	pexprNormalized->Release();
	return pexprCanonical;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildInSub
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildInSub(const CDSLOp *pop,
								  const CDSLModel *pmodel) const
{
	if (2 != pop->UlChildren() || nullptr == pop->Pdrgpsym() ||
		(1 != pop->Pdrgpsym()->Size() && 5 != pop->Pdrgpsym()->Size()))
	{
		return nullptr;
	}

	CExpression *pexprOuter = PexprBuild((*pop)[0], pmodel);
	CExpression *pexprInner = PexprBuild((*pop)[1], pmodel);
	if (nullptr == pexprOuter || nullptr == pexprInner)
	{
		CRefCount::SafeRelease(pexprOuter);
		CRefCount::SafeRelease(pexprInner);
		return nullptr;
	}

	if (m_prule->Pexprdefs()->FHasBindings())
	{
		const CDSLSymbol *attrs = PsymResolve((*pop->Pdrgpsym())[0]);
		CExpression *any = pmodel->PexprInSubCarrier(attrs);
		const CDSLOp *inner = (*pop)[1];
		const BOOL projected = EdslopProj == inner->Edslop() && !inner->FDistinct();
		CColRefArray *schema = projected
			? PdrgpcrResolveCols(PsymResolve((*inner->Pdrgpsym())[1]), pmodel) : nullptr;
		const CColRef *selected = nullptr != any && CDSLMatchView::FPlainEqAny(any)
			? CScalarSubqueryAny::PopConvert(any->Pop())->Pcr() : nullptr;
		// Preserve the original operator (including comparison metadata) and
		// exact output column. Fresh/ambiguous output mappings fail closed.
		if (nullptr == selected || !pexprInner->DeriveOutputColumns()->FMember(selected) ||
			(projected ? nullptr == schema || 1 != schema->Size() || (*schema)[0] != selected
				: !CDSLMatchView::FSingleValueOutput(pexprInner, selected)) ||
			!pexprOuter->DeriveOutputColumns()->ContainsAll((*any)[1]->DeriveUsedColumns()) ||
			!CDSLConstraintChecker::FQueryDemandInsensitive(pexprInner))
		{
			pexprOuter->Release();
			pexprInner->Release();
			return nullptr;
		}
		any->Pop()->AddRef();
		(*any)[1]->AddRef();
		CExpression *predicate = GPOS_NEW(m_mp) CExpression(
			m_mp, any->Pop(), pexprInner, (*any)[1]);
		return GPOS_NEW(m_mp) CExpression(m_mp,
			GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprOuter, predicate);
	}

	if (5 == pop->Pdrgpsym()->Size())
	{
		CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
		const CDSLSymbol *psymOuter = PsymResolve((*pdrgpsym)[0]);
		const CDSLSymbol *psymInner = PsymResolve((*pdrgpsym)[1]);
		CExpression *pexprSourcePred =
			pmodel->PexprJoinPred(psymOuter, psymInner);
		const CDSLSymbol *psymInSubOwner = psymOuter;
		if (nullptr == pexprSourcePred)
		{
			pexprSourcePred = pmodel->PexprInSubPred(psymInSubOwner);
		}
		if (nullptr == pexprSourcePred)
		{
			ULONG ulBoundInSub = 0;
			const CDSLOp *popSourceInSub = PopOnlyBoundInSub(
				m_prule->PfragSrc()->PopRoot(), pmodel, &ulBoundInSub);
			if (1 == ulBoundInSub && nullptr != popSourceInSub)
			{
				psymInSubOwner = (*popSourceInSub->Pdrgpsym())[0];
				pexprSourcePred = pmodel->PexprInSubPred(psymInSubOwner);
			}
		}

		CColRefArray *pdrgpcrOuterSource =
			PdrgpcrResolveCols(psymOuter, pmodel);
		CColRefArray *pdrgpcrInnerSource =
			PdrgpcrResolveCols(psymInner, pmodel);
		CColRefArray *pdrgpcrOuterTarget = nullptr;
		CColRefArray *pdrgpcrInnerTarget = nullptr;
		if (nullptr != pdrgpcrOuterSource && nullptr != pdrgpcrInnerSource)
		{
			pdrgpcrOuterTarget = PdrgpcrMapToTarget(
				(*pop)[0], pexprOuter, pdrgpcrOuterSource, pmodel);
			pdrgpcrInnerTarget = PdrgpcrMapToTarget(
				(*pop)[1], pexprInner, pdrgpcrInnerSource, pmodel);
		}
		CExpression *pexprTargetPred = PexprRemapPredicateToChildren(
			(*pop)[0], pexprOuter, (*pop)[1], pexprInner,
			pexprSourcePred, pmodel);

		const CDSLSymbol *psymResidual = PsymResolve((*pdrgpsym)[2]);
		const CDSLSymbol *psymOuterDeps = PsymResolve((*pdrgpsym)[3]);
		const CDSLSymbol *psymInnerDeps = PsymResolve((*pdrgpsym)[4]);
		CExpression *pexprSourceResidual = pmodel->PexprPred(psymResidual);
		CColRefArray *pdrgpcrOuterDepsSource =
			PdrgpcrResolveCols(psymOuterDeps, pmodel);
		CColRefArray *pdrgpcrInnerDepsSource =
			PdrgpcrResolveCols(psymInnerDeps, pmodel);
		CExpression *pexprTargetResidual =
			PexprRemapPredicateToChildren(
				(*pop)[0], pexprOuter, (*pop)[1], pexprInner,
				pexprSourceResidual, pmodel);
		CColRefArray *pdrgpcrOuterDepsTarget = nullptr;
		CColRefArray *pdrgpcrInnerDepsTarget = nullptr;
		if (nullptr != pdrgpcrOuterDepsSource &&
			nullptr != pdrgpcrInnerDepsSource)
		{
			pdrgpcrOuterDepsTarget = PdrgpcrMapToTarget(
				(*pop)[0], pexprOuter, pdrgpcrOuterDepsSource, pmodel);
			pdrgpcrInnerDepsTarget = PdrgpcrMapToTarget(
				(*pop)[1], pexprInner, pdrgpcrInnerDepsSource, pmodel);
		}

		CColRefArray *pdrgpcrActualOuter =
			GPOS_NEW(m_mp) CColRefArray(m_mp);
		CColRefArray *pdrgpcrActualInner =
			GPOS_NEW(m_mp) CColRefArray(m_mp);
		CExpressionArray *pdrgpexprActualResidual =
			GPOS_NEW(m_mp) CExpressionArray(m_mp);
		BOOL fValid = nullptr != pexprTargetPred &&
			nullptr != pexprTargetResidual &&
			nullptr != pdrgpcrOuterTarget && nullptr != pdrgpcrInnerTarget &&
			nullptr != pdrgpcrOuterDepsTarget &&
			nullptr != pdrgpcrInnerDepsTarget &&
			CDSLMatchView::FSplitJoinPredicate(
				m_mp, pexprTargetPred, pexprOuter, pdrgpcrActualOuter,
				pdrgpcrActualInner, pdrgpexprActualResidual) &&
			0 < pdrgpcrActualOuter->Size() &&
			0 < pdrgpexprActualResidual->Size();
		CExpression *pexprActualResidual = nullptr;
		CColRefArray *pdrgpcrActualOuterDeps = nullptr;
		CColRefArray *pdrgpcrActualInnerDeps = nullptr;
		if (fValid)
		{
			pexprActualResidual = CPredicateUtils::PexprConjunction(
				m_mp, pdrgpexprActualResidual);
			CColRefSet *pcrsOuterDeps = GPOS_NEW(m_mp) CColRefSet(
				m_mp, *pexprActualResidual->DeriveUsedColumns());
			pcrsOuterDeps->Intersection(pexprOuter->DeriveOutputColumns());
			CColRefSet *pcrsInnerDeps = GPOS_NEW(m_mp) CColRefSet(
				m_mp, *pexprActualResidual->DeriveUsedColumns());
			pcrsInnerDeps->Intersection(pexprInner->DeriveOutputColumns());
			pdrgpcrActualOuterDeps = pcrsOuterDeps->Pdrgpcr(m_mp);
			pdrgpcrActualInnerDeps = pcrsInnerDeps->Pdrgpcr(m_mp);
			pcrsOuterDeps->Release();
			pcrsInnerDeps->Release();
			fValid = CColRef::Equals(pdrgpcrOuterTarget,
								   pdrgpcrActualOuter) &&
				CColRef::Equals(pdrgpcrInnerTarget,
							 pdrgpcrActualInner) &&
				pexprTargetResidual->Matches(pexprActualResidual) &&
				CColRef::Equals(pdrgpcrOuterDepsTarget,
							 pdrgpcrActualOuterDeps) &&
				CColRef::Equals(pdrgpcrInnerDepsTarget,
							 pdrgpcrActualInnerDeps);
		}
		else
		{
			pdrgpexprActualResidual->Release();
		}

		CRefCount::SafeRelease(pexprActualResidual);
		CRefCount::SafeRelease(pexprTargetResidual);
		CRefCount::SafeRelease(pdrgpcrActualOuterDeps);
		CRefCount::SafeRelease(pdrgpcrActualInnerDeps);
		CRefCount::SafeRelease(pdrgpcrOuterTarget);
		CRefCount::SafeRelease(pdrgpcrInnerTarget);
		CRefCount::SafeRelease(pdrgpcrOuterDepsTarget);
		CRefCount::SafeRelease(pdrgpcrInnerDepsTarget);
		pdrgpcrActualOuter->Release();
		pdrgpcrActualInner->Release();
		if (!fValid)
		{
			CRefCount::SafeRelease(pexprTargetPred);
			pexprOuter->Release();
			pexprInner->Release();
			return nullptr;
		}

		CExpression *pexprCarrier =
			pmodel->PexprInSubCarrier(psymInSubOwner);
		CXform::EXformId exfidOrigin = CXform::ExfInvalid;
		if (nullptr != pexprCarrier &&
			COperator::EopLogicalLeftSemiJoin ==
				pexprCarrier->Pop()->Eopid())
		{
			exfidOrigin = CLogicalLeftSemiJoin::PopConvert(
				pexprCarrier->Pop())->OriginXform();
		}
		CExpression *pexprResult = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalLeftSemiJoin(m_mp, exfidOrigin),
			pexprOuter, pexprInner, pexprTargetPred);
		return pexprResult;
	}

	const CDSLSymbol *psymTargetAttrs =
		PsymResolve((*pop->Pdrgpsym())[0]);
	const CDSLSymbol *psymSourceAttrs = psymTargetAttrs;
	CExpression *pexprPredBound =
		pmodel->PexprInSubPred(psymSourceAttrs);
	if (nullptr == pexprPredBound)
	{
		ULONG ulBoundInSub = 0;
		const CDSLOp *popSourceInSub = PopOnlyBoundInSub(
			m_prule->PfragSrc()->PopRoot(), pmodel, &ulBoundInSub);
		if (1 == ulBoundInSub && nullptr != popSourceInSub)
		{
			psymSourceAttrs = (*popSourceInSub->Pdrgpsym())[0];
			pexprPredBound = pmodel->PexprInSubPred(psymSourceAttrs);
		}
	}
	CColRefArray *pdrgpcrSourceAttrs =
		PdrgpcrResolveCols(psymSourceAttrs, pmodel);
	CColRefArray *pdrgpcrTargetAttrs =
		PdrgpcrResolveCols(psymTargetAttrs, pmodel);
	CExpression *pexprCarrier =
		pmodel->PexprInSubCarrier(psymSourceAttrs);
	CExpression *pexprPred =
		(nullptr == pexprPredBound)
			? nullptr
			: PexprRemapInSubPredicate(m_mp, pexprPredBound,
									 pdrgpcrSourceAttrs,
									 pdrgpcrTargetAttrs);
	if (nullptr == pexprPred)
	{
		pexprOuter->Release();
		pexprInner->Release();
		return nullptr;
	}
	if (nullptr == pdrgpcrTargetAttrs ||
		!FColSetContainsArray(pexprOuter->DeriveOutputColumns(),
						  pdrgpcrTargetAttrs))
	{
		pexprOuter->Release();
		pexprInner->Release();
		pexprPred->Release();
		return nullptr;
	}

	// The saved comparison must reference exactly one column produced by the
	// rebuilt inner. This is the plain single-column IN shape WeTune models.
	CColRefSet *pcrsInnerUsed =
		GPOS_NEW(m_mp) CColRefSet(m_mp, *pexprPred->DeriveUsedColumns());
	pcrsInnerUsed->Intersection(pexprInner->DeriveOutputColumns());
	if (1 != pcrsInnerUsed->Size())
	{
		pcrsInnerUsed->Release();
		pexprOuter->Release();
		pexprInner->Release();
		pexprPred->Release();
		return nullptr;
	}
	CColRef *pcrInner = pcrsInnerUsed->PcrFirst();
	pcrsInnerUsed->Release();

	CExpression *pexprResult = nullptr;
	if (nullptr != pexprCarrier &&
		COperator::EopLogicalLeftSemiJoin ==
			pexprCarrier->Pop()->Eopid())
	{
		CXform::EXformId exfidOrigin =
			CLogicalLeftSemiJoin::PopConvert(pexprCarrier->Pop())
				->OriginXform();
		pexprResult = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalLeftSemiJoin(m_mp, exfidOrigin),
			pexprOuter, pexprInner, pexprPred);
	}
	else
	{
		pexprResult = CUtils::PexprLogicalApply<CLogicalLeftSemiApplyIn>(
			m_mp, pexprOuter, pexprInner, pcrInner,
			COperator::EopScalarSubqueryAny, pexprPred);
	}

	return pexprResult;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildQuantified
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildQuantified(const CDSLOp *pop,
									   const CDSLModel *pmodel) const
{
	if ((EdslopAny != pop->Edslop() && EdslopAll != pop->Edslop()) ||
		2 != pop->UlChildren() || nullptr == pop->Pdrgpsym() ||
		2 != pop->Pdrgpsym()->Size())
	{
		return nullptr;
	}

	const CDSLSymbol *psymPred = PsymResolve((*pop->Pdrgpsym())[0]);
	// Explicit predicate references retain the source comparison's carrier
	// and dependency metadata. Stop at a constructor: it is not an alias.
	for (ULONG depth = 0; depth < m_prule->Pexprdefs()->UlDefinitions(); ++depth)
	{
		const auto *def = m_prule->Pexprdefs()->Pdef(psymPred);
		if (nullptr == def || CDSLExpressionDefinitions::EBuild != def->Binding() ||
			EdslexprRef != def->Edslexpr())
			break;
		psymPred = PsymResolve(def->PsymOperand(0));
	}
	const CDSLSymbol *psymTargetAttrs =
		PsymResolve((*pop->Pdrgpsym())[1]);
	CExpression *pexprPredBound = pmodel->PexprPred(psymPred);
	const auto *target_def = m_prule->Pexprdefs()->Pdef(psymPred);
	CExpression *pexprConstructed = nullptr;
	if (nullptr == pexprPredBound && nullptr != target_def &&
		EdslexprCompare == target_def->Edslexpr())
	{
		pexprConstructed = PexprResolvePredicate(psymPred, pmodel);
		pexprPredBound = pexprConstructed;
	}
	CColRefArray *pdrgpcrTargetAttrs =
		PdrgpcrResolveCols(psymTargetAttrs, pmodel);
	const CDSLOp *popSource = PopSourceQuantifiedForPredicate(
		m_prule->PfragSrc()->PopRoot(), psymPred);
	CColRefArray *pdrgpcrSourceAttrs =
		nullptr == popSource
			? nullptr
			: PdrgpcrResolveCols((*popSource->Pdrgpsym())[1], pmodel);
	if (nullptr == popSource)
	{
		const CDSLOp *popRoot = m_prule->PfragSrc()->PopRoot();
		const CDSLExpressionDefinitions::CDefinition *pdef =
			nullptr != popRoot && EdslopFilter == popRoot->Edslop() &&
					nullptr != popRoot->Pdrgpsym() &&
					0 < popRoot->Pdrgpsym()->Size()
				? m_prule->Pexprdefs()->Pdef((*popRoot->Pdrgpsym())[0])
				: nullptr;
		const BOOL fMatchingDefinition =
			nullptr != pdef &&
			((EdslopAny == pop->Edslop() &&
			  EdslexprAny == pdef->Edslexpr()) ||
			 (EdslopAll == pop->Edslop() &&
			  EdslexprAll == pdef->Edslexpr()));
		if (fMatchingDefinition &&
			3 == pdef->Arity() && pdef->PsymOperand(0) == psymPred &&
			pdef->PsymOperand(1) == psymTargetAttrs)
		{
			pdrgpcrSourceAttrs = pdrgpcrTargetAttrs;
		}
	}
	if (nullptr == pexprPredBound ||
		(nullptr == pexprConstructed && nullptr == pdrgpcrSourceAttrs) ||
		nullptr == pdrgpcrTargetAttrs)
	{
		CRefCount::SafeRelease(pexprConstructed);
		return nullptr;
	}
	if (nullptr != pexprConstructed)
	{
		CColRefSet *declared = GPOS_NEW(m_mp) CColRefSet(m_mp);
		declared->Include(pdrgpcrTargetAttrs);
		const BOOL exact = declared->Equals((*pexprConstructed)[0]->DeriveUsedColumns());
		declared->Release();
		if (!exact)
		{
			pexprConstructed->Release();
			return nullptr;
		}
	}

	CExpression *pexprOuter = PexprBuild((*pop)[0], pmodel);
	CExpression *pexprInner = PexprBuild((*pop)[1], pmodel);
	CExpression *pexprPred = nullptr != pexprConstructed
		? pexprConstructed : PexprRemapInSubPredicate(
			m_mp, pexprPredBound, pdrgpcrSourceAttrs, pdrgpcrTargetAttrs);
	if (nullptr == pexprOuter || nullptr == pexprInner ||
		nullptr == pexprPred ||
		!FColSetContainsArray(pexprOuter->DeriveOutputColumns(),
							 pdrgpcrTargetAttrs))
	{
		CRefCount::SafeRelease(pexprOuter);
		CRefCount::SafeRelease(pexprInner);
		CRefCount::SafeRelease(pexprPred);
		return nullptr;
	}

	CColRefSet *pcrsInnerUsed =
		GPOS_NEW(m_mp) CColRefSet(m_mp, *pexprPred->DeriveUsedColumns());
	pcrsInnerUsed->Intersection(pexprInner->DeriveOutputColumns());
	if (1 != pcrsInnerUsed->Size())
	{
		pcrsInnerUsed->Release();
		pexprOuter->Release();
		pexprInner->Release();
		pexprPred->Release();
		return nullptr;
	}
	CColRef *pcrInner = pcrsInnerUsed->PcrFirst();
	pcrsInnerUsed->Release();

	CExpression *pexprResult = nullptr;
	CExpression *pexprCarrier =
		nullptr == popSource
			? nullptr
			: pmodel->PexprInSubCarrier((*popSource->Pdrgpsym())[1]);
	const BOOL fCorrelated =
		(nullptr != pexprCarrier &&
		 CLogicalApply::PopConvert(pexprCarrier->Pop())->FCorrelated()) ||
		pexprInner->HasOuterRefs();
	if (EdslopAny == pop->Edslop())
	{
		if (fCorrelated)
		{
			pexprResult =
				CUtils::PexprLogicalApply<CLogicalLeftSemiCorrelatedApplyIn>(
					m_mp, pexprOuter, pexprInner, pcrInner,
					COperator::EopScalarSubqueryAny, pexprPred);
		}
		else
		{
			pexprResult = CUtils::PexprLogicalApply<CLogicalLeftSemiApplyIn>(
				m_mp, pexprOuter, pexprInner, pcrInner,
				COperator::EopScalarSubqueryAny, pexprPred);
		}
	}
	else
	{
		if (fCorrelated)
		{
			pexprResult = CUtils::PexprLogicalApply<
				CLogicalLeftAntiSemiCorrelatedApplyNotIn>(
				m_mp, pexprOuter, pexprInner, pcrInner,
				COperator::EopScalarSubqueryAll, pexprPred);
		}
		else
		{
			CExpression *pexprViolation =
				CDSLMatchView::PexprInverseComparison(m_mp, pexprPred);
			pexprPred->Release();
			if (nullptr == pexprViolation)
			{
				pexprOuter->Release();
				pexprInner->Release();
				return nullptr;
			}
			pexprResult =
				CUtils::PexprLogicalApply<CLogicalLeftAntiSemiApplyNotIn>(
					m_mp, pexprOuter, pexprInner, pcrInner,
					COperator::EopScalarSubqueryAll, pexprViolation);
		}
	}

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
		pexprResult = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprResult,
			CPredicateUtils::PexprConjunction(m_mp, pdrgpexprCopy));
	}
	// Only the target operators declared by the rule are new.
	return pexprResult;
}
