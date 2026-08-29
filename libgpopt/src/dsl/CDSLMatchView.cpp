//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLMatchView.cpp
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLMatchView.h"

#include "gpopt/base/CKeyCollection.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalFullOuterJoin.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CLogicalSetOp.h"
#include "gpopt/operators/CLogicalUnion.h"
#include "gpopt/operators/CLogicalUnionAll.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarSubqueryAny.h"
#include "naucrates/md/IMDType.h"

using namespace gpopt;

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
