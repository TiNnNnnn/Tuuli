#include "unittest/gpopt/dsl/CDSLOrderLimitTest.h"

#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringConst.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/CDistributionSpecHashed.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CLogicalAssert.h"
#include "gpopt/operators/CLogicalMaxOneRow.h"
#include "gpopt/operators/CLogicalSequenceProject.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarWindowFunc.h"
#include "gpopt/xforms/CXformUtils.h"
#include "naucrates/md/CMDIdGPDB.h"
#include "naucrates/md/CMDAggregateGPDB.h"
#include "naucrates/md/CMDTypeInt4GPDB.h"
#include "naucrates/dxl/gpdb_types.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

namespace
{
COrderSpec *
PosOne(CMemoryPool *mp, CColRef *pcr, EDslSortDir edslsort)
{
	const BOOL fAsc = EdslsortAsc == edslsort;
	IMDId *pmdid = pcr->RetrieveType()->GetMdidForCmpType(
		fAsc ? IMDType::EcmptL : IMDType::EcmptG);
	pmdid->AddRef();
	COrderSpec *pos = GPOS_NEW(mp) COrderSpec(mp);
	pos->Append(pmdid, pcr, fAsc ? COrderSpec::EntLast : COrderSpec::EntFirst);
	return pos;
}

CExpression *
PexprLimit(CMemoryPool *mp, CExpression *pexprChild, COrderSpec *pos,
		   BOOL fHasCount, LINT offset, LINT count)
{
	pexprChild->AddRef();
	return GPOS_NEW(mp) CExpression(mp,
		GPOS_NEW(mp) CLogicalLimit(
			mp, pos, true /*global*/, fHasCount, false /*top DML*/),
		pexprChild, CUtils::PexprScalarConstInt8(mp, offset),
		CUtils::PexprScalarConstInt8(mp, count, !fHasCount /*is null*/));
}

CDSLRule *
Prule(CMemoryPool *mp, const CHAR *szRule)
{
	return CDSLRuleParser::PdslruleParse(mp, szRule, "EQ", nullptr);
}

CExpression *
PexprWindowRows(CMemoryPool *mp, CDSLTestFixture &fix,
				CExpression *pexprChild, CColRef *pcrPartition,
				CColRef *pcrArgument)
{
	CExpressionArray *pdrgpexprDist = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexprDist->Append(CUtils::PexprScalarIdent(mp, pcrPartition));
	CDistributionSpec *pds =
		GPOS_NEW(mp) CDistributionSpecHashed(pdrgpexprDist, true);
	COrderSpecArray *pdrgpos = GPOS_NEW(mp) COrderSpecArray(mp);
	CWindowFrameArray *pdrgpwf = GPOS_NEW(mp) CWindowFrameArray(mp);

	CScalarWindowFunc *popWindow = GPOS_NEW(mp) CScalarWindowFunc(
		mp, GPOS_NEW(mp) CMDIdGPDB(IMDId::EmdidGeneral, GPDB_INT4_AGG_MAX),
		GPOS_NEW(mp) CMDIdGPDB(IMDId::EmdidGeneral, GPDB_INT4_OID),
		GPOS_NEW(mp) CWStringConst(mp, GPOS_WSZ_LIT("max")),
		CScalarWindowFunc::EwsImmediate, false, false, true);
	CExpressionArray *pdrgpexprArgs = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexprArgs->Append(CUtils::PexprScalarIdent(mp, pcrArgument));
	CExpression *pexprWindow =
		GPOS_NEW(mp) CExpression(mp, popWindow, pdrgpexprArgs);
	CColRef *pcrOutput = fix.PcrCreateInt4("win");
	CExpressionArray *pdrgpexprElems = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexprElems->Append(
		CUtils::PexprScalarProjectElement(mp, pcrOutput, pexprWindow));
	CExpression *pexprList = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprElems);

