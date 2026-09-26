//---------------------------------------------------------------------------
// MONSOON DSL rule engine — CDSLInstantiatorColumns.cpp
// Column identity, projection remapping and comparison repair shared by builders.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLInstantiator.h"
#include "CDSLInstantiatorUtils.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogicalSetOp.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarProjectList.h"

using namespace gpopt;
using namespace gpopt::dslinstantiator;

namespace gpopt
{
namespace dslinstantiator
{
CColRefArray *
PdrgpcrLiveOutput(CMemoryPool *mp, CExpression *pexpr)
{
	CColRefArray *pdrgpcr = GPOS_NEW(mp) CColRefArray(mp);
	CColRefSetIter iter(*pexpr->DeriveOutputColumns());
	while (iter.Advance())
	{
		CColRef *pcr = iter.Pcr();
		if (CColRef::EUsed == pcr->GetUsage(true, true))
		{
			pdrgpcr->Append(pcr);
		}
	}
	return pdrgpcr;
}

BOOL
FColSetContainsArray(const CColRefSet *pcrs,
					 const CColRefArray *pdrgpcr)
{
	for (ULONG ul = 0; ul < pdrgpcr->Size(); ul++)
	{
		if (!pcrs->FMember((*pdrgpcr)[ul]))
		{
			return false;
		}
	}
	return true;
}

// Remap predicate dependencies without detaching unchanged subquery inputs
// from Memo. Deep-copying a Global(Local(...)) input loses its xform history
// and lets native splitting add another Local stage on every DSL rewrite.
CExpression *
PexprRemapPredicate(CMemoryPool *mp, CExpression *pexpr,
				   UlongToColRefMap *mapping)
{
	GPOS_CHECK_STACK_SIZE;
	CExpressionArray *children = GPOS_NEW(mp) CExpressionArray(mp);
	BOOL unchanged = true;
	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		CExpression *child = PexprRemapPredicate(mp, (*pexpr)[ul], mapping);
		unchanged = unchanged && child == (*pexpr)[ul];
		children->Append(child);
	}
	COperator *pop = pexpr->Pop()->PopCopyWithRemappedColumns(
		mp, mapping, false /*must_exist*/);
	if (unchanged && pop->Matches(pexpr->Pop()))
	{
		pop->Release();
		children->Release();
		pexpr->AddRef();
		return pexpr;
	}
	return GPOS_NEW(mp) CExpression(mp, pop, children);
}

// Re-resolve comparison operators after a column remap changes operand types.
// PexprCopyWithRemappedColumns replaces CScalarIdent nodes but intentionally
// preserves the original CScalarCmp operator mdid, which is invalid for (for
// example) an int4 join key remapped to an int8 key. CUtils selects the target
// comparison operator and inserts casts using the active metadata accessor.
CExpression *
PexprRebuildComparisons(CMemoryPool *mp, CExpression *pexpr)
{
	// An unchanged relational input retained by PexprRemapPredicate already
	// has valid operators. Keep its Memo identity during type repair as well.
	if (nullptr != pexpr->Pgexpr())
	{
		pexpr->AddRef();
		return pexpr;
	}
	if (COperator::EopScalarCmp == pexpr->Pop()->Eopid())
	{
		if (2 != pexpr->Arity())
		{
			return nullptr;
		}
		const IMDType::ECmpType ecmpt =
			CScalarCmp::PopConvert(pexpr->Pop())->ParseCmpType();
		if (IMDType::EcmptOther <= ecmpt)
		{
			return nullptr;
		}
		CExpression *pexprLeft = PexprRebuildComparisons(mp, (*pexpr)[0]);
		if (nullptr == pexprLeft)
		{
			return nullptr;
		}
		CExpression *pexprRight = PexprRebuildComparisons(mp, (*pexpr)[1]);
		if (nullptr == pexprRight)
		{
			pexprLeft->Release();
			return nullptr;
		}
		return CUtils::PexprScalarCmp(mp, pexprLeft, pexprRight, ecmpt);
	}

	CExpressionArray *pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		CExpression *pexprChild =
			PexprRebuildComparisons(mp, (*pexpr)[ul]);
		if (nullptr == pexprChild)
		{
			pdrgpexprChildren->Release();
			return nullptr;
		}
		pdrgpexprChildren->Append(pexprChild);
	}
	pexpr->Pop()->AddRef();
	return GPOS_NEW(mp) CExpression(mp, pexpr->Pop(), pdrgpexprChildren);
}
}  // namespace dslinstantiator
}  // namespace gpopt

