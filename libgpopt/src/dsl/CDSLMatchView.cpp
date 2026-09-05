//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLMatchView.cpp
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLMatchView.h"

#include "gpopt/base/CKeyCollection.h"
#include "gpopt/base/CCastUtils.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalFullOuterJoin.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CLogicalLeftSemiApplyIn.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CLogicalSetOp.h"
#include "gpopt/operators/CLogicalUnion.h"
#include "gpopt/operators/CLogicalUnionAll.h"
#include "gpopt/operators/CNormalizer.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarSubqueryAny.h"
#include "gpopt/xforms/CSubqueryHandler.h"
#include "gpopt/xforms/CXformUtils.h"
#include "naucrates/md/IMDScalarOp.h"
#include "naucrates/md/IMDType.h"

using namespace gpopt;

namespace
{
CColRef *
PcrJoinKeyOperand(CExpression *pexpr)
{
	if (COperator::EopScalarIdent == pexpr->Pop()->Eopid())
	{
		return const_cast<CColRef *>(
			CScalarIdent::PopConvert(pexpr->Pop())->Pcr());
	}
	if (CCastUtils::FBinaryCoercibleCastedScId(pexpr))
	{
		return const_cast<CColRef *>(
			CScalarIdent::PopConvert((*pexpr)[0]->Pop())->Pcr());
	}
	return nullptr;
}

void
CountSubqueryKinds(CExpression *pexpr, ULONG *pulScalar,
				   ULONG *pulOtherSubquery)
{
	switch (pexpr->Pop()->Eopid())
	{
		case COperator::EopScalarSubquery:
			(*pulScalar)++;
			break;
		case COperator::EopScalarSubqueryExists:
		case COperator::EopScalarSubqueryNotExists:
		case COperator::EopScalarSubqueryAny:
		case COperator::EopScalarSubqueryAll:
			(*pulOtherSubquery)++;
			break;
		default:
			break;
	}
	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		CountSubqueryKinds((*pexpr)[ul], pulScalar, pulOtherSubquery);
	}
}
}  // namespace

CExpression *
CDSLMatchView::PexprInverseComparison(CMemoryPool *mp,
									  CExpression *pexprCmp)
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

CExpression *
CDSLMatchView::PexprLowerSubqueries(CMemoryPool *mp,
										  CExpression *pexprUnary,
										  BOOL fEnforceCorrelatedApply,
										  BOOL fScalarOnly)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexprUnary);

	const COperator::EOperatorId eopid = pexprUnary->Pop()->Eopid();
	if ((COperator::EopLogicalSelect != eopid &&
		 COperator::EopLogicalProject != eopid &&
		 COperator::EopLogicalGbAgg != eopid) ||
		2 != pexprUnary->Arity())
	{
		return nullptr;
	}

	CExpression *pexprScalar = (*pexprUnary)[1];
	ULONG ulScalar = 0;
	ULONG ulOtherSubquery = 0;
	CountSubqueryKinds(pexprScalar, &ulScalar, &ulOtherSubquery);
	if ((fScalarOnly && (0 == ulScalar || 0 != ulOtherSubquery)) ||
		(!fScalarOnly && 0 == ulScalar + ulOtherSubquery))
	{
		return nullptr;
	}

	CExpression *pexprOuter = (*pexprUnary)[0];
	pexprOuter->AddRef();
	CExpression *pexprNewOuter = nullptr;
	CExpression *pexprResidual = nullptr;
	CSubqueryHandler handler(mp, fEnforceCorrelatedApply);
	if (!handler.FProcess(pexprOuter, pexprScalar,
					  CSubqueryHandler::EsqctxtFilter, &pexprNewOuter,
					  &pexprResidual))
	{
		CRefCount::SafeRelease(pexprNewOuter);
		CRefCount::SafeRelease(pexprResidual);
		return nullptr;
	}

	CExpression *pexprLowered = nullptr;
	if (COperator::EopLogicalProject == eopid)
	{
		pexprLowered = CUtils::PexprLogicalProject(
			mp, pexprNewOuter, pexprResidual, false /*fNewComputedCol*/);
	}
	else if (COperator::EopLogicalGbAgg == eopid)
	{
		CLogicalGbAgg *popGbAgg = CLogicalGbAgg::PopConvert(pexprUnary->Pop());
		popGbAgg->Pdrgpcr()->AddRef();
		pexprLowered = CUtils::PexprLogicalGbAgg(
			mp, popGbAgg->Pdrgpcr(), pexprNewOuter, pexprResidual,
			popGbAgg->Egbaggtype());
	}
	else
	{
		pexprLowered =
			CUtils::PexprLogicalSelect(mp, pexprNewOuter, pexprResidual);
	}
	CExpression *pexprNormalized =
		CNormalizer::PexprNormalize(mp, pexprLowered);
	pexprLowered->Release();
	CExpression *pexprCanonical =
		CNormalizer::PexprPullUpProjections(mp, pexprNormalized);
	pexprNormalized->Release();
	return pexprCanonical;
}

