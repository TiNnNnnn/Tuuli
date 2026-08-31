//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLAggTest.cpp
//
//	@doc:
//		Implementation of dedup elimination and corpus-format real Agg three-stage
//		tests. The latter uses bare Agg<a a f s p>, matching MONSOON/dataset/rules.
//
//		As with join elimination the source and target output-column SETS are not
//		equal (the GbAgg outputs only its grouping cols; the Select outputs the
//		child's full columns — a superset), so the invariant asserted is
//		ContainsAll, not Equals.
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLAggTest.h"

#include "gpos/base.h"
#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalLeftAntiSemiApply.h"
#include "gpopt/operators/CLogicalLeftAntiSemiJoin.h"
#include "gpopt/operators/CLogicalLeftSemiApply.h"
#include "gpopt/operators/CScalarAggFunc.h"
#include "gpopt/operators/CScalarSortGroupClause.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

// Semantic source: WeTune dedup identity — a deduplicated projection over a
// relation equals a plain projection when the deduplicated columns are unique
// (SELECT DISTINCT k = SELECT k when k is a key). Proj* (dedup) routes to the
// GbAgg bucket; the target is the bare Input (drop the dedup). This is the DSL
// analogue of ORCA's CXformSimplifyGbAgg::FDropGbAgg.
#define GPOPT_DSL_DISTINCT_ELIM_RULE                                        \
	"Proj*<a0 s0>(Input<t0>)|"                                              \
	"Input<t2>|"                                                            \
	"AttrsSub(a0,t0);Unique(t0,a0);TableEq(t2,t0)"

#define GPOPT_DSL_DISTINCT_TO_PROJ_RULE                                     \
	"Proj*<a0 s0>(Input<t0>)|"                                              \
	"Proj<a1 s1>(Input<t1>)|"                                               \
	"AttrsSub(a0,t0);Unique(t0,a0);TableEq(t1,t0);"                         \
	"AttrsEq(a1,a0);SchemaEq(s1,s0)"

#define GPOPT_DSL_AGG_IDENTITY_RULE                                         \
	"Agg<a0 a1 f0 s0 p0>(Input<t0>)|"                                      \
	"Agg<a2 a3 f1 s1 p1>(Input<t1>)|"                                      \
	"AttrsSub(a0,t0);AttrsSub(a1,t0);"                                     \
	"TableEq(t1,t0);AttrsEq(a2,a0);AttrsEq(a3,a1);"                        \
	"FuncEq(f1,f0);SchemaEq(s1,s0);PredicateEq(p1,p0)"

#define GPOPT_DSL_AGG_MINIMAL_GROUPING_RULE                                \
	"Agg<a0 a1 f0 s0 p0>(Input<t0>)|"                                      \
	"Agg<a2 a3 f1 s1 p1>(Input<t1>)|"                                      \
	"AttrsSub(a0,t0);AttrsSub(a1,t0);"                                     \
	"TableEq(t1,t0);AttrsEq(a2,a0);AttrsEq(a3,a1);"                        \
	"FuncEq(f1,f0);SchemaEq(s1,s0);PredicateEq(p1,p0);"                    \
	"MinimalGrouping(a0,s0)"

#define GPOPT_DSL_AGG_FILTER_COMMUTE_RULE                                  \
	"Agg<a0 a1 f0 s0 p0>(Filter<p1 a2 a3>(Input<t0>))|"                   \
	"Filter<p2 a6 a7>(Agg<a4 a5 f1 s1 p3>(Input<t1>))|"                  \
	"TableEq(t1,t0);AttrsEq(a4,a0);AttrsEq(a5,a1);FuncEq(f1,f0);"         \
	"SchemaEq(s1,s0);PredicateEq(p3,p0);PredicateEq(p2,p1);"              \
	"AttrsEq(a6,a2);AttrsEq(a7,a3);AttrsSub(a2,a0);"                      \
	"AggFilterCommute(a0,a1,f0,s0,p0,p1,a2)"

#define GPOPT_DSL_AGG_CORRELATION_PULLUP_RULE                              \
	"SemiApply<p0 a0 a1 a2>(Input<t0>,Agg<a3 a4 f0 s0 p1>(Filter<p2 a5 " \
	"a6>(Input<t1>)))|SemiJoin<p3 a7 a8>(Input<t2>,Agg<a9 a10 f1 s1 "      \
	"p4>(Input<t3>))|TableEq(t2,t0);TableEq(t3,t1);"                        \
	"PredicateAnd(p3,p0,p2);AttrsEq(a2,a6);AttrsUnion(a7,a0,a6);"           \
	"AttrsUnion(a8,a1,a5);AttrsUnion(a9,a3,a5);AttrsEq(a10,a4);"            \
	"FuncEq(f1,f0);SchemaUnion(s1,s0,a5);PredicateEq(p4,p1);"               \
	"AggCorrelationPullup(p0,p2,p3,a3,a9,a4,f0,s0,s1,p1,a5,a6);"           \
	"CorrelationEquality(p2,a5,a6);"                                       \
	"AggCorrelationGrouping(p2,a3,a9,a4,f0,s0,s1,p1,a5,a6);"              \
	"AttrsSub(a0,t0);AttrsSub(a1,s0);AttrsSub(a3,t1);AttrsSub(a4,t1);"       \
	"AttrsSub(a5,t1);AttrsSub(a6,t0)"