const CDSLOp *
CDSLInstantiator::PopSourceFilterForPredicate(
	const CDSLOp *pop, const CDSLSymbol *psymPred) const
{
	if (EdslopFilter == pop->Edslop() && nullptr != pop->Pdrgpsym() &&
		(2 == pop->Pdrgpsym()->Size() || 3 == pop->Pdrgpsym()->Size()) &&
		(*pop->Pdrgpsym())[0] == psymPred)
	{
		return pop;
	}
	for (ULONG ul = 0; ul < pop->UlChildren(); ul++)
	{
		const CDSLOp *popFound =
			PopSourceFilterForPredicate((*pop)[ul], psymPred);
		if (nullptr != popFound)
		{
			return popFound;
		}
	}
	return nullptr;
}

const CDSLOp *
CDSLInstantiator::PopSourceProjForSchema(
	const CDSLOp *pop, const CDSLSymbol *psymSchema) const
{
	if (EdslopProj == pop->Edslop() && nullptr != pop->Pdrgpsym() &&
		(2 == pop->Pdrgpsym()->Size() || 3 == pop->Pdrgpsym()->Size()) &&
		(*pop->Pdrgpsym())[1] == psymSchema)
	{
		return pop;
	}
	for (ULONG ul = 0; ul < pop->UlChildren(); ul++)
	{
		const CDSLOp *popFound =
			PopSourceProjForSchema((*pop)[ul], psymSchema);
		if (nullptr != popFound)
		{
			return popFound;
		}
	}
	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprRemapProjectList
//
//	@doc:
//		Copy the exact scalar expressions captured from a source Proj, remapping
//		their referenced columns from the source attrs vector to the target attrs
//		vector. Keeping this operation independent of plain Proj versus Proj*
//		preserves expression identity for projection and projection-dedup targets.
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprRemapProjectList(
	const CDSLSymbol *psymTargetAttrs, const CDSLSymbol *psymSchema,
	const CDSLModel *pmodel) const
{
	CExpression *pexprProjList = pmodel->PexprProjList(psymSchema);
	if (nullptr == pexprProjList)
	{
		return nullptr;
	}

	const CDSLOp *popSourceProj = PopSourceProjForSchema(
		m_prule->PfragSrc()->PopRoot(), psymSchema);
	if (nullptr == popSourceProj || nullptr == popSourceProj->Pdrgpsym() ||
		2 != popSourceProj->Pdrgpsym()->Size())
	{
		return nullptr;
	}

	CColRefArray *pdrgpcrSourceAttrs = PdrgpcrResolveCols(
		(*popSourceProj->Pdrgpsym())[0], pmodel);
	CColRefArray *pdrgpcrTargetAttrs =
		PdrgpcrResolveCols(psymTargetAttrs, pmodel);
	if (nullptr == pdrgpcrSourceAttrs || nullptr == pdrgpcrTargetAttrs ||
		pdrgpcrSourceAttrs->Size() != pdrgpcrTargetAttrs->Size())
	{
		return nullptr;
	}

	UlongToColRefMap *colref_mapping = GPOS_NEW(m_mp) UlongToColRefMap(m_mp);
	BOOL fNeedsRemap = false;
	for (ULONG ul = 0; ul < pdrgpcrSourceAttrs->Size(); ul++)
	{
		CColRef *pcrSource = (*pdrgpcrSourceAttrs)[ul];
		CColRef *pcrTarget = (*pdrgpcrTargetAttrs)[ul];
		if (pcrSource == pcrTarget)
		{
			continue;
		}
		if (!pcrSource->RetrieveType()->MDId()->Equals(
				pcrTarget->RetrieveType()->MDId()) ||
			pcrSource->TypeModifier() != pcrTarget->TypeModifier())
		{
			colref_mapping->Release();
			return nullptr;
		}
		const ULONG ulSourceId = pcrSource->Id();
		CColRef *pcrExisting = colref_mapping->Find(&ulSourceId);
		if (nullptr != pcrExisting)
		{
			if (pcrExisting != pcrTarget)
			{
				colref_mapping->Release();
				return nullptr;
			}
			continue;
		}
		BOOL fInserted GPOS_ASSERTS_ONLY = colref_mapping->Insert(
			GPOS_NEW(m_mp) ULONG(pcrSource->Id()), pcrTarget);
		GPOS_ASSERT(fInserted);
		fNeedsRemap = true;
	}

	CExpression *pexprTargetProjList = nullptr;
	if (!fNeedsRemap)
	{
		pexprProjList->AddRef();
		pexprTargetProjList = pexprProjList;
	}
	else
	{
		CExpressionArray *pdrgpexprTargetElems =
			GPOS_NEW(m_mp) CExpressionArray(m_mp);
		for (ULONG ul = 0; ul < pexprProjList->Arity(); ul++)
		{
			CExpression *pexprSourceElem = (*pexprProjList)[ul];
			if (COperator::EopScalarProjectElement !=
					pexprSourceElem->Pop()->Eopid() ||
				1 != pexprSourceElem->Arity())
			{
				pdrgpexprTargetElems->Release();
				colref_mapping->Release();
				return nullptr;
			}
			CExpression *pexprScalar =
				PexprRemapPredicate(m_mp, (*pexprSourceElem)[0], colref_mapping);
			pexprSourceElem->Pop()->AddRef();
			pdrgpexprTargetElems->Append(GPOS_NEW(m_mp) CExpression(
				m_mp, pexprSourceElem->Pop(), pexprScalar));
		}
		pexprTargetProjList = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CScalarProjectList(m_mp),
			pdrgpexprTargetElems);
	}
	colref_mapping->Release();
	return pexprTargetProjList;
}