BOOL
CDSLMatchView::FSplitJoinPredicate(
	CMemoryPool *mp, CExpression *pexprPred, CExpression *pexprLeftRel,
	CColRefArray *pdrgpcrLeft, CColRefArray *pdrgpcrRight,
	CExpressionArray *pdrgpexprResidual)
{
	CColRefSet *pcrsLeft = pexprLeftRel->DeriveOutputColumns();
	CExpressionArray *pdrgpexprConj =
		CPredicateUtils::PdrgpexprConjuncts(mp, pexprPred);
	for (ULONG ul = 0; ul < pdrgpexprConj->Size(); ul++)
	{
		CExpression *pexprConj = (*pdrgpexprConj)[ul];
		if (2 != pexprConj->Arity() ||
			!CPredicateUtils::IsEqualityOp(pexprConj))
		{
			pexprConj->AddRef();
			pdrgpexprResidual->Append(pexprConj);
			continue;
		}

		CColRef *pcr0 = PcrJoinKeyOperand((*pexprConj)[0]);
		CColRef *pcr1 = PcrJoinKeyOperand((*pexprConj)[1]);
		if (nullptr == pcr0 || nullptr == pcr1)
		{
			pexprConj->AddRef();
			pdrgpexprResidual->Append(pexprConj);
			continue;
		}
		const BOOL f0Left = pcrsLeft->FMember(pcr0);
		const BOOL f1Left = pcrsLeft->FMember(pcr1);
		if (f0Left && !f1Left)
		{
			pdrgpcrLeft->Append(pcr0);
			pdrgpcrRight->Append(pcr1);
		}
		else if (f1Left && !f0Left)
		{
			pdrgpcrLeft->Append(pcr1);
			pdrgpcrRight->Append(pcr0);
		}
		else
		{
			pexprConj->AddRef();
			pdrgpexprResidual->Append(pexprConj);
		}
	}
	pdrgpexprConj->Release();
	return true;
}