#define GPOPT_DSL_AGG_ANTI_CORRELATION_PULLUP_RULE                         \
	"AntiApply<p0 a0 a1 a2>(Input<t0>,Agg<a3 a4 f0 s0 p1>(Filter<p2 a5 " \
	"a6>(Input<t1>)))|AntiJoin<p3 a7 a8>(Input<t2>,Agg<a9 a10 f1 s1 "      \
	"p4>(Input<t3>))|TableEq(t2,t0);TableEq(t3,t1);"                        \
	"PredicateAnd(p3,p0,p2);AttrsEq(a2,a6);AttrsUnion(a7,a0,a6);"           \
	"AttrsUnion(a8,a1,a5);AttrsUnion(a9,a3,a5);AttrsEq(a10,a4);"            \
	"FuncEq(f1,f0);SchemaUnion(s1,s0,a5);PredicateEq(p4,p1);"               \
	"AggCorrelationPullup(p0,p2,p3,a3,a9,a4,f0,s0,s1,p1,a5,a6);"           \
	"AttrsSub(a0,t0);AttrsSub(a1,s0);AttrsSub(a3,t1);AttrsSub(a4,t1);"       \
	"AttrsSub(a5,t1);AttrsSub(a6,t0)"

#define GPOPT_DSL_CORRELATION_EQUALITY_RULE                               \
	"Filter<p0 a0 a1>(Input<t0>)|Filter<p1 a2 a3>(Input<t1>)|"            \
	"TableEq(t1,t0);PredicateEq(p1,p0);AttrsEq(a2,a0);AttrsEq(a3,a1);"    \
	"CorrelationEquality(p0,a0,a1)"

#define GPOPT_DSL_INTERSECT_GROUPING_RULE                                  \
	"Proj*<a2 s0>(InnerJoin<a0 a1>(Input<t0>,Input<t1>))|"                 \
	"Proj<a7 s2>(InnerJoin<a3 a4>(Proj*<a5 s1>(Input<t2>),Input<t3>))|"   \
	"AttrsSub(a0,a2);AttrsSub(a1,t1);"                                    \
	"TableEq(t2,t0);TableEq(t3,t1);AttrsEq(a3,a0);AttrsEq(a4,a1);"       \
	"AttrsIntersect(a5,a2,t0);AttrsIntersect(s1,s0,t0);"                  \
	"AttrsEq(a7,a2);SchemaEq(s2,s0)"

static CDSLRule *
PdslruleParseLocal(CMemoryPool *mp, const CHAR *sz_dsl)
{
	CWStringDynamic strErr(mp);
	return CDSLRuleParser::PdslruleParse(mp, sz_dsl, "EQ" /*verdict*/, &strErr);
}

//---------------------------------------------------------------------------
//	@function:
//		PexprDedupGbAgg
//
//	@doc:
//		Build GbAgg(grouping=[t0.c0], Get t0[2]) with an EMPTY agg list — ORCA's
//		SELECT DISTINCT c0. t0's c0 is a unique key iff fUniqueKey. Hands back the
//		Get (for pointer-identity checks) and the GbAgg root. Caller owns both refs.
//---------------------------------------------------------------------------
static void
BuildDedupGbAgg(CDSLTestFixture &fix, BOOL fUniqueKey, CExpression **ppGet,
				CExpression **ppGbAgg)
{
	CMemoryPool *mp = fix.Pmp();

	CColRefArray *pdrgpcrT0 = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet(
		"t0", 2, &pdrgpcrT0, fUniqueKey ? 0 /*ulKeyCol*/ : gpos::ulong_max);

	// group by the (unique) key column c0.
	CColRefArray *pdrgpcrGrp = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGrp->Append((*pdrgpcrT0)[0]);
	CExpression *pexprGbAgg = fix.PexprLogicalGbAgg(pexprGet, pdrgpcrGrp);
	pdrgpcrGrp->Release();

	*ppGet = pexprGet;
	*ppGbAgg = pexprGbAgg;
}

static void
BuildRealGbAgg(CDSLTestFixture &fix, CExpression **ppGet,
			   CExpression **ppGbAgg, CColRefArray **ppdrgpcrInput,
			   CColRef **ppcrAggOut)
{
	CMemoryPool *mp = fix.Pmp();
	CColRefArray *pdrgpcrInput = nullptr;
	CExpression *pexprGet =
		fix.PexprLogicalGet("t0", 2, &pdrgpcrInput);
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrInput)[0]);
	CColRef *pcrAggOut = fix.PcrCreateInt4("max_c1");
	CExpression *pexprGbAgg = fix.PexprLogicalGbAgg(
		pexprGet, pdrgpcrGroup, pcrAggOut, (*pdrgpcrInput)[1]);
	pdrgpcrGroup->Release();

	*ppGet = pexprGet;
	*ppGbAgg = pexprGbAgg;
	*ppdrgpcrInput = pdrgpcrInput;
	*ppcrAggOut = pcrAggOut;
}

