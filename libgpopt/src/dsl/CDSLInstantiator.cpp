//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLInstantiator.cpp
//
//	@doc:
//		Implementation of the target builder (see CDSLInstantiator.h). Migrates
//		the SEMANTICS of WeTune Instantiation for structural operators.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLInstantiator.h"

#include "gpos/base.h"
#include "gpos/common/CAutoRef.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/dsl/CDSLExprListUtils.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalConstTableGet.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalJoin.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CLogicalLeftAntiSemiApply.h"
#include "gpopt/operators/CLogicalLeftAntiSemiApplyNotIn.h"
#include "gpopt/operators/CLogicalLeftAntiSemiCorrelatedApplyNotIn.h"
#include "gpopt/operators/CLogicalLeftSemiApply.h"
#include "gpopt/operators/CLogicalLeftSemiApplyIn.h"
#include "gpopt/operators/CLogicalLeftSemiCorrelatedApplyIn.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CLogicalSetOp.h"
#include "gpopt/operators/CLogicalUnion.h"
#include "gpopt/operators/CLogicalUnionAll.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarAggFunc.h"
#include "gpopt/operators/CScalarConst.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarValuesList.h"
#include "naucrates/md/IMDScalarOp.h"

using namespace gpopt;

namespace
{
BOOL
FAggNameEquals(CMemoryPool *mp, const CWStringConst *pstrActual,
			   const CHAR *szExpected)
{
	CWStringConst strExpected(mp, szExpected);
	return pstrActual->Equals(&strExpected);
}

BOOL
FAggFuncMatches(CMemoryPool *mp, const CDSLOp *popAgg,
				const CScalarAggFunc *popFunc)
{
	if (popAgg->FDistinct() != popFunc->IsDistinct())
	{
		return false;
	}
	switch (popAgg->Edslaggfunc())
	{
		case EdslaggfuncUnknown:
			return true;
		case EdslaggfuncSentinel:
			return false;
		case EdslaggfuncAverage:
			return FAggNameEquals(mp, popFunc->PstrAggFunc(), "avg") ||
				   FAggNameEquals(mp, popFunc->PstrAggFunc(), "average");
		default:
			return FAggNameEquals(
				mp, popFunc->PstrAggFunc(),
				CDSLOpKindTable::SzAggFuncName(popAgg->Edslaggfunc()));
	}
}

BOOL
FColArraysSameSet(CMemoryPool *mp, const CColRefArray *pdrgpcrFirst,
				  const CColRefArray *pdrgpcrSecond)
{
	CColRefSet *pcrsFirst = GPOS_NEW(mp) CColRefSet(mp);
	CColRefSet *pcrsSecond = GPOS_NEW(mp) CColRefSet(mp);
	pcrsFirst->Include(const_cast<CColRefArray *>(pdrgpcrFirst));
	pcrsSecond->Include(const_cast<CColRefArray *>(pdrgpcrSecond));
	BOOL fEqual = pcrsFirst->Equals(pcrsSecond);
	pcrsFirst->Release();
	pcrsSecond->Release();
	return fEqual;
}

// A project list made exclusively of column references can be deduplicated on
// those referenced columns directly. Besides avoiding a redundant Project, this
// preserves the dependency columns that existing parents may still require.
// Any non-leaf scalar must instead be evaluated before deduplication.
BOOL
FProjectListIsColumnOnly(const CExpression *pexprProjectList)
{
	if (nullptr == pexprProjectList ||
		COperator::EopScalarProjectList != pexprProjectList->Pop()->Eopid())
	{
		return false;
	}
	for (ULONG ul = 0; ul < pexprProjectList->Arity(); ul++)
	{
		CExpression *pexprElem = (*pexprProjectList)[ul];
		if (COperator::EopScalarProjectElement != pexprElem->Pop()->Eopid() ||
			1 != pexprElem->Arity() ||
			COperator::EopScalarIdent != (*pexprElem)[0]->Pop()->Eopid())
		{
			return false;
		}
	}
	return true;
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

// Re-resolve comparison operators after a column remap changes operand types.
// PexprCopyWithRemappedColumns replaces CScalarIdent nodes but intentionally
// preserves the original CScalarCmp operator mdid, which is invalid for (for
// example) an int4 join key remapped to an int8 key. CUtils selects the target
// comparison operator and inserts casts using the active metadata accessor.
CExpression *
PexprRebuildComparisons(CMemoryPool *mp, CExpression *pexpr)
{
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

// Find the only source InSub node whose predicate was bound by the matcher.
// A direct target-to-source AttrsEq lookup is preferred by the caller; this
// conservative fallback supports proven rewrites which move IN across an
// equality join and therefore name the other join-key attrs on the target.
const CDSLOp *
PopOnlyBoundInSub(const CDSLOp *pop, const CDSLModel *pmodel,
				  ULONG *pulMatches)
{
	const CDSLOp *popFound = nullptr;
	if (EdslopInSubFilter == pop->Edslop() && nullptr != pop->Pdrgpsym() &&
		(1 == pop->Pdrgpsym()->Size() || 5 == pop->Pdrgpsym()->Size()) &&
		nullptr != pmodel->PexprInSubPred((*pop->Pdrgpsym())[0]))
	{
		(*pulMatches)++;
		popFound = pop;
	}
	for (ULONG ul = 0; ul < pop->UlChildren(); ul++)
	{
		const CDSLOp *popChild =
			PopOnlyBoundInSub((*pop)[ul], pmodel, pulMatches);
		if (nullptr != popChild)
		{
			popFound = popChild;
		}
	}
	return popFound;
}

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
	CExpression *pexprRemapped = pexprPred->PexprCopyWithRemappedColumns(
		mp, phm, false /*must_exist*/);
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

// ORCA represents ALL as an anti-apply over rows that violate the original
// comparison. Build that internal predicate from metadata instead of assuming
// a particular comparison family (for example, equality/inequality).
CExpression *
PexprInverseComparison(CMemoryPool *mp, CExpression *pexprCmp)
{
	if (nullptr == pexprCmp ||
		COperator::EopScalarCmp != pexprCmp->Pop()->Eopid() ||
		2 != pexprCmp->Arity())
	{
		return nullptr;
	}
	CScalarCmp *popCmp = CScalarCmp::PopConvert(pexprCmp->Pop());
	CMDAccessor *pmda = COptCtxt::PoctxtFromTLS()->Pmda();
	IMDId *pmdidInverse =
		pmda->RetrieveScOp(popCmp->MdIdOp())->GetInverseOpMdid();
	if (!IMDId::IsValid(pmdidInverse))
	{
		return nullptr;
	}
	const CWStringConst *pstrInverse =
		pmda->RetrieveScOp(pmdidInverse)->Mdname().GetMDName();
	(*pexprCmp)[0]->AddRef();
	(*pexprCmp)[1]->AddRef();
	pmdidInverse->AddRef();
	return CUtils::PexprScalarCmp(mp, (*pexprCmp)[0], (*pexprCmp)[1],
								 *pstrInverse, pmdidInverse);
}

// Copy a recorded LogicalLimit chain while replacing its deepest relational
// child. Operators retain their exact order specs/global flags and scalar
// offset/count expressions; the recorded source tree itself is never mutated.
CExpression *
PexprRestoreLimitShell(CMemoryPool *mp, CExpression *pexprShell,
					   CExpression *pexprChild)
{
	GPOS_ASSERT(COperator::EopLogicalLimit == pexprShell->Pop()->Eopid());
	GPOS_ASSERT(3 == pexprShell->Arity());

	CExpression *pexprRestoredChild = nullptr;
	if (COperator::EopLogicalLimit == (*pexprShell)[0]->Pop()->Eopid() &&
		3 == (*pexprShell)[0]->Arity())
	{
		pexprRestoredChild =
			PexprRestoreLimitShell(mp, (*pexprShell)[0], pexprChild);
	}
	else
	{
		pexprChild->AddRef();
		pexprRestoredChild = pexprChild;
	}
	pexprShell->Pop()->AddRef();
	(*pexprShell)[1]->AddRef();
	(*pexprShell)[2]->AddRef();
	return GPOS_NEW(mp) CExpression(mp, pexprShell->Pop(),
									pexprRestoredChild, (*pexprShell)[1],
									(*pexprShell)[2]);
}

// Remap only the scalar inputs of a project list. Project-element operators
// define the shell's stable output schema and must not be substituted merely
// because an equivalent input column is selected by the target rule.
CExpression *
PexprRemapProjectListInputs(CMemoryPool *mp, CExpression *pexprList,
							 UlongToColRefMap *colref_mapping)
{
	if (COperator::EopScalarProjectList != pexprList->Pop()->Eopid())
	{
		return nullptr;
	}

	CExpressionArray *pdrgpexprElems = GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < pexprList->Arity(); ul++)
	{
		CExpression *pexprElem = (*pexprList)[ul];
		if (COperator::EopScalarProjectElement !=
				pexprElem->Pop()->Eopid() ||
			1 != pexprElem->Arity())
		{
			pdrgpexprElems->Release();
			return nullptr;
		}
		CExpression *pexprScalar =
			(*pexprElem)[0]->PexprCopyWithRemappedColumns(
				mp, colref_mapping, false /*must_exist*/);
		pexprElem->Pop()->AddRef();
		pdrgpexprElems->Append(GPOS_NEW(mp) CExpression(
			mp, pexprElem->Pop(), pexprScalar));
	}
	return GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprElems);
}

// A GbAgg grouping CColRef is both an input identity and an output identity.
// Replacing it in the operator would leak the target-side column into the
// parent memo group. When a rewritten child no longer produces a grouping
// column, define that stable source identity from its mapped target column in a
// projection below the aggregate instead. The projection is omitted when all
// grouping identities remain available.
CExpression *
PexprEnsureAggGroupingColumns(CMemoryPool *mp, CExpression *pexprChild,
							  CColRefArray *pdrgpcrGrouping,
							  UlongToColRefMap *colref_mapping)
{
	CColRefSet *pcrsChild = pexprChild->DeriveOutputColumns();
	CExpressionArray *pdrgpexprElems = GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < pdrgpcrGrouping->Size(); ul++)
	{
		CColRef *pcrSource = (*pdrgpcrGrouping)[ul];
		if (pcrsChild->FMember(pcrSource))
		{
			continue;
		}

		const ULONG ulSourceId = pcrSource->Id();
		CColRef *pcrTarget = colref_mapping->Find(&ulSourceId);
		if (nullptr == pcrTarget || !pcrsChild->FMember(pcrTarget) ||
			!pcrSource->RetrieveType()->MDId()->Equals(
				pcrTarget->RetrieveType()->MDId()) ||
			pcrSource->TypeModifier() != pcrTarget->TypeModifier())
		{
			pdrgpexprElems->Release();
			return nullptr;
		}

		CExpression *pexprIdent = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarIdent(mp, pcrTarget));
		pdrgpexprElems->Append(GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarProjectElement(mp, pcrSource),
			pexprIdent));
	}

	if (0 == pdrgpexprElems->Size())
	{
		pdrgpexprElems->Release();
		pexprChild->AddRef();
		return pexprChild;
	}

	CExpression *pexprList = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprElems);
	pexprChild->AddRef();
	return GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalProject(mp), pexprChild, pexprList);
}