CExpression *
CDSLMatchView::PexprBinarySetOp(CMemoryPool *mp,
								CExpression *pexprSetOp)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexprSetOp);

	const COperator::EOperatorId eopid = pexprSetOp->Pop()->Eopid();
	const ULONG ulArity = pexprSetOp->Arity();
	if ((COperator::EopLogicalUnion != eopid &&
		 COperator::EopLogicalUnionAll != eopid) ||
		2 >= ulArity)
	{
		return nullptr;
	}

	CLogicalSetOp *popSource =
		CLogicalSetOp::PopConvert(pexprSetOp->Pop());
	CColRef2dArray *pdrgpdrgpcrSource = popSource->PdrgpdrgpcrInput();
	if (ulArity != pdrgpdrgpcrSource->Size() ||
		0 == popSource->PdrgpcrOutput()->Size())
	{
		return nullptr;
	}

	// The tail's output identities are the original second input identities,
	// exactly the identities expected at the binary root's right position.
	CColRefArray *pdrgpcrTailOutput = (*pdrgpdrgpcrSource)[1];
	pdrgpcrTailOutput->AddRef();
	CColRef2dArray *pdrgpdrgpcrTail =
		GPOS_NEW(mp) CColRef2dArray(mp, ulArity - 1);
	CExpressionArray *pdrgpexprTail =
		GPOS_NEW(mp) CExpressionArray(mp, ulArity - 1);
	for (ULONG ul = 1; ul < ulArity; ul++)
	{
		CColRefArray *pdrgpcrInput = (*pdrgpdrgpcrSource)[ul];
		pdrgpcrInput->AddRef();
		pdrgpdrgpcrTail->Append(pdrgpcrInput);
		(*pexprSetOp)[ul]->AddRef();
		pdrgpexprTail->Append((*pexprSetOp)[ul]);
	}
	CExpression *pexprTail = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CLogicalUnionAll(
			mp, pdrgpcrTailOutput, pdrgpdrgpcrTail),
		pdrgpexprTail);

	CColRef2dArray *pdrgpdrgpcrBinary =
		GPOS_NEW(mp) CColRef2dArray(mp, 2);
	for (ULONG ul = 0; ul < 2; ul++)
	{
		CColRefArray *pdrgpcrInput = (*pdrgpdrgpcrSource)[ul];
		pdrgpcrInput->AddRef();
		pdrgpdrgpcrBinary->Append(pdrgpcrInput);
	}
	popSource->PdrgpcrOutput()->AddRef();
	COperator *popBinary = COperator::EopLogicalUnion == eopid
		? static_cast<COperator *>(GPOS_NEW(mp) CLogicalUnion(
			  mp, popSource->PdrgpcrOutput(), pdrgpdrgpcrBinary))
		: static_cast<COperator *>(GPOS_NEW(mp) CLogicalUnionAll(
			  mp, popSource->PdrgpcrOutput(), pdrgpdrgpcrBinary));
	CExpressionArray *pdrgpexprBinary =
		GPOS_NEW(mp) CExpressionArray(mp, 2);
	(*pexprSetOp)[0]->AddRef();
	pdrgpexprBinary->Append((*pexprSetOp)[0]);
	pdrgpexprBinary->Append(pexprTail);
	return GPOS_NEW(mp) CExpression(mp, popBinary, pdrgpexprBinary);
}

CExpression *
CDSLMatchView::PexprDistinctUnion(CMemoryPool *mp,
								  CExpression *pexprGbAgg)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexprGbAgg);

	if (COperator::EopLogicalGbAgg != pexprGbAgg->Pop()->Eopid() ||
		2 != pexprGbAgg->Arity() || 0 != (*pexprGbAgg)[1]->Arity())
	{
		return nullptr;
	}

	CLogicalGbAgg *popGbAgg =
		CLogicalGbAgg::PopConvert(pexprGbAgg->Pop());
	CExpression *pexprUnionAll = (*pexprGbAgg)[0];
	if (COperator::EgbaggtypeGlobal != popGbAgg->Egbaggtype() ||
		nullptr != popGbAgg->PdrgpcrMinimal() ||
		COperator::EopLogicalUnionAll !=
			pexprUnionAll->Pop()->Eopid())
	{
		return nullptr;
	}

	CLogicalSetOp *popUnionAll =
		CLogicalSetOp::PopConvert(pexprUnionAll->Pop());
	CColRefArray *pdrgpcrGrouping = popGbAgg->Pdrgpcr();
	CColRefArray *pdrgpcrOutput = popUnionAll->PdrgpcrOutput();
	CColRef2dArray *pdrgpdrgpcrInput =
		popUnionAll->PdrgpdrgpcrInput();
	if (2 > pexprUnionAll->Arity() ||
		pexprUnionAll->Arity() != pdrgpdrgpcrInput->Size() ||
		0 == pdrgpcrOutput->Size() ||
		pdrgpcrGrouping->Size() != pdrgpcrOutput->Size())
	{
		return nullptr;
	}

	CColRefSet *pcrsGrouping =
		GPOS_NEW(mp) CColRefSet(mp, pdrgpcrGrouping);
	CColRefSet *pcrsOutput = GPOS_NEW(mp) CColRefSet(mp, pdrgpcrOutput);
	const BOOL fFullRow = pcrsGrouping->Equals(pcrsOutput);
	pcrsOutput->Release();
	pcrsGrouping->Release();
	if (!fFullRow)
	{
		return nullptr;
	}

	// Reuse the set-op's positional maps verbatim. They are semantic state and
	// cannot be reconstructed from the unordered aggregate output set.
	pdrgpcrOutput->AddRef();
	pdrgpdrgpcrInput->AddRef();
	CExpressionArray *pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp, pexprUnionAll->Arity());
	for (ULONG ul = 0; ul < pexprUnionAll->Arity(); ul++)
	{
		(*pexprUnionAll)[ul]->AddRef();
		pdrgpexprChildren->Append((*pexprUnionAll)[ul]);
	}
	return GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CLogicalUnion(mp, pdrgpcrOutput,
									pdrgpdrgpcrInput),
		pdrgpexprChildren);
}