static void
BuildDistinctGbAgg(CDSLTestFixture &fix, BOOL fUniqueKey,
				   CExpression **ppGet, CExpression **ppGbAgg)
{
	CMemoryPool *mp = fix.Pmp();
	CColRefArray *pdrgpcrInput = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet(
		"t0", 2, &pdrgpcrInput,
		fUniqueKey ? 0 /*ulKeyCol*/ : gpos::ulong_max);
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrInput)[0]);
	CColRef *pcrAggOut = fix.PcrCreateInt4("max_distinct_c1");
	CExpression *pexprGbAgg = fix.PexprLogicalGbAgg(
		pexprGet, pdrgpcrGroup, pcrAggOut, (*pdrgpcrInput)[1]);
	pdrgpcrGroup->Release();

	CExpression *pexprPrEl = (*(*pexprGbAgg)[1])[0];
	CExpression *pexprFunc = (*pexprPrEl)[0];
	CScalarAggFunc::PopConvert(pexprFunc->Pop())->SetIsDistinct(true);
	(*pexprFunc)[EaggfuncIndexDistinct]->PdrgPexpr()->Append(
		GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarSortGroupClause(
					mp, 0 /*tle_sort_group_ref*/, 96 /*eqop*/, 97 /*sortop*/,
					false /*nulls_first*/, true /*hashable*/)));
	*ppGet = pexprGet;
	*ppGbAgg = pexprGbAgg;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLAggTest::EresUnittest
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLAggTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_MatchBindsDedupGbAgg),
		GPOS_UNITTEST_FUNC(
			CDSLAggTest::EresUnittest_InstantiateProducesSelectOverChild),
		GPOS_UNITTEST_FUNC(
			CDSLAggTest::EresUnittest_InstantiateDedupToPlainProj),
		GPOS_UNITTEST_FUNC(
			CDSLAggTest::EresUnittest_InstantiateIntersectedGrouping),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_RejectsWithoutUnique),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_RejectsNonEmptyAggList),
		GPOS_UNITTEST_FUNC(
			CDSLAggTest::EresUnittest_InstantiateDistinctAggregateToPlain),
		GPOS_UNITTEST_FUNC(
			CDSLAggTest::EresUnittest_DistinctAggregateRejectsWithoutUnique),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_MatchBindsRealAgg),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_InstantiateRealAgg),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_MinimalGroupingMetadata),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_CopySplitGlobalGbAgg),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_HavingRoundTrip),
		GPOS_UNITTEST_FUNC(
			CDSLAggTest::EresUnittest_AggFilterCommuteGroupingGuard),
		GPOS_UNITTEST_FUNC(
			CDSLAggTest::EresUnittest_AggCorrelationPullup),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_RejectsWrongAggFunction),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_NoFireOnWrongRoot),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLAggTest::EresUnittest_CopySplitGlobalGbAgg()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	CColRefArray *pdrgpcrMinimal = GPOS_NEW(mp) CColRefArray(mp);
	CLogicalGbAgg *popOriginal = GPOS_NEW(mp) CLogicalGbAgg(
		mp, pdrgpcrGroup, pdrgpcrMinimal, COperator::EgbaggtypeGlobal);
	GPOS_ASSERT(popOriginal->FGlobal());
	GPOS_ASSERT(popOriginal->FGeneratesDuplicates());

	UlongToColRefMap *colref_mapping = GPOS_NEW(mp) UlongToColRefMap(mp);
	COperator *popCopy = popOriginal->PopCopyWithRemappedColumns(
		mp, colref_mapping, false /*must_exist*/);
	colref_mapping->Release();

	CLogicalGbAgg *popGbAggCopy = CLogicalGbAgg::PopConvert(popCopy);
	GPOS_ASSERT(popGbAggCopy->FGlobal());
	GPOS_ASSERT(popGbAggCopy->FGeneratesDuplicates());
	GPOS_ASSERT(nullptr == popGbAggCopy->PdrgpcrArgDQA());

	popCopy->Release();
	popOriginal->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_MinimalGroupingMetadata()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_AGG_MINIMAL_GROUPING_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrInput = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet(
		"fd_agg", 3, &pdrgpcrInput, 0 /*ulKeyCol*/);
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrInput)[0]);
	pdrgpcrGroup->Append((*pdrgpcrInput)[1]);
	CColRef *pcrAggOut = fix.PcrCreateInt4("max_fd_c2");
	CExpression *pexprAgg = fix.PexprLogicalGbAgg(
		pexprGet, pdrgpcrGroup, pcrAggOut, (*pdrgpcrInput)[2]);
	pdrgpcrGroup->Release();

	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprAgg, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTarget ||
			COperator::EopLogicalGbAgg != pexprTarget->Pop()->Eopid())
		{
			eres = GPOS_FAILED;
		}
		else
		{
			CLogicalGbAgg *popTarget =
				CLogicalGbAgg::PopConvert(pexprTarget->Pop());
			if (2 != popTarget->Pdrgpcr()->Size() ||
				nullptr == popTarget->PdrgpcrMinimal() ||
				1 != popTarget->PdrgpcrMinimal()->Size() ||
				(*pdrgpcrInput)[0] != (*popTarget->PdrgpcrMinimal())[0])
			{
				eres = GPOS_FAILED;
			}
		}
	}

	// The metadata constructor is one-shot. This is the property-level analogue
	// of native SimplifyGbAgg's PdrgpcrMinimal promise guard.
	if (nullptr != pexprTarget)
	{
		CDSLModel *pmodelAgain = GPOS_NEW(mp) CDSLModel(mp);
		if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprTarget,
						 pmodelAgain) ||
			checker.FCheck(prule, pmodelAgain))
		{
			eres = GPOS_FAILED;
		}
		pmodelAgain->Release();
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprAgg->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_AggFilterCommuteGroupingGuard()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_AGG_FILTER_COMMUTE_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrInput = nullptr;
	CExpression *pexprGet =
		fix.PexprLogicalGet("t0", 2, &pdrgpcrInput);
	CColRef *pcrOuter = fix.PcrCreateInt4("outer_g");
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrInput)[0]);
	CColRef *pcrAggOut = fix.PcrCreateInt4("max_c1");

	CExpression *pexprPred =
		fix.PexprEqPred((*pdrgpcrInput)[0], pcrOuter);
	CExpression *pexprSelect =
		fix.PexprLogicalSelect(pexprGet, pexprPred);
	CExpression *pexprPlainAgg = fix.PexprLogicalGbAgg(
		pexprSelect, pdrgpcrGroup, pcrAggOut, (*pdrgpcrInput)[1]);
	// Preprocessing can attach a child-dependent minimal grouping set before the
	// expression reaches Cascade.  A real DSL Agg must still match that memo
	// representation, while its target is rebuilt from the full grouping set.
	CColRefArray *pdrgpcrMinimal = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrMinimal->Append((*pdrgpcrInput)[0]);
	pdrgpcrGroup->AddRef();
	(*pexprPlainAgg)[0]->AddRef();
	(*pexprPlainAgg)[1]->AddRef();
	CExpression *pexprAgg = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CLogicalGbAgg(
			mp, pdrgpcrGroup, pdrgpcrMinimal,
			COperator::EgbaggtypeGlobal),
		(*pexprPlainAgg)[0], (*pexprPlainAgg)[1]);
	pexprPlainAgg->Release();

	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprAgg, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTarget = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTarget ||
			COperator::EopLogicalSelect != pexprTarget->Pop()->Eopid() ||
			COperator::EopLogicalGbAgg != (*pexprTarget)[0]->Pop()->Eopid() ||
			nullptr != CLogicalGbAgg::PopConvert(
						(*pexprTarget)[0]->Pop())->PdrgpcrMinimal() ||
			(*(*pexprTarget)[0])[0] != pexprGet ||
			!(*pexprTarget)[1]->Matches(pexprPred))
		{
			eres = GPOS_FAILED;
		}
	}

	// An identity-shaped Agg rule must retain the source annotation. Otherwise
	// it creates a second Global aggregate that native split/collapse xforms can
	// repeatedly expand with fresh aggregate-output columns.
	CDSLRule *pruleIdentity =
		PdslruleParseLocal(mp, GPOPT_DSL_AGG_IDENTITY_RULE);
	CDSLModel *pmodelIdentity = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcherIdentity(mp, pruleIdentity);
	CExpression *pexprIdentity = nullptr;
	if (nullptr == pruleIdentity ||
		!matcherIdentity.FMatch(
			pruleIdentity->PfragSrc()->PopRoot(), pexprAgg, pmodelIdentity) ||
		!checker.FCheck(pruleIdentity, pmodelIdentity))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprIdentity = inst.PexprInstantiate(pruleIdentity, pmodelIdentity);
		if (nullptr == pexprIdentity ||
			COperator::EopLogicalGbAgg != pexprIdentity->Pop()->Eopid() ||
			nullptr == CLogicalGbAgg::PopConvert(
						pexprIdentity->Pop())->PdrgpcrMinimal())
		{
			eres = GPOS_FAILED;
		}
	}

	// A predicate on a non-grouping input column must not commute. It would not
	// be evaluable above the aggregate and is outside the proved semantic domain.
	CExpression *pexprNonGroupPred =
		fix.PexprEqPred((*pdrgpcrInput)[1], pcrOuter);
	CExpression *pexprNonGroupSelect =
		fix.PexprLogicalSelect(pexprGet, pexprNonGroupPred);
	CExpression *pexprNonGroupAgg = fix.PexprLogicalGbAgg(
		pexprNonGroupSelect, pdrgpcrGroup, pcrAggOut, (*pdrgpcrInput)[1]);
	CDSLModel *pmodelReject = GPOS_NEW(mp) CDSLModel(mp);
	if (!matcher.FMatch(
			prule->PfragSrc()->PopRoot(), pexprNonGroupAgg, pmodelReject) ||
		checker.FCheck(prule, pmodelReject))
	{
		eres = GPOS_FAILED;
	}

	pmodelReject->Release();
	CRefCount::SafeRelease(pexprIdentity);
	pmodelIdentity->Release();
	CRefCount::SafeRelease(pruleIdentity);
	pmodel->Release();
	CRefCount::SafeRelease(pexprTarget);
	pexprNonGroupAgg->Release();
	pexprNonGroupSelect->Release();
	pexprNonGroupPred->Release();
	pexprAgg->Release();
	pexprSelect->Release();
	pexprPred->Release();
	pdrgpcrGroup->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_AggCorrelationPullup()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_AGG_CORRELATION_PULLUP_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrOuter = nullptr;
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("agg_corr_outer", 2, &pdrgpcrOuter);
	CExpression *pexprInner =
		fix.PexprLogicalGet("agg_corr_inner", 2, &pdrgpcrInner);
	CExpression *pexprCorrelation =
		fix.PexprEqPred((*pdrgpcrInner)[0], (*pdrgpcrOuter)[0]);
	CExpression *pexprSelect =
		fix.PexprLogicalSelect(pexprInner, pexprCorrelation);
	pexprCorrelation->Release();
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrInner)[1]);
	CColRef *pcrAggOut = fix.PcrCreateInt4("max_corr_v");
	CExpression *pexprAgg = fix.PexprLogicalGbAgg(
		pexprSelect, pdrgpcrGroup, pcrAggOut, (*pdrgpcrInner)[1]);
	pexprSelect->Release();
	pexprOuter->AddRef();
	pexprAgg->AddRef();
	CExpression *pexprApply =
		CUtils::PexprLogicalApply<CLogicalLeftSemiApply>(
			mp, pexprOuter, pexprAgg, (*pdrgpcrInner)[1],
			COperator::EopScalarSubqueryExists);

	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	CDSLRule *pruleCorrelation =
		PdslruleParseLocal(mp, GPOPT_DSL_CORRELATION_EQUALITY_RULE);
	CDSLModel *pmodelCorrelation = GPOS_NEW(mp) CDSLModel(mp);
	if (nullptr == pruleCorrelation)
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLMatcher matcherCorrelation(mp, pruleCorrelation);
		if (!matcherCorrelation.FMatch(
				pruleCorrelation->PfragSrc()->PopRoot(), (*pexprAgg)[0],
				pmodelCorrelation) ||
			!checker.FCheck(pruleCorrelation, pmodelCorrelation))
		{
			eres = GPOS_FAILED;
		}
	}
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprApply, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTarget ||
			COperator::EopLogicalLeftSemiJoin != pexprTarget->Pop()->Eopid() ||
			COperator::EopLogicalGbAgg != (*pexprTarget)[1]->Pop()->Eopid())
		{
			eres = GPOS_FAILED;
		}
		else
		{
			CLogicalGbAgg *popTargetAgg =
				CLogicalGbAgg::PopConvert((*pexprTarget)[1]->Pop());
			CColRefSet *pcrsTargetGroup = GPOS_NEW(mp) CColRefSet(mp);
			pcrsTargetGroup->Include(popTargetAgg->Pdrgpcr());
			if (2 != popTargetAgg->Pdrgpcr()->Size() ||
				!pcrsTargetGroup->FMember((*pdrgpcrInner)[0]) ||
				!pcrsTargetGroup->FMember((*pdrgpcrInner)[1]) ||
				!(*pexprTarget)[1]->DeriveOutputColumns()->FMember(
					(*pdrgpcrInner)[0]))
			{
				eres = GPOS_FAILED;
			}
			pcrsTargetGroup->Release();
		}
	}

	// The same grouping contract is polarity-independent: ordinary NOT EXISTS
	// builds an anti semi join with the identical expanded aggregate schema.
	CDSLRule *pruleAnti =
		PdslruleParseLocal(mp, GPOPT_DSL_AGG_ANTI_CORRELATION_PULLUP_RULE);
	CExpression *pexprAntiTarget = nullptr;
	CDSLModel *pmodelAnti = GPOS_NEW(mp) CDSLModel(mp);
	if (nullptr == pruleAnti)
	{
		eres = GPOS_FAILED;
	}
	else
	{
		pexprOuter->AddRef();
		pexprAgg->AddRef();
		CExpression *pexprAntiApply =
			CUtils::PexprLogicalApply<CLogicalLeftAntiSemiApply>(
				mp, pexprOuter, pexprAgg, (*pdrgpcrInner)[1],
				COperator::EopScalarSubqueryNotExists);
		CDSLMatcher matcherAnti(mp, pruleAnti);
		if (!matcherAnti.FMatch(pruleAnti->PfragSrc()->PopRoot(),
							pexprAntiApply, pmodelAnti) ||
			!checker.FCheck(pruleAnti, pmodelAnti))
		{
			eres = GPOS_FAILED;
		}
		else
		{
			CDSLInstantiator instantiator(mp);
			pexprAntiTarget =
				instantiator.PexprInstantiate(pruleAnti, pmodelAnti);
			if (nullptr == pexprAntiTarget ||
				COperator::EopLogicalLeftAntiSemiJoin !=
					pexprAntiTarget->Pop()->Eopid() ||
				COperator::EopLogicalGbAgg !=
					(*pexprAntiTarget)[1]->Pop()->Eopid() ||
				2 != CLogicalGbAgg::PopConvert(
						 (*pexprAntiTarget)[1]->Pop())->Pdrgpcr()->Size())
			{
				eres = GPOS_FAILED;
			}
		}
		pexprAntiApply->Release();

		// If the correlation key is already grouped, ordered unions are stable
		// identities and the same atomic rule must still decorrelate the Apply.
		CExpression *pexprGroupedCorrelation =
			fix.PexprEqPred((*pdrgpcrInner)[0], (*pdrgpcrOuter)[0]);
		CExpression *pexprGroupedSelect =
			fix.PexprLogicalSelect(pexprInner, pexprGroupedCorrelation);
		pexprGroupedCorrelation->Release();
		CColRefArray *pdrgpcrExistingGroup = GPOS_NEW(mp) CColRefArray(mp);
		pdrgpcrExistingGroup->Append((*pdrgpcrInner)[0]);
		pdrgpcrExistingGroup->Append((*pdrgpcrInner)[1]);
		CExpression *pexprGroupedAgg = fix.PexprLogicalGbAgg(
			pexprGroupedSelect, pdrgpcrExistingGroup, pcrAggOut,
			(*pdrgpcrInner)[1]);
		pexprGroupedSelect->Release();
		pexprOuter->AddRef();
		pexprGroupedAgg->AddRef();
		CExpression *pexprGroupedAntiApply =
			CUtils::PexprLogicalApply<CLogicalLeftAntiSemiApply>(
				mp, pexprOuter, pexprGroupedAgg, (*pdrgpcrInner)[1],
				COperator::EopScalarSubqueryNotExists);
		CDSLModel *pmodelGrouped = GPOS_NEW(mp) CDSLModel(mp);
		if (!matcherAnti.FMatch(pruleAnti->PfragSrc()->PopRoot(),
							pexprGroupedAntiApply, pmodelGrouped) ||
			!checker.FCheck(pruleAnti, pmodelGrouped))
		{
			eres = GPOS_FAILED;
		}
		pmodelGrouped->Release();
		pexprGroupedAntiApply->Release();
		pexprGroupedAgg->Release();
		pdrgpcrExistingGroup->Release();
	}

	// The same shape with independent local/outer atoms is correlated but is
	// not an equality edge, so the generic semantic contract must reject it.
	CColRef *rgpcrAtoms[] = {(*pdrgpcrInner)[0], (*pdrgpcrOuter)[0]};
	CExpression *pexprNonEquality =
		fix.PexprConjunctionOfAtoms(rgpcrAtoms, GPOS_ARRAY_SIZE(rgpcrAtoms));
	CExpression *pexprBadSelect =
		fix.PexprLogicalSelect(pexprInner, pexprNonEquality);
	pexprNonEquality->Release();
	CExpression *pexprBadAgg = fix.PexprLogicalGbAgg(
		pexprBadSelect, pdrgpcrGroup, pcrAggOut, (*pdrgpcrInner)[1]);
	pexprBadSelect->Release();
	pexprOuter->AddRef();
	pexprBadAgg->AddRef();
	CExpression *pexprBadApply =
		CUtils::PexprLogicalApply<CLogicalLeftSemiApply>(
			mp, pexprOuter, pexprBadAgg, (*pdrgpcrInner)[1],
			COperator::EopScalarSubqueryExists);
	CDSLModel *pmodelBad = GPOS_NEW(mp) CDSLModel(mp);
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprBadApply,
						pmodelBad) || checker.FCheck(prule, pmodelBad))
	{
		eres = GPOS_FAILED;
	}
	CDSLModel *pmodelCorrelationBad = GPOS_NEW(mp) CDSLModel(mp);
	if (nullptr != pruleCorrelation)
	{
		CDSLMatcher matcherCorrelation(mp, pruleCorrelation);
		if (!matcherCorrelation.FMatch(
				pruleCorrelation->PfragSrc()->PopRoot(), (*pexprBadAgg)[0],
				pmodelCorrelationBad) ||
			checker.FCheck(pruleCorrelation, pmodelCorrelationBad))
		{
			eres = GPOS_FAILED;
		}
	}

	pmodelCorrelationBad->Release();
	pmodelBad->Release();
	pexprBadApply->Release();
	pexprBadAgg->Release();
	CRefCount::SafeRelease(pexprTarget);
	CRefCount::SafeRelease(pexprAntiTarget);
	pmodelAnti->Release();
	CRefCount::SafeRelease(pruleAnti);
	pmodel->Release();
	pexprApply->Release();
	pexprAgg->Release();
	pdrgpcrGroup->Release();
	pexprInner->Release();
	pexprOuter->Release();
	pmodelCorrelation->Release();
	CRefCount::SafeRelease(pruleCorrelation);
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_InstantiateIntersectedGrouping()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_INTERSECT_GROUPING_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrLeft = nullptr;
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprLeft =
		fix.PexprLogicalGet("left", 2, &pdrgpcrLeft);
	CExpression *pexprRight =
		fix.PexprLogicalGet("right", 2, &pdrgpcrRight);
	CExpression *pexprPred =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprPred);
	pexprPred->Release();

	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrLeft)[0]);
	pdrgpcrGroup->Append((*pdrgpcrRight)[1]);
	pdrgpcrGroup->Append((*pdrgpcrLeft)[1]);
	CExpression *pexprSource =
		fix.PexprLogicalGbAgg(pexprJoin, pdrgpcrGroup);
	pdrgpcrGroup->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTgt = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTgt = inst.PexprInstantiate(prule, pmodel);
		CExpression *pexprTargetJoin =
			nullptr == pexprTgt ? nullptr : (*pexprTgt)[0];
		CExpression *pexprPushed =
			nullptr == pexprTargetJoin ? nullptr : (*pexprTargetJoin)[0];
		CLogicalGbAgg *popPushed =
			nullptr != pexprPushed &&
				COperator::EopLogicalGbAgg == pexprPushed->Pop()->Eopid()
			? CLogicalGbAgg::PopConvert(pexprPushed->Pop())
			: nullptr;
		if (nullptr == pexprTgt ||
			COperator::EopLogicalSelect != pexprTgt->Pop()->Eopid() ||
			nullptr == pexprTargetJoin ||
			COperator::EopLogicalInnerJoin !=
				pexprTargetJoin->Pop()->Eopid() ||
			nullptr == popPushed || 2 != popPushed->Pdrgpcr()->Size() ||
			(*popPushed->Pdrgpcr())[0] != (*pdrgpcrLeft)[0] ||
			(*popPushed->Pdrgpcr())[1] != (*pdrgpcrLeft)[1] ||
			!pexprTgt->DeriveOutputColumns()->ContainsAll(
				pexprSource->DeriveOutputColumns()))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprSource->Release();
	pexprJoin->Release();
	pexprLeft->Release();
	pexprRight->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_InstantiateDedupToPlainProj()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_DISTINCT_TO_PROJ_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprGbAgg = nullptr;
	BuildDedupGbAgg(fix, true /*fUniqueKey*/, &pexprGet, &pexprGbAgg);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTgt = nullptr;
	GPOS_RESULT eres = GPOS_OK;

	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGbAgg, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTgt = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt ||
			COperator::EopLogicalSelect != pexprTgt->Pop()->Eopid() ||
			(*pexprTgt)[0] != pexprGet)
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprGet->Release();
	pexprGbAgg->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_MatchBindsRealAgg()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_AGG_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprGbAgg = nullptr;
	CColRefArray *pdrgpcrInput = nullptr;
	CColRef *pcrAggOut = nullptr;
	BuildRealGbAgg(fix, &pexprGet, &pexprGbAgg, &pdrgpcrInput, &pcrAggOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLOp *popSrc = prule->PfragSrc()->PopRoot();
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(popSrc, pexprGbAgg, pmodel) || 6 != pmodel->Size())
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLSymbolArray *syms = popSrc->Pdrgpsym();
		CColRefArray *group = pmodel->PdrgpcrAttrs((*syms)[0]);
		CColRefArray *inputs = pmodel->PdrgpcrAttrs((*syms)[1]);
		CExpressionArray *funcs = pmodel->PdrgpexprFunc((*syms)[2]);
		CColRefArray *schema = pmodel->PdrgpcrSchema((*syms)[3]);
		CExpression *having = pmodel->PexprPred((*syms)[4]);
		if (1 != group->Size() || (*group)[0] != (*pdrgpcrInput)[0] ||
			1 != inputs->Size() || (*inputs)[0] != (*pdrgpcrInput)[1] ||
			1 != funcs->Size() ||
			COperator::EopScalarAggFunc != (*funcs)[0]->Pop()->Eopid() ||
			2 != schema->Size() || (*schema)[1] != pcrAggOut ||
			!CUtils::FScalarConstTrue(having))
		{
			eres = GPOS_FAILED;
		}
	}

	pmodel->Release();
	pexprGet->Release();
	pexprGbAgg->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_InstantiateRealAgg()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_AGG_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprGbAgg = nullptr;
	CColRefArray *pdrgpcrInput = nullptr;
	CColRef *pcrAggOut = nullptr;
	BuildRealGbAgg(fix, &pexprGet, &pexprGbAgg, &pdrgpcrInput, &pcrAggOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTgt = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGbAgg, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTgt = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt ||
			COperator::EopLogicalGbAgg != pexprTgt->Pop()->Eopid() ||
			(*pexprTgt)[0] != pexprGet || 1 != (*pexprTgt)[1]->Arity() ||
			!pexprTgt->DeriveOutputColumns()->Equals(
				pexprGbAgg->DeriveOutputColumns()))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprGet->Release();
	pexprGbAgg->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_HavingRoundTrip()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_AGG_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprGbAgg = nullptr;
	CColRefArray *pdrgpcrInput = nullptr;
	CColRef *pcrAggOut = nullptr;
	BuildRealGbAgg(fix, &pexprGet, &pexprGbAgg, &pdrgpcrInput, &pcrAggOut);
	CExpression *pexprHaving = fix.PexprPredAtom(pcrAggOut);
	CExpression *pexprSelect =
		fix.PexprLogicalSelect(pexprGbAgg, pexprHaving);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTgt = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	CDSLOp *popSrc = prule->PfragSrc()->PopRoot();
	if (!matcher.FMatch(popSrc, pexprSelect, pmodel) ||
		pmodel->PexprPred((*popSrc->Pdrgpsym())[4]) != pexprHaving ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTgt = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt ||
			COperator::EopLogicalSelect != pexprTgt->Pop()->Eopid() ||
			COperator::EopLogicalGbAgg != (*pexprTgt)[0]->Pop()->Eopid() ||
			(*(*pexprTgt)[0])[0] != pexprGet || (*pexprTgt)[1] != pexprHaving ||
			!pexprTgt->DeriveOutputColumns()->Equals(
				pexprSelect->DeriveOutputColumns()))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprHaving->Release();
	pexprSelect->Release();
	pexprGet->Release();
	pexprGbAgg->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_RejectsWrongAggFunction()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp,
		"Agg_min<a0 a1 a2 f0 s0 p0>(Input<t0>)|Input<t1>|TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprGbAgg = nullptr;
	CColRefArray *pdrgpcrInput = nullptr;
	CColRef *pcrAggOut = nullptr;
	BuildRealGbAgg(fix, &pexprGet, &pexprGbAgg, &pdrgpcrInput, &pcrAggOut);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_RESULT eres = matcher.FMatch(
		prule->PfragSrc()->PopRoot(), pexprGbAgg, pmodel)
		? GPOS_FAILED
		: GPOS_OK;

	pmodel->Release();
	pexprGet->Release();
	pexprGbAgg->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLAggTest::EresUnittest_MatchBindsDedupGbAgg
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLAggTest::EresUnittest_MatchBindsDedupGbAgg()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_DISTINCT_ELIM_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprGbAgg = nullptr;
	BuildDedupGbAgg(fix, true /*fUniqueKey*/, &pexprGet, &pexprGbAgg);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGbAgg, pmodel))
	{
		// the dedup GbAgg must match the Proj* source root
		eres = GPOS_FAILED;
	}
	else if (!pmodel->FDedupDrop())
	{
		// and the matcher must have flagged a dedup drop
		eres = GPOS_FAILED;
	}

	pmodel->Release();
	pexprGet->Release();
	pexprGbAgg->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLAggTest::EresUnittest_InstantiateProducesSelectOverChild
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLAggTest::EresUnittest_InstantiateProducesSelectOverChild()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_DISTINCT_ELIM_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprGbAgg = nullptr;
	BuildDedupGbAgg(fix, true /*fUniqueKey*/, &pexprGet, &pexprGbAgg);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTgt = nullptr;

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGbAgg, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else if (!checker.FCheck(prule, pmodel))
	{
		// Unique(t0,a0) holds (c0 is t0's key) so the check must pass
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTgt = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt ||
			COperator::EopLogicalSelect != pexprTgt->Pop()->Eopid())
		{
			// dedup drop => target root is Select(child, TRUE) (FDropGbAgg idiom)
			eres = GPOS_FAILED;
		}
		else if ((*pexprTgt)[0] != pexprGet)
		{
			// THE elimination proof: the Select's relational child is the reused
			// t0 Get (pointer identity) — the GbAgg is gone.
			eres = GPOS_FAILED;
		}
		else if (COperator::EopLogicalGet != (*pexprTgt)[0]->Pop()->Eopid())
		{
			// and it is a plain Get, not a GbAgg
			eres = GPOS_FAILED;
		}
		else
		{
			// the deduplicated columns survive in the target output (Select over
			// the Get outputs a SUPERSET of the GbAgg's grouping-only output).
			CColRefSet *pcrsOut = pexprTgt->DeriveOutputColumns();
			CColRefArray *pdrgpcrGrp = pmodel->PdrgpcrAttrs(
				prule->PfragSrc()->PopRoot()->Pdrgpsym()->operator[](0));
			if (nullptr == pdrgpcrGrp || 0 == pdrgpcrGrp->Size())
			{
				eres = GPOS_FAILED;
			}
			else
			{
				CColRefSet *pcrsGrp = GPOS_NEW(mp) CColRefSet(mp);
				pcrsGrp->Include(pdrgpcrGrp);
				if (!pcrsOut->ContainsAll(pcrsGrp))
				{
					eres = GPOS_FAILED;
				}
				pcrsGrp->Release();
			}
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprGet->Release();
	pexprGbAgg->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLAggTest::EresUnittest_RejectsWithoutUnique
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLAggTest::EresUnittest_RejectsWithoutUnique()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_DISTINCT_ELIM_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprGbAgg = nullptr;
	// grouping column is NOT a key: the dedup is not redundant.
	BuildDedupGbAgg(fix, false /*fUniqueKey*/, &pexprGet, &pexprGbAgg);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGbAgg, pmodel))
	{
		// structural match still succeeds (empty agg list is a pure dedup)
		eres = GPOS_FAILED;
	}
	else if (checker.FCheck(prule, pmodel))
	{
		// but Unique(t0,a0) must gate the fire (c0 is not a key)
		eres = GPOS_FAILED;
	}

	pmodel->Release();
	pexprGet->Release();
	pexprGbAgg->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLAggTest::EresUnittest_RejectsNonEmptyAggList
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLAggTest::EresUnittest_RejectsNonEmptyAggList()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_DISTINCT_ELIM_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrT0 = nullptr;
	CExpression *pexprGet =
		fix.PexprLogicalGet("t0", 2, &pdrgpcrT0, 0 /*ulKeyCol*/);

	CColRefArray *pdrgpcrGrp = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGrp->Append((*pdrgpcrT0)[0]);
	// GbAgg WITH a (dummy) aggregate function => non-empty agg list.
	CColRef *pcrAgg = fix.PcrCreateInt4("agg0");
	CExpression *pexprGbAgg = fix.PexprLogicalGbAgg(pexprGet, pdrgpcrGrp, pcrAgg);
	pdrgpcrGrp->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);

	GPOS_RESULT eres = GPOS_OK;
	if (matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGbAgg, pmodel))
	{
		// a GbAgg that computes an aggregate is not a pure dedup => must not match
		eres = GPOS_FAILED;
	}

	pmodel->Release();
	pexprGet->Release();
	pexprGbAgg->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_InstantiateDistinctAggregateToPlain()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_DISTINCT_TO_PROJ_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprGbAgg = nullptr;
	BuildDistinctGbAgg(fix, true /*fUniqueKey*/, &pexprGet, &pexprGbAgg);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTgt = nullptr;
	GPOS_RESULT eres = GPOS_OK;

	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGbAgg, pmodel) ||
		pmodel->PexprDistinctAgg() != pexprGbAgg ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTgt = inst.PexprInstantiate(prule, pmodel);
		CExpression *pexprSourceFunc =
			(*(*(*pexprGbAgg)[1])[0])[0];
		CExpression *pexprTargetFunc = nullptr;
		if (nullptr != pexprTgt &&
			COperator::EopLogicalGbAgg == pexprTgt->Pop()->Eopid())
		{
			pexprTargetFunc = (*(*(*pexprTgt)[1])[0])[0];
		}
		if (nullptr == pexprTgt ||
			COperator::EopLogicalGbAgg != pexprTgt->Pop()->Eopid() ||
			(*pexprTgt)[0] != pexprGet ||
			nullptr == pexprTargetFunc ||
			!CScalarAggFunc::PopConvert(pexprSourceFunc->Pop())->IsDistinct() ||
			CScalarAggFunc::PopConvert(pexprTargetFunc->Pop())->IsDistinct() ||
			0 != (*pexprTargetFunc)[EaggfuncIndexDistinct]->Arity())
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprGet->Release();
	pexprGbAgg->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_DistinctAggregateRejectsWithoutUnique()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_DISTINCT_TO_PROJ_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprGbAgg = nullptr;
	BuildDistinctGbAgg(fix, false /*fUniqueKey*/, &pexprGet, &pexprGbAgg);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGbAgg, pmodel) ||
		checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}

	pmodel->Release();
	pexprGet->Release();
	pexprGbAgg->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLAggTest::EresUnittest_NoFireOnWrongRoot
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLAggTest::EresUnittest_NoFireOnWrongRoot()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_DISTINCT_ELIM_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	// a bare Get (not a GbAgg).
	CColRefArray *pdrgpcrT0 = nullptr;
	CExpression *pexprGet =
		fix.PexprLogicalGet("t0", 2, &pdrgpcrT0, 0 /*ulKeyCol*/);

	// a plain Project (CLogicalProject, not a dedup GbAgg).
	CColRefArray *pdrgpcrProj = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrProj->Append((*pdrgpcrT0)[0]);
	CExpression *pexprProject = fix.PexprLogicalProject(pexprGet, pdrgpcrProj);
	pdrgpcrProj->Release();

	CDSLMatcher matcher(mp);

	GPOS_RESULT eres = GPOS_OK;

	CDSLModel *pmodel1 = GPOS_NEW(mp) CDSLModel(mp);
	if (matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGet, pmodel1))
	{
		// Proj* source root must not match a bare Get
		eres = GPOS_FAILED;
	}
	pmodel1->Release();

	CDSLModel *pmodel2 = GPOS_NEW(mp) CDSLModel(mp);
	if (matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprProject, pmodel2))
	{
		// nor a plain CLogicalProject (that is the non-distinct Proj shell's job)
		eres = GPOS_FAILED;
	}
	pmodel2->Release();

	pexprProject->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

// EOF