// Restore Project(GbAgg(...)) around a rewritten deepest input. Aggregate
// scalar arguments follow the target attrs, while grouping and project-element
// output identities remain the source schema exposed to parent operators.
CExpression *
PexprRestoreProjectAggShell(CMemoryPool *mp, CExpression *pexprShell,
							CExpression *pexprChild,
							UlongToColRefMap *colref_mapping)
{
	const COperator::EOperatorId eopid = pexprShell->Pop()->Eopid();
	GPOS_ASSERT(COperator::EopLogicalProject == eopid ||
				COperator::EopLogicalGbAgg == eopid);
	GPOS_ASSERT(2 == pexprShell->Arity());

	CExpression *pexprRestoredChild = nullptr;
	if (COperator::EopLogicalGbAgg == (*pexprShell)[0]->Pop()->Eopid())
	{
		pexprRestoredChild = PexprRestoreProjectAggShell(
			mp, (*pexprShell)[0], pexprChild, colref_mapping);
	}
	else
	{
		pexprChild->AddRef();
		pexprRestoredChild = pexprChild;
	}
	if (nullptr == pexprRestoredChild)
	{
		return nullptr;
	}

	COperator *popRestored = nullptr;
	if (COperator::EopLogicalGbAgg == eopid)
	{
		CLogicalGbAgg *popGbAgg =
			CLogicalGbAgg::PopConvert(pexprShell->Pop());
		CExpression *pexprGroupingChild = PexprEnsureAggGroupingColumns(
			mp, pexprRestoredChild, popGbAgg->Pdrgpcr(), colref_mapping);
		pexprRestoredChild->Release();
		pexprRestoredChild = pexprGroupingChild;
		if (nullptr == pexprRestoredChild)
		{
			return nullptr;
		}
		pexprShell->Pop()->AddRef();
		popRestored = pexprShell->Pop();
	}
	else
	{
		pexprShell->Pop()->AddRef();
		popRestored = pexprShell->Pop();
	}

	CExpression *pexprList = nullptr;
	// A restored aggregate exposes its stable source identities again, so an
	// outer Project should keep using its original scalar inputs when they are
	// available. Aggregate arguments, by contrast, consume the rewritten child
	// and must follow the target-side mapping.
	if (COperator::EopLogicalProject == eopid &&
		pexprRestoredChild->DeriveOutputColumns()->ContainsAll(
			(*pexprShell)[1]->DeriveUsedColumns()))
	{
		(*pexprShell)[1]->AddRef();
		pexprList = (*pexprShell)[1];
	}
	else
	{
		pexprList = PexprRemapProjectListInputs(
			mp, (*pexprShell)[1], colref_mapping);
	}
	if (nullptr == pexprList ||
		!pexprRestoredChild->DeriveOutputColumns()->ContainsAll(
			pexprList->DeriveUsedColumns()))
	{
		CRefCount::SafeRelease(pexprList);
		popRestored->Release();
		pexprRestoredChild->Release();
		return nullptr;
	}
	return GPOS_NEW(mp) CExpression(mp, popRestored,
									pexprRestoredChild, pexprList);
}

