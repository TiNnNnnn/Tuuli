//---------------------------------------------------------------------------
// MONSOON DSL rule engine — CDSLInstantiatorRelational.cpp
// Filter, set, order, limit and window builders; no rule scheduling or matching.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLInstantiator.h"
#include "CDSLInstantiatorUtils.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLExpressionDefinitions.h"
#include "gpopt/dsl/CDSLFilterMatcher.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CLogicalLeftSemiApplyIn.h"
#include "gpopt/operators/CLogicalSequenceProject.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CLogicalSetOp.h"
#include "gpopt/operators/CLogicalUnion.h"
#include "gpopt/operators/CLogicalUnionAll.h"
#include "gpopt/operators/CLogicalIntersect.h"
#include "gpopt/operators/CLogicalIntersectAll.h"
#include "gpopt/operators/CLogicalDifference.h"
#include "gpopt/operators/CLogicalDifferenceAll.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarConst.h"
#include "gpopt/xforms/CXformUtils.h"

using namespace gpopt;
using namespace gpopt::dslinstantiator;

namespace
{
BOOL
FContainsGbAgg(const CExpression *pexpr)
{
	if (COperator::EopLogicalGbAgg == pexpr->Pop()->Eopid())
	{
		return true;
	}
	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		if (FContainsGbAgg((*pexpr)[ul]))
		{
			return true;
		}
	}
	return false;
}

BOOL
FNullScalarConst(const CExpression *pexpr)
{
	return nullptr != pexpr && COperator::EopScalarConst == pexpr->Pop()->Eopid() &&
		CScalarConst::PopConvert(pexpr->Pop())->GetDatum()->IsNull();
}

// A set-op's output identities are anchored to its first input. Reordering
// branches therefore cannot merely reorder the operator's input-column arrays:
// later ORCA property derivation assumes every non-first input maps to distinct
// output CColRefs. Copy the moved child and remap its set-op columns into the
// identities required at the target position. Consumes pexpr.
CExpression *
PexprRemapSetOpChild(CMemoryPool *mp, CExpression *pexpr,
					 const CColRefArray *pdrgpcrFrom,
					 const CColRefArray *pdrgpcrTo)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != pdrgpcrFrom);
	GPOS_ASSERT(nullptr != pdrgpcrTo);
	GPOS_ASSERT(pdrgpcrFrom->Size() == pdrgpcrTo->Size());

	UlongToColRefMap *colref_mapping = GPOS_NEW(mp) UlongToColRefMap(mp);
	BOOL fNeedsRemap = false;
	for (ULONG ul = 0; ul < pdrgpcrFrom->Size(); ul++)
	{
		CColRef *pcrFrom = (*pdrgpcrFrom)[ul];
		CColRef *pcrTo = (*pdrgpcrTo)[ul];
		if (pcrFrom == pcrTo)
		{
			continue;
		}
		BOOL fInserted GPOS_ASSERTS_ONLY = colref_mapping->Insert(
			GPOS_NEW(mp) ULONG(pcrFrom->Id()), pcrTo);
		GPOS_ASSERT(fInserted);
		fNeedsRemap = true;
	}

	if (!fNeedsRemap)
	{
		colref_mapping->Release();
		return pexpr;
	}

	CExpression *pexprRemapped = pexpr->PexprCopyWithRemappedColumns(
		mp, colref_mapping, false /*must_exist*/);
	colref_mapping->Release();
	pexpr->Release();
	return pexprRemapped;
}
}  // namespace