BOOL
CDSLMatchView::FAggregate(CExpression *pexpr, BOOL fAllowHaving,
						  SAggregate *pview)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != pview);

	pview->m_pexprAgg = pexpr;
	pview->m_pexprHaving = nullptr;
	if (fAllowHaving && COperator::EopLogicalSelect == pexpr->Pop()->Eopid() &&
		2 == pexpr->Arity())
	{
		pview->m_pexprAgg = (*pexpr)[0];
		pview->m_pexprHaving = (*pexpr)[1];
	}

	const COperator::EOperatorId eopid =
		pview->m_pexprAgg->Pop()->Eopid();
	return (COperator::EopLogicalGbAgg == eopid ||
			COperator::EopLogicalGbAggDeduplicate == eopid) &&
		   2 == pview->m_pexprAgg->Arity();
}

BOOL
CDSLMatchView::FOrderLimit(CExpression *pexpr, SOrderLimit *pview)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != pview);

	if (COperator::EopLogicalLimit != pexpr->Pop()->Eopid() ||
		3 != pexpr->Arity())
	{
		return false;
	}

	CLogicalLimit *popLimit = CLogicalLimit::PopConvert(pexpr->Pop());
	if (!popLimit->FGlobal() || popLimit->IsTopLimitUnderDMLorCTAS())
	{
		return false;
	}

	pview->m_pexprChild = (*pexpr)[0];
	pview->m_pexprOffset = (*pexpr)[1];
	pview->m_pexprCount = (*pexpr)[2];
	pview->m_pos = popLimit->Pos();
	pview->m_fHasLimit =
		popLimit->FHasCount() || !CUtils::FHasZeroOffset(pexpr);
	return true;
}

BOOL
CDSLMatchView::FDedupIdentity(CExpression *pexpr,
							CExpression **ppexprDedup,
							CColRefArray **ppdrgpcrGrouping)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != ppexprDedup);
	GPOS_ASSERT(nullptr != ppdrgpcrGrouping);

	if (COperator::EopLogicalSelect != pexpr->Pop()->Eopid() ||
		2 != pexpr->Arity() || !CUtils::FScalarConstTrue((*pexpr)[1]))
	{
		return false;
	}

	CExpression *pexprDedup = (*pexpr)[0];
	if (COperator::EopLogicalGbAgg != pexprDedup->Pop()->Eopid() ||
		2 != pexprDedup->Arity() || 0 != (*pexprDedup)[1]->Arity())
	{
		return false;
	}

	CLogicalGbAgg *popGbAgg =
		CLogicalGbAgg::PopConvert(pexprDedup->Pop());
	if (COperator::EgbaggtypeGlobal != popGbAgg->Egbaggtype() ||
		nullptr != popGbAgg->PdrgpcrMinimal() ||
		nullptr == popGbAgg->Pdrgpcr() || 0 == popGbAgg->Pdrgpcr()->Size())
	{
		return false;
	}

	*ppexprDedup = pexprDedup;
	*ppdrgpcrGrouping = popGbAgg->Pdrgpcr();
	return true;
}