// Rebuild a matched GbAgg while removing only aggregate DISTINCT semantics.
// The relational child, grouping metadata, output columns, ordinary/direct
// arguments and ORDER BY metadata are preserved; the DISTINCT flag and its
// sort-group metadata are cleared on freshly built aggregate operators.
CExpression *
PexprWithoutDistinctAgg(CMemoryPool *mp, CExpression *pexprAgg)
{
	GPOS_ASSERT(COperator::EopLogicalGbAgg == pexprAgg->Pop()->Eopid());
	CLogicalGbAgg *popGbAgg = CLogicalGbAgg::PopConvert(pexprAgg->Pop());
	CExpression *pexprOldList = (*pexprAgg)[1];
	CExpressionArray *pdrgpexprNewElems =
		GPOS_NEW(mp) CExpressionArray(mp);

	for (ULONG ul = 0; ul < pexprOldList->Arity(); ul++)
	{
		CExpression *pexprOldElem = (*pexprOldList)[ul];
		CExpression *pexprOldFunc = (*pexprOldElem)[0];
		CScalarAggFunc *popOldFunc =
			CScalarAggFunc::PopConvert(pexprOldFunc->Pop());
		if (!popOldFunc->IsDistinct())
		{
			pexprOldElem->AddRef();
			pdrgpexprNewElems->Append(pexprOldElem);
			continue;
		}

		popOldFunc->MDId()->AddRef();
		popOldFunc->GetArgTypes()->AddRef();
		IMDId *pmdidResolved = nullptr;
		if (popOldFunc->FHasAmbiguousReturnType())
		{
			pmdidResolved = popOldFunc->MdidType();
			pmdidResolved->AddRef();
		}
		CScalarAggFunc *popNewFunc = CUtils::PopAggFunc(
			mp, popOldFunc->MDId(),
			GPOS_NEW(mp)
				CWStringConst(mp, popOldFunc->PstrAggFunc()->GetBuffer()),
			false /*is_distinct*/, popOldFunc->Eaggfuncstage(),
			popOldFunc->FSplit(), pmdidResolved, popOldFunc->AggKind(),
			popOldFunc->GetArgTypes(), popOldFunc->FRepSafe(),
			popOldFunc->IsAggStar());
		CExpressionArray *pdrgpexprArgs = GPOS_NEW(mp) CExpressionArray(mp);
		for (ULONG ulArg = 0; ulArg < pexprOldFunc->Arity(); ulArg++)
		{
			if (EaggfuncIndexDistinct == ulArg)
			{
				pdrgpexprArgs->Append(GPOS_NEW(mp) CExpression(
					mp, GPOS_NEW(mp) CScalarValuesList(mp),
					GPOS_NEW(mp) CExpressionArray(mp)));
				continue;
			}
			CExpression *pexprArg = (*pexprOldFunc)[ulArg];
			pexprArg->AddRef();
			pdrgpexprArgs->Append(pexprArg);
		}
		CExpression *pexprNewFunc =
			GPOS_NEW(mp) CExpression(mp, popNewFunc, pdrgpexprArgs);
		pexprOldElem->Pop()->AddRef();
		pdrgpexprNewElems->Append(GPOS_NEW(mp) CExpression(
			mp, pexprOldElem->Pop(), pexprNewFunc));
	}

	CExpression *pexprNewList = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprNewElems);
	// The adapter only matches the original user-level global aggregate. Keep
	// its constructor form and stage: passing a null minimal-group array to the
	// split-aggregate overload would silently turn it into the full group set.
	GPOS_ASSERT(nullptr == popGbAgg->PdrgpcrMinimal());
	popGbAgg->Pdrgpcr()->AddRef();
	CColRefArray *pdrgpcrArgDQA = popGbAgg->PdrgpcrArgDQA();
	if (nullptr != pdrgpcrArgDQA)
	{
		pdrgpcrArgDQA->AddRef();
	}
	CLogicalGbAgg *popNewAgg = GPOS_NEW(mp) CLogicalGbAgg(
		mp, popGbAgg->Pdrgpcr(), popGbAgg->Egbaggtype(),
		popGbAgg->FGeneratesDuplicates(), pdrgpcrArgDQA,
		popGbAgg->AggStage());
	(*pexprAgg)[0]->AddRef();
	return GPOS_NEW(mp) CExpression(mp, popNewAgg, (*pexprAgg)[0],
									pexprNewList);
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

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::CDSLInstantiator
//---------------------------------------------------------------------------
CDSLInstantiator::CDSLInstantiator(CMemoryPool *mp)
	: m_mp(mp),
	  m_phmAlias(nullptr),
	  m_phmDerivedCols(nullptr),
	  m_prule(nullptr),
	  m_pdrgpsymBuiltInputs(nullptr)
{
	GPOS_ASSERT(nullptr != mp);
	m_phmAlias = GPOS_NEW(mp) CDSLSymbolAliasMap(mp);
	m_phmDerivedCols = GPOS_NEW(mp) CDSLSymbolToRefMap(mp);
	m_pdrgpsymBuiltInputs = GPOS_NEW(mp) CDSLSymbolArray(mp);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::~CDSLInstantiator
//---------------------------------------------------------------------------
CDSLInstantiator::~CDSLInstantiator()
{
	m_pdrgpsymBuiltInputs->Release();
	m_phmDerivedCols->Release();
	m_phmAlias->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::BuildAliasMap
//
//	@doc:
//		From each equality constraint *Eq(x,y), link the target-side symbol to the
//		source-side symbol whose binding it reuses. Both symbols share a kind
//		(the DSL guarantees *Eq relates same-kind symbols), so we only need the
//		side to orient the alias.
//---------------------------------------------------------------------------
void
CDSLInstantiator::BuildAliasMap(const CDSLRule *prule)
{
	CDSLConstraintArray *pdrgpcon = prule->Pdrgpcon();
	const ULONG ulCon = pdrgpcon->Size();
	for (ULONG ul = 0; ul < ulCon; ul++)
	{
		const CDSLConstraint *pcon = (*pdrgpcon)[ul];
		switch (pcon->Edslcon())
		{
			case EdslconTableEq:
			case EdslconAttrsEq:
			case EdslconPredicateEq:
			case EdslconSchemaEq:
			case EdslconFuncEq:
			case EdslconScalarEq:
			case EdslconExprListEq:
				break;	// an aliasing equality
			default:
				continue;  // structural constraint: not an alias
		}

		CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
		if (2 != pdrgpsym->Size())
		{
			continue;
		}
		CDSLSymbol *psym0 = (*pdrgpsym)[0];
		CDSLSymbol *psym1 = (*pdrgpsym)[1];

		// orient: target-side symbol aliases the source-side symbol.
		CDSLSymbol *psymTgt = nullptr;
		CDSLSymbol *psymSrc = nullptr;
		if (EdslsideTarget == psym0->Eside() &&
			EdslsideSource == psym1->Eside())
		{
			psymTgt = psym0;
			psymSrc = psym1;
		}
		else if (EdslsideSource == psym0->Eside() &&
				 EdslsideTarget == psym1->Eside())
		{
			psymTgt = psym1;
			psymSrc = psym0;
		}
		else
		{
			// both same side (e.g. two source symbols in one class): no target
			// alias to record here.
			continue;
		}

		// first alias wins (a target symbol may appear in several *Eq; any source
		// representative of its class is fine since they are all co-bound).
		if (nullptr == m_phmAlias->Find(psymTgt))
		{
			BOOL fOk = m_phmAlias->Insert(psymTgt, psymSrc);
			GPOS_ASSERT(fOk);
			(void) fOk;
		}
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PsymResolve
//---------------------------------------------------------------------------
const CDSLSymbol *
CDSLInstantiator::PsymResolve(const CDSLSymbol *psym) const
{
	CDSLSymbol *psymSrc = m_phmAlias->Find(psym);
	return (nullptr != psymSrc) ? psymSrc : psym;
}

CExpression *
CDSLInstantiator::PexprResolveScalar(const CDSLSymbol *psym,
									const CDSLModel *pmodel) const
{
	if (nullptr == psym || EdslsymScalar != psym->Esymkind())
	{
		return nullptr;
	}
	const CDSLSymbol *psymResolved = PsymResolve(psym);
	CExpression *pexpr = pmodel->PexprScalar(psymResolved);
	if (nullptr != pexpr)
	{
		pexpr->AddRef();
		return pexpr;
	}
	if (nullptr == m_prule)
	{
		return nullptr;
	}

	CDSLConstraintArray *pdrgpcon = m_prule->Pdrgpcon();
	for (ULONG ul = 0; ul < pdrgpcon->Size(); ul++)
	{
		const CDSLConstraint *pcon = (*pdrgpcon)[ul];
		if ((EdslconScalarOne != pcon->Edslcon() &&
			 EdslconScalarZero != pcon->Edslcon()) ||
			1 != pcon->Pdrgpsym()->Size() || (*pcon->Pdrgpsym())[0] != psym)
		{
			continue;
		}
		return CUtils::PexprScalarConstInt8(
			m_mp, EdslconScalarOne == pcon->Edslcon() ? 1 : 0);
	}
	return nullptr;
}

CExpression *
CDSLInstantiator::PexprResolvePredicate(const CDSLSymbol *psym,
									   const CDSLModel *pmodel,
									   ULONG ulDepth) const
{
	if (nullptr == psym || nullptr == m_prule ||
		EdslsymPred != psym->Esymkind() ||
		ulDepth > m_prule->Pdrgpcon()->Size())
	{
		return nullptr;
	}
	psym = PsymResolve(psym);
	CExpression *pexprBound = pmodel->PexprPred(psym);
	if (nullptr != pexprBound)
	{
		pexprBound->AddRef();
		return pexprBound;
	}

	const CDSLConstraint *pconDefinition = nullptr;
	CDSLConstraintArray *pdrgpcon = m_prule->Pdrgpcon();
	for (ULONG ul = 0; ul < pdrgpcon->Size(); ul++)
	{
		const CDSLConstraint *pcon = (*pdrgpcon)[ul];
		if (EdslconPredicateAnd != pcon->Edslcon() ||
			3 != pcon->Pdrgpsym()->Size() || (*pcon->Pdrgpsym())[0] != psym)
		{
			continue;
		}
		if (nullptr != pconDefinition)
		{
			return nullptr;
		}
		pconDefinition = pcon;
	}
	if (nullptr == pconDefinition)
	{
		return nullptr;
	}

	CExpression *pexprLeft = PexprResolvePredicate(
		(*pconDefinition->Pdrgpsym())[1], pmodel, ulDepth + 1);
	CExpression *pexprRight = PexprResolvePredicate(
		(*pconDefinition->Pdrgpsym())[2], pmodel, ulDepth + 1);
	if (nullptr == pexprLeft || nullptr == pexprRight)
	{
		CRefCount::SafeRelease(pexprLeft);
		CRefCount::SafeRelease(pexprRight);
		return nullptr;
	}
	CExpression *pexprResult =
		CPredicateUtils::PexprConjunction(m_mp, pexprLeft, pexprRight);
	pexprLeft->Release();
	pexprRight->Release();
	return pexprResult;
}

CColRefArray *
CDSLInstantiator::PdrgpcrResolveCols(const CDSLSymbol *psym,
									const CDSLModel *pmodel,
									ULONG ulDepth) const
{
	if (nullptr == psym || nullptr == m_prule ||
		ulDepth > m_prule->Pdrgpcon()->Size())
	{
		return nullptr;
	}
	psym = PsymResolve(psym);
	if (EdslsymAttrs != psym->Esymkind() &&
		EdslsymSchema != psym->Esymkind())
	{
		return nullptr;
	}

	CRefCount *pval = pmodel->PvalLookup(psym);
	if (nullptr != pval)
	{
		return dynamic_cast<CColRefArray *>(pval);
	}
	CRefCount *pvalDerived = m_phmDerivedCols->Find(psym);
	if (nullptr != pvalDerived)
	{
		return dynamic_cast<CColRefArray *>(pvalDerived);
	}

	const CDSLConstraint *pconDef = nullptr;
	BOOL fEmptyDef = false;
	CDSLConstraintArray *pdrgpcon = m_prule->Pdrgpcon();
	for (ULONG ul = 0; ul < pdrgpcon->Size(); ul++)
	{
		const CDSLConstraint *pcon = (*pdrgpcon)[ul];
		if (EdslconAttrsEmpty == pcon->Edslcon() &&
			1 == pcon->Pdrgpsym()->Size() && (*pcon->Pdrgpsym())[0] == psym)
		{
			if (fEmptyDef || nullptr != pconDef ||
				EdslsymAttrs != psym->Esymkind())
			{
				return nullptr;
			}
			fEmptyDef = true;
			continue;
		}
		if (EdslconAttrsIntersect == pcon->Edslcon() &&
			3 == pcon->Pdrgpsym()->Size() &&
			(*pcon->Pdrgpsym())[0] == psym)
		{
			if (fEmptyDef || nullptr != pconDef)
			{
				return nullptr;
			}
			pconDef = pcon;
		}
	}
	if (fEmptyDef)
	{
		CColRefArray *pdrgpcrEmpty = GPOS_NEW(m_mp) CColRefArray(m_mp);
		if (!m_phmDerivedCols->Insert(const_cast<CDSLSymbol *>(psym),
									 pdrgpcrEmpty))
		{
			pdrgpcrEmpty->Release();
			return nullptr;
		}
		return pdrgpcrEmpty;
	}
	if (nullptr == pconDef)
	{
		return nullptr;
	}

	const CDSLSymbol *psymInput = (*pconDef->Pdrgpsym())[1];
	const CDSLSymbol *psymDomain =
		PsymResolve((*pconDef->Pdrgpsym())[2]);
	if (psymInput->Esymkind() != psym->Esymkind())
	{
		return nullptr;
	}
	CColRefArray *pdrgpcrInput =
		PdrgpcrResolveCols(psymInput, pmodel, ulDepth + 1);
	if (nullptr == pdrgpcrInput)
	{
		return nullptr;
	}

	CColRefSet *pcrsDomain = GPOS_NEW(m_mp) CColRefSet(m_mp);
	if (EdslsymTable == psymDomain->Esymkind())
	{
		CExpression *pexprDomain = pmodel->PexprTable(psymDomain);
		if (nullptr == pexprDomain)
		{
			pcrsDomain->Release();
			return nullptr;
		}
		pcrsDomain->Include(pexprDomain->DeriveOutputColumns());
	}
	else if (EdslsymAttrs == psymDomain->Esymkind() ||
			 EdslsymSchema == psymDomain->Esymkind())
	{
		CColRefArray *pdrgpcrDomain =
			PdrgpcrResolveCols(psymDomain, pmodel, ulDepth + 1);
		if (nullptr == pdrgpcrDomain)
		{
			pcrsDomain->Release();
			return nullptr;
		}
		pcrsDomain->Include(pdrgpcrDomain);
	}
	else
	{
		pcrsDomain->Release();
		return nullptr;
	}

	CColRefArray *pdrgpcrResult = GPOS_NEW(m_mp) CColRefArray(m_mp);
	for (ULONG ul = 0; ul < pdrgpcrInput->Size(); ul++)
	{
		CColRef *pcr = (*pdrgpcrInput)[ul];
		if (pcrsDomain->FMember(pcr))
		{
			pdrgpcrResult->Append(pcr);
		}
	}
	pcrsDomain->Release();
	if (!m_phmDerivedCols->Insert(const_cast<CDSLSymbol *>(psym),
								 pdrgpcrResult))
	{
		pdrgpcrResult->Release();
		return nullptr;
	}
	return pdrgpcrResult;
}

CExpression *
CDSLInstantiator::PexprResolveExpr(const CDSLSymbol *psym,
								   const CDSLModel *pmodel,
								   ULONG ulDepth) const
{
	if (nullptr == psym || EdslsymExpr != psym->Esymkind() ||
		ulDepth > m_prule->Pdrgpcon()->Size())
	{
		return nullptr;
	}
	psym = PsymResolve(psym);
	CExpression *pexprBound = pmodel->PexprExpr(psym);
	if (nullptr != pexprBound)
	{
		pexprBound->AddRef();
		return pexprBound;
	}

	const CDSLConstraint *pconDef = nullptr;
	ULONG ulDefOutput = 0;
	CDSLConstraintArray *pdrgpcon = m_prule->Pdrgpcon();
	for (ULONG ul = 0; ul < pdrgpcon->Size(); ul++)
	{
		const CDSLConstraint *pcon = (*pdrgpcon)[ul];
		ULONG ulOutput = gpos::ulong_max;
		if (EdslconExprConcat == pcon->Edslcon() &&
			3 == pcon->Pdrgpsym()->Size() &&
			(*pcon->Pdrgpsym())[0] == psym)
		{
			ulOutput = 0;
		}
		else if (EdslconExprSplit == pcon->Edslcon() &&
				 4 == pcon->Pdrgpsym()->Size())
		{
			if ((*pcon->Pdrgpsym())[0] == psym)
			{
				ulOutput = 0;
			}
			else if ((*pcon->Pdrgpsym())[1] == psym)
			{
				ulOutput = 1;
			}
		}
		if (gpos::ulong_max == ulOutput)
		{
			continue;
		}
		if (nullptr != pconDef)
		{
			return nullptr;  // ambiguous derived definition
		}
		pconDef = pcon;
		ulDefOutput = ulOutput;
	}
	if (nullptr == pconDef)
	{
		return nullptr;
	}

	if (EdslconExprSplit == pconDef->Edslcon())
	{
		CExpression *pexprUpper = PexprResolveExpr(
			(*pconDef->Pdrgpsym())[2], pmodel, ulDepth + 1);
		CExpression *pexprLower = PexprResolveExpr(
			(*pconDef->Pdrgpsym())[3], pmodel, ulDepth + 1);
		CExpression *pexprMerged = nullptr;
		CExpression *pexprResidual = nullptr;
		const BOOL fSplit = CDSLExprListUtils::FSplit(
			m_mp, pexprUpper, pexprLower, &pexprMerged, &pexprResidual);
		CRefCount::SafeRelease(pexprUpper);
		CRefCount::SafeRelease(pexprLower);
		if (!fSplit)
		{
			return nullptr;
		}
		CExpression *pexprResult =
			0 == ulDefOutput ? pexprMerged : pexprResidual;
		(0 == ulDefOutput ? pexprResidual : pexprMerged)->Release();
		return pexprResult;
	}

	CExpression *pexprLeft = PexprResolveExpr(
		(*pconDef->Pdrgpsym())[1], pmodel, ulDepth + 1);
	CExpression *pexprRight = PexprResolveExpr(
		(*pconDef->Pdrgpsym())[2], pmodel, ulDepth + 1);
	CExpression *pexprResult = CDSLExprListUtils::PexprConcat(
		m_mp, pexprLeft, pexprRight);
	CRefCount::SafeRelease(pexprLeft);
	CRefCount::SafeRelease(pexprRight);
	return pexprResult;
}

const CDSLOp *
CDSLInstantiator::PopSourceFilterForPredicate(
	const CDSLOp *pop, const CDSLSymbol *psymPred) const
{
	if (EdslopFilter == pop->Edslop() && nullptr != pop->Pdrgpsym() &&
		2 == pop->Pdrgpsym()->Size() && (*pop->Pdrgpsym())[0] == psymPred)
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
		2 == pop->Pdrgpsym()->Size() && (*pop->Pdrgpsym())[1] == psymSchema)
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
				(*pexprSourceElem)[0]->PexprCopyWithRemappedColumns(
					m_mp, colref_mapping, false /*must_exist*/);
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

CExpression *
CDSLInstantiator::PexprBuildFilterPredicate(
	const CDSLOp *popFilter, const CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != m_prule);
	CDSLSymbolArray *pdrgpsymTarget = popFilter->Pdrgpsym();
	if (nullptr == pdrgpsymTarget || 2 != pdrgpsymTarget->Size())
	{
		return nullptr;
	}

	const CDSLSymbol *psymSourcePred = PsymResolve((*pdrgpsymTarget)[0]);
	CExpression *pexprBound = pmodel->PexprPred(psymSourcePred);
	const CDSLOp *popSourceFilter = PopSourceFilterForPredicate(
		m_prule->PfragSrc()->PopRoot(), psymSourcePred);
	if (nullptr == pexprBound || nullptr == popSourceFilter ||
		nullptr == popSourceFilter->Pdrgpsym() ||
		2 != popSourceFilter->Pdrgpsym()->Size())
	{
		return nullptr;
	}

	const CDSLSymbol *psymSourceAttrs = (*popSourceFilter->Pdrgpsym())[1];
	const CDSLSymbol *psymTargetAttrs = PsymResolve((*pdrgpsymTarget)[1]);
	CColRefArray *pdrgpcrFrom =
		PdrgpcrResolveCols(psymSourceAttrs, pmodel);
	CColRefArray *pdrgpcrTo =
		PdrgpcrResolveCols(psymTargetAttrs, pmodel);
	if (nullptr == pdrgpcrFrom || nullptr == pdrgpcrTo ||
		pdrgpcrFrom->Size() != pdrgpcrTo->Size())
	{
		return nullptr;
	}

	UlongToColRefMap *phm = GPOS_NEW(m_mp) UlongToColRefMap(m_mp);
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
				GPOS_NEW(m_mp) ULONG(pcrFrom->Id()), pcrTo);
			GPOS_ASSERT(fInserted);
			fRemap = true;
		}
	}

	if (!fRemap)
	{
		phm->Release();
		pexprBound->AddRef();
		return pexprBound;
	}
	CExpression *pexprRemapped = pexprBound->PexprCopyWithRemappedColumns(
		m_mp, phm, false /*must_exist*/);
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
//		CDSLInstantiator::PexprBuildInput
//
//	@doc:
//		Input<t>: reuse the relational subtree bound to t (resolved through the
//		alias map). AddRef-graft it into the target.
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildInput(const CDSLOp *pop,
								  const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	if (nullptr == pdrgpsym || 1 != pdrgpsym->Size())
	{
		return nullptr;
	}
	const CDSLSymbol *psymTable = PsymResolve((*pdrgpsym)[0]);
	CExpression *pexpr = pmodel->PexprTable(psymTable);
	if (nullptr == pexpr)
	{
		return nullptr;
	}
	BOOL fAlreadyBuilt = false;
	for (ULONG ul = 0; ul < m_pdrgpsymBuiltInputs->Size(); ul++)
	{
		if ((*m_pdrgpsymBuiltInputs)[ul] == psymTable)
		{
			fAlreadyBuilt = true;
			break;
		}
	}
	if (!fAlreadyBuilt)
	{
		const_cast<CDSLSymbol *>(psymTable)->AddRef();
		m_pdrgpsymBuiltInputs->Append(
			const_cast<CDSLSymbol *>(psymTable));
		pexpr->AddRef();
		return pexpr;
	}

	// A repeated target occurrence denotes another range variable. Reusing the
	// same CColRefs would conflate both occurrences and, inside a SetOp, violate
	// the per-input column identity contract. Copy the complete subtree using a
	// fresh output-column map, as native ORCA distribution xforms do.
	CColRefArray *pdrgpcrFrom =
		pexpr->DeriveOutputColumns()->Pdrgpcr(m_mp);
	UlongToColRefMap *phm = GPOS_NEW(m_mp) UlongToColRefMap(m_mp);
	CColRefArray *pdrgpcrTo =
		CUtils::PdrgpcrCopy(m_mp, pdrgpcrFrom, false, phm);
	CExpression *pexprCopy = pexpr->PexprCopyWithRemappedColumns(
		m_mp, phm, true /*must_exist*/);
	pdrgpcrTo->Release();
	phm->Release();
	pdrgpcrFrom->Release();
	return pexprCopy;
}

CExpression *
CDSLInstantiator::PexprBuildEmpty(const CDSLOp *pop,
								  const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	if (nullptr == pdrgpsym || 1 != pdrgpsym->Size())
	{
		return nullptr;
	}
	const CDSLSymbol *psymTable = PsymResolve((*pdrgpsym)[0]);
	CExpression *pexprSchemaSource = pmodel->PexprTable(psymTable);
	if (nullptr == pexprSchemaSource)
	{
		return nullptr;
	}
	CColRefArray *pdrgpcrOutput =
		pexprSchemaSource->DeriveOutputColumns()->Pdrgpcr(m_mp);
	return GPOS_NEW(m_mp) CExpression(
		m_mp,
		GPOS_NEW(m_mp) CLogicalConstTableGet(
			m_mp, pdrgpcrOutput, GPOS_NEW(m_mp) IDatum2dArray(m_mp)));
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
			pexprSourcePred->PexprCopyWithRemappedColumns(
				m_mp, phmPred, false /*must_exist*/);
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
	CExpressionArray *pdrgpexpr = GPOS_NEW(m_mp) CExpressionArray(m_mp);
	const CDSLOp *popCurrent = pop;
	const CDSLOp *popCarrierFilter = nullptr;
	while (nullptr != popCurrent && EdslopFilter == popCurrent->Edslop())
	{
		CDSLSymbolArray *pdrgpsym = popCurrent->Pdrgpsym();
		if (nullptr == pdrgpsym || 2 != pdrgpsym->Size() ||
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
	// over the rebuilt child. This is the construction-time half of target-side
	// AttrsSub checking.
	for (ULONG ul = 0; ul < pdrgpexpr->Size(); ul++)
	{
		if (!pexprChild->DeriveOutputColumns()->ContainsAll(
				(*pdrgpexpr)[ul]->DeriveUsedColumns()))
		{
			pexprChild->Release();
			pdrgpexpr->Release();
			return nullptr;
		}
	}
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
//		CDSLInstantiator::PexprBuildJoin
//
//	@doc:
//		InnerJoin/LeftJoin/SemiJoin/SemiApply rebuild both relational children and
//		graft the
//		SOURCE-matched predicate, building the join operator the TARGET op names.
//		For Inner/LeftJoin, <p a a> carries a complete predicate without extracted
//		equality keys; SemiJoin always carries the complete predicate, including
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
	const BOOL fValidSymbols = fSemiJoin
		? 3 == ulSymbols
		: (fSemiApply ? 4 == ulSymbols
					  : (2 == ulSymbols || 3 == ulSymbols ||
						 4 == ulSymbols || 5 == ulSymbols ||
						 7 == ulSymbols));
	if (nullptr == pdrgpsym || !fValidSymbols)
	{
		return nullptr;
	}
	const BOOL fPredicateOnly =
		3 == ulSymbols || (fSemiApply && 4 == ulSymbols);
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

	// build the join operator the TARGET names.
	COperator *popJoin = nullptr;
	switch (pop->Edslop())
	{
		case EdslopInnerJoin:
			popJoin = GPOS_NEW(m_mp) CLogicalInnerJoin(m_mp);
			break;
		case EdslopLeftJoin:
			popJoin = GPOS_NEW(m_mp) CLogicalLeftOuterJoin(m_mp);
			break;
		case EdslopSemiJoin:
			popJoin = GPOS_NEW(m_mp) CLogicalLeftSemiJoin(m_mp);
			break;
		case EdslopSemiApply:
			popJoin = GPOS_NEW(m_mp) CLogicalLeftSemiApply(m_mp);
			break;
		default:
			pexprTargetPred->Release();
			pexprLeft->Release();
			pexprRight->Release();
			return nullptr;
	}

	CExpression *pexprResult = GPOS_NEW(m_mp)
		CExpression(m_mp, popJoin, pexprLeft, pexprRight, pexprTargetPred);
	if (!fSemiApply && (4 == ulSymbols || 7 == ulSymbols))
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
	if (fSemiApply)
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
	return pexprResult;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildCompute
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildCompute(const CDSLOp *pop,
								 const CDSLModel *pmodel) const
{
	if (1 != pop->UlChildren() || nullptr == pop->Pdrgpsym() ||
		3 != pop->Pdrgpsym()->Size())
	{
		return nullptr;
	}

	const CDSLSymbol *psymExpr = PsymResolve((*pop->Pdrgpsym())[0]);
	const CDSLSymbol *psymAttrs = PsymResolve((*pop->Pdrgpsym())[1]);
	const CDSLSymbol *psymSchema = PsymResolve((*pop->Pdrgpsym())[2]);
	CExpression *pexprList = PexprResolveExpr(psymExpr, pmodel);
	CColRefArray *pdrgpcrAttrs =
		PdrgpcrResolveCols(psymAttrs, pmodel);
	CColRefArray *pdrgpcrSchema =
		PdrgpcrResolveCols(psymSchema, pmodel);
	BOOL fOwnAttrs = false;
	BOOL fOwnSchema = false;
	if (nullptr != pexprList && nullptr == pdrgpcrAttrs)
	{
		pdrgpcrAttrs = pexprList->DeriveUsedColumns()->Pdrgpcr(m_mp);
		fOwnAttrs = true;
	}
	if (nullptr != pexprList && nullptr == pdrgpcrSchema &&
		COperator::EopScalarProjectList == pexprList->Pop()->Eopid())
	{
		pdrgpcrSchema = GPOS_NEW(m_mp) CColRefArray(m_mp);
		fOwnSchema = true;
		for (ULONG ul = 0; ul < pexprList->Arity(); ul++)
		{
			CExpression *pexprElem = (*pexprList)[ul];
			if (COperator::EopScalarProjectElement !=
				pexprElem->Pop()->Eopid())
			{
				pdrgpcrSchema->Release();
				pdrgpcrSchema = nullptr;
				fOwnSchema = false;
				break;
			}
			pdrgpcrSchema->Append(
				CScalarProjectElement::PopConvert(pexprElem->Pop())->Pcr());
		}
	}
	if (nullptr == pexprList || nullptr == pdrgpcrAttrs ||
		nullptr == pdrgpcrSchema ||
		COperator::EopScalarProjectList != pexprList->Pop()->Eopid() ||
		pexprList->Arity() != pdrgpcrSchema->Size())
	{
		if (fOwnAttrs)
		{
			pdrgpcrAttrs->Release();
		}
		if (fOwnSchema)
		{
			pdrgpcrSchema->Release();
		}
		CRefCount::SafeRelease(pexprList);
		return nullptr;
	}

	// Guard the three independently aliased target symbols against an invalid
	// combination. The expression artifact is authoritative: attrs must be its
	// exact dependency set and schema its ordered list of defined columns.
	CColRefSet *pcrsAttrs = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsAttrs->Include(pdrgpcrAttrs);
	if (!pcrsAttrs->Equals(pexprList->DeriveUsedColumns()))
	{
		pcrsAttrs->Release();
		if (fOwnAttrs)
		{
			pdrgpcrAttrs->Release();
		}
		if (fOwnSchema)
		{
			pdrgpcrSchema->Release();
		}
		pexprList->Release();
		return nullptr;
	}
	pcrsAttrs->Release();
	for (ULONG ul = 0; ul < pexprList->Arity(); ul++)
	{
		CExpression *pexprElem = (*pexprList)[ul];
		if (COperator::EopScalarProjectElement != pexprElem->Pop()->Eopid() ||
			CScalarProjectElement::PopConvert(pexprElem->Pop())->Pcr() !=
				(*pdrgpcrSchema)[ul])
		{
			if (fOwnAttrs)
			{
				pdrgpcrAttrs->Release();
			}
			if (fOwnSchema)
			{
				pdrgpcrSchema->Release();
			}
			pexprList->Release();
			return nullptr;
		}
	}

	CExpression *pexprChild = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprChild)
	{
		if (fOwnAttrs)
		{
			pdrgpcrAttrs->Release();
		}
		if (fOwnSchema)
		{
			pdrgpcrSchema->Release();
		}
		pexprList->Release();
		return nullptr;
	}
	if (!FColSetContainsArray(pexprChild->DeriveOutputColumns(), pdrgpcrAttrs))
	{
		pexprChild->Release();
		if (fOwnAttrs)
		{
			pdrgpcrAttrs->Release();
		}
		if (fOwnSchema)
		{
			pdrgpcrSchema->Release();
		}
		pexprList->Release();
		return nullptr;
	}

	if (fOwnAttrs)
	{
		pdrgpcrAttrs->Release();
	}
	if (fOwnSchema)
	{
		pdrgpcrSchema->Release();
	}
	return GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CLogicalProject(m_mp), pexprChild, pexprList);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildProj
//
//	@doc:
//		Proj<a s>: rebuild the relational child and the SOURCE-matched project list.
//		The list's project-element operators (and therefore schema/output CColRefs)
//		stay unchanged, while scalar children are remapped positionally from the
//		source Proj attrs to the target Proj attrs. This implements proven equality
//		column substitutions such as projecting the other side of an inner-join
//		equality; merely grafting the old list would report "applied" without doing
//		the requested rewrite.
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildProj(const CDSLOp *pop,
								 const CDSLModel *pmodel) const
{
	if (1 != pop->UlChildren() || nullptr == pop->Pdrgpsym() ||
		2 != pop->Pdrgpsym()->Size())
	{
		return nullptr;
	}

	CExpression *pexprChild = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprChild)
	{
		return nullptr;
	}

	const CDSLSymbol *psymAttrs = PsymResolve((*pop->Pdrgpsym())[0]);
	const CDSLSymbol *psymSchema = PsymResolve((*pop->Pdrgpsym())[1]);
	if (pmodel->FVirtualIdentityProj(psymSchema) && !pop->FDistinct())
	{
		CColRefArray *pdrgpcrAttrs =
			PdrgpcrResolveCols(psymAttrs, pmodel);
		CColRefArray *pdrgpcrSchema =
			PdrgpcrResolveCols(psymSchema, pmodel);
		CColRefSet *pcrsRequired = GPOS_NEW(m_mp) CColRefSet(m_mp);
		if (nullptr != pdrgpcrSchema)
		{
			pcrsRequired->Include(pdrgpcrSchema);
		}
		const BOOL fIdentity = nullptr != pdrgpcrAttrs &&
			nullptr != pdrgpcrSchema &&
			CColRef::Equals(pdrgpcrAttrs, pdrgpcrSchema) &&
			pexprChild->DeriveOutputColumns()->ContainsAll(pcrsRequired);
		pcrsRequired->Release();
		if (!fIdentity)
		{
			pexprChild->Release();
			return nullptr;
		}
		return pexprChild;
	}
	CExpression *pexprAggShell = pmodel->PexprProjAggShell(psymSchema);
	if (nullptr != pexprAggShell)
	{
		const CDSLOp *popSourceProj = PopSourceProjForSchema(
			m_prule->PfragSrc()->PopRoot(), psymSchema);
		if (nullptr == popSourceProj || nullptr == popSourceProj->Pdrgpsym() ||
			2 != popSourceProj->Pdrgpsym()->Size())
		{
			pexprChild->Release();
			return nullptr;
		}
		CColRefArray *pdrgpcrSourceAttrs = PdrgpcrResolveCols(
			(*popSourceProj->Pdrgpsym())[0], pmodel);
		CColRefArray *pdrgpcrTargetAttrs =
			PdrgpcrResolveCols(psymAttrs, pmodel);
		if (nullptr == pdrgpcrSourceAttrs ||
			nullptr == pdrgpcrTargetAttrs ||
			pdrgpcrSourceAttrs->Size() != pdrgpcrTargetAttrs->Size())
		{
			pexprChild->Release();
			return nullptr;
		}

		UlongToColRefMap *colref_mapping =
			GPOS_NEW(m_mp) UlongToColRefMap(m_mp);
		CColRefSet *pcrsTarget = GPOS_NEW(m_mp) CColRefSet(m_mp);
		BOOL fValid = true;
		for (ULONG ul = 0; fValid && ul < pdrgpcrSourceAttrs->Size(); ul++)
		{
			CColRef *pcrSource = (*pdrgpcrSourceAttrs)[ul];
			CColRef *pcrTarget = (*pdrgpcrTargetAttrs)[ul];
			pcrsTarget->Include(pcrTarget);
			if (!pcrSource->RetrieveType()->MDId()->Equals(
					pcrTarget->RetrieveType()->MDId()) ||
				pcrSource->TypeModifier() != pcrTarget->TypeModifier())
			{
				fValid = false;
				break;
			}
			if (pcrSource == pcrTarget)
			{
				continue;
			}
			const ULONG ulSourceId = pcrSource->Id();
			CColRef *pcrExisting = colref_mapping->Find(&ulSourceId);
			if (nullptr != pcrExisting)
			{
				fValid = pcrExisting == pcrTarget;
				continue;
			}
			BOOL fInserted GPOS_ASSERTS_ONLY = colref_mapping->Insert(
				GPOS_NEW(m_mp) ULONG(ulSourceId), pcrTarget);
			GPOS_ASSERT(fInserted);
		}
		fValid = fValid &&
			pexprChild->DeriveOutputColumns()->ContainsAll(pcrsTarget);
		pcrsTarget->Release();
		if (!fValid)
		{
			colref_mapping->Release();
			pexprChild->Release();
			return nullptr;
		}

		CExpression *pexprRestored = PexprRestoreProjectAggShell(
			m_mp, pexprAggShell, pexprChild, colref_mapping);
		colref_mapping->Release();
		pexprChild->Release();
		return pexprRestored;
	}
	CExpression *pexprLimitShell =
		pmodel->PexprProjLimitShell(psymSchema);
	if (nullptr != pexprLimitShell)
	{
		CExpression *pexprWrapped =
			PexprRestoreLimitShell(m_mp, pexprLimitShell, pexprChild);
		pexprChild->Release();
		pexprChild = pexprWrapped;
	}

	// Proj* is ORCA's pure-dedup Global GbAgg. Unlike the special root-level
	// elimination rule, a Proj* nested in Union/Join must be rebuilt, not dropped.
	if (pop->FDistinct())
	{
		CColRefArray *pdrgpcrAttrsBound =
			PdrgpcrResolveCols(psymAttrs, pmodel);
		CColRefArray *pdrgpcrSchemaBound =
			PdrgpcrResolveCols(psymSchema, pmodel);
		if (nullptr == pdrgpcrAttrsBound || nullptr == pdrgpcrSchemaBound ||
			0 == pdrgpcrSchemaBound->Size() ||
			pdrgpcrAttrsBound->Size() != pdrgpcrSchemaBound->Size())
		{
			pexprChild->Release();
			return nullptr;
		}

		// A target projection can move from a SetOp output into one of its
		// branches. Resolve that positional edge before checking the concrete
		// child: output CColRefs are commonly the first branch's identities and
		// therefore cannot be used directly in later branches.
		CAutoRef<CColRefArray> aMappedAttrs;
		CAutoRef<CColRefArray> aMappedSchema;
		CColRefArray *pdrgpcrAttrs = pdrgpcrAttrsBound;
		CColRefArray *pdrgpcrSchema = pdrgpcrSchemaBound;
		CExpression *pexprBoundProjectList =
			pmodel->PexprProjList(psymSchema);
		const BOOL fMapSetOpPosition = nullptr == pexprBoundProjectList ||
			FProjectListIsColumnOnly(pexprBoundProjectList);
		if (fMapSetOpPosition &&
			!FColSetContainsArray(pexprChild->DeriveOutputColumns(),
								  pdrgpcrAttrsBound))
		{
			aMappedAttrs = PdrgpcrMapToTarget(
				(*pop)[0], pexprChild, pdrgpcrAttrsBound, pmodel);
			pdrgpcrAttrs = aMappedAttrs.Value();
		}
		if (fMapSetOpPosition &&
			!FColSetContainsArray(pexprChild->DeriveOutputColumns(),
								  pdrgpcrSchemaBound))
		{
			aMappedSchema = PdrgpcrMapToTarget(
				(*pop)[0], pexprChild, pdrgpcrSchemaBound, pmodel);
			pdrgpcrSchema = aMappedSchema.Value();
		}
		if (nullptr == pdrgpcrAttrs || nullptr == pdrgpcrSchema ||
			pdrgpcrAttrs->Size() != pdrgpcrSchema->Size())
		{
			pexprChild->Release();
			return nullptr;
		}

		// Proj* over a matched computed Proj means SELECT DISTINCT e(a), not
		// DISTINCT a. Rebuild the exact captured expression list first and group
		// by its output schema. attrs remain the expression dependencies while the
		// saved scalar tree remains the expression itself.
		CExpression *pexprBoundProjList =
			pmodel->PexprProjList(psymSchema);
		if (nullptr != pexprBoundProjList &&
			!FProjectListIsColumnOnly(pexprBoundProjList))
		{
			CExpression *pexprTargetProjList =
				PexprRemapProjectList(psymAttrs, psymSchema, pmodel);
			if (nullptr == pexprTargetProjList ||
				!pexprChild->DeriveOutputColumns()->ContainsAll(
					pexprTargetProjList->DeriveUsedColumns()))
			{
				CRefCount::SafeRelease(pexprTargetProjList);
				pexprChild->Release();
				return nullptr;
			}

			CExpression *pexprProject = GPOS_NEW(m_mp) CExpression(
				m_mp, GPOS_NEW(m_mp) CLogicalProject(m_mp), pexprChild,
				pexprTargetProjList);
			CColRefSet *pcrsSchema = GPOS_NEW(m_mp) CColRefSet(m_mp);
			pcrsSchema->Include(pdrgpcrSchema);
			const BOOL fSchemaProduced =
				pexprProject->DeriveOutputColumns()->ContainsAll(pcrsSchema);
			pcrsSchema->Release();
			if (!fSchemaProduced)
			{
				pexprProject->Release();
				return nullptr;
			}

			pdrgpcrSchema->AddRef();
			pdrgpcrSchema->AddRef();
			CExpression *pexprEmptyList = GPOS_NEW(m_mp) CExpression(
				m_mp, GPOS_NEW(m_mp) CScalarProjectList(m_mp),
				GPOS_NEW(m_mp) CExpressionArray(m_mp));
			return GPOS_NEW(m_mp) CExpression(
				m_mp,
				GPOS_NEW(m_mp) CLogicalGbAgg(
					m_mp, pdrgpcrSchema, pdrgpcrSchema,
					COperator::EgbaggtypeGlobal,
					false /* fGeneratesDuplicates */,
					nullptr /* pdrgpcrArgDQA */),
				pexprProject, pexprEmptyList);
		}

		CColRefSet *pcrsChild = pexprChild->DeriveOutputColumns();
		CColRefSet *pcrsGrouping = GPOS_NEW(m_mp) CColRefSet(m_mp);
		// The target attrs may deliberately name an equivalent join-key column
		// while SchemaEq keeps the source projection schema. Proj* has no scalar
		// project list in ORCA; its concrete operation is therefore grouping by
		// the resolved attrs. Requiring attrs == schema rejected precisely these
		// proven column-substitution rules before they could enter the memo.
		pcrsGrouping->Include(pdrgpcrAttrs);
		BOOL fValid = pcrsChild->ContainsAll(pcrsGrouping);
		if (!fValid)
		{
			pcrsGrouping->Release();
			pexprChild->Release();
			return nullptr;
		}
		for (ULONG ul = 0; ul < pdrgpcrSchema->Size(); ul++)
		{
			CColRef *pcrOutput = (*pdrgpcrSchema)[ul];
			CColRef *pcrInput = (*pdrgpcrAttrs)[ul];
			if (pcrOutput != pcrInput &&
				(!pcrOutput->RetrieveType()->MDId()->Equals(
					 pcrInput->RetrieveType()->MDId()) ||
				 pcrOutput->TypeModifier() != pcrInput->TypeModifier()))
			{
				pcrsGrouping->Release();
				pexprChild->Release();
				return nullptr;
			}
		}

		// DISTINCT is idempotent. Reuse an existing pure global dedup with the
		// same grouping set instead of manufacturing an indefinitely deep chain
		// when a bottom-up or Cascade rule reaches its own result again.
		if (COperator::EopLogicalGbAgg == pexprChild->Pop()->Eopid() &&
			2 == pexprChild->Arity() && 0 == (*pexprChild)[1]->Arity())
		{
			CLogicalGbAgg *popChildGbAgg =
				CLogicalGbAgg::PopConvert(pexprChild->Pop());
			CColRefSet *pcrsSchema = GPOS_NEW(m_mp) CColRefSet(m_mp);
			pcrsSchema->Include(pdrgpcrSchema);
			const BOOL fSameDedup = popChildGbAgg->FGlobal() &&
				FColArraysSameSet(m_mp, popChildGbAgg->Pdrgpcr(),
								 pdrgpcrAttrs) &&
				pexprChild->DeriveOutputColumns()->ContainsAll(pcrsSchema);
			pcrsSchema->Release();
			if (fSameDedup)
			{
				pcrsGrouping->Release();
				return pexprChild;
			}
		}

		// The rewritten key is already the rule-selected minimal grouping. Keep
		// that provenance on the generated GbAgg so the Proj* source matcher does
		// not consume its own result and alternate equivalent join keys forever.
		pdrgpcrAttrs->AddRef();
		pdrgpcrAttrs->AddRef();
		CExpression *pexprEmptyList = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CScalarProjectList(m_mp),
			GPOS_NEW(m_mp) CExpressionArray(m_mp));
		CExpression *pexprGbAgg = GPOS_NEW(m_mp) CExpression(
			m_mp,
			GPOS_NEW(m_mp) CLogicalGbAgg(
				m_mp, pdrgpcrAttrs, pdrgpcrAttrs,
				COperator::EgbaggtypeGlobal,
				false /* fGeneratesDuplicates */, nullptr /* pdrgpcrArgDQA */),
			pexprChild, pexprEmptyList);

		if (FColArraysSameSet(m_mp, pdrgpcrAttrs, pdrgpcrSchema))
		{
			pcrsGrouping->Release();
			return pexprGbAgg;
		}

		// Restore the source-visible schema after grouping on substituted keys.
		// CLogicalProject is compute-scalar (not column pruning), but that is
		// sufficient: the memo's required columns request the original schema and
		// the substituted grouping columns may remain as harmless extra outputs.
		CExpressionArray *pdrgpexprPrEl =
			GPOS_NEW(m_mp) CExpressionArray(m_mp);
		for (ULONG ul = 0; ul < pdrgpcrSchema->Size(); ul++)
		{
			CColRef *pcrOutput = (*pdrgpcrSchema)[ul];
			CColRef *pcrInput = (*pdrgpcrAttrs)[ul];
			if (pcrOutput == pcrInput || pcrsGrouping->FMember(pcrOutput))
			{
				continue;
			}
			pdrgpexprPrEl->Append(GPOS_NEW(m_mp) CExpression(
				m_mp, GPOS_NEW(m_mp) CScalarProjectElement(m_mp, pcrOutput),
				GPOS_NEW(m_mp) CExpression(
					m_mp, GPOS_NEW(m_mp) CScalarIdent(m_mp, pcrInput))));
		}
		if (0 == pdrgpexprPrEl->Size())
		{
			pcrsGrouping->Release();
			pdrgpexprPrEl->Release();
			return pexprGbAgg;
		}
		pcrsGrouping->Release();
		CExpression *pexprProjectList = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CScalarProjectList(m_mp), pdrgpexprPrEl);
		return GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalProject(m_mp), pexprGbAgg,
			pexprProjectList);
	}

	CExpression *pexprProjList = pmodel->PexprProjList(psymSchema);
	if (nullptr == pexprProjList)
	{
		// A Proj* source is represented by GbAgg and therefore has no matched
		// CScalarProjectList to reuse.  In the common corpus rule
		// Proj*(Input) -> Proj(Input), the target Proj is a pass-through view of
		// the same bound attrs/schema. ORCA has no column-pruning logical Project,
		// so return the child here; PexprInstantiate will apply the memo-safe
		// Select(child, TRUE) dedup-drop shell.
		CColRefArray *pdrgpcrAttrs =
			PdrgpcrResolveCols(psymAttrs, pmodel);
		CColRefArray *pdrgpcrSchema =
			PdrgpcrResolveCols(psymSchema, pmodel);
		if (!pmodel->FDedupDrop() || nullptr == pdrgpcrAttrs ||
			nullptr == pdrgpcrSchema || 0 == pdrgpcrSchema->Size() ||
			!FColArraysSameSet(m_mp, pdrgpcrAttrs, pdrgpcrSchema))
		{
			pexprChild->Release();
			return nullptr;
		}

		CColRefSet *pcrsSchema = GPOS_NEW(m_mp) CColRefSet(m_mp);
		pcrsSchema->Include(pdrgpcrSchema);
		const BOOL fContains =
			pexprChild->DeriveOutputColumns()->ContainsAll(pcrsSchema);
		pcrsSchema->Release();
		if (!fContains)
		{
			pexprChild->Release();
			return nullptr;
		}
		return pexprChild;
	}

	CExpression *pexprTargetProjList =
		PexprRemapProjectList(psymAttrs, psymSchema, pmodel);
	if (nullptr == pexprTargetProjList)
	{
		pexprChild->Release();
		return nullptr;
	}

	if (!pexprChild->DeriveOutputColumns()->ContainsAll(
			pexprTargetProjList->DeriveUsedColumns()))
	{
		pexprTargetProjList->Release();
		pexprChild->Release();
		return nullptr;
	}

	return GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CLogicalProject(m_mp), pexprChild,
		pexprTargetProjList);
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
		(0 != pop->Pdrgpsym()->Size() && 2 != pop->Pdrgpsym()->Size()))
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
		if (2 == pop->Pdrgpsym()->Size())
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
			for (ULONG ulSource = 0; ulSource < 2; ulSource++)
			{
				CColRefArray *pdrgpcrInput =
					(*pdrgpdrgpcrCandidateInput)[ulSource];
				rgfFits[ulTarget][ulSource] =
					pdrgpcrInput->Size() == pdrgpcrCandidateOutput->Size() &&
					FColSetContainsArray(pcrsTarget, pdrgpcrInput);
			}
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
	if (nullptr == pdrgpcrOutput && 2 == pop->Pdrgpsym()->Size())
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
			rgpdrgpcrInput[0] = PdrgpcrMapToTarget(
				(*pop)[0], pexprLeft, pdrgpcrAttrs, pmodel);
			rgpdrgpcrInput[1] = PdrgpcrMapToTarget(
				(*pop)[1], pexprRight, pdrgpcrAttrs, pmodel);
			if (nullptr != rgpdrgpcrInput[0] &&
				nullptr != rgpdrgpcrInput[1])
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
	if (2 == pop->Pdrgpsym()->Size())
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

	COperator *popSet = pop->FDistinct()
		? static_cast<COperator *>(GPOS_NEW(m_mp) CLogicalUnion(
			  m_mp, pdrgpcrOutput, pdrgpdrgpcrInput))
		: static_cast<COperator *>(GPOS_NEW(m_mp) CLogicalUnionAll(
			  m_mp, pdrgpcrOutput, pdrgpdrgpcrInput));
	return GPOS_NEW(m_mp) CExpression(m_mp, popSet, pdrgpexprChildren);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildAgg
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildAgg(const CDSLOp *pop,
								const CDSLModel *pmodel) const
{
	if (1 != pop->UlChildren())
	{
		return nullptr;
	}
	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	if (nullptr == pdrgpsym ||
		(5 != pdrgpsym->Size() && 6 != pdrgpsym->Size()))
	{
		return nullptr;
	}
	const BOOL fLegacy = 5 == pdrgpsym->Size();
	const ULONG ulFunc = fLegacy ? 2 : 3;
	const ULONG ulSchema = fLegacy ? 3 : 4;
	const ULONG ulHaving = fLegacy ? 4 : 5;

	const CDSLSymbol *psymGroup = PsymResolve((*pdrgpsym)[0]);
	const CDSLSymbol *psymAggInputs = PsymResolve((*pdrgpsym)[1]);
	const CDSLSymbol *psymFuncs = PsymResolve((*pdrgpsym)[ulFunc]);
	const CDSLSymbol *psymSchema = PsymResolve((*pdrgpsym)[ulSchema]);
	const CDSLSymbol *psymHaving = PsymResolve((*pdrgpsym)[ulHaving]);

	CColRefArray *pdrgpcrGroup =
		PdrgpcrResolveCols(psymGroup, pmodel);
	CColRefArray *pdrgpcrAggInputs =
		PdrgpcrResolveCols(psymAggInputs, pmodel);
	CExpressionArray *pdrgpexprFuncs = pmodel->PdrgpexprFunc(psymFuncs);
	CColRefArray *pdrgpcrSchema =
		PdrgpcrResolveCols(psymSchema, pmodel);
	CExpression *pexprHaving = pmodel->PexprPred(psymHaving);
	if (nullptr == pdrgpcrGroup || nullptr == pdrgpcrAggInputs ||
		nullptr == pdrgpexprFuncs || nullptr == pdrgpcrSchema ||
		nullptr == pexprHaving)
	{
		return nullptr;
	}

	// The repository's established Agg<a a f s p> format has no explicit
	// aggregate-output symbol. In a GbAgg schema, grouping columns are passed
	// through and every remaining schema column is defined by one aggregate
	// project element, so recover the output array as schema - groupByAttrs.
	CColRefArray *pdrgpcrAggOutputs = nullptr;
	BOOL fOwnAggOutputs = false;
	if (fLegacy)
	{
		fOwnAggOutputs = true;
		pdrgpcrAggOutputs = GPOS_NEW(m_mp) CColRefArray(m_mp);
		CColRefSet *pcrsGroup = GPOS_NEW(m_mp) CColRefSet(m_mp);
		pcrsGroup->Include(pdrgpcrGroup);
		for (ULONG ul = 0; ul < pdrgpcrSchema->Size(); ul++)
		{
			CColRef *pcr = (*pdrgpcrSchema)[ul];
			if (!pcrsGroup->FMember(pcr))
			{
				pdrgpcrAggOutputs->Append(pcr);
			}
		}
		pcrsGroup->Release();
	}
	else
	{
		const CDSLSymbol *psymAggOutputs = PsymResolve((*pdrgpsym)[2]);
		pdrgpcrAggOutputs = PdrgpcrResolveCols(psymAggOutputs, pmodel);
	}
	if (nullptr == pdrgpcrAggOutputs ||
		pdrgpcrAggOutputs->Size() != pdrgpexprFuncs->Size())
	{
		if (fOwnAggOutputs)
		{
			pdrgpcrAggOutputs->Release();
		}
		return nullptr;
	}

	CExpression *pexprChild = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprChild)
	{
		if (fOwnAggOutputs)
		{
			pdrgpcrAggOutputs->Release();
		}
		return nullptr;
	}

	CColRefSet *pcrsChild = pexprChild->DeriveOutputColumns();
	CColRefSet *pcrsGroup = GPOS_NEW(m_mp) CColRefSet(m_mp);
	CColRefSet *pcrsFuncInputs = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsGroup->Include(pdrgpcrGroup);
	for (ULONG ul = 0; ul < pdrgpexprFuncs->Size(); ul++)
	{
		CExpression *pexprFunc = (*pdrgpexprFuncs)[ul];
		if (COperator::EopScalarAggFunc != pexprFunc->Pop()->Eopid() ||
			!FAggFuncMatches(
				m_mp, pop,
				CScalarAggFunc::PopConvert(pexprFunc->Pop())))
		{
			pcrsGroup->Release();
			pcrsFuncInputs->Release();
			pexprChild->Release();
			if (fOwnAggOutputs)
			{
				pdrgpcrAggOutputs->Release();
			}
			return nullptr;
		}
		pcrsFuncInputs->Include(pexprFunc->DeriveUsedColumns());
	}

	CColRefArray *pdrgpcrActualInputs = pcrsFuncInputs->Pdrgpcr(m_mp);
	BOOL fInputsValid = FColArraysSameSet(
		m_mp, pdrgpcrAggInputs, pdrgpcrActualInputs);
	pdrgpcrActualInputs->Release();

	CColRefSet *pcrsExpectedSchema = GPOS_NEW(m_mp) CColRefSet(m_mp);
	CColRefSet *pcrsSchema = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsExpectedSchema->Include(pdrgpcrGroup);
	pcrsExpectedSchema->Include(pdrgpcrAggOutputs);
	pcrsSchema->Include(pdrgpcrSchema);
	BOOL fSchemaValid = pcrsExpectedSchema->Equals(pcrsSchema);

	BOOL fColumnsValid = fInputsValid && fSchemaValid &&
					 pcrsChild->ContainsAll(pcrsGroup) &&
					 pcrsChild->ContainsAll(pcrsFuncInputs);
	pcrsGroup->Release();
	pcrsFuncInputs->Release();
	pcrsExpectedSchema->Release();
	pcrsSchema->Release();
	if (!fColumnsValid)
	{
		pexprChild->Release();
		if (fOwnAggOutputs)
		{
			pdrgpcrAggOutputs->Release();
		}
		return nullptr;
	}

	CExpressionArray *pdrgpexprPrEl =
		GPOS_NEW(m_mp) CExpressionArray(m_mp);
	for (ULONG ul = 0; ul < pdrgpexprFuncs->Size(); ul++)
	{
		CExpression *pexprFunc = (*pdrgpexprFuncs)[ul];
		pexprFunc->AddRef();
		pdrgpexprPrEl->Append(GPOS_NEW(m_mp) CExpression(
			m_mp,
			GPOS_NEW(m_mp) CScalarProjectElement(
				m_mp, (*pdrgpcrAggOutputs)[ul]),
			pexprFunc));
	}
	CExpression *pexprAggList = GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CScalarProjectList(m_mp), pdrgpexprPrEl);

	pdrgpcrGroup->AddRef();
	CExpression *pexprResult = GPOS_NEW(m_mp) CExpression(
		m_mp,
		GPOS_NEW(m_mp) CLogicalGbAgg(m_mp, pdrgpcrGroup,
									 COperator::EgbaggtypeGlobal),
		pexprChild, pexprAggList);

	if (!CUtils::FScalarConstTrue(pexprHaving))
	{
		pexprHaving->AddRef();
		pexprResult = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprResult,
			pexprHaving);
	}
	if (fOwnAggOutputs)
	{
		pdrgpcrAggOutputs->Release();
	}
	return pexprResult;
}

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

	CExpressionArray *pdrgpexprResidual =
		pmodel->PdrgpexprExistsResidual();
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
	return pexprResult;
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
		CExpressionArray *pdrgpexprResidual =
			pmodel->PdrgpexprInSubResidual();
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

	CExpressionArray *pdrgpexprResidual =
		pmodel->PdrgpexprInSubResidual();
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
	const CDSLSymbol *psymTargetAttrs =
		PsymResolve((*pop->Pdrgpsym())[1]);
	CExpression *pexprPredBound = pmodel->PexprPred(psymPred);
	CColRefArray *pdrgpcrTargetAttrs =
		PdrgpcrResolveCols(psymTargetAttrs, pmodel);
	const CDSLOp *popSource = PopSourceQuantifiedForPredicate(
		m_prule->PfragSrc()->PopRoot(), psymPred);
	CColRefArray *pdrgpcrSourceAttrs =
		nullptr == popSource
			? nullptr
			: PdrgpcrResolveCols((*popSource->Pdrgpsym())[1], pmodel);
	if (nullptr == pexprPredBound || nullptr == pdrgpcrSourceAttrs ||
		nullptr == pdrgpcrTargetAttrs)
	{
		return nullptr;
	}

	CExpression *pexprOuter = PexprBuild((*pop)[0], pmodel);
	CExpression *pexprInner = PexprBuild((*pop)[1], pmodel);
	CExpression *pexprPred = PexprRemapInSubPredicate(
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
		pmodel->PexprInSubCarrier((*popSource->Pdrgpsym())[1]);
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
		CExpression *pexprViolation =
			PexprInverseComparison(m_mp, pexprPred);
		pexprPred->Release();
		if (nullptr == pexprViolation)
		{
			pexprOuter->Release();
			pexprInner->Release();
			return nullptr;
		}
		if (fCorrelated)
		{
			pexprResult = CUtils::PexprLogicalApply<
				CLogicalLeftAntiSemiCorrelatedApplyNotIn>(
				m_mp, pexprOuter, pexprInner, pcrInner,
				COperator::EopScalarSubqueryAll, pexprViolation);
		}
		else
		{
			pexprResult =
				CUtils::PexprLogicalApply<CLogicalLeftAntiSemiApplyNotIn>(
					m_mp, pexprOuter, pexprInner, pcrInner,
					COperator::EopScalarSubqueryAll, pexprViolation);
		}
	}

	CExpressionArray *pdrgpexprResidual =
		pmodel->PdrgpexprInSubResidual();
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
	return pexprResult;
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
	if (nullptr == pop->Pdrgpsym() || 1 != pop->Pdrgpsym()->Size() ||
		(EdslsortAsc != pop->Edslsort() &&
		 EdslsortDesc != pop->Edslsort()))
	{
		return nullptr;
	}

	const CDSLSymbol *psymAttrs = PsymResolve((*pop->Pdrgpsym())[0]);
	CColRefArray *pdrgpcr = PdrgpcrResolveCols(psymAttrs, pmodel);
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

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuild
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuild(const CDSLOp *pop, const CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != pop);

	switch (pop->Edslop())
	{
		case EdslopInput:
			return PexprBuildInput(pop, pmodel);
		case EdslopEmpty:
			return PexprBuildEmpty(pop, pmodel);
		case EdslopFilter:
			return PexprBuildFilter(pop, pmodel);
		case EdslopProj:
			return PexprBuildProj(pop, pmodel);
		case EdslopCompute:
			return PexprBuildCompute(pop, pmodel);
		case EdslopAgg:
			return PexprBuildAgg(pop, pmodel);
		case EdslopExists:
		case EdslopNotExists:
			return PexprBuildExists(pop, pmodel);
		case EdslopInSubFilter:
			return PexprBuildInSub(pop, pmodel);
		case EdslopAny:
		case EdslopAll:
			return PexprBuildQuantified(pop, pmodel);
		case EdslopUnion:
			return PexprBuildUnion(pop, pmodel);
		case EdslopSort:
			return PexprBuildSort(pop, pmodel);
		case EdslopLimit:
			return PexprBuildLimit(pop, pmodel);
		case EdslopInnerJoin:
		case EdslopLeftJoin:
		case EdslopSemiJoin:
		case EdslopSemiApply:
			return PexprBuildJoin(pop, pmodel);
		default:
			return nullptr;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprFreshRoot
//
//	@doc:
//		Cascades contract (CEngine::PgroupInsert): an xform result ROOT must be a
//		freshly-built CExpression (Pgexpr()==NULL); a memo-extracted node as root
//		trips the "A valid group is expected" assertion. CHILDREN may freely reuse
//		memo subtrees. Operator-eliminating rules (e.g. Filter(Input<t0>) ->
//		Input<t1>) build a target whose root IS a reused memo subtree, so we must
//		re-root it. Copy only the root operator and keep its memo-bound children.
//		Deep-copying the subtree would make CEngine recursively insert every level
//		again; equivalent-group merging can then turn an ordinary ancestor/child
//		chain into a circular memo dependency. An empty column map preserves the
//		root's CColRefs and therefore its output-column invariant. Fresh-rooted
//		targets (Filter/Join) are returned as-is.
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprFreshRoot(CExpression *pexpr) const
{
	if (nullptr == pexpr || nullptr == pexpr->Pgexpr())
	{
		// already a freshly-built root (or NULL) — nothing to do.
		return pexpr;
	}

	// Re-root via an identity operator remap (empty mapping => colrefs pass
	// through), while grafting the existing memo-bound children unchanged.
	UlongToColRefMap *colref_mapping = GPOS_NEW(m_mp) UlongToColRefMap(m_mp);
	COperator *popFresh = pexpr->Pop()->PopCopyWithRemappedColumns(
		m_mp, colref_mapping, false /*must_exist*/);
	colref_mapping->Release();

	CExpressionArray *pdrgpexprChildren =
		GPOS_NEW(m_mp) CExpressionArray(m_mp, pexpr->Arity());
	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		CExpression *pexprChild = (*pexpr)[ul];
		pexprChild->AddRef();
		pdrgpexprChildren->Append(pexprChild);
	}
	CExpression *pexprFresh = GPOS_NEW(m_mp)
		CExpression(m_mp, popFresh, pdrgpexprChildren);
	pexpr->Release();
	return pexprFresh;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprInstantiate
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprInstantiate(const CDSLRule *prule,
								   const CDSLModel *pmodel)
{
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(nullptr != pmodel);

	m_prule = prule;
	BuildAliasMap(prule);
	const CDSLOp *popSrcRoot = prule->PfragSrc()->PopRoot();
	const CDSLOp *popTgtRoot = prule->PfragTgt()->PopRoot();
	const BOOL fVirtualDqaSource =
		EdslopProj == popSrcRoot->Edslop() && popSrcRoot->FDistinct() &&
		1 == popSrcRoot->UlChildren() &&
		EdslopInput == (*popSrcRoot)[0]->Edslop();
	const BOOL fVirtualDqaTarget =
		EdslopInput == popTgtRoot->Edslop() ||
		(EdslopProj == popTgtRoot->Edslop() && !popTgtRoot->FDistinct() &&
		 1 == popTgtRoot->UlChildren() &&
		 EdslopInput == (*popTgtRoot)[0]->Edslop());
	CExpression *pexprTgt = nullptr;
	if (nullptr != pmodel->PexprDistinctAgg())
	{
		// A DQA GbAgg is the ORCA representation of the *outer* WeTune
		// Agg(Proj*) pair. Only a rule replacing that virtual source root can be
		// reconstructed without inventing or discarding the surrounding Agg.
		if (!fVirtualDqaSource || !fVirtualDqaTarget)
		{
			return nullptr;
		}
		pexprTgt = PexprWithoutDistinctAgg(
			m_mp, pmodel->PexprDistinctAgg());
	}
	else
	{
		pexprTgt = PexprBuild(popTgtRoot, pmodel);
	}

	// EXISTS/IN are represented before decorrelation as one conjunct of a
	// CLogicalSelect. Their matchers retain every sibling conjunct. When the
	// target keeps the same subquery operator, PexprBuildExists/InSub attaches
	// those residuals at the corresponding structural position. An eliminating
	// rule (for example InSubFilter(...) -> Input<...>) has no target-side
	// builder at which to do that, so restore the source Select shell here.
	const EDslOpKind edslopSrc = popSrcRoot->Edslop();
	const EDslOpKind edslopTgt = popTgtRoot->Edslop();
	CExpressionArray *pdrgpexprResidual = nullptr;
	if ((EdslopExists == edslopSrc || EdslopNotExists == edslopSrc) &&
		edslopSrc != edslopTgt)
	{
		pdrgpexprResidual = pmodel->PdrgpexprExistsResidual();
	}
	else if (EdslopInSubFilter == edslopSrc &&
			 EdslopInSubFilter != edslopTgt)
	{
		pdrgpexprResidual = pmodel->PdrgpexprInSubResidual();
	}
	else if ((EdslopAny == edslopSrc || EdslopAll == edslopSrc) &&
			 edslopSrc != edslopTgt)
	{
		pdrgpexprResidual = pmodel->PdrgpexprInSubResidual();
	}
	if (nullptr != pexprTgt && nullptr != pdrgpexprResidual &&
		0 < pdrgpexprResidual->Size())
	{
		CExpressionArray *pdrgpexprCopy =
			GPOS_NEW(m_mp) CExpressionArray(m_mp);
		for (ULONG ul = 0; ul < pdrgpexprResidual->Size(); ul++)
		{
			CExpression *pexprConj = (*pdrgpexprResidual)[ul];
			pexprConj->AddRef();
			pdrgpexprCopy->Append(pexprConj);
		}
		pexprTgt = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprTgt,
			CPredicateUtils::PexprConjunction(m_mp, pdrgpexprCopy));
	}

	// dedup drop: the source root was a redundant SELECT DISTINCT (pure-dedup
	// CLogicalGbAgg whose grouping cols form a key). PexprBuild produced the
	// resolved relational child (a bare Input target); wrap it in Select(child,
	// TRUE) to drop the GbAgg, exactly like ORCA's CXformSimplifyGbAgg::FDropGbAgg.
	// This keeps the memo group's output-column invariant: the trivial Select
	// outputs the child's columns (a superset of the GbAgg's grouping-only output),
	// which is the same substitution the native xform makes. The Select is a fresh
	// CExpression, so PexprFreshRoot returns it as-is (no remap needed).
	if (nullptr != pexprTgt && pmodel->FDedupDrop() &&
		EdslopProj == prule->PfragSrc()->PopRoot()->Edslop() &&
		prule->PfragSrc()->PopRoot()->FDistinct() &&
		!(EdslopProj == prule->PfragTgt()->PopRoot()->Edslop() &&
		  prule->PfragTgt()->PopRoot()->FDistinct()))
	{
		pexprTgt = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprTgt,
			CPredicateUtils::PexprConjunction(m_mp, nullptr));
	}
	return PexprFreshRoot(pexprTgt);
}

// EOF