	pexprChild->AddRef();
	return CUtils::PexprLogicalSequenceProject(
		mp, COperator::EsptypeGlobalOneStep, pds, pdrgpos, pdrgpwf,
		pexprChild, pexprList);
}
}  // namespace

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(
			CDSLOrderLimitTest::EresUnittest_FusedLimitSortRoundTrip),
		GPOS_UNITTEST_FUNC(
			CDSLOrderLimitTest::EresUnittest_SortOverLimitStaysNested),
		GPOS_UNITTEST_FUNC(
			CDSLOrderLimitTest::EresUnittest_PlainLimitRejectsHiddenOrder),
		GPOS_UNITTEST_FUNC(
			CDSLOrderLimitTest::EresUnittest_OffsetOnlyLimitRoundTrip),
		GPOS_UNITTEST_FUNC(
			CDSLOrderLimitTest::EresUnittest_NonDefaultNullOrderRejects),
		GPOS_UNITTEST_FUNC(
			CDSLOrderLimitTest::EresUnittest_TargetScalarConstants),
		GPOS_UNITTEST_FUNC(
			CDSLOrderLimitTest::EresUnittest_WindowRowsRoundTrip),
		GPOS_UNITTEST_FUNC(
			CDSLOrderLimitTest::EresUnittest_RowNumberConstructiveTarget),
		GPOS_UNITTEST_FUNC(
			CDSLOrderLimitTest::EresUnittest_MaxOneRowReplacement),
	};
	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_RowNumberConstructiveTarget()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *pdrgpcr = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("ranked", 1, &pdrgpcr);
	CDSLRule *prule = Prule(mp,
		"Input<t0>|RowNumber<a0 o0 r0>(Input<t1>)|"
		"TableEq(t1,t0);AttrsEmpty(a0);OrderEmpty(o0);RankAttrs(a1,r0)");
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (nullptr == prule ||
		!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGet, pmodel) ||
		!checker.FCheck(prule, pmodel) ||
		nullptr == (pexprTarget = instantiator.PexprInstantiate(prule, pmodel)) ||
		COperator::EopLogicalSequenceProject != pexprTarget->Pop()->Eopid() ||
		2 != pexprTarget->Arity() || 1 != (*pexprTarget)[1]->Arity())
	{
		eres = GPOS_FAILED;
	}

	CRefCount::SafeRelease(pexprTarget);
	CExpression *pexprLive = CXformUtils::PexprWindowWithRowNumber(
		mp, pexprGet, pdrgpcr);
	CDSLRule *pruleIdentity = Prule(mp,
		"RowNumber<a0 o0 r0>(Input<t0>)|"
		"RowNumber<a1 o1 r1>(Input<t1>)|"
		"TableEq(t1,t0);AttrsEq(a1,a0);OrderEq(o1,o0);RankEq(r1,r0);"
		"ErrorFree(r0);Deterministic(r0)");
	CDSLModel *pmodelIdentity = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcherIdentity(mp, pruleIdentity);
	CDSLInstantiator instantiatorIdentity(mp);
	CExpression *pexprIdentity = nullptr;
	if (GPOS_OK == eres &&
		(nullptr == pruleIdentity ||
		 !matcherIdentity.FMatch(pruleIdentity->PfragSrc()->PopRoot(),
								 pexprLive, pmodelIdentity) ||
		 !checker.FCheck(pruleIdentity, pmodelIdentity) ||
		 nullptr == (pexprIdentity =
			 instantiatorIdentity.PexprInstantiate(pruleIdentity, pmodelIdentity)) ||
		 !pexprIdentity->Matches(pexprLive)))
	{
		eres = GPOS_FAILED;
	}
	CRefCount::SafeRelease(pexprIdentity);
	pmodelIdentity->Release();
	CRefCount::SafeRelease(pruleIdentity);
	pexprLive->Release();
	pmodel->Release();
	CRefCount::SafeRelease(prule);
	pexprGet->Release();
	return eres;
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_MaxOneRowReplacement()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CExpression *pexprGet = fix.PexprLogicalGet("scalar_input", 1, nullptr);
	pexprGet->AddRef();
	CExpression *pexprLive = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalMaxOneRow(mp), pexprGet);

	CDSLRule *prule = Prule(mp,
		"MaxOneRow(Input<t0>)|AssertMaxOneRow(Input<t1>)|TableEq(t1,t0)");
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	GPOS_RESULT eres = GPOS_OK;
	if (nullptr == prule ||
		!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprLive, pmodel))
	{
		eres = GPOS_FAILED;
	}

	CExpression *pexprTarget = nullptr;
	if (GPOS_OK == eres)
	{
		CDSLConstraintChecker checker(mp);
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		if (!checker.FCheck(prule, pmodel) || nullptr == pexprTarget ||
			COperator::EopLogicalAssert != pexprTarget->Pop()->Eopid() ||
			2 != pexprTarget->Arity() ||
			COperator::EopLogicalSequenceProject !=
				(*pexprTarget)[0]->Pop()->Eopid() ||
			gpos::CException::ExmiSQLMaxOneRow !=
				CLogicalAssert::PopConvert(pexprTarget->Pop())
					->Pexc()->Minor())
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	CRefCount::SafeRelease(prule);
	pexprLive->Release();
	pexprGet->Release();
	return eres;
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_WindowRowsRoundTrip()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *pdrgpcr = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("windowed", 2, &pdrgpcr);
	CExpression *pexprLive =
		PexprWindowRows(mp, fix, pexprGet, (*pdrgpcr)[0], (*pdrgpcr)[1]);

	CDSLRule *prule = Prule(mp,
		"WindowRows<a0 o0 w0>(Input<t0>)|"
		"WindowRows<a1 o1 w1>(Input<t1>)|"
		"TableEq(t1,t0);AttrsEq(a1,a0);OrderEq(o1,o0);WindowEq(w1,w0);"
		"ErrorFree(w0);ErrorFree(w1)");
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	GPOS_RESULT eres = GPOS_OK;
	if (nullptr == prule ||
		!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprLive, pmodel))
	{
		eres = GPOS_FAILED;
	}

	CExpression *pexprTarget = nullptr;
	if (GPOS_OK == eres)
	{
		CDSLConstraintChecker checker(mp);
		CDSLInstantiator instantiator(mp);
		if (!checker.FCheck(prule, pmodel) ||
			nullptr == (pexprTarget = instantiator.PexprInstantiate(prule, pmodel)) ||
			COperator::EopLogicalSequenceProject !=
				pexprTarget->Pop()->Eopid() ||
			!pexprTarget->Matches(pexprLive))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	CRefCount::SafeRelease(prule);
	pexprLive->Release();
	pexprGet->Release();
	return eres;
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_FusedLimitSortRoundTrip()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *pdrgpcr = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("ordered", 2, &pdrgpcr);
	CExpression *pexprLive = PexprLimit(
		mp, pexprGet, PosOne(mp, (*pdrgpcr)[0], EdslsortAsc), true, 0, 7);

	CDSLRule *prule = Prule(mp,
		"Limit<n0 n1>(SortAsc<a0>(Input<t0>))|"
		"Limit<n2 n3>(SortAsc<a1>(Input<t1>))|"
		"TableEq(t1,t0);AttrsEq(a1,a0);ScalarEq(n2,n0);"
		"ScalarEq(n3,n1)");
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_RESULT eres = GPOS_OK;
	if (nullptr == prule
		|| !matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprLive, pmodel)
		|| 4 != pmodel->Size())
	{
		eres = GPOS_FAILED;
	}

	CExpression *pexprTgt = nullptr;
	if (GPOS_OK == eres)
	{
		CDSLInstantiator instantiator(mp);
		pexprTgt = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt
			|| COperator::EopLogicalLimit != pexprTgt->Pop()->Eopid()
			|| 1
				!= CLogicalLimit::PopConvert(pexprTgt->Pop())
					   ->Pos()
					   ->UlSortColumns()
			|| !CLogicalLimit::PopConvert(pexprTgt->Pop())->FHasCount()
			|| COperator::EopLogicalLimit == (*pexprTgt)[0]->Pop()->Eopid()
			|| !(*pexprTgt)[2]->Matches((*pexprLive)[2]))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	CRefCount::SafeRelease(prule);
	pexprLive->Release();
	pexprGet->Release();
	return eres;
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_SortOverLimitStaysNested()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *pdrgpcr = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("nested", 2, &pdrgpcr);
	CExpression *pexprInner
		= PexprLimit(mp, pexprGet, GPOS_NEW(mp) COrderSpec(mp), true, 0, 5);
	CExpression *pexprOuter = PexprLimit(
		mp, pexprInner, PosOne(mp, (*pdrgpcr)[0], EdslsortDesc), false, 0, 0);

	CDSLRule *prule = Prule(mp,
		"SortDesc<a0>(Limit<n0 n1>(Input<t0>))|"
		"SortDesc<a1>(Limit<n2 n3>(Input<t1>))|"
		"TableEq(t1,t0);AttrsEq(a1,a0);ScalarEq(n2,n0);"
		"ScalarEq(n3,n1)");
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_RESULT eres = GPOS_OK;
	if (nullptr == prule
		|| !matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprOuter, pmodel))
	{
		eres = GPOS_FAILED;
	}

	CExpression *pexprTgt = nullptr;
	if (GPOS_OK == eres)
	{
		CDSLInstantiator instantiator(mp);
		pexprTgt = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt
			|| COperator::EopLogicalLimit != pexprTgt->Pop()->Eopid()
			|| CLogicalLimit::PopConvert(pexprTgt->Pop())->FHasCount()
			|| COperator::EopLogicalLimit != (*pexprTgt)[0]->Pop()->Eopid()
			|| !CLogicalLimit::PopConvert((*pexprTgt)[0]->Pop())->FHasCount())
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	CRefCount::SafeRelease(prule);
	pexprOuter->Release();
	pexprInner->Release();
	pexprGet->Release();
	return eres;
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_PlainLimitRejectsHiddenOrder()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *pdrgpcr = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("hidden_order", 1, &pdrgpcr);
	CExpression *pexprLive = PexprLimit(
		mp, pexprGet, PosOne(mp, (*pdrgpcr)[0], EdslsortAsc), true, 0, 3);
	CDSLRule *prule = Prule(mp,
		"Limit<n0 n1>(Input<t0>)|Limit<n2 n3>(Input<t1>)|"
		"TableEq(t1,t0);ScalarEq(n2,n0);ScalarEq(n3,n1)");
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_RESULT eres = (nullptr != prule
						   && !matcher.FMatch(
							   prule->PfragSrc()->PopRoot(), pexprLive, pmodel))
		? GPOS_OK
		: GPOS_FAILED;

	pmodel->Release();
	CRefCount::SafeRelease(prule);
	pexprLive->Release();
	pexprGet->Release();
	return eres;
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_OffsetOnlyLimitRoundTrip()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CExpression *pexprGet = fix.PexprLogicalGet("offset_only", 1, nullptr);
	CExpression *pexprLive
		= PexprLimit(mp, pexprGet, GPOS_NEW(mp) COrderSpec(mp), false, 2, 0);
	CDSLRule *prule = Prule(mp,
		"Limit<n0 n1>(Input<t0>)|Limit<n2 n3>(Input<t1>)|"
		"TableEq(t1,t0);ScalarEq(n2,n0);ScalarEq(n3,n1)");
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_RESULT eres = GPOS_OK;
	if (nullptr == prule
		|| !matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprLive, pmodel))
	{
		eres = GPOS_FAILED;
	}

	CExpression *pexprTgt = nullptr;
	if (GPOS_OK == eres)
	{
		CDSLInstantiator instantiator(mp);
		pexprTgt = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt
			|| CLogicalLimit::PopConvert(pexprTgt->Pop())->FHasCount()
			|| CUtils::FHasZeroOffset(pexprTgt)
			|| !CLogicalLimit::PopConvert(pexprTgt->Pop())->Pos()->IsEmpty())
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	CRefCount::SafeRelease(prule);
	pexprLive->Release();
	pexprGet->Release();
	return eres;
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_NonDefaultNullOrderRejects()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *pdrgpcr = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("null_order", 1, &pdrgpcr);
	IMDId *pmdid
		= (*pdrgpcr)[0]->RetrieveType()->GetMdidForCmpType(IMDType::EcmptL);
	pmdid->AddRef();
	COrderSpec *pos = GPOS_NEW(mp) COrderSpec(mp);
	pos->Append(pmdid, (*pdrgpcr)[0], COrderSpec::EntFirst);
	CExpression *pexprLive = PexprLimit(mp, pexprGet, pos, false, 0, 0);
	CDSLRule *prule = Prule(mp,
		"SortAsc<a0>(Input<t0>)|SortAsc<a1>(Input<t1>)|"
		"TableEq(t1,t0);AttrsEq(a1,a0)");
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_RESULT eres = (nullptr != prule
						   && !matcher.FMatch(
							   prule->PfragSrc()->PopRoot(), pexprLive, pmodel))
		? GPOS_OK
		: GPOS_FAILED;

	pmodel->Release();
	CRefCount::SafeRelease(prule);
	pexprLive->Release();
	pexprGet->Release();
	return eres;
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_TargetScalarConstants()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CExpression *pexprGet = fix.PexprLogicalGet("target_constants", 1, nullptr);
	CDSLRule *prule = Prule(mp,
		"Input<t0>|Limit<n0 n1>(Input<t1>)|"
		"TableEq(t1,t0);ScalarOne(n0);ScalarZero(n1)");
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	GPOS_RESULT eres = GPOS_OK;
	if (nullptr == prule ||
		!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGet, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}

	CExpression *pexprTgt = nullptr;
	if (GPOS_OK == eres)
	{
		CDSLInstantiator instantiator(mp);
		pexprTgt = instantiator.PexprInstantiate(prule, pmodel);
		CExpression *pexprOne = CUtils::PexprScalarConstInt8(mp, 1);
		if (nullptr == pexprTgt ||
			COperator::EopLogicalLimit != pexprTgt->Pop()->Eopid() ||
			!CLogicalLimit::PopConvert(pexprTgt->Pop())->FHasCount() ||
			!CUtils::FHasZeroOffset(pexprTgt) ||
			!(*pexprTgt)[2]->Matches(pexprOne))
		{
			eres = GPOS_FAILED;
		}
		pexprOne->Release();
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	CRefCount::SafeRelease(prule);
	pexprGet->Release();
	return eres;
}

// EOF