BOOL
CDSLMatchView::FDroppedDedupIdentity(CExpression *pexpr,
								 CExpression **ppexprChild)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != ppexprChild);
	*ppexprChild = nullptr;

	if (COperator::EopLogicalSelect != pexpr->Pop()->Eopid() ||
		2 != pexpr->Arity() || !CUtils::FScalarConstTrue((*pexpr)[1]))
	{
		return false;
	}

	CExpression *pexprChild = (*pexpr)[0];
	CColRefSet *pcrsOutput = pexprChild->DeriveOutputColumns();
	CKeyCollection *pkc = pexprChild->DeriveKeyCollection();
	if (nullptr == pkc || 0 == pcrsOutput->Size() ||
		!pkc->FKey(pcrsOutput, false /*fExactMatch*/))
	{
		return false;
	}

	*ppexprChild = pexprChild;
	return true;
}

CExpression *
CDSLMatchView::PexprPeelOrderLimit(CExpression *pexpr,
								  CExpression **ppexprFirstShell)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != ppexprFirstShell);

	*ppexprFirstShell = nullptr;
	while (COperator::EopLogicalLimit == pexpr->Pop()->Eopid() &&
		   3 == pexpr->Arity())
	{
		if (nullptr == *ppexprFirstShell)
		{
			*ppexprFirstShell = pexpr;
		}
		pexpr = (*pexpr)[0];
	}
	return pexpr;
}

BOOL
CDSLMatchView::FDirectExists(CExpression *pexpr)
{
	return nullptr != pexpr &&
		   COperator::EopScalarSubqueryExists == pexpr->Pop()->Eopid() &&
		   1 == pexpr->Arity();
}

BOOL
CDSLMatchView::FPlainEqAny(CExpression *pexpr)
{
	return nullptr != pexpr &&
		   COperator::EopScalarSubqueryAny == pexpr->Pop()->Eopid() &&
		   2 == pexpr->Arity() &&
		   IMDType::EcmptEq == CUtils::ParseCmpType(
				CScalarSubqueryAny::PopConvert(pexpr->Pop())->MdIdOp());
}

CExpression *
CDSLMatchView::PexprRebaseInSubCarrier(CMemoryPool *mp,
									   CExpression *pexprCarrier,
									   CExpression *pexprRel)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexprCarrier);
	GPOS_ASSERT(nullptr != pexprRel);

	if (COperator::EopLogicalSelect == pexprCarrier->Pop()->Eopid() &&
		2 == pexprCarrier->Arity())
	{
		pexprCarrier->Pop()->AddRef();
		pexprRel->AddRef();
		(*pexprCarrier)[1]->AddRef();
		return GPOS_NEW(mp) CExpression(mp, pexprCarrier->Pop(), pexprRel,
									  (*pexprCarrier)[1]);
	}

	if (COperator::EopLogicalLeftSemiApplyIn ==
			pexprCarrier->Pop()->Eopid() &&
		3 == pexprCarrier->Arity())
	{
		pexprCarrier->Pop()->AddRef();
		pexprRel->AddRef();
		(*pexprCarrier)[1]->AddRef();
		(*pexprCarrier)[2]->AddRef();
		return GPOS_NEW(mp) CExpression(
			mp, pexprCarrier->Pop(), pexprRel, (*pexprCarrier)[1],
			(*pexprCarrier)[2]);
	}

	if (COperator::EopLogicalLeftSemiJoin ==
			pexprCarrier->Pop()->Eopid() &&
		3 == pexprCarrier->Arity())
	{
		pexprCarrier->Pop()->AddRef();
		pexprRel->AddRef();
		(*pexprCarrier)[1]->AddRef();
		(*pexprCarrier)[2]->AddRef();
		return GPOS_NEW(mp) CExpression(
			mp, pexprCarrier->Pop(), pexprRel, (*pexprCarrier)[1],
			(*pexprCarrier)[2]);
	}

	return nullptr;
}