CExpression *
CDSLInstantiator::PexprBuildFilterPredicate(
	const CDSLOp *popFilter, const CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != m_prule);
	CDSLSymbolArray *pdrgpsymTarget = popFilter->Pdrgpsym();
	if (nullptr == pdrgpsymTarget ||
		(2 != pdrgpsymTarget->Size() && 3 != pdrgpsymTarget->Size()))
	{
		return nullptr;
	}

	const CDSLSymbol *psymSourcePred = PsymResolve((*pdrgpsymTarget)[0]);
	CExpression *pexprBound = pmodel->PexprPred(psymSourcePred);
	if (m_prule->Pexprdefs()->FHasBindings() || nullptr == pexprBound ||
		pmodel->FDerivedBinding(psymSourcePred))
	{
		// Resolve constructed predicates and explicit captured terms without
		// assuming they came from an entire source Filter. Validate the exact
		// declared dependency partitions against the resulting expression.
		CExpression *pexprDerived =
			PexprResolvePredicate((*pdrgpsymTarget)[0], pmodel);
		if (nullptr == pexprDerived)
		{
			return nullptr;
		}
		CColRefSet *pcrsDeclared = GPOS_NEW(m_mp) CColRefSet(m_mp);
		for (ULONG ulPart = 1; ulPart < pdrgpsymTarget->Size(); ulPart++)
		{
			CColRefArray *pdrgpcrPart = PdrgpcrResolveCols(
				PsymResolve((*pdrgpsymTarget)[ulPart]), pmodel);
			if (nullptr == pdrgpcrPart)
			{
				const CDSLSymbol *psymPart =
					PsymResolve((*pdrgpsymTarget)[ulPart]);
				if (2 != pdrgpsymTarget->Size() ||
					EdslsideTarget != psymPart->Eside())
				{
					pcrsDeclared->Release();
					pexprDerived->Release();
					return nullptr;
				}
				// A target-only two-symbol Filter dependency vector is metadata:
				// derive it from the predicate just constructed. If the symbol is
				// consumed elsewhere it still has to be defined and resolved there.
				pcrsDeclared->Include(pexprDerived->DeriveUsedColumns());
				continue;
			}
			pcrsDeclared->Include(pdrgpcrPart);
		}
		const BOOL fDependenciesExact =
			pcrsDeclared->Equals(pexprDerived->DeriveUsedColumns());
		pcrsDeclared->Release();
		if (!fDependenciesExact)
		{
			pexprDerived->Release();
			return nullptr;
		}
		return pexprDerived;
	}
	const CDSLOp *popSourceFilter = PopSourceFilterForPredicate(
		m_prule->PfragSrc()->PopRoot(), psymSourcePred);
	if (nullptr == popSourceFilter ||
		nullptr == popSourceFilter->Pdrgpsym() ||
		popSourceFilter->Pdrgpsym()->Size() != pdrgpsymTarget->Size())
	{
		// A target Filter may reuse a complete predicate bound by another source
		// operator (for example Apply). No remap is needed when its declared
		// dependency vector is already the predicate's exact column set.
		CColRefSet *pcrsDeclared = GPOS_NEW(m_mp) CColRefSet(m_mp);
		for (ULONG ulPart = 1; ulPart < pdrgpsymTarget->Size(); ulPart++)
		{
			CColRefArray *pdrgpcrPart = PdrgpcrResolveCols(
				PsymResolve((*pdrgpsymTarget)[ulPart]), pmodel);
			if (nullptr == pdrgpcrPart)
			{
				pcrsDeclared->Release();
				return nullptr;
			}
			pcrsDeclared->Include(pdrgpcrPart);
		}
		const BOOL fDependenciesExact =
			pcrsDeclared->Equals(pexprBound->DeriveUsedColumns());
		pcrsDeclared->Release();
		if (!fDependenciesExact)
		{
			return nullptr;
		}
		pexprBound->AddRef();
		return pexprBound;
	}

	UlongToColRefMap *phm = GPOS_NEW(m_mp) UlongToColRefMap(m_mp);
	BOOL fRemap = false;
	BOOL fTypeChange = false;
	for (ULONG ulPart = 1; ulPart < pdrgpsymTarget->Size(); ulPart++)
	{
		const CDSLSymbol *psymSourceAttrs =
			(*popSourceFilter->Pdrgpsym())[ulPart];
		const CDSLSymbol *psymTargetAttrs =
			PsymResolve((*pdrgpsymTarget)[ulPart]);
		CColRefArray *pdrgpcrFrom =
			PdrgpcrResolveCols(psymSourceAttrs, pmodel);
		CColRefArray *pdrgpcrTo =
			PdrgpcrResolveCols(psymTargetAttrs, pmodel);
		if (nullptr == pdrgpcrFrom || nullptr == pdrgpcrTo ||
			pdrgpcrFrom->Size() != pdrgpcrTo->Size())
		{
			phm->Release();
			return nullptr;
		}
		for (ULONG ul = 0; ul < pdrgpcrFrom->Size(); ul++)
		{
			CColRef *pcrFrom = (*pdrgpcrFrom)[ul];
			CColRef *pcrTo = (*pdrgpcrTo)[ul];
			fTypeChange = fTypeChange ||
				!pcrFrom->RetrieveType()->MDId()->Equals(
					pcrTo->RetrieveType()->MDId());
			if (pcrFrom == pcrTo)
			{
				continue;
			}
			ULONG ulSourceId = pcrFrom->Id();
			CColRef *pcrExisting = phm->Find(&ulSourceId);
			if (nullptr != pcrExisting && pcrExisting != pcrTo)
			{
				phm->Release();
				return nullptr;
			}
			if (nullptr == pcrExisting)
			{
				BOOL fInserted GPOS_ASSERTS_ONLY = phm->Insert(
					GPOS_NEW(m_mp) ULONG(ulSourceId), pcrTo);
				GPOS_ASSERT(fInserted);
			}
			fRemap = true;
		}
	}

	if (!fRemap)
	{
		phm->Release();
		pexprBound->AddRef();
		return pexprBound;
	}
	CExpression *pexprRemapped = PexprRemapPredicate(m_mp, pexprBound, phm);
	phm->Release();
	if (fTypeChange)
	{
		CExpression *pexprTyped =
			PexprRebuildComparisons(m_mp, pexprRemapped);
		pexprRemapped->Release();
		return pexprTyped;
	}
	return pexprRemapped;
}

