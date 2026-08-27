//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLMatchView.cpp
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLMatchView.h"

#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CScalarSubqueryAny.h"
#include "naucrates/md/IMDType.h"

using namespace gpopt;

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

	return COperator::EopLogicalGbAgg ==
			   pview->m_pexprAgg->Pop()->Eopid() &&
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
