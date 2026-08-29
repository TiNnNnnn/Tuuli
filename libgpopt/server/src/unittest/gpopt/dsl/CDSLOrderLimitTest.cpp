#include "unittest/gpopt/dsl/CDSLOrderLimitTest.h"

#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/operators/CLogicalLimit.h"
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
	};
	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
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