CExpression *
CDSLInstantiator::PexprBuildFilterCarrier(
	const CDSLOp *popFilter, const CDSLModel *pmodel,
	CExpression *pexprOuter) const
{
	CDSLSymbolArray *pdrgpsym = popFilter->Pdrgpsym();
	if (nullptr == pdrgpsym ||
		(2 != pdrgpsym->Size() && 4 != pdrgpsym->Size()))
	{
		return nullptr;
	}
	const CDSLSymbol *psymSourcePred = PsymResolve((*pdrgpsym)[0]);
	CExpression *pexprCarrier =
		pmodel->PexprFilterCarrier(psymSourcePred);
	CExpression *pexprPred = PexprBuildFilterPredicate(popFilter, pmodel);
	if (nullptr == pexprCarrier || nullptr == pexprPred ||
		3 != pexprCarrier->Arity())
	{
		CRefCount::SafeRelease(pexprPred);
		return nullptr;
	}

	CExpression *pexprInner = (*pexprCarrier)[1];
	pexprOuter->AddRef();
	pexprInner->AddRef();
	if (COperator::EopLogicalLeftSemiApplyIn ==
			pexprCarrier->Pop()->Eopid())
	{
		CLogicalApply *popApply =
			CLogicalApply::PopConvert(pexprCarrier->Pop());
		CColRefArray *pdrgpcrInner = popApply->PdrgPcrInner();
		if (nullptr == pdrgpcrInner || 1 != pdrgpcrInner->Size())
		{
			pexprOuter->Release();
			pexprInner->Release();
			pexprPred->Release();
			return nullptr;
		}
		return CUtils::PexprLogicalApply<CLogicalLeftSemiApplyIn>(
			m_mp, pexprOuter, pexprInner, (*pdrgpcrInner)[0],
			popApply->EopidOriginSubq(), pexprPred);
	}
	if (COperator::EopLogicalLeftSemiJoin == pexprCarrier->Pop()->Eopid())
	{
		CXform::EXformId exfidOrigin =
			CLogicalLeftSemiJoin::PopConvert(pexprCarrier->Pop())
				->OriginXform();
		return GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalLeftSemiJoin(m_mp, exfidOrigin),
			pexprOuter, pexprInner, pexprPred);
	}

	pexprOuter->Release();
	pexprInner->Release();
	pexprPred->Release();
	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildFilter
//
//	@doc:
//		A target Filter chain maps to one ORCA Select. Collect every bound target
//		predicate, append matcher residuals once, remove normalized duplicates, and
//		build a single conjunction. Rebuilding one Select per DSL Filter would both
//		misrepresent ORCA and repeat residual predicates at every nesting level.
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildFilter(const CDSLOp *pop,
								   const CDSLModel *pmodel) const
{
	if (m_prule->Pexprdefs()->FHasBindings())
	{
		CExpression *child = PexprBuild((*pop)[0], pmodel);
		CExpression *predicate = PexprBuildFilterPredicate(pop, pmodel);
		const BOOL correlated = 3 == pop->Pdrgpsym()->Size();
		if (nullptr == child || nullptr == predicate ||
			(!correlated &&
			 !child->DeriveOutputColumns()->ContainsAll(
				 predicate->DeriveUsedColumns())))
		{
			CRefCount::SafeRelease(child);
			CRefCount::SafeRelease(predicate);
			return nullptr;
		}
		if (correlated)
		{
			// The union alone is insufficient: swapping local/outer vectors must
			// not silently change the scope in which an actual column is read.
			for (ULONG i = 1; i < 3; i++)
			{
				CColRefArray *declared = PdrgpcrResolveCols(
					PsymResolve((*pop->Pdrgpsym())[i]), pmodel);
				CColRefArray *actual = CDSLFilterMatcher::PdrgpcrDependencies(
					m_mp, pop, predicate, child, i);
				const BOOL valid =
					nullptr != declared && CColRef::Equals(declared, actual);
				actual->Release();
				if (!valid)
				{
					child->Release();
					predicate->Release();
					return nullptr;
				}
			}
		}
		// Preserve the proved target tree, including nested filters and NOTs.
		return GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), child, predicate);
	}
	CExpressionArray *pdrgpexpr = GPOS_NEW(m_mp) CExpressionArray(m_mp);
	const CDSLOp *popCurrent = pop;
	const CDSLOp *popCarrierFilter = nullptr;
	while (nullptr != popCurrent && EdslopFilter == popCurrent->Edslop())
	{
		CDSLSymbolArray *pdrgpsym = popCurrent->Pdrgpsym();
		if (nullptr == pdrgpsym ||
			(2 != pdrgpsym->Size() && 3 != pdrgpsym->Size()) ||
			1 != popCurrent->UlChildren())
		{
			pdrgpexpr->Release();
			return nullptr;
		}
		const CDSLSymbol *psymSourcePred =
			PsymResolve((*pdrgpsym)[0]);
		if (nullptr != pmodel->PexprFilterCarrier(psymSourcePred))
		{
			if (nullptr != popCarrierFilter)
			{
				pdrgpexpr->Release();
				return nullptr;
			}
			popCarrierFilter = popCurrent;
		}
		else
		{
			CExpression *pexprPredBound =
				PexprBuildFilterPredicate(popCurrent, pmodel);
			if (nullptr == pexprPredBound)
			{
				pdrgpexpr->Release();
				return nullptr;
			}
			pdrgpexpr->Append(pexprPredBound);
		}
		popCurrent = (*popCurrent)[0];
	}

	CExpression *pexprChild = PexprBuild(popCurrent, pmodel);
	if (nullptr == pexprChild)
	{
		pdrgpexpr->Release();
		return nullptr;
	}
	if (nullptr != popCarrierFilter)
	{
		CExpression *pexprWrapped = PexprBuildFilterCarrier(
			popCarrierFilter, pmodel, pexprChild);
		pexprChild->Release();
		pexprChild = pexprWrapped;
		if (nullptr == pexprChild)
		{
			pdrgpexpr->Release();
			return nullptr;
		}
	}
	CExpressionArray *pdrgpexprResidual = pmodel->PdrgpexprResidual();
	if (nullptr != pdrgpexprResidual)
	{
		const ULONG ulResidual = pdrgpexprResidual->Size();
		for (ULONG ul = 0; ul < ulResidual; ul++)
		{
			CExpression *pexprR = (*pdrgpexprResidual)[ul];
			pexprR->AddRef();
			pdrgpexpr->Append(pexprR);
		}
	}
	// Both remapped target predicates and untouched residuals must be evaluable
	// over the rebuilt child plus any explicitly declared correlated outer
	// dependencies. This is the construction-time half of target-side AttrsSub
	// checking; treating outer refs as child outputs would reject every valid
	// Filter<p local outer> target.
	CColRefSet *pcrsAvailable = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsAvailable->Include(pexprChild->DeriveOutputColumns());
	const CDSLOp *popAvailable = pop;
	while (nullptr != popAvailable &&
		   EdslopFilter == popAvailable->Edslop())
	{
		CDSLSymbolArray *pdrgpsym = popAvailable->Pdrgpsym();
		if (nullptr != pdrgpsym && 3 == pdrgpsym->Size())
		{
			CColRefArray *pdrgpcrOuter = PdrgpcrResolveCols(
				PsymResolve((*pdrgpsym)[2]), pmodel);
			if (nullptr == pdrgpcrOuter)
			{
				pcrsAvailable->Release();
				pexprChild->Release();
				pdrgpexpr->Release();
				return nullptr;
			}
			pcrsAvailable->Include(pdrgpcrOuter);
		}
		popAvailable = (*popAvailable)[0];
	}
	for (ULONG ul = 0; ul < pdrgpexpr->Size(); ul++)
	{
		if (!pcrsAvailable->ContainsAll(
				(*pdrgpexpr)[ul]->DeriveUsedColumns()))
		{
			pcrsAvailable->Release();
			pexprChild->Release();
			pdrgpexpr->Release();
			return nullptr;
		}
	}
	pcrsAvailable->Release();
	if (0 == pdrgpexpr->Size())
	{
		pdrgpexpr->Release();
		return pexprChild;
	}

	// Duplicate Filter predicates may already have been collapsed in the source
	// ORCA expression. Keep the target in that same canonical representation.
	CExpressionArray *pdrgpexprDedup =
		CUtils::PdrgpexprDedup(m_mp, pdrgpexpr);
	pdrgpexpr->Release();
	CExpression *pexprPred =
		CPredicateUtils::PexprConjunction(m_mp, pdrgpexprDedup);

	return GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprChild, pexprPred);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildUnion
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildUnion(const CDSLOp *pop,
								  const CDSLModel *pmodel) const
{
	if (2 != pop->UlChildren() || nullptr == pop->Pdrgpsym() ||
		(0 != pop->Pdrgpsym()->Size() && 2 != pop->Pdrgpsym()->Size() &&
		 4 != pop->Pdrgpsym()->Size()))
	{
		return nullptr;
	}

	CExpression *pexprLeft = PexprBuild((*pop)[0], pmodel);
	CExpression *pexprRight = PexprBuild((*pop)[1], pmodel);
	if (nullptr == pexprLeft || nullptr == pexprRight)
	{
		CRefCount::SafeRelease(pexprLeft);
		CRefCount::SafeRelease(pexprRight);
		return nullptr;
	}

	CExpression *rgpexprTarget[2] = {pexprLeft, pexprRight};
	CExpressionArray *pdrgpexprBindings = pmodel->PdrgpexprUnionBindings();
	CColRefArray *pdrgpcrOutput = nullptr;
	CColRefArray *rgpdrgpcrInput[2] = {nullptr, nullptr};
	ULONG rgulSourceForTarget[2] = {0, 1};
	BOOL fOwnsInputMappings = false;

	for (ULONG ulBinding = 0;
		 nullptr != pdrgpexprBindings && ulBinding < pdrgpexprBindings->Size();
		 ulBinding++)
	{
		CExpression *pexprSource = (*pdrgpexprBindings)[ulBinding];
		if (2 != pexprSource->Arity())
		{
			continue;
		}
		CLogicalSetOp *popSource =
			CLogicalSetOp::PopConvert(pexprSource->Pop());
		CColRefArray *pdrgpcrCandidateOutput = popSource->PdrgpcrOutput();
		CColRef2dArray *pdrgpdrgpcrCandidateInput =
			popSource->PdrgpdrgpcrInput();
		if (2 != pdrgpdrgpcrCandidateInput->Size() ||
			0 == pdrgpcrCandidateOutput->Size())
		{
			continue;
		}
		if (2 <= pop->Pdrgpsym()->Size())
		{
			CColRefArray *pdrgpcrDeclared = PdrgpcrResolveCols(
				PsymResolve((*pop->Pdrgpsym())[0]), pmodel);
			if (nullptr == pdrgpcrDeclared ||
				!CColRef::Equals(pdrgpcrDeclared,
								 pdrgpcrCandidateOutput))
			{
				continue;
			}
		}

		BOOL rgfFits[2][2];
		for (ULONG ulTarget = 0; ulTarget < 2; ulTarget++)
		{
			CColRefSet *pcrsTarget =
				rgpexprTarget[ulTarget]->DeriveOutputColumns();
			// Four-slot templates declare the positional input maps. Reuse a
			// source map only if it is the declared map, not merely available
			// in the target's unordered output set.
			CColRefArray *declared = 4 == pop->Pdrgpsym()->Size()
				? PdrgpcrResolveCols(PsymResolve((*pop->Pdrgpsym())[2 + ulTarget]), pmodel)
				: nullptr;
			CColRefArray *mapped = nullptr == declared ? nullptr
				: PdrgpcrMapToTarget((*pop)[ulTarget], rgpexprTarget[ulTarget], declared, pmodel);
			for (ULONG ulSource = 0; ulSource < 2; ulSource++)
			{
				CColRefArray *pdrgpcrInput =
					(*pdrgpdrgpcrCandidateInput)[ulSource];
				rgfFits[ulTarget][ulSource] =
					pdrgpcrInput->Size() == pdrgpcrCandidateOutput->Size() &&
					FColSetContainsArray(pcrsTarget, pdrgpcrInput) &&
					(4 != pop->Pdrgpsym()->Size() ||
					 (nullptr != mapped && CColRef::Equals(mapped, pdrgpcrInput)));
			}
			CRefCount::SafeRelease(mapped);
		}

		if (rgfFits[0][0] && rgfFits[1][1])
		{
			pdrgpcrOutput = pdrgpcrCandidateOutput;
			rgpdrgpcrInput[0] = (*pdrgpdrgpcrCandidateInput)[0];
			rgpdrgpcrInput[1] = (*pdrgpdrgpcrCandidateInput)[1];
			rgulSourceForTarget[0] = 0;
			rgulSourceForTarget[1] = 1;
			break;
		}
		if (rgfFits[0][1] && rgfFits[1][0])
		{
			pdrgpcrOutput = pdrgpcrCandidateOutput;
			// Keep the set-op position maps stable and move the child semantics
			// into those identities below. Swapping these arrays directly can put
			// an output CColRef in a non-first input and violates ORCA invariants.
			rgpdrgpcrInput[0] = (*pdrgpdrgpcrCandidateInput)[0];
			rgpdrgpcrInput[1] = (*pdrgpdrgpcrCandidateInput)[1];
			rgulSourceForTarget[0] = 1;
			rgulSourceForTarget[1] = 0;
			break;
		}
	}

	// A target SetOp may be newly introduced rather than a reshaping of a
	// source SetOp (Join-over-Union distribution is the canonical example).
	// Its explicit full-row output binding supplies the stable output order;
	// derive each input array by following Input-copy and source-SetOp positional
	// correspondences through the already-built target branch.
	if (nullptr == pdrgpcrOutput && 2 <= pop->Pdrgpsym()->Size())
	{
		const CDSLSymbol *psymAttrs = PsymResolve((*pop->Pdrgpsym())[0]);
		const CDSLSymbol *psymSchema = PsymResolve((*pop->Pdrgpsym())[1]);
		CColRefArray *pdrgpcrAttrs =
			PdrgpcrResolveCols(psymAttrs, pmodel);
		CColRefArray *pdrgpcrSchema =
			PdrgpcrResolveCols(psymSchema, pmodel);
		if (nullptr != pdrgpcrAttrs && nullptr != pdrgpcrSchema &&
			CColRef::Equals(pdrgpcrAttrs, pdrgpcrSchema))
		{
			CColRefArray *pdrgpcrLeftAttrs = 4 == pop->Pdrgpsym()->Size()
				? PdrgpcrResolveCols(PsymResolve((*pop->Pdrgpsym())[2]), pmodel)
				: pdrgpcrAttrs;
			CColRefArray *pdrgpcrRightAttrs = 4 == pop->Pdrgpsym()->Size()
				? PdrgpcrResolveCols(PsymResolve((*pop->Pdrgpsym())[3]), pmodel)
				: pdrgpcrAttrs;
			rgpdrgpcrInput[0] = nullptr == pdrgpcrLeftAttrs
				? nullptr
				: PdrgpcrMapToTarget((*pop)[0], pexprLeft,
								 pdrgpcrLeftAttrs, pmodel);
			rgpdrgpcrInput[1] = nullptr == pdrgpcrRightAttrs
				? nullptr
				: PdrgpcrMapToTarget((*pop)[1], pexprRight,
								 pdrgpcrRightAttrs, pmodel);
			BOOL valid = nullptr != rgpdrgpcrInput[0] && nullptr != rgpdrgpcrInput[1];
			for (ULONG child = 0; valid && child < 2; ++child)
			{
				valid = rgpdrgpcrInput[child]->Size() == pdrgpcrAttrs->Size();
				for (ULONG col = 0; valid && col < pdrgpcrAttrs->Size(); ++col)
				{
					CColRef *input = (*rgpdrgpcrInput[child])[col];
					CColRef *output = (*pdrgpcrAttrs)[col];
					valid = input->RetrieveType()->MDId()->Equals(output->RetrieveType()->MDId()) &&
						input->TypeModifier() == output->TypeModifier();
					// Non-first inputs cannot own a SetOp output identity.
					for (ULONG i = 0; valid && child == 1 && i < pdrgpcrAttrs->Size(); ++i)
						valid = input != (*pdrgpcrAttrs)[i];
				}
			}
			if (valid)
			{
				pdrgpcrOutput = pdrgpcrAttrs;
				fOwnsInputMappings = true;
			}
			else
			{
				CRefCount::SafeRelease(rgpdrgpcrInput[0]);
				CRefCount::SafeRelease(rgpdrgpcrInput[1]);
			}
		}
	}

	if (nullptr == pdrgpcrOutput)
	{
		pexprLeft->Release();
		pexprRight->Release();
		return nullptr;
	}

	// The optional Union output symbols describe the complete ordered set-op
	// row.  Do not silently accept a target declaration that resolves to some
	// other columns: that would make the textual rule stronger than the
	// instantiated expression.  Legacy Union(...) rules have no such contract.
	if (2 <= pop->Pdrgpsym()->Size())
	{
		const CDSLSymbol *psymAttrs = PsymResolve((*pop->Pdrgpsym())[0]);
		const CDSLSymbol *psymSchema = PsymResolve((*pop->Pdrgpsym())[1]);
		CColRefArray *pdrgpcrAttrs =
			PdrgpcrResolveCols(psymAttrs, pmodel);
		CColRefArray *pdrgpcrSchema =
			PdrgpcrResolveCols(psymSchema, pmodel);
		if (nullptr == pdrgpcrAttrs || nullptr == pdrgpcrSchema ||
			!CColRef::Equals(pdrgpcrAttrs, pdrgpcrOutput) ||
			!CColRef::Equals(pdrgpcrSchema, pdrgpcrOutput))
		{
			pexprLeft->Release();
			pexprRight->Release();
			return nullptr;
		}
	}

	// Moving a memo-derived aggregate by deep column remapping recreates its
	// Local/Global split as fresh groups, losing the native xform provenance that
	// prevents incompatible aggregate xforms from running again. Identity-shaped
	// Union rules (including the real corpus Proj* rules) need no copy and remain
	// supported; conservatively reject only a branch move across a GbAgg subtree.
	for (ULONG ulTarget = 0; ulTarget < 2; ulTarget++)
	{
		if (rgulSourceForTarget[ulTarget] != ulTarget &&
			FContainsGbAgg(rgpexprTarget[ulTarget]))
		{
			pexprLeft->Release();
			pexprRight->Release();
			return nullptr;
		}
	}

	for (ULONG ulTarget = 0; ulTarget < 2; ulTarget++)
	{
		ULONG ulSource = rgulSourceForTarget[ulTarget];
		rgpexprTarget[ulTarget] = PexprRemapSetOpChild(
			m_mp, rgpexprTarget[ulTarget], rgpdrgpcrInput[ulSource],
			rgpdrgpcrInput[ulTarget]);
	}
	pexprLeft = rgpexprTarget[0];
	pexprRight = rgpexprTarget[1];

	pdrgpcrOutput->AddRef();
	CColRef2dArray *pdrgpdrgpcrInput =
		GPOS_NEW(m_mp) CColRef2dArray(m_mp);
	CExpressionArray *pdrgpexprChildren =
		GPOS_NEW(m_mp) CExpressionArray(m_mp);
	for (ULONG ul = 0; ul < 2; ul++)
	{
		CExpression *pexprChild = rgpexprTarget[ul];
		// Re-expand the associative n-ary view used by the matcher. This keeps
		// the target alternative identical in shape to native Union2UnionAll,
		// rather than leaving an artificial binary UnionAll spine in the memo.
		if (!pop->FDistinct() && pmodel->FIsNaryUnionTail(pexprChild) &&
			COperator::EopLogicalUnionAll == pexprChild->Pop()->Eopid())
		{
			CLogicalSetOp *popChild =
				CLogicalSetOp::PopConvert(pexprChild->Pop());
			CColRef2dArray *pdrgpdrgpcrChild =
				popChild->PdrgpdrgpcrInput();
			if (CColRef::Equals(popChild->PdrgpcrOutput(),
								rgpdrgpcrInput[ul]) &&
				pexprChild->Arity() == pdrgpdrgpcrChild->Size())
			{
				for (ULONG ulChild = 0; ulChild < pexprChild->Arity();
					 ulChild++)
				{
					CColRefArray *pdrgpcrChild =
						(*pdrgpdrgpcrChild)[ulChild];
					pdrgpcrChild->AddRef();
					pdrgpdrgpcrInput->Append(pdrgpcrChild);
					(*pexprChild)[ulChild]->AddRef();
					pdrgpexprChildren->Append((*pexprChild)[ulChild]);
				}
				pexprChild->Release();
				continue;
			}
		}
		if (!fOwnsInputMappings)
		{
			rgpdrgpcrInput[ul]->AddRef();
		}
		pdrgpdrgpcrInput->Append(rgpdrgpcrInput[ul]);
		pdrgpexprChildren->Append(pexprChild);
	}

	COperator *popSet = nullptr;
	if (EdslopUnion == pop->Edslop())
		popSet = pop->FDistinct()
			? static_cast<COperator *>(GPOS_NEW(m_mp) CLogicalUnion(
				  m_mp, pdrgpcrOutput, pdrgpdrgpcrInput))
			: static_cast<COperator *>(GPOS_NEW(m_mp) CLogicalUnionAll(
				  m_mp, pdrgpcrOutput, pdrgpdrgpcrInput));
	else if (EdslopIntersect == pop->Edslop())
		popSet = pop->FDistinct()
			? static_cast<COperator *>(GPOS_NEW(m_mp) CLogicalIntersect(
				  m_mp, pdrgpcrOutput, pdrgpdrgpcrInput))
			: static_cast<COperator *>(GPOS_NEW(m_mp) CLogicalIntersectAll(
				  m_mp, pdrgpcrOutput, pdrgpdrgpcrInput));
	else if (EdslopExcept == pop->Edslop())
		popSet = pop->FDistinct()
			? static_cast<COperator *>(GPOS_NEW(m_mp) CLogicalDifference(
				  m_mp, pdrgpcrOutput, pdrgpdrgpcrInput))
			: static_cast<COperator *>(GPOS_NEW(m_mp) CLogicalDifferenceAll(
				  m_mp, pdrgpcrOutput, pdrgpdrgpcrInput));
	else
	{
		pdrgpcrOutput->Release();
		pdrgpdrgpcrInput->Release();
		pdrgpexprChildren->Release();
		return nullptr;
	}
	return GPOS_NEW(m_mp) CExpression(m_mp, popSet, pdrgpexprChildren);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PosBuildSort
//---------------------------------------------------------------------------
COrderSpec *
CDSLInstantiator::PosBuildSort(const CDSLOp *pop,
							   const CDSLModel *pmodel,
							   CExpression *pexprChild) const
{
	GPOS_ASSERT(nullptr != pop);
	GPOS_ASSERT(EdslopSort == pop->Edslop());
	if (nullptr == pop->Pdrgpsym() || 1 != pop->Pdrgpsym()->Size())
	{
		return nullptr;
	}
	const CDSLSymbol *symbol = PsymResolve((*pop->Pdrgpsym())[0]);
	if (EdslsortSpec == pop->Edslsort())
	{
		if (EdslsymOrder != symbol->Esymkind())
			return nullptr;
		COrderSpecArray *orders = pmodel->PdrgposOrder(symbol);
		if (nullptr == orders || 1 != orders->Size())
			return nullptr;
		COrderSpec *pos = (*orders)[0];
		CColRefSet *output = pexprChild->DeriveOutputColumns();
		CColRefSet *required = pos->PcrsUsed(m_mp);
		const BOOL valid = output->ContainsAll(required);
		required->Release();
		if (!valid)
			return nullptr;
		pos->AddRef();
		return pos;
	}
	if (EdslsortAsc != pop->Edslsort() &&
		EdslsortDesc != pop->Edslsort())
		return nullptr;

	CColRefArray *pdrgpcr = PdrgpcrResolveCols(symbol, pmodel);
	if (nullptr == pdrgpcr || 0 == pdrgpcr->Size())
	{
		return nullptr;
	}

	CColRefSet *pcrsOutput = pexprChild->DeriveOutputColumns();
	for (ULONG ul = 0; ul < pdrgpcr->Size(); ul++)
	{
		if (!pcrsOutput->FMember((*pdrgpcr)[ul]))
		{
			return nullptr;
		}
	}

	const IMDType::ECmpType ecmpt =
		(EdslsortAsc == pop->Edslsort()) ? IMDType::EcmptL
										 : IMDType::EcmptG;
	const COrderSpec::ENullTreatment ent =
		(EdslsortAsc == pop->Edslsort()) ? COrderSpec::EntLast
										 : COrderSpec::EntFirst;
	COrderSpec *pos = GPOS_NEW(m_mp) COrderSpec(m_mp);
	for (ULONG ul = 0; ul < pdrgpcr->Size(); ul++)
	{
		CColRef *pcr = (*pdrgpcr)[ul];
		IMDId *pmdid = pcr->RetrieveType()->GetMdidForCmpType(ecmpt);
		if (!IMDId::IsValid(pmdid))
		{
			pos->Release();
			return nullptr;
		}
		pmdid->AddRef();
		pos->Append(pmdid, pcr, ent);
	}
	return pos;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildSort
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildSort(const CDSLOp *pop,
								 const CDSLModel *pmodel) const
{
	if (1 != pop->UlChildren())
	{
		return nullptr;
	}
	CExpression *pexprChild = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprChild)
	{
		return nullptr;
	}
	COrderSpec *pos = PosBuildSort(pop, pmodel, pexprChild);
	if (nullptr == pos)
	{
		pexprChild->Release();
		return nullptr;
	}

	return GPOS_NEW(m_mp) CExpression(
		m_mp,
		GPOS_NEW(m_mp) CLogicalLimit(m_mp, pos, true /*global*/,
									 false /*has count*/, false /*top DML*/),
		pexprChild, CUtils::PexprScalarConstInt8(m_mp, 0),
		CUtils::PexprScalarConstInt8(m_mp, 0, true /*is null*/));
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildLimit
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildLimit(const CDSLOp *pop,
								  const CDSLModel *pmodel) const
{
	if (1 != pop->UlChildren() || nullptr == pop->Pdrgpsym() ||
		2 != pop->Pdrgpsym()->Size())
	{
		return nullptr;
	}

	CExpression *pexprCount =
		PexprResolveScalar((*pop->Pdrgpsym())[0], pmodel);
	CExpression *pexprOffset =
		PexprResolveScalar((*pop->Pdrgpsym())[1], pmodel);
	if (nullptr == pexprCount || nullptr == pexprOffset)
	{
		CRefCount::SafeRelease(pexprCount);
		CRefCount::SafeRelease(pexprOffset);
		return nullptr;
	}

	const CDSLOp *popChild = (*pop)[0];
	const CDSLOp *popSort = nullptr;
	CExpression *pexprChild = nullptr;
	if (EdslopSort == popChild->Edslop())
	{
		popSort = popChild;
		if (1 != popSort->UlChildren())
		{
			return nullptr;
		}
		pexprChild = PexprBuild((*popSort)[0], pmodel);
	}
	else
	{
		pexprChild = PexprBuild(popChild, pmodel);
	}
	if (nullptr == pexprChild)
	{
		pexprCount->Release();
		pexprOffset->Release();
		return nullptr;
	}

	COrderSpec *pos = nullptr;
	if (nullptr != popSort)
	{
		pos = PosBuildSort(popSort, pmodel, pexprChild);
	}
	else
	{
		pos = GPOS_NEW(m_mp) COrderSpec(m_mp);
	}
	if (nullptr == pos)
	{
		pexprChild->Release();
		pexprCount->Release();
		pexprOffset->Release();
		return nullptr;
	}

	return GPOS_NEW(m_mp) CExpression(
		m_mp,
		GPOS_NEW(m_mp) CLogicalLimit(m_mp, pos, true /*global*/,
									 !FNullScalarConst(pexprCount),
									 false /*top DML*/),
		pexprChild, pexprOffset, pexprCount);
}

CExpression *
CDSLInstantiator::PexprBuildWindow(const CDSLOp *pop,
								   const CDSLModel *pmodel) const
{
	const BOOL fFrame = EdslopWindowFrame == pop->Edslop();
	if ((!fFrame && EdslopWindowRows != pop->Edslop()) ||
		1 != pop->UlChildren() || nullptr == pop->Pdrgpsym() ||
		(fFrame ? 4 : 3) != pop->Pdrgpsym()->Size())
	{
		return nullptr;
	}

	const ULONG ulWindow = fFrame ? 3 : 2;
	const CDSLSymbol *psymWindow =
		PsymResolve((*pop->Pdrgpsym())[ulWindow]);
	CExpression *pexprProjectList = pmodel->PexprWindow(psymWindow);
	CExpression *pexprCarrier = pmodel->PexprWindowCarrier(psymWindow);
	const CDSLSymbol *psymCarrier = psymWindow;
	for (ULONG ulDepth = 0;
		 nullptr == pexprCarrier && nullptr != m_prule &&
		 ulDepth < m_prule->Pexprdefs()->UlDefinitions(); ulDepth++)
	{
		const CDSLSymbol *psymPredecessor = nullptr;
		for (ULONG ul = 0; ul < m_prule->Pexprdefs()->UlDefinitions(); ul++)
		{
			const CDSLExpressionDefinitions::CDefinition *pdef =
				m_prule->Pexprdefs()->PdefAt(ul);
			if (0 < pdef->Arity() && pdef->PsymOperand(0) == psymCarrier)
			{
				psymPredecessor = pdef->PsymOutput();
				break;
			}
		}
		if (nullptr == psymPredecessor ||
			EdslsymWindow != psymPredecessor->Esymkind())
		{
			break;
		}
		psymCarrier = psymPredecessor;
		pexprCarrier = pmodel->PexprWindowCarrier(psymCarrier);
	}
	if (nullptr == pexprCarrier ||
		nullptr == pexprProjectList ||
		COperator::EopLogicalSequenceProject !=
			pexprCarrier->Pop()->Eopid() ||
		2 != pexprCarrier->Arity())
	{
		return nullptr;
	}

	CLogicalSequenceProject *popSource =
		CLogicalSequenceProject::PopConvert(pexprCarrier->Pop());
	if (fFrame != popSource->FHasFrameSpecs())
	{
		return nullptr;
	}
	CExpression *pexprChild = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprChild)
	{
		return nullptr;
	}

	// Reusing an opaque window shell is valid only while every input column it
	// references remains available. A future operator that remaps columns must
	// provide an explicit remapping contract instead of silently producing a
	// dangling SequenceProject.
	CColRefSet *pcrsOutput = pexprChild->DeriveOutputColumns();
	CColRefSet *pcrsProject = pexprProjectList->DeriveUsedColumns();
	if (!pcrsOutput->ContainsAll(popSource->PcrsLocalUsed()) ||
		!pcrsOutput->ContainsAll(pcrsProject))
	{
		pexprChild->Release();
		return nullptr;
	}

	popSource->Pds()->AddRef();
	popSource->Pdrgpos()->AddRef();
	popSource->Pdrgpwf()->AddRef();
	pexprProjectList->AddRef();
	return CUtils::PexprLogicalSequenceProject(
		m_mp, popSource->Pspt(), popSource->Pds(), popSource->Pdrgpos(),
		popSource->Pdrgpwf(), pexprChild, pexprProjectList);
}

CExpression *
CDSLInstantiator::PexprBuildRowNumber(const CDSLOp *pop,
									  const CDSLModel *pmodel) const
{
	if (EdslopRowNumber != pop->Edslop() || 1 != pop->UlChildren() ||
		nullptr == pop->Pdrgpsym() || 3 != pop->Pdrgpsym()->Size())
	{
		return nullptr;
	}
	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	CColRefArray *pdrgpcrPartition =
		PdrgpcrResolveCols((*pdrgpsym)[0], pmodel);
	COrderSpecArray *pdrgpos =
		pmodel->PdrgposOrder(PsymResolve((*pdrgpsym)[1]));
	CColRefArray *pdrgpcrRank =
		pmodel->PdrgpcrRank(PsymResolve((*pdrgpsym)[2]));
	if (nullptr == pdrgpcrPartition || nullptr == pdrgpos ||
		nullptr == pdrgpcrRank || 1 != pdrgpcrRank->Size())
	{
		return nullptr;
	}
	CExpression *pexprChild = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprChild)
	{
		return nullptr;
	}
	CExpression *pexpr = CXformUtils::PexprWindowWithRowNumber(
		m_mp, pexprChild, pdrgpcrPartition, (*pdrgpcrRank)[0], pdrgpos);
	pexprChild->Release();
	return pexpr;
}

CExpression *
CDSLInstantiator::PexprBuildAssertMaxOneRow(const CDSLOp *pop,
										const CDSLModel *pmodel) const
{
	if (EdslopAssertMaxOneRow != pop->Edslop() ||
		1 != pop->UlChildren() || nullptr == pop->Pdrgpsym() ||
		0 != pop->Pdrgpsym()->Size())
	{
		return nullptr;
	}
	CExpression *pexprChild = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprChild)
	{
		return nullptr;
	}
	CExpression *pexprAssert =
		CXformUtils::PexprAssertOneRow(m_mp, pexprChild);
	pexprChild->Release();
	return pexprAssert;
}