CColRef *
CDSLInstantiator::PcrMapToTarget(const CDSLOp *popTarget,
								 CExpression *pexprTarget,
								 CColRef *pcrSource,
								 const CDSLModel *pmodel) const
{
	if (pexprTarget->DeriveOutputColumns()->FMember(pcrSource))
	{
		return pcrSource;
	}

	if (EdslopInput == popTarget->Edslop())
	{
		auto input_map = m_input_col_maps.find(popTarget);
		if (input_map != m_input_col_maps.end())
		{
			ULONG ulSourceId = pcrSource->Id();
			CColRef *pcrMapped = input_map->second->Find(&ulSourceId);
			if (nullptr != pcrMapped)
			{
				return pcrMapped;
			}
		}
		const CDSLSymbol *psymTable =
			PsymResolve((*popTarget->Pdrgpsym())[0]);
		CExpression *pexprSource = pmodel->PexprTable(psymTable);
		CColRefArray *pdrgpcrSource =
			pexprSource->DeriveOutputColumns()->Pdrgpcr(m_mp);
		CColRefArray *pdrgpcrTarget =
			pexprTarget->DeriveOutputColumns()->Pdrgpcr(m_mp);
		CColRef *pcrMapped = nullptr;
		if (pdrgpcrSource->Size() == pdrgpcrTarget->Size())
		{
			for (ULONG ul = 0; ul < pdrgpcrSource->Size(); ul++)
			{
				if ((*pdrgpcrSource)[ul] == pcrSource)
				{
					pcrMapped = (*pdrgpcrTarget)[ul];
					break;
				}
			}
		}
		pdrgpcrTarget->Release();
		pdrgpcrSource->Release();
		if (nullptr != pcrMapped)
		{
			return pcrMapped;
		}
	}

	for (ULONG ul = 0;
		 ul < popTarget->UlChildren() && ul < pexprTarget->Arity(); ul++)
	{
		CColRef *pcrMapped = PcrMapToTarget(
			(*popTarget)[ul], (*pexprTarget)[ul], pcrSource, pmodel);
		if (nullptr != pcrMapped)
		{
			return pcrMapped;
		}
	}

	// A source SetOp output column denotes the column at the same position in
	// every input. Follow that positional edge and ask which candidate is
	// produced by this target subtree. Skip identity edges (the first input is
	// commonly also the output identity) to avoid a trivial recursion cycle.
	CExpressionArray *pdrgpexprBindings = pmodel->PdrgpexprUnionBindings();
	for (ULONG ulBinding = 0;
		 nullptr != pdrgpexprBindings &&
		 ulBinding < pdrgpexprBindings->Size();
		 ulBinding++)
	{
		CLogicalSetOp *popSet = CLogicalSetOp::PopConvert(
			(*pdrgpexprBindings)[ulBinding]->Pop());
		CColRefArray *pdrgpcrOutput = popSet->PdrgpcrOutput();
		ULONG ulPos = pdrgpcrOutput->Size();
		for (ULONG ul = 0; ul < pdrgpcrOutput->Size(); ul++)
		{
			if ((*pdrgpcrOutput)[ul] == pcrSource)
			{
				ulPos = ul;
				break;
			}
		}
		if (ulPos == pdrgpcrOutput->Size())
		{
			continue;
		}
		CColRef2dArray *pdrgpdrgpcrInput = popSet->PdrgpdrgpcrInput();
		for (ULONG ulInput = 0; ulInput < pdrgpdrgpcrInput->Size();
			 ulInput++)
		{
			CColRefArray *pdrgpcrInput = (*pdrgpdrgpcrInput)[ulInput];
			if (ulPos >= pdrgpcrInput->Size() ||
				(*pdrgpcrInput)[ulPos] == pcrSource)
			{
				continue;
			}
			CColRef *pcrMapped = PcrMapToTarget(
				popTarget, pexprTarget, (*pdrgpcrInput)[ulPos], pmodel);
			if (nullptr != pcrMapped)
			{
				return pcrMapped;
			}
		}
	}
	return nullptr;
}