CExpression *
CDSLMatchView::PexprApplyInPredicate(CMemoryPool *mp,
									CExpression *pexprApply)
{
	if (COperator::EopLogicalLeftSemiApplyIn !=
			pexprApply->Pop()->Eopid() ||
		3 != pexprApply->Arity() ||
		!CUtils::FScalarConstTrue((*pexprApply)[2]))
	{
		return nullptr;
	}

	CExpression *pexprSelect = (*pexprApply)[1];
	if (COperator::EopLogicalSelect != pexprSelect->Pop()->Eopid() ||
		2 != pexprSelect->Arity())
	{
		return nullptr;
	}

	CColRefArray *pdrgpcrInner =
		CLogicalLeftSemiApplyIn::PopConvert(pexprApply->Pop())->PdrgPcrInner();
	if (nullptr == pdrgpcrInner || 1 != pdrgpcrInner->Size())
	{
		return nullptr;
	}

	CExpressionArray *pdrgpexprConjuncts =
		CPredicateUtils::PdrgpexprConjuncts(mp, (*pexprSelect)[1]);
	CExpression *pexprComparison = nullptr;
	ULONG ulComparison = 0;
	for (ULONG ul = 0; ul < pdrgpexprConjuncts->Size(); ul++)
	{
		CExpression *pexprConjunct = (*pdrgpexprConjuncts)[ul];
		CColRefSet *pcrsUsed = pexprConjunct->DeriveUsedColumns();
		if (COperator::EopScalarCmp == pexprConjunct->Pop()->Eopid() &&
			pcrsUsed->FMember((*pdrgpcrInner)[0]) &&
			!pcrsUsed->IsDisjoint(
				(*pexprApply)[0]->DeriveOutputColumns()))
		{
			if (nullptr != pexprComparison &&
				!CUtils::Equals(pexprComparison, pexprConjunct))
			{
				pdrgpexprConjuncts->Release();
				return nullptr;
			}
			pexprComparison = pexprConjunct;
			ulComparison = ul;
		}
	}
	if (nullptr == pexprComparison)
	{
		pdrgpexprConjuncts->Release();
		return nullptr;
	}

	CExpressionArray *pdrgpexprResidual =
		GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < pdrgpexprConjuncts->Size(); ul++)
	{
		if (ul != ulComparison)
		{
			(*pdrgpexprConjuncts)[ul]->AddRef();
			pdrgpexprResidual->Append((*pdrgpexprConjuncts)[ul]);
		}
	}

	CExpression *pexprInner = nullptr;
	if (0 == pdrgpexprResidual->Size())
	{
		pdrgpexprResidual->Release();
		pexprInner = (*pexprSelect)[0];
		pexprInner->AddRef();
	}
	else
	{
		(*pexprSelect)[0]->AddRef();
		pexprInner = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CLogicalSelect(mp), (*pexprSelect)[0],
			CPredicateUtils::PexprConjunction(mp, pdrgpexprResidual));
	}

	pexprApply->Pop()->AddRef();
	(*pexprApply)[0]->AddRef();
	pexprComparison->AddRef();
	CExpression *pexprResult = GPOS_NEW(mp) CExpression(
		mp, pexprApply->Pop(), (*pexprApply)[0], pexprInner,
		pexprComparison);
	pdrgpexprConjuncts->Release();
	return pexprResult;
}

