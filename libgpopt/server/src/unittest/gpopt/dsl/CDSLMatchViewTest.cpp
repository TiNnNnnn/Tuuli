//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLMatchViewTest.h"

#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

GPOS_RESULT
CDSLMatchViewTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(
			CDSLMatchViewTest::EresUnittest_AggregateAndOrderViews),
		GPOS_UNITTEST_FUNC(
			CDSLMatchViewTest::EresUnittest_JoinSpineAndCarrierViews),
		GPOS_UNITTEST_FUNC(
			CDSLMatchViewTest::EresUnittest_NullRejectedInnerJoinView),
		GPOS_UNITTEST_FUNC(
			CDSLMatchViewTest::EresUnittest_CorrelatedInnerJoinFilterView),
	};
	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLMatchViewTest::EresUnittest_CorrelatedInnerJoinFilterView()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CColRefArray *pdrgpcrLeft = nullptr;
	CExpression *pexprLeft =
		fix.PexprLogicalGet("match_view_corr_left", 1, &pdrgpcrLeft);
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprRight =
		fix.PexprLogicalGet("match_view_corr_right", 1, &pdrgpcrRight);
	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("match_view_corr_outer", 1, &pdrgpcrOuter);

	CExpressionArray *pdrgpexprConj = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexprConj->Append(
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]));
	pdrgpexprConj->Append(
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrOuter)[0]));
	CExpression *pexprPred =
		CPredicateUtils::PexprConjunction(mp, pdrgpexprConj);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprPred);
	pexprLeft->Release();
	pexprRight->Release();
	pexprPred->Release();

	CExpression *pexprView =
		CDSLMatchView::PexprCorrelatedInnerJoinFilter(mp, pexprJoin);
	GPOS_ASSERT(nullptr != pexprView);
	GPOS_ASSERT(COperator::EopLogicalSelect == pexprView->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalInnerJoin ==
				(*pexprView)[0]->Pop()->Eopid());
	GPOS_ASSERT(CUtils::FScalarConstTrue((*(*pexprView)[0])[2]));
	GPOS_ASSERT((*pexprView)[1]->Matches((*pexprJoin)[2]));

	pexprView->Release();
	pexprJoin->Release();
	pexprOuter->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLMatchViewTest::EresUnittest_AggregateAndOrderViews()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CColRefArray *pdrgpcr = nullptr;
	CExpression *pexprGet =
		fix.PexprLogicalGet("match_view_agg", 2, &pdrgpcr);
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcr)[0]);
	CExpression *pexprAgg =
		fix.PexprLogicalGbAgg(pexprGet, pdrgpcrGroup);
	pdrgpcrGroup->Release();
	CExpression *pexprHaving =
		fix.PexprEqPred((*pdrgpcr)[0], (*pdrgpcr)[0]);
	CExpression *pexprSelect =
		fix.PexprLogicalSelect(pexprAgg, pexprHaving);
	pexprHaving->Release();

	CDSLMatchView::SAggregate aggView;
	GPOS_ASSERT(CDSLMatchView::FAggregate(pexprSelect, true, &aggView));
	GPOS_ASSERT(aggView.m_pexprAgg == pexprAgg);
	GPOS_ASSERT(nullptr != aggView.m_pexprHaving);
	GPOS_ASSERT(!CDSLMatchView::FAggregate(pexprSelect, false, &aggView));
	CExpression *pexprTrue = CUtils::PexprScalarConstBool(mp, true);
	CExpression *pexprIdentity =
		fix.PexprLogicalSelect(pexprAgg, pexprTrue);
	pexprTrue->Release();
	CExpression *pexprDedup = nullptr;
	CColRefArray *pdrgpcrIdentity = nullptr;
	GPOS_ASSERT(CDSLMatchView::FDedupIdentity(
		pexprIdentity, &pexprDedup, &pdrgpcrIdentity));
	GPOS_ASSERT(pexprDedup == pexprAgg);
	GPOS_ASSERT(1 == pdrgpcrIdentity->Size());

	COrderSpec *pos = GPOS_NEW(mp) COrderSpec(mp);
	pexprGet->AddRef();
	CExpression *pexprLimit = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CLogicalLimit(
			mp, pos, true /*global*/, true /*has count*/, false /*top DML*/),
		pexprGet, CUtils::PexprScalarConstInt8(mp, 0),
		CUtils::PexprScalarConstInt8(mp, 5));
	CDSLMatchView::SOrderLimit limitView;
	GPOS_ASSERT(CDSLMatchView::FOrderLimit(pexprLimit, &limitView));
	GPOS_ASSERT(limitView.m_pexprChild == pexprGet);
	GPOS_ASSERT(limitView.m_fHasLimit);

	CExpression *pexprFirstShell = nullptr;
	GPOS_ASSERT(pexprGet == CDSLMatchView::PexprPeelOrderLimit(
		pexprLimit, &pexprFirstShell));
	GPOS_ASSERT(pexprFirstShell == pexprLimit);

	pexprLimit->Release();
	pexprIdentity->Release();
	pexprSelect->Release();
	pexprAgg->Release();
	pexprGet->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLMatchViewTest::EresUnittest_JoinSpineAndCarrierViews()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CColRefArray *pdrgpcrLeft = nullptr;
	CExpression *pexprLeft =
		fix.PexprLogicalGet("match_view_left", 1, &pdrgpcrLeft);
	CExpression *pexprFilter =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrLeft)[0]);
	CExpression *pexprSelect =
		fix.PexprLogicalSelect(pexprLeft, pexprFilter);
	pexprFilter->Release();
	pexprLeft->Release();

	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprRight =
		fix.PexprLogicalGet("match_view_right", 1, &pdrgpcrRight);
	CExpression *pexprJoinPred =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprJoin = fix.PexprLogicalInnerJoin(
		pexprSelect, pexprRight, pexprJoinPred);
	pexprSelect->Release();
	pexprRight->Release();
	pexprJoinPred->Release();

	CDSLMatchView::SJoinSpineRouteArray *pdrgproute =
		CDSLMatchView::PdrgprouteJoinSpine(
			mp, pexprJoin, COperator::EopLogicalSelect);
	GPOS_ASSERT(1 == pdrgproute->Size());
	CDSLMatchView::SJoinSpineRoute *proute = (*pdrgproute)[0];
	GPOS_ASSERT(COperator::EopLogicalInnerJoin ==
				proute->m_pexprRel->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalSelect ==
				proute->m_pexprCarrier->Pop()->Eopid());

	CExpression *pexprRebased = CDSLMatchView::PexprRebaseInSubCarrier(
		mp, proute->m_pexprCarrier, proute->m_pexprRel);
	GPOS_ASSERT(nullptr != pexprRebased);
	GPOS_ASSERT((*pexprRebased)[0] == proute->m_pexprRel);
	GPOS_ASSERT(CDSLMatchView::FDirectExists(nullptr) == false);
	GPOS_ASSERT(CDSLMatchView::FPlainEqAny((*pexprRebased)[1]) == false);

	pexprRebased->Release();
	pdrgproute->Release();
	pexprJoin->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLMatchViewTest::EresUnittest_NullRejectedInnerJoinView()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CColRefArray *pdrgpcrLeft = nullptr;
	CExpression *pexprLeft =
		fix.PexprLogicalGet("match_view_loj_left", 1, &pdrgpcrLeft);
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprRight =
		fix.PexprLogicalGet("match_view_loj_right", 1, &pdrgpcrRight);
	CExpression *pexprJoinPred =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprLeftJoin = fix.PexprLogicalLeftOuterJoin(
		pexprLeft, pexprRight, pexprJoinPred);
	pexprLeft->Release();
	pexprRight->Release();
	pexprJoinPred->Release();

	CExpression *pexprRejecting = CUtils::PexprNegate(
		mp, fix.PexprPredAtom((*pdrgpcrRight)[0]));
	CExpression *pexprSelect =
		fix.PexprLogicalSelect(pexprLeftJoin, pexprRejecting);
	pexprRejecting->Release();
	CExpression *pexprView =
		CDSLMatchView::PexprNullRejectedInnerJoin(mp, pexprSelect);
	GPOS_ASSERT(nullptr != pexprView);
	GPOS_ASSERT(COperator::EopLogicalSelect == pexprView->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalInnerJoin ==
				(*pexprView)[0]->Pop()->Eopid());
	pexprView->Release();
	pexprSelect->Release();

	CExpression *pexprPreservedPred =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrLeft)[0]);
	CExpression *pexprPreservedSelect =
		fix.PexprLogicalSelect(pexprLeftJoin, pexprPreservedPred);
	pexprPreservedPred->Release();
	GPOS_ASSERT(nullptr == CDSLMatchView::PexprNullRejectedInnerJoin(
		mp, pexprPreservedSelect));
	pexprPreservedSelect->Release();
	pexprLeftJoin->Release();
	return GPOS_OK;
}

// EOF