CColRefArray *
CDSLInstantiator::PdrgpcrMapToTarget(
	const CDSLOp *popTarget, CExpression *pexprTarget,
	const CColRefArray *pdrgpcrSource, const CDSLModel *pmodel) const
{
	CColRefArray *pdrgpcrMapped = GPOS_NEW(m_mp) CColRefArray(m_mp);
	for (ULONG ul = 0; ul < pdrgpcrSource->Size(); ul++)
	{
		CColRef *pcrMapped = PcrMapToTarget(
			popTarget, pexprTarget, (*pdrgpcrSource)[ul], pmodel);
		if (nullptr == pcrMapped ||
			!(*pdrgpcrSource)[ul]->RetrieveType()->MDId()->Equals(
				pcrMapped->RetrieveType()->MDId()))
		{
			pdrgpcrMapped->Release();
			return nullptr;
		}
		pdrgpcrMapped->Append(pcrMapped);
	}
	return pdrgpcrMapped;
}

CExpression *
CDSLInstantiator::PexprRemapPredicateToChildren(
	const CDSLOp *popLeft, CExpression *pexprLeft,
	const CDSLOp *popRight, CExpression *pexprRight,
	CExpression *pexprSourcePred, const CDSLModel *pmodel) const
{
	if (nullptr == pexprSourcePred)
	{
		return nullptr;
	}

	UlongToColRefMap *phmPred = GPOS_NEW(m_mp) UlongToColRefMap(m_mp);
	CColRefArray *pdrgpcrUsed =
		pexprSourcePred->DeriveUsedColumns()->Pdrgpcr(m_mp);
	BOOL fRemapPred = false;
	for (ULONG ul = 0; ul < pdrgpcrUsed->Size(); ul++)
	{
		CColRef *pcrSource = (*pdrgpcrUsed)[ul];
		if (pexprLeft->DeriveOutputColumns()->FMember(pcrSource) ||
			pexprRight->DeriveOutputColumns()->FMember(pcrSource))
		{
			continue;
		}
		CColRef *pcrLeft =
			PcrMapToTarget(popLeft, pexprLeft, pcrSource, pmodel);
		CColRef *pcrRight =
			PcrMapToTarget(popRight, pexprRight, pcrSource, pmodel);
		if ((nullptr == pcrLeft) == (nullptr == pcrRight))
		{
			pdrgpcrUsed->Release();
			phmPred->Release();
			return nullptr;
		}
		CColRef *pcrTarget = nullptr != pcrLeft ? pcrLeft : pcrRight;
		BOOL fInserted GPOS_ASSERTS_ONLY = phmPred->Insert(
			GPOS_NEW(m_mp) ULONG(pcrSource->Id()), pcrTarget);
		GPOS_ASSERT(fInserted);
		fRemapPred = true;
	}
	pdrgpcrUsed->Release();

	CExpression *pexprTargetPred = nullptr;
	if (fRemapPred)
	{
		CExpression *pexprCopied =
			PexprRemapPredicate(m_mp, pexprSourcePred, phmPred);
		pexprTargetPred = PexprRebuildComparisons(m_mp, pexprCopied);
		pexprCopied->Release();
	}
	else
	{
		pexprSourcePred->AddRef();
		pexprTargetPred = pexprSourcePred;
	}
	phmPred->Release();

	CColRefSet *pcrsAvailable = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsAvailable->Union(pexprLeft->DeriveOutputColumns());
	pcrsAvailable->Union(pexprRight->DeriveOutputColumns());
	const BOOL fAvailable = nullptr != pexprTargetPred &&
		pcrsAvailable->ContainsAll(pexprTargetPred->DeriveUsedColumns());
	pcrsAvailable->Release();
	if (!fAvailable)
	{
		CRefCount::SafeRelease(pexprTargetPred);
		return nullptr;
	}
	return pexprTargetPred;
}