CExpression *
CDSLMatchView::PexprNullRejectedInnerJoin(CMemoryPool *mp,
									 CExpression *pexprSelect)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexprSelect);

	if (COperator::EopLogicalSelect != pexprSelect->Pop()->Eopid() ||
		2 != pexprSelect->Arity())
	{
		return nullptr;
	}
	CExpression *pexprOuterJoin = (*pexprSelect)[0];
	CExpression *pexprPred = (*pexprSelect)[1];
	const COperator::EOperatorId eopid = pexprOuterJoin->Pop()->Eopid();
	if ((COperator::EopLogicalLeftOuterJoin != eopid &&
		 COperator::EopLogicalFullOuterJoin != eopid) ||
		3 != pexprOuterJoin->Arity())
	{
		return nullptr;
	}
	const BOOL fRejectsRight = CPredicateUtils::FNullRejecting(
		mp, pexprPred, (*pexprOuterJoin)[1]->DeriveOutputColumns());
	const BOOL fRejectsLeft =
		COperator::EopLogicalFullOuterJoin != eopid ||
		CPredicateUtils::FNullRejecting(
			mp, pexprPred, (*pexprOuterJoin)[0]->DeriveOutputColumns());
	if (!fRejectsLeft || !fRejectsRight)
	{
		return nullptr;
	}

	(*pexprOuterJoin)[0]->AddRef();
	(*pexprOuterJoin)[1]->AddRef();
	(*pexprOuterJoin)[2]->AddRef();
	pexprPred->AddRef();
	CExpression *pexprInnerJoin = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalInnerJoin(mp), (*pexprOuterJoin)[0],
		(*pexprOuterJoin)[1], (*pexprOuterJoin)[2]);
	return GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), pexprInnerJoin, pexprPred);
}

CExpression *
CDSLMatchView::PexprCorrelatedInnerJoinFilter(CMemoryPool *mp,
										  CExpression *pexprJoin)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexprJoin);

	if (COperator::EopLogicalInnerJoin != pexprJoin->Pop()->Eopid() ||
		3 != pexprJoin->Arity())
	{
		return nullptr;
	}
	if ((*pexprJoin)[2]->DeriveHasSubquery())
	{
		return CXformUtils::PexprSeparateSubqueryPreds(mp, pexprJoin);
	}

	CColRefSet *pcrsInputs = GPOS_NEW(mp) CColRefSet(
		mp, *(*pexprJoin)[0]->DeriveOutputColumns());
	pcrsInputs->Union((*pexprJoin)[1]->DeriveOutputColumns());
	CColRefSet *pcrsOuter = GPOS_NEW(mp) CColRefSet(
		mp, *(*pexprJoin)[2]->DeriveUsedColumns());
	pcrsOuter->Exclude(pcrsInputs);
	const BOOL fCorrelated = 0 < pcrsOuter->Size();
	pcrsOuter->Release();
	pcrsInputs->Release();
	if (!fCorrelated)
	{
		return nullptr;
	}

	(*pexprJoin)[0]->AddRef();
	(*pexprJoin)[1]->AddRef();
	(*pexprJoin)[2]->AddRef();
	CExpression *pexprLocalJoin = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalInnerJoin(mp), (*pexprJoin)[0],
		(*pexprJoin)[1], CUtils::PexprScalarConstBool(mp, true));
	return GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), pexprLocalJoin,
		(*pexprJoin)[2]);
}

CExpressionArray *
CDSLMatchView::PdrgpexprNullRejectedLeftJoins(CMemoryPool *mp,
										 CExpression *pexprSelect)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexprSelect);
	CExpressionArray *pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	if (COperator::EopLogicalSelect != pexprSelect->Pop()->Eopid() ||
		2 != pexprSelect->Arity())
	{
		return pdrgpexpr;
	}
	CExpression *pexprFullJoin = (*pexprSelect)[0];
	CExpression *pexprPred = (*pexprSelect)[1];
	if (COperator::EopLogicalFullOuterJoin !=
			pexprFullJoin->Pop()->Eopid() ||
		3 != pexprFullJoin->Arity())
	{
		return pdrgpexpr;
	}

	for (ULONG ulPreserved = 0; ulPreserved < 2; ulPreserved++)
	{
		if (!CPredicateUtils::FNullRejecting(
				mp, pexprPred,
				(*pexprFullJoin)[ulPreserved]->DeriveOutputColumns()))
		{
			continue;
		}
		CExpression *pexprPreserved = (*pexprFullJoin)[ulPreserved];
		CExpression *pexprNullable = (*pexprFullJoin)[1 - ulPreserved];
		pexprPreserved->AddRef();
		pexprNullable->AddRef();
		(*pexprFullJoin)[2]->AddRef();
		pdrgpexpr->Append(GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CLogicalLeftOuterJoin(mp), pexprPreserved,
			pexprNullable, (*pexprFullJoin)[2]));
	}
	return pdrgpexpr;
}

CDSLMatchView::SJoinSpineRouteArray *
CDSLMatchView::PdrgprouteJoinSpine(CMemoryPool *mp, CExpression *pexpr,
								  COperator::EOperatorId eopidCarrier,
								  ULONG ulDepth)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	SJoinSpineRouteArray *pdrgproute =
		GPOS_NEW(mp) SJoinSpineRouteArray(mp);
	if (64 <= ulDepth)
	{
		return pdrgproute;
	}

	if (eopidCarrier == pexpr->Pop()->Eopid())
	{
		if (0 < pexpr->Arity())
		{
			(*pexpr)[0]->AddRef();
			pexpr->AddRef();
			pdrgproute->Append(
				GPOS_NEW(mp) SJoinSpineRoute((*pexpr)[0], pexpr));
		}
		if (COperator::EopLogicalSelect != eopidCarrier ||
			2 != pexpr->Arity())
		{
			return pdrgproute;
		}
	}

	ULONG ulSides = 0;
	switch (pexpr->Pop()->Eopid())
	{
		case COperator::EopLogicalInnerJoin:
			ulSides = 2;
			break;
		case COperator::EopLogicalLeftOuterJoin:
			ulSides = 1;
			break;
		case COperator::EopLogicalSelect:
			ulSides = 1;
			break;
		default:
			return pdrgproute;
	}
	if ((COperator::EopLogicalInnerJoin == pexpr->Pop()->Eopid() ||
		 COperator::EopLogicalLeftOuterJoin == pexpr->Pop()->Eopid()) &&
		3 != pexpr->Arity())
	{
		return pdrgproute;
	}

	for (ULONG ulSide = 0; ulSide < ulSides; ulSide++)
	{
		SJoinSpineRouteArray *pdrgprouteChild = PdrgprouteJoinSpine(
			mp, (*pexpr)[ulSide], eopidCarrier, ulDepth + 1);
		for (ULONG ulRoute = 0; ulRoute < pdrgprouteChild->Size(); ulRoute++)
		{
			SJoinSpineRoute *prouteChild = (*pdrgprouteChild)[ulRoute];
			CExpressionArray *pdrgpexpr =
				GPOS_NEW(mp) CExpressionArray(mp);
			for (ULONG ulChild = 0; ulChild < pexpr->Arity(); ulChild++)
			{
				CExpression *pexprChild =
					(ulChild == ulSide) ? prouteChild->m_pexprRel
										 : (*pexpr)[ulChild];
				pexprChild->AddRef();
				pdrgpexpr->Append(pexprChild);
			}
			pexpr->Pop()->AddRef();
			CExpression *pexprRouted = GPOS_NEW(mp) CExpression(
				mp, pexpr->Pop(), pdrgpexpr);
			prouteChild->m_pexprCarrier->AddRef();
			pdrgproute->Append(GPOS_NEW(mp) SJoinSpineRoute(
				pexprRouted, prouteChild->m_pexprCarrier));
		}
		pdrgprouteChild->Release();
	}
	return pdrgproute;
}

// EOF
