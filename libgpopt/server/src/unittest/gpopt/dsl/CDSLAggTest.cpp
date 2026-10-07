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
#include "gpos/common/CAutoRef.h"
#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLExprListUtils.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/dsl/CDSLRulePrefixIndex.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CLogicalGbAggDeduplicate.h"
#include "gpopt/operators/CLogicalLeftAntiSemiApply.h"
#include "gpopt/operators/CLogicalLeftAntiSemiJoin.h"
#include "gpopt/operators/CLogicalLeftSemiApply.h"
#include "gpopt/operators/CScalarAggFunc.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarSortGroupClause.h"
#include "gpopt/operators/CScalarSubquery.h"
#include "gpopt/operators/CScalarValuesList.h"
#include "gpopt/xforms/CXformContext.h"
#include "gpopt/xforms/CXformResult.h"
#include "gpopt/xforms/CXformSplitGbAgg.h"
#include "gpopt/xforms/CXformSplitGbAggDedup.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

#include <string>

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
	"t1 := t0;a2 := a0;a3 := a1;"                                        \
	"f1 := f0;s1 := s0;p1 := p0;"                                        \
	"MinimalGrouping(a0,s0)"

#define GPOPT_DSL_AGG_KEYED_OUTPUT_RULE                                    \
	"Agg<a0 a1 f0 s0 p0>(Input<t0>)|"                                      \
	"Agg<a2 a3 f1 s1 p1>(Input<t1>)|"                                      \
	"OutputAttrs(a0,t0);Unique(t0,a0);OutputAttrs(a2,t0);Unique(t0,a2);AttrsSub(a1,t0);"                \
	"TableEq(t1,t0);AttrsEq(a3,a1);FuncEq(f1,f0);"                          \
	"SchemaEq(s1,s0);PredicateEq(p1,p0)"

#define GPOPT_DSL_KEYED_OUTPUT_DEDUP_RULE                                  \
	"Input<t0>|Proj*<a0 s0>(Input<t1>)|"                                   \
	"OutputAttrs(a0,t0);Unique(t0,a0);SchemaFromAttrs(s0,a0);TableEq(t1,t0)"

#define GPOPT_DSL_AGG_FILTER_COMMUTE_RULE                                  \
	"Agg<a0 a1 f0 s0 p0>(Filter<p1 a2 a3>(Input<t0>))|"                   \
	"Filter<p2 a6 a7>(Agg<a4 a5 f1 s1 p3>(Input<t1>))|"                  \
	"TableEq(t1,t0);AttrsEq(a4,a0);AttrsEq(a5,a1);FuncEq(f1,f0);"         \
	"SchemaEq(s1,s0);PredicateEq(p3,p0);PredicateEq(p2,p1);"              \
	"AttrsEq(a6,a2);AttrsEq(a7,a3);AttrsSub(a2,a0);"                      \
	"AttrsNonEmpty(a2)"

#define GPOPT_DSL_AGG_EXTERNAL_SEMI_APPLY_RULE                            \
	"SemiApply<p0 a0 a1 a2>(Input<t0>,Agg<a3 a4 f0 s0 p1>(Filter<p2 a5 " \
	"a6>(Input<t1>)))|Filter<p4 a11 a12>(Proj*<a14 s2>(InnerJoin<p3 a9 " \
	"a10>(Input<t2>,Agg<a7 a8 f1 s1 p5>(Input<t3>))))|"                   \
	"TableEq(t2,t0);TableEq(t3,t1);AttrsEq(a7,a3);AttrsEq(a8,a4);"       \
	"FuncEq(f1,f0);SchemaEq(s1,s0);PredicateEq(p5,p1);"                  \
	"PredicateAnd(p6,p0,p2);"                                            \
	"PredicateDomainSplit(p6,p3,p4,a9,a10,a11,a12,t0,t1);"              \
	"CorrelationEquality(p4,a11,a12);AttrsSub(a5,a3);"                   \
	"AttrsNonEmpty(a5);OutputAttrs(a13,t0);Unique(t0,a13);"                \
	"AttrsUnion(a14,a13,a11);SchemaFromAttrs(s2,a14)"

#define GPOPT_DSL_AGG_CORRELATION_COMPOSITION_RULE                         \
	"SemiApply<p0 a0 a1 a2>(Input<t0>,Agg<a3 a4 f0 s0 p1>(Filter<p2 a5 " \
	"a6>(Input<t1>)))|SemiJoin<p3 a7 a8>(Input<t2>,Agg<a9 a10 f1 s1 "      \
	"p4>(Input<t3>))|TableEq(t2,t0);TableEq(t3,t1);"                        \
	"PredicateAnd(p3,p0,p2);AttrsEq(a2,a6);AttrsUnion(a7,a0,a6);"           \
	"AttrsUnion(a8,a1,a5);AttrsUnion(a9,a3,a5);AttrsEq(a10,a4);"            \
	"FuncEq(f1,f0);SchemaUnion(s1,s0,a5);PredicateEq(p4,p1);"               \
	"CorrelationEquality(p2,a5,a6);AttrsNonEmpty(a5);"                     \
	"AttrsSub(a0,t0);AttrsSub(a1,s0);AttrsSub(a3,t1);AttrsSub(a4,t1);"       \
	"AttrsSub(a5,t1);AttrsSub(a6,t0)"

#define GPOPT_DSL_AGG_ANTI_CORRELATION_COMPOSITION_RULE                    \
	"AntiApply<p0 a0 a1 a2>(Input<t0>,Agg<a3 a4 f0 s0 p1>(Filter<p2 a5 " \
	"a6>(Input<t1>)))|AntiJoin<p3 a7 a8>(Input<t2>,Agg<a9 a10 f1 s1 "      \
	"p4>(Input<t3>))|TableEq(t2,t0);TableEq(t3,t1);"                        \
	"PredicateAnd(p3,p0,p2);AttrsEq(a2,a6);AttrsUnion(a7,a0,a6);"           \
	"AttrsUnion(a8,a1,a5);AttrsUnion(a9,a3,a5);AttrsEq(a10,a4);"            \
	"FuncEq(f1,f0);SchemaUnion(s1,s0,a5);PredicateEq(p4,p1);"               \
	"CorrelationEquality(p2,a5,a6);AttrsNonEmpty(a5);"                    \
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
	CDSLRule *prule =
		CDSLRuleParser::PdslruleParse(mp, sz_dsl, "EQ" /*verdict*/, &strErr);
	if (nullptr == prule)
	{
		GPOS_TRACE(strErr.GetBuffer());
	}
	return prule;
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

static GPOS_RESULT
EresAggregateExpressionBindings()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	for (ULONG mode = 0; mode < 3; ++mode)
	{
		const BOOL explicit_outputs = 1 == mode;
		const std::string text = std::string("Agg<a0 a1 ") +
			(explicit_outputs ? "a2 " : "") + "f0 s0 " +
			(2 == mode ? "Not(p0)" : "p0") + ">(Input<t0>)|Agg<a4 a5 " +
			(explicit_outputs ? "a6 " : "") + "f1 s1 Not(Not(p0))>(Input<t1>)|" +
			"t1 := t0;a4 := a0;a5 := a1;" + (explicit_outputs ? "a6 := a2;" : "") +
			"f1 := f0;s1 := s0";
		CAutoRef<CDSLRule> rule(PdslruleParseLocal(mp, text.c_str()));
		GPOS_UNITTEST_ASSERT(nullptr != rule.Value());
		CExpression *get = nullptr, *agg = nullptr;
		CColRefArray *input = nullptr;
		CColRef *output = nullptr;
		BuildRealGbAgg(fix, &get, &agg, &input, &output);
		CAutoRef<CExpression> get_owner(get), agg_owner(agg);
		CAutoRef<CExpression> predicate(fix.PexprPredAtom(output));
		CAutoRef<CExpression> source(fix.PexprLogicalSelect(agg, predicate.Value()));
		CAutoRef<CDSLModel> model(GPOS_NEW(mp) CDSLModel(mp));
		CDSLMatcher matcher(mp, rule.Value());
		const BOOL matched = matcher.FMatch(rule->PfragSrc()->PopRoot(), source.Value(), model.Value());
		if (2 == mode)
		{
			GPOS_UNITTEST_ASSERT(!matched);
			continue;
		}
		GPOS_UNITTEST_ASSERT(matched);
		CDSLConstraintChecker checker(mp);
		GPOS_UNITTEST_ASSERT(checker.FCheck(rule.Value(), model.Value()));
		CDSLInstantiator instantiator(mp);
		CAutoRef<CExpression> target(instantiator.PexprInstantiate(rule.Value(), model.Value()));
		GPOS_UNITTEST_ASSERT(nullptr != target.Value());
		GPOS_UNITTEST_ASSERT(COperator::EopLogicalSelect == target->Pop()->Eopid());
		CExpression *having = (*target)[1];
		GPOS_UNITTEST_ASSERT(COperator::EopScalarBoolOp == having->Pop()->Eopid() &&
			1 == having->Arity() && 1 == (*having)[0]->Arity() &&
			(*(*having)[0])[0]->Matches(predicate.Value()));
		GPOS_UNITTEST_ASSERT(COperator::EopLogicalGbAgg == (*target)[0]->Pop()->Eopid() &&
			(*(*target)[0])[0] == get && (*(*target)[0])[1]->Matches((*agg)[1]) &&
			target->DeriveOutputColumns()->Equals(source->DeriveOutputColumns()));
	}
	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLAggTest::EresUnittest
//---------------------------------------------------------------------------
static GPOS_RESULT
EresAggregateIdentityMetadata()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	for (const CHAR *text : {GPOPT_DSL_AGG_IDENTITY_RULE,
		"Agg<a0 a1 f0 s0 p0>(Input<t0>)|Agg<a2 a3 f1 s1 p1>(Input<t1>)|"
		"t1 := t0;a2 := a0;a3 := a1;f1 := f0;s1 := s0;p1 := p0"})
	for (ULONG kind = 0; kind < 3; ++kind)
	{
		CAutoRef<CDSLRule> rule(PdslruleParseLocal(mp, text));
		GPOS_UNITTEST_ASSERT(nullptr != rule.Value());
		CAutoRef<CExpression> original(CUtils::PexprCountStar(
			mp, fix.PexprLogicalGet("identity_count", 1)));
		CAutoRef<CXformContext> context(GPOS_NEW(mp) CXformContext(mp));
		CAutoRef<CXformResult> splitResult(GPOS_NEW(mp) CXformResult(mp));
		CExpression *source = original.Value();
		if (kind == 1)
		{
			// An ordinary global COUNT can have known (empty) minimal keys.
			CColRefArray *grouping = CLogicalGbAgg::PopConvert(source->Pop())->Pdrgpcr();
			grouping->AddRef();
			(*source)[0]->AddRef();
			(*source)[1]->AddRef();
			source = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CLogicalGbAgg(mp, grouping, GPOS_NEW(mp) CColRefArray(mp),
					COperator::EgbaggtypeGlobal, false, nullptr), (*source)[0], (*source)[1]);
		}
		else if (kind == 2)
		{
			CAutoRef<CXformSplitGbAgg> split(GPOS_NEW(mp) CXformSplitGbAgg(mp));
			split->Transform(context.Value(), splitResult.Value(), source);
			GPOS_UNITTEST_ASSERT(splitResult->Size() == 1);
			source = (*splitResult->Pdrgpexpr())[0];
			source->AddRef();
		}
		else
		{
			source->AddRef();
		}
		CAutoRef<CExpression> ownedSource(source);
		CAutoRef<CDSLModel> model(GPOS_NEW(mp) CDSLModel(mp));
		GPOS_UNITTEST_ASSERT(CDSLMatcher(mp, rule.Value()).FMatch(
			rule->PfragSrc()->PopRoot(), source, model.Value()));
		GPOS_UNITTEST_ASSERT(CDSLConstraintChecker(mp).FCheck(rule.Value(), model.Value()));
		CAutoRef<CExpression> target(CDSLInstantiator(mp).PexprInstantiate(rule.Value(), model.Value()));
		GPOS_UNITTEST_ASSERT(nullptr != target.Value());
		CLogicalGbAgg *before = CLogicalGbAgg::PopConvert(source->Pop());
		CLogicalGbAgg *after = CLogicalGbAgg::PopConvert(target->Pop());
		GPOS_UNITTEST_ASSERT(before->FGeneratesDuplicates() == after->FGeneratesDuplicates());
		GPOS_UNITTEST_ASSERT(before->AggStage() == after->AggStage());
		GPOS_UNITTEST_ASSERT(source->Matches(target.Value()));
		GPOS_UNITTEST_ASSERT(source->Pop()->HashValue() == target->Pop()->HashValue());
	}
	return GPOS_OK;
}

static GPOS_RESULT
EresAggregateContexts()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	for (BOOL outputs : {false, true})
	for (BOOL safety : {false, true})
	for (ULONG mode = 0; mode < 5; ++mode)
	{
		// These are constructor checks, not asserted equivalent optimization
		// rules: modes 1/2 deliberately change one aggregate's input column.
		const std::string text = std::string("Agg<a0 a1 ") + (outputs ? "a2 " : "") +
			"f0 s0 p0>(Input<t0>)|Agg<a3 a4 " + (outputs ? "a5 " : "") +
			"f1 s1 p1>(Input<t1>)|t1 := t0;a3 := a0;" + (outputs ? "a5 := a2;" : "") +
			"s1 := s0;p1 := p0;Context(n0) := f0;" + (mode == 4 ? "" : "Column(a6) := n0;") +
			(mode == 3 ? "ScalarOne(n1);" : mode == 0 ? "n1 := Column(a6);" : "n1 := Column(a0);") +
			"f2 := f0;f3 := Context(f2,n1);f1 := f3;" +
			(safety ? "ErrorFree(f1);Deterministic(f1);" : "") +
			(mode == 2 ? "a4 := a1" : "FuncAttrs(a4,f1)");
		CAutoRef<CDSLRule> rule(PdslruleParseLocal(mp, text.c_str()));
		GPOS_UNITTEST_ASSERT(nullptr != rule.Value());
		CExpression *get = nullptr, *agg = nullptr;
		CColRefArray *input = nullptr;
		CColRef *output = nullptr;
		BuildRealGbAgg(fix, &get, &agg, &input, &output);
		CAutoRef<CExpression> getOwner(get), aggOwner(agg);
		CExpression *original = (*(*(*agg)[1])[0])[0];
		for (BOOL nested : {false, true})
		{
			CAutoRef<CExpression> srf(fix.PexprGenerateSeries((*input)[1]));
			CExpression *invalid = CDSLExprListUtils::PexprReplaceAt(mp, original, {0, 0}, nested ? original : srf.Value());
			GPOS_UNITTEST_ASSERT(nullptr != invalid);
			CAutoRef<CExpression> invalidList(GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
				CUtils::PexprScalarProjectElement(mp, output, invalid)));
			CAutoRef<CExpressionArray> invalidFunctions(CDSLExprListUtils::PdrgpexprFunctions(mp, invalidList.Value()));
			GPOS_UNITTEST_ASSERT(nullptr == invalidFunctions.Value());
		}
		CAutoRef<CExpression> constant(CUtils::PexprScalarConstInt4(mp, 7));
		CExpression *prefix = CDSLExprListUtils::PexprReplaceAt(mp, original, {0, 0}, constant.Value());
		GPOS_UNITTEST_ASSERT(nullptr != prefix);
		CExpressionArray *items = GPOS_NEW(mp) CExpressionArray(mp);
		items->Append(CUtils::PexprScalarProjectElement(mp, fix.PcrCreateInt4("prefix"), prefix));
		// A shared expression pointer appears twice. Context must replace only
		// its first occurrence, preserving the rest of the arbitrary-length list.
		original->AddRef();
		items->Append(CUtils::PexprScalarProjectElement(mp, output, original));
		original->AddRef();
		items->Append(CUtils::PexprScalarProjectElement(mp, fix.PcrCreateInt4("suffix"), original));
		agg->Pop()->AddRef();
		get->AddRef();
		CExpression *group = GPOS_NEW(mp) CExpression(mp, agg->Pop(), get,
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp), items));
		CAutoRef<CExpression> groupOwner(group);
		CAutoRef<CExpression> having(fix.PexprEqPred(output, output));
		CAutoRef<CExpression> source(fix.PexprLogicalSelect(group, having.Value()));
		CAutoRef<CDSLModel> model(GPOS_NEW(mp) CDSLModel(mp));
		GPOS_UNITTEST_ASSERT(CDSLMatcher(mp, rule.Value()).FMatch(rule->PfragSrc()->PopRoot(), source.Value(), model.Value()));
		const BOOL checked = CDSLConstraintChecker(mp).FCheck(rule.Value(), model.Value());
		CAutoRef<CExpression> target(checked ? CDSLInstantiator(mp).PexprInstantiate(rule.Value(), model.Value()) : nullptr);
		GPOS_UNITTEST_ASSERT((nullptr != target.Value()) == (mode < 2));
		if (mode >= 2) continue;
		GPOS_UNITTEST_ASSERT(COperator::EopLogicalSelect == target->Pop()->Eopid());
		GPOS_UNITTEST_ASSERT((*target)[1]->Matches(having.Value()));
		CExpression *rebuilt = (*target)[0];
		GPOS_UNITTEST_ASSERT(rebuilt->Pop()->Matches(group->Pop()) && (*rebuilt)[0] == get);
		GPOS_UNITTEST_ASSERT(target->DeriveOutputColumns()->Equals(source->DeriveOutputColumns()));
		CExpression *list = (*rebuilt)[1];
		GPOS_UNITTEST_ASSERT(3 == list->Arity() && (*list)[0]->Matches((*(*group)[1])[0]) &&
			(*list)[2]->Matches((*(*group)[1])[2]));
		CExpression *changed = (*(*list)[1])[0];
		GPOS_UNITTEST_ASSERT(changed->Pop()->Matches(original->Pop()));
		GPOS_UNITTEST_ASSERT(CUtils::FScalarIdent((*(*changed)[0])[0], (*input)[mode == 0 ? 1 : 0]));
	}
	return GPOS_OK;
}

static GPOS_RESULT
EresNamedAggregateStage()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CAutoRef<CExpression> original(CUtils::PexprCountStar(
		mp, fix.PexprLogicalGet("named_count", 1)));
	CAutoRef<CXformContext> context(GPOS_NEW(mp) CXformContext(mp));
	CAutoRef<CXformResult> result(GPOS_NEW(mp) CXformResult(mp));
	CAutoRef<CXformSplitGbAgg> split(GPOS_NEW(mp) CXformSplitGbAgg(mp));
	split->Transform(context.Value(), result.Value(), original.Value());
	GPOS_UNITTEST_ASSERT(result->Size() == 1);
	CExpression *finalizer = (*result->Pdrgpexpr())[0];
	// The finalizer is still named count, but combines transition states;
	// counting its input rows would have different semantics.
	GPOS_UNITTEST_ASSERT(CScalarAggFunc::PopConvert(
		(*(*(*finalizer)[1])[0])[0]->Pop())->FSplit());
	for (BOOL explicit_outputs : {false, true})
	for (BOOL named_source : {false, true})
	for (CExpression *source : {original.Value(), finalizer})
	{
		const std::string text = std::string(named_source ? "Agg_count" : "Agg") +
			"<a0 a1 " + (explicit_outputs ? "a2 " : "") +
			"f0 s0 p0>(Input<t0>)|Agg_count<a3 a4 " +
			(explicit_outputs ? "a5 " : "") + "f1 s1 p1>(Input<t1>)|"
			"t1 := t0;a3 := a0;a4 := a1;" +
			(explicit_outputs ? "a5 := a2;" : "") + "f1 := f0;s1 := s0;p1 := p0";
		CAutoRef<CDSLRule> rule(PdslruleParseLocal(mp, text.c_str()));
		GPOS_UNITTEST_ASSERT(nullptr != rule.Value());
		CAutoRef<CDSLModel> model(GPOS_NEW(mp) CDSLModel(mp));
		const BOOL matched = CDSLMatcher(mp, rule.Value()).FMatch(
			rule->PfragSrc()->PopRoot(), source, model.Value());
		GPOS_UNITTEST_ASSERT(matched == (!named_source || source == original.Value()));
		if (!matched)
			continue;
		GPOS_UNITTEST_ASSERT(CDSLConstraintChecker(mp).FCheck(rule.Value(), model.Value()));
		CAutoRef<CExpression> target(CDSLInstantiator(mp).PexprInstantiate(rule.Value(), model.Value()));
		GPOS_UNITTEST_ASSERT((nullptr != target.Value()) == (source == original.Value()));
		if (nullptr != target.Value())
			GPOS_UNITTEST_ASSERT(source->Matches(target.Value()));
	}
	return GPOS_OK;
}

GPOS_RESULT
CDSLAggTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(EresAggregateContexts),
		GPOS_UNITTEST_FUNC(EresNamedAggregateStage),
		GPOS_UNITTEST_FUNC(EresAggregateIdentityMetadata),
		GPOS_UNITTEST_FUNC(EresAggregateExpressionBindings),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_MatchBindsDedupGbAgg),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_MatchSplitDedupInput),
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
		GPOS_UNITTEST_FUNC(
			CDSLAggTest::EresUnittest_RealAggPreservesDistinctFunction),
		GPOS_UNITTEST_FUNC(
			CDSLAggTest::EresUnittest_InstantiateOutputAttrsGrouping),
		GPOS_UNITTEST_FUNC(
			CDSLAggTest::EresUnittest_InstantiateSchemaFromAttrs),
		GPOS_UNITTEST_FUNC(
			CDSLAggTest::EresUnittest_ConstraintLocalValueChain),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_MinimalGroupingMetadata),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_CopySplitGlobalGbAgg),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_CopyDedupGbAgg),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_DedupConstructionIdempotence),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_LowerSubqueryPreservesGrouping),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_SplitAggregateCopyNotResplit),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_HavingRoundTrip),
		GPOS_UNITTEST_FUNC(
			CDSLAggTest::EresUnittest_AggFilterMovementGroupingGuard),
		GPOS_UNITTEST_FUNC(
			CDSLAggTest::EresUnittest_AggExternalSemiApplyRuleRoundTrip),
		GPOS_UNITTEST_FUNC(
			CDSLAggTest::EresUnittest_AggCorrelationComposition),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_RejectsWrongAggFunction),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_NoFireOnWrongRoot),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLAggTest::EresUnittest_AggExternalSemiApplyRuleRoundTrip()
{
	CAutoMemoryPool amp;
	CDSLRule *prule =
		PdslruleParseLocal(amp.Pmp(), GPOPT_DSL_AGG_EXTERNAL_SEMI_APPLY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}
	prule->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_DedupConstructionIdempotence()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	const CHAR *rules[] = {"Proj*<a0 s0>(Input<t0>)|Proj*<a1 s1>(Input<t1>)|"
						   "TableEq(t1,t0);AttrsEq(a1,a0);SchemaEq(s1,s0)",
						   "Proj*<a0 s0>(Input<t0>)|Proj*<a1 s1>(Input<t1>)|"
						   "t1 := t0;a1 := a0;s1 := s0"};
	for (const CHAR *text : rules)
		for (ULONG kind = 0; kind < 6; ++kind)
		{
			CDSLRule *rule = PdslruleParseLocal(mp, text);
			GPOS_UNITTEST_ASSERT(nullptr != rule);
			CColRefArray *columns = nullptr;
			CExpression *input = fix.PexprLogicalGet("dedup_identity", 2, &columns);
			CColRefArray *grouping = GPOS_NEW(mp) CColRefArray(mp);
			grouping->Append((*columns)[0]);
			(*columns)[0]->MarkAsUsed();
			CExpression *child;
			if (kind < 4)
			{
				const auto stage =
					kind < 2 ? COperator::EgbaggtypeGlobal : COperator::EgbaggtypeLocal;
				grouping->AddRef();
				CLogicalGbAgg *op;
				if (kind % 2)
				{
					grouping->AddRef();
					op = GPOS_NEW(mp) CLogicalGbAggDeduplicate(mp, grouping, stage, grouping);
				}
				else
					op = GPOS_NEW(mp) CLogicalGbAgg(mp, grouping, stage);
				input->AddRef();
				child = GPOS_NEW(mp)
					CExpression(mp, op, input,
								GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp)));
			}
			else
				child = fix.PexprLogicalGbAgg(
					input, kind == 5 ? columns : grouping,
					kind == 4 ? fix.PcrCreateInt4("computed_max") : nullptr, (*columns)[1]);
			CExpression *source = fix.PexprLogicalGbAgg(child, grouping);
			CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
			GPOS_UNITTEST_ASSERT(
				CDSLMatcher(mp, rule).FMatch(rule->PfragSrc()->PopRoot(), source, model));
			GPOS_UNITTEST_ASSERT(CDSLConstraintChecker(mp).FCheck(rule, model));
			CExpression *target = CDSLInstantiator(mp).PexprInstantiate(rule, model);
			GPOS_UNITTEST_ASSERT(nullptr != target);
			// Only a pure, global, same-key dedup can replace the outer DISTINCT.
			GPOS_UNITTEST_ASSERT((target == child) == (kind < 2));
			if (kind >= 2)
				GPOS_UNITTEST_ASSERT(COperator::EopLogicalGbAgg == target->Pop()->Eopid() &&
									 CLogicalGbAgg::PopConvert(target->Pop())->FGlobal());
			target->Release();
			model->Release();
			source->Release();
			child->Release();
			grouping->Release();
			input->Release();
			rule->Release();
		}
	return GPOS_OK;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_CopyDedupGbAgg()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRef *key = fix.PcrCreateInt4("dedup_key");
	CColRef *mapped = fix.PcrCreateInt4("mapped_key");
	const auto columns = [&]()
	{
		CColRefArray *result = GPOS_NEW(mp) CColRefArray(mp);
		result->Append(key);
		return result;
	};
	for (auto stage : {COperator::EgbaggtypeGlobal, COperator::EgbaggtypeLocal})
		for (BOOL minimal : {false, true})
		{
			CLogicalGbAggDeduplicate *original =
				minimal ? GPOS_NEW(mp)
							  CLogicalGbAggDeduplicate(mp, columns(), columns(), stage, columns())
						: GPOS_NEW(mp) CLogicalGbAggDeduplicate(mp, columns(), stage, columns());
			for (BOOL remap : {false, true})
			{
				UlongToColRefMap *mapping = GPOS_NEW(mp) UlongToColRefMap(mp);
				if (remap)
					mapping->Insert(GPOS_NEW(mp) ULONG(key->Id()), mapped);
				CLogicalGbAggDeduplicate *copy = CLogicalGbAggDeduplicate::PopConvert(
					original->PopCopyWithRemappedColumns(mp, mapping, remap));
				GPOS_UNITTEST_ASSERT(copy->FGeneratesDuplicates() ==
									 original->FGeneratesDuplicates());
				GPOS_UNITTEST_ASSERT(copy->Egbaggtype() == stage);
				GPOS_UNITTEST_ASSERT((remap ? mapped : key) == (*copy->Pdrgpcr())[0]);
				GPOS_UNITTEST_ASSERT((remap ? mapped : key) == (*copy->PdrgpcrKeys())[0]);
				GPOS_UNITTEST_ASSERT(minimal == (nullptr != copy->PdrgpcrMinimal()));
				if (minimal)
					GPOS_UNITTEST_ASSERT((remap ? mapped : key) == (*copy->PdrgpcrMinimal())[0]);
				if (!remap)
					GPOS_UNITTEST_ASSERT(original->Matches(copy) &&
										 original->HashValue() == copy->HashValue());
				copy->Release();
				mapping->Release();
			}
			original->Release();
		}
	return GPOS_OK;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_LowerSubqueryPreservesGrouping()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *columns = nullptr, *inner_columns = nullptr;
	CExpression *outer = fix.PexprLogicalGet("grouping_outer", 2, &columns, 0);
	CExpression *inner = fix.PexprLogicalGet("grouping_inner", 1, &inner_columns);
	CColRef *selected = fix.PcrCreateInt4("inner_max");
	CColRefArray *empty = GPOS_NEW(mp) CColRefArray(mp);
	CExpression *query = fix.PexprLogicalGbAgg(inner, empty, selected, (*inner_columns)[0]);
	inner->Release();
	empty->Release();
	CExpression *subquery = GPOS_NEW(mp) CExpression(mp,
		GPOS_NEW(mp) CScalarSubquery(mp, selected, false, false), query);
	// MAX((SELECT MAX(...))) exercises actual subquery lowering inside an
	// aggregate argument without changing the outer grouping contract.
	CExpression *prototype = CUtils::PexprAgg(mp, fix.Pmda(), IMDType::EaggMax,
		selected, false, false);
	CExpressionArray *arguments = GPOS_NEW(mp) CExpressionArray(mp);
	arguments->Append(GPOS_NEW(mp) CExpression(mp,
		GPOS_NEW(mp) CScalarValuesList(mp), subquery));
	for (ULONG i = 1; i < prototype->Arity(); ++i)
	{
		(*prototype)[i]->AddRef();
		arguments->Append((*prototype)[i]);
	}
	prototype->Pop()->AddRef();
	CExpression *aggregate = GPOS_NEW(mp) CExpression(mp, prototype->Pop(), arguments);
	prototype->Release();
	CExpression *projects = GPOS_NEW(mp) CExpression(mp,
		GPOS_NEW(mp) CScalarProjectList(mp), CUtils::PexprScalarProjectElement(mp,
			fix.PcrCreateInt4("outer_max"), aggregate));
	CColRefArray *minimal = GPOS_NEW(mp) CColRefArray(mp);
	minimal->Append((*columns)[0]);
	columns->AddRef();
	CLogicalGbAgg *op = GPOS_NEW(mp) CLogicalGbAgg(mp, columns, minimal,
		COperator::EgbaggtypeGlobal, false, nullptr);
	CExpression *source = GPOS_NEW(mp) CExpression(mp, op, outer, projects);
	CExpression *lowered = CDSLMatchView::PexprLowerSubqueries(mp, source);
	GPOS_UNITTEST_ASSERT(nullptr != lowered &&
		COperator::EopLogicalGbAgg == lowered->Pop()->Eopid());
	const auto *result = CLogicalGbAgg::PopConvert(lowered->Pop());
	GPOS_UNITTEST_ASSERT(nullptr != result->PdrgpcrMinimal() &&
		1 == result->PdrgpcrMinimal()->Size() &&
		(*minimal)[0] == (*result->PdrgpcrMinimal())[0]);
	GPOS_UNITTEST_ASSERT(result->Matches(op));
	lowered->Release();
	source->Release();
	return GPOS_OK;
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
	GPOS_UNITTEST_ASSERT(!popOriginal->FGeneratesDuplicates());

	UlongToColRefMap *colref_mapping = GPOS_NEW(mp) UlongToColRefMap(mp);
	COperator *popCopy = popOriginal->PopCopyWithRemappedColumns(
		mp, colref_mapping, false /*must_exist*/);
	colref_mapping->Release();

	CLogicalGbAgg *popGbAggCopy = CLogicalGbAgg::PopConvert(popCopy);
	GPOS_ASSERT(popGbAggCopy->FGlobal());
	GPOS_UNITTEST_ASSERT(!popGbAggCopy->FGeneratesDuplicates());
	GPOS_ASSERT(nullptr == popGbAggCopy->PdrgpcrArgDQA());

	popCopy->Release();
	popOriginal->Release();

	CDSLTestFixture fix(mp);
	CColRef *key = fix.PcrCreateInt4("copy_grouping");
	CColRef *replacement = fix.PcrCreateInt4("new_grouping");
	const auto columns = [&](CColRef *col) {
		CColRefArray *result = GPOS_NEW(mp) CColRefArray(mp);
		result->Append(col);
		return result;
	};
	BOOL ok = true;
	for (auto type : {COperator::EgbaggtypeLocal, COperator::EgbaggtypeGlobal,
					 COperator::EgbaggtypeIntermediate})
		for (auto stage : {CLogicalGbAgg::EasOthers,
						   CLogicalGbAgg::EasTwoStageScalarDQA,
						   CLogicalGbAgg::EasThreeStageScalarDQA})
		{
			const BOOL hasDqa = type == COperator::EgbaggtypeIntermediate ||
				stage != CLogicalGbAgg::EasOthers;
			CLogicalGbAgg *original = GPOS_NEW(mp) CLogicalGbAgg(mp, columns(key),
				type, type == COperator::EgbaggtypeLocal,
				hasDqa ? columns(key) : nullptr, stage);
			UlongToColRefMap *mapping = GPOS_NEW(mp) UlongToColRefMap(mp);
			CLogicalGbAgg *remapped = CLogicalGbAgg::PopConvert(
				original->PopCopyWithRemappedColumns(mp, mapping, false));
			mapping->Release();
			// A no-op column copy must not derive a previously unknown annotation.
			GPOS_UNITTEST_ASSERT(nullptr == remapped->PdrgpcrMinimal());
			GPOS_UNITTEST_ASSERT(original->Matches(remapped));
			GPOS_UNITTEST_ASSERT(original->HashValue() == remapped->HashValue());
			mapping = GPOS_NEW(mp) UlongToColRefMap(mp);
			mapping->Insert(GPOS_NEW(mp) ULONG(key->Id()), replacement);
			CLogicalGbAgg *renamed = CLogicalGbAgg::PopConvert(
				original->PopCopyWithRemappedColumns(mp, mapping, true));
			mapping->Release();
			GPOS_UNITTEST_ASSERT(nullptr == renamed->PdrgpcrMinimal());
			GPOS_UNITTEST_ASSERT((*renamed->Pdrgpcr())[0] == replacement);
			GPOS_UNITTEST_ASSERT(!hasDqa || (*renamed->PdrgpcrArgDQA())[0] == replacement);
			GPOS_UNITTEST_ASSERT(renamed->AggStage() == original->AggStage());
			GPOS_UNITTEST_ASSERT(renamed->FGeneratesDuplicates() == original->FGeneratesDuplicates());
			renamed->Release();
			CLogicalGbAgg *regrouped = original->PopCopyWithAggregateColumns(mp,
				columns(replacement), columns(replacement),
				hasDqa ? columns(key) : nullptr);
			for (CLogicalGbAgg *copy : {remapped, regrouped})
			{
				ok &= copy->Egbaggtype() == type && copy->AggStage() == stage &&
					copy->FGeneratesDuplicates() == original->FGeneratesDuplicates() &&
					(hasDqa == (nullptr != copy->PdrgpcrArgDQA())) &&
					(!hasDqa || (*copy->PdrgpcrArgDQA())[0] == key) &&
					(*copy->Pdrgpcr())[0] == (copy == remapped ? key : replacement) &&
					(copy == remapped ? nullptr == copy->PdrgpcrMinimal() :
						(*copy->PdrgpcrMinimal())[0] == (*copy->Pdrgpcr())[0]);
				copy->Release();
			}
			original->Release();
		}
	return ok ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_SplitAggregateCopyNotResplit()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	for (ULONG kind = 0; kind < 3; ++kind)
	{
		const BOOL dedup = kind != 0;
		CAutoRef<CExpression> input(fix.PexprLogicalGet("split_input", 1));
		input->AddRef();
		CAutoRef<CExpression> aggregate(
			dedup
				? GPOS_NEW(mp) CExpression(
					  mp,
					  kind == 2
						  ? GPOS_NEW(mp) CLogicalGbAggDeduplicate(
								mp, input->DeriveOutputColumns()->Pdrgpcr(mp),
								COperator::EgbaggtypeGlobal,
								input->DeriveOutputColumns()->Pdrgpcr(mp))
						  : GPOS_NEW(mp) CLogicalGbAgg(
								mp, input->DeriveOutputColumns()->Pdrgpcr(mp),
								COperator::EgbaggtypeGlobal),
					  input.Value(),
					  GPOS_NEW(mp)
						  CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp)))
				: CUtils::PexprCountStar(mp, input.Value()));
		CAutoRef<CXformSplitGbAgg> split(
			kind == 2 ? GPOS_NEW(mp) CXformSplitGbAggDedup(mp)
					  : GPOS_NEW(mp) CXformSplitGbAgg(mp));
		CAutoRef<CXformContext> context(GPOS_NEW(mp) CXformContext(mp));
		CAutoRef<CXformResult> result(GPOS_NEW(mp) CXformResult(mp));
		split->Transform(context.Value(), result.Value(), aggregate.Value());
		GPOS_UNITTEST_ASSERT(result->Size() == 1);
		CExpression *global = (*result->Pdrgpexpr())[0];
		// Only the partial Local stage may emit multiple rows per group.
		GPOS_UNITTEST_ASSERT(!CLogicalGbAgg::PopConvert(global->Pop())
			->FGeneratesDuplicates());
		GPOS_UNITTEST_ASSERT(CLogicalGbAgg::PopConvert((*global)[0]->Pop())
			->FGeneratesDuplicates());
		if (!dedup)
			GPOS_UNITTEST_ASSERT(
				CScalarAggFunc::PopConvert((*(*(*global)[1])[0])[0]->Pop())
					->FSplit());
		// Copying drops Memo lineage. Scalar stages and pure-dedup local keys
		// must still prevent re-splitting the same finalizer.
		for (BOOL rename : {false, true})
		{
			CAutoRef<UlongToColRefMap> mapping(GPOS_NEW(mp)
												   UlongToColRefMap(mp));
			if (rename)
			{
				CAutoRef<CColRefArray> columns(
					global->DeriveOutputColumns()->Pdrgpcr(mp));
				CAutoRef<CColRefArray> renamed(CUtils::PdrgpcrCopy(
					mp, columns.Value(), false /*all computed*/,
					mapping.Value()));
			}
			CAutoRef<CExpression> copied(global->PexprCopyWithRemappedColumns(
				mp, mapping.Value(), false /*must_exist*/));
			CAutoRef<CXformResult> again(GPOS_NEW(mp) CXformResult(mp));
			split->Transform(context.Value(), again.Value(), copied.Value());
			GPOS_UNITTEST_ASSERT(again->Size() == 0);
		}
	}
	// A local dedup with more keys does not already deduplicate the outer keys.
	// Keep this genuine splitting candidate (and the corresponding global
	// child).
	for (auto child_type :
		 {COperator::EgbaggtypeLocal, COperator::EgbaggtypeGlobal})
	{
		CColRefArray *columns = nullptr;
		CExpression *input = fix.PexprLogicalGet("split_wider", 2, &columns);
		columns->AddRef();
		CColRefArray *outer_keys = GPOS_NEW(mp) CColRefArray(mp);
		outer_keys->Append((*columns)[0]);
		CExpression *child = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CLogicalGbAgg(mp, columns, child_type), input,
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp)));
		CAutoRef<CExpression> outer(GPOS_NEW(mp) CExpression(
			mp,
			GPOS_NEW(mp)
				CLogicalGbAgg(mp, outer_keys, COperator::EgbaggtypeGlobal),
			child,
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp))));
		CAutoRef<CXformSplitGbAgg> split(GPOS_NEW(mp) CXformSplitGbAgg(mp));
		CAutoRef<CXformContext> context(GPOS_NEW(mp) CXformContext(mp));
		CAutoRef<CXformResult> result(GPOS_NEW(mp) CXformResult(mp));
		split->Transform(context.Value(), result.Value(), outer.Value());
		GPOS_UNITTEST_ASSERT(result->Size() == 1);
	}
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
CDSLAggTest::EresUnittest_ConstraintLocalValueChain()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp,
		"Proj<a9 s9>(Input<t0>)|Proj*<a0 s0>(Input<t1>)|TableEq(t1,t0);"
			"OutputAttrs(a9,t0);Unique(t0,a9);AttrsIntersect(a7,a9,t0);"
			"AttrsEq(a0,a9);AttrsUnion(a8,a7,a0);"
			"SchemaFromAttrs(s0,a8)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrOutput = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet(
		"constraint_local", 3, &pdrgpcrOutput, 0 /*key*/);
	CExpression *pexprProject =
		fix.PexprLogicalProject(pexprGet, pdrgpcrOutput);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprProject, pmodel) ||
		!checker.FCheck(prule, pmodel) || !checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		CLogicalGbAgg *popTarget =
			nullptr != pexprTarget &&
				COperator::EopLogicalGbAgg == pexprTarget->Pop()->Eopid()
			? CLogicalGbAgg::PopConvert(pexprTarget->Pop())
			: nullptr;
		if (nullptr == popTarget || 3 != popTarget->Pdrgpcr()->Size() ||
			!pexprTarget->DeriveOutputColumns()->Equals(
				pexprGet->DeriveOutputColumns()))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprProject->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_AggFilterMovementGroupingGuard()
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
CDSLAggTest::EresUnittest_AggCorrelationComposition()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_AGG_CORRELATION_COMPOSITION_RULE);
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
		PdslruleParseLocal(mp,
						GPOPT_DSL_AGG_ANTI_CORRELATION_COMPOSITION_RULE);
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
	// A typed DISTINCT column view must not materialize an empty Project
	// when uniqueness permits removing DISTINCT. Check both SELECT-list forms.
	const CHAR *typedRules[] = {
		"Proj*<a0 s0>(Input<t0>)|Proj<a1 s1>(Input<t1>)|"
		"t1 := t0;a1 := a0;s1 := s0;Unique(t0,a0)",
		"Proj*<a0 s0 e0>(Input<t0>)|Proj<a1 s1 e1>(Input<t1>)|"
		"t1 := t0;a1 := a0;s1 := s0;e1 := e0;Unique(t0,a0)"};
	for (const CHAR *text : typedRules)
	{
		CDSLRule *typed = PdslruleParseLocal(mp, text);
		GPOS_UNITTEST_ASSERT(nullptr != typed);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher typedMatcher(mp, typed);
		CDSLInstantiator inst(mp);
		CExpression *target = nullptr;
		if (typedMatcher.FMatch(typed->PfragSrc()->PopRoot(), pexprGbAgg, model) &&
			checker.FCheck(typed, model))
			target = inst.PexprInstantiate(typed, model);
		if (target != pexprGet)
			eres = GPOS_FAILED;
		CRefCount::SafeRelease(target);
		model->Release();
		typed->Release();
	}
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
	// Typed Group captures preserve the complete ordered output domain.
	// Probe matching only: malformed source metadata is not an EQ rule.
	for (ULONG shape = 0; shape < 6; ++shape)
	for (BOOL exact : {false, true})
	{
		CColRefArray *grouping = GPOS_NEW(mp) CColRefArray(mp);
		if (4 != shape) grouping->Append(2 == shape ? fix.PcrCreateInt4("external_group_key")
			: (*pdrgpcrInput)[5 == shape ? 1 : 0]);
		if (1 == shape) grouping->Append((*pdrgpcrInput)[0]);
		CExpression *source = fix.PexprLogicalGbAgg(pexprGet, grouping,
			3 == shape ? (*pdrgpcrInput)[0] : pcrAggOut, (*pdrgpcrInput)[1]);
		grouping->Release();
		CDSLRule *rule = PdslruleParseLocal(mp, exact
			? "Agg<a0 a1 f0 s0 p0>(Input<t0>)|Agg<a2 a3 f1 s1 p1>(Input<t1>)|"
			  "t1 := t0;a2 := a0;a3 := a1;f1 := f0;s1 := s0;p1 := p0"
			: GPOPT_DSL_AGG_IDENTITY_RULE);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		if (nullptr == rule || CDSLMatcher(mp, rule).FMatch(
			rule->PfragSrc()->PopRoot(), source, model) != (!exact || 0 == shape || 4 <= shape))
		{
			GPOS_TRACE_FORMAT("Agg source domain: shape=%lu exact=%d", shape, exact);
			eres = GPOS_FAILED;
		}
		model->Release();
		CRefCount::SafeRelease(rule);
		source->Release();
	}
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
CDSLAggTest::EresUnittest_RealAggPreservesDistinctFunction()
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
	CExpression *pexprAgg = nullptr;
	BuildDistinctGbAgg(fix, false /*fUniqueKey*/, &pexprGet, &pexprAgg);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
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
			!CScalarAggFunc::PopConvert(
				(*(*(*pexprTarget)[1])[0])[0]->Pop())->IsDistinct())
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprGet->Release();
	pexprAgg->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_InstantiateOutputAttrsGrouping()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_AGG_KEYED_OUTPUT_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrInput = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet(
		"keyed_agg", 2, &pdrgpcrInput, 0 /*key*/);
	for (ULONG ul = 0; ul < pdrgpcrInput->Size(); ul++)
	{
		(*pdrgpcrInput)[ul]->MarkAsUsed();
	}
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrInput)[0]);
	pdrgpcrGroup->Append((*pdrgpcrInput)[1]);
	CColRef *pcrAggOut = fix.PcrCreateInt4("max_keyed_c1");
	CExpression *pexprSource = fix.PexprLogicalGbAgg(
		pexprGet, pdrgpcrGroup, pcrAggOut, (*pdrgpcrInput)[1]);
	pdrgpcrGroup->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		CLogicalGbAgg *popTarget =
			nullptr != pexprTarget &&
				COperator::EopLogicalGbAgg == pexprTarget->Pop()->Eopid()
			? CLogicalGbAgg::PopConvert(pexprTarget->Pop())
			: nullptr;
		CColRefSet *pcrsExpected = GPOS_NEW(mp) CColRefSet(mp);
		pcrsExpected->Include(pdrgpcrInput);
		CColRefSet *pcrsActual = GPOS_NEW(mp) CColRefSet(mp);
		if (nullptr != popTarget)
		{
			pcrsActual->Include(popTarget->Pdrgpcr());
		}
		if (nullptr == popTarget || 2 != popTarget->Pdrgpcr()->Size() ||
			!pcrsExpected->Equals(pcrsActual) || (*pexprTarget)[0] != pexprGet)
		{
			eres = GPOS_FAILED;
		}
		pcrsActual->Release();
		pcrsExpected->Release();
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprSource->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_InstantiateSchemaFromAttrs()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_KEYED_OUTPUT_DEDUP_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrOutput = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet(
		"keyed_dedup", 3, &pdrgpcrOutput, 0 /*key*/);
	for (ULONG ul = 0; ul < pdrgpcrOutput->Size(); ul++)
	{
		(*pdrgpcrOutput)[ul]->MarkAsUsed();
	}
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGet, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		CLogicalGbAgg *popTarget =
			nullptr != pexprTarget &&
				COperator::EopLogicalGbAgg == pexprTarget->Pop()->Eopid()
			? CLogicalGbAgg::PopConvert(pexprTarget->Pop())
			: nullptr;
		if (nullptr == popTarget || 3 != popTarget->Pdrgpcr()->Size() ||
			0 != (*pexprTarget)[1]->Arity() || (*pexprTarget)[0] != pexprGet ||
			!pexprTarget->DeriveOutputColumns()->Equals(
				pexprGet->DeriveOutputColumns()))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprGet->Release();
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
	// Agg's HAVING view also exists below another literal operator. The trie
	// must admit every source accepted by the full matcher, with or without it.
	CDSLRule *nested = PdslruleParseLocal(mp,
		"InnerJoin<p2 a6 a7>(Input<t2>,Agg<a0 a1 f0 s0 p0>(Input<t0>))|"
		"InnerJoin<p3 a8 a9>(Input<t3>,Agg<a2 a3 f1 s1 p1>(Input<t1>))|"
		"t3 := t2;t1 := t0;p3 := p2;a8 := a6;a9 := a7;"
		"a2 := a0;a3 := a1;f1 := f0;s1 := s0;p1 := p0");
	GPOS_UNITTEST_ASSERT(nullptr != nested);
	CExpression *outer = fix.PexprLogicalGet("having_outer", 1);
	CExpression *on = CUtils::PexprScalarConstBool(mp, true);
	for (CExpression *right : {pexprGbAgg, pexprSelect, pexprGet})
	{
		CExpression *join = fix.PexprLogicalInnerJoin(outer, right, on);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		const BOOL matched = CDSLMatcher(mp, nested).FMatch(
			nested->PfragSrc()->PopRoot(), join, model);
		CDSLRulePrefixIndex index(mp);
		index.Insert(nested, 0, join->Pop()->Eopid());
		CDSLRuleArray *candidates = index.PdrgpruleCandidates(mp, join);
		if (matched != (right != pexprGet) ||
			(matched && (1 != candidates->Size() || (*candidates)[0] != nested)))
			eres = GPOS_FAILED;
		candidates->Release();
		model->Release();
		join->Release();
	}
	on->Release();
	outer->Release();
	nested->Release();

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
//		CDSLAggTest::EresUnittest_MatchSplitDedupInput
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLAggTest::EresUnittest_MatchSplitDedupInput()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_DISTINCT_ELIM_RULE);
	GPOS_ASSERT(nullptr != prule);
	GPOS_RESULT eres = GPOS_OK;
	CDSLRule *typed = PdslruleParseLocal(mp,
		"Proj*<a0 s0>(Input<t0>)|Proj*<a1 s1>(Input<t1>)|"
		"t1 := t0;a1 := a0;s1 := s0");
	GPOS_ASSERT(nullptr != typed);
	enum
	{
		NestedLocal,
		SpecializedLocal,
		WiderLocal,
		GlobalChild,
		IncompatibleKeys,
		AggregateFunction,
		ScalarGlobal,
		IdentitySelect,
		IdentityProject,
		FilteringSelect,
		IdentityOverGlobal,
		Cases
	};
	for (ULONG test = 0; test < Cases; test++)
	{
		CColRefArray *pdrgpcrInput = nullptr;
		CExpression *pexprGet =
			fix.PexprLogicalGet("split_dedup", 2, &pdrgpcrInput);
		CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
		pdrgpcrGroup->Append((*pdrgpcrInput)[0]);
		CColRefArray *pdrgpcrLocal = GPOS_NEW(mp) CColRefArray(mp);
		if (IncompatibleKeys != test)
		{
			pdrgpcrLocal->Append((*pdrgpcrInput)[0]);
		}
		if (WiderLocal == test || IncompatibleKeys == test)
		{
			pdrgpcrLocal->Append((*pdrgpcrInput)[1]);
		}
		CExpression *pexprFunctions =
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp));
		if (AggregateFunction == test)
		{
			CExpression *pexprReal = fix.PexprLogicalGbAgg(
				pexprGet, pdrgpcrGroup, fix.PcrCreateInt4("max_value"),
				(*pdrgpcrInput)[1]);
			pexprFunctions->Release();
			pexprFunctions = (*pexprReal)[1];
			pexprFunctions->AddRef();
			pexprReal->Release();
		}
		pexprGet->AddRef();
		CLogicalGbAgg *local = nullptr;
		if (SpecializedLocal == test)
		{
			pdrgpcrLocal->AddRef();
			local = GPOS_NEW(mp) CLogicalGbAggDeduplicate(
				mp, pdrgpcrLocal, COperator::EgbaggtypeLocal, pdrgpcrLocal);
		}
		else
		{
			local = GPOS_NEW(mp) CLogicalGbAgg(
				mp, pdrgpcrLocal,
				(GlobalChild == test || IdentityOverGlobal == test) ? COperator::EgbaggtypeGlobal
									: COperator::EgbaggtypeLocal);
		}
		CExpression *pexprLocal = GPOS_NEW(mp) CExpression(
			mp, local, pexprGet, pexprFunctions);
		if (IdentitySelect == test || FilteringSelect == test || IdentityOverGlobal == test)
			pexprLocal = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CLogicalSelect(mp), pexprLocal,
				CUtils::PexprScalarConstBool(mp, FilteringSelect != test));
		if (IdentityProject == test)
			pexprLocal = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CLogicalProject(mp), pexprLocal,
				GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp)));
		if (NestedLocal == test)
		{
			pdrgpcrGroup->AddRef();
			pexprLocal = GPOS_NEW(mp) CExpression(
				mp,
				GPOS_NEW(mp)
					CLogicalGbAgg(mp, pdrgpcrGroup, COperator::EgbaggtypeLocal),
				pexprLocal,
				GPOS_NEW(mp)
					CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp)));
		}
		CColRefArray *pdrgpcrGlobal = pdrgpcrGroup;
		if (ScalarGlobal == test)
		{
			pdrgpcrGlobal = GPOS_NEW(mp) CColRefArray(mp);
		}
		CExpression *pexprGlobal =
			fix.PexprLogicalGbAgg(pexprLocal, pdrgpcrGlobal);
		const BOOL fPeel = NestedLocal == test || WiderLocal == test ||
			SpecializedLocal == test || IdentitySelect == test || IdentityProject == test;
		CExpression *pexprExpected = fPeel ? pexprGet : pexprLocal;
		if (pexprExpected != CDSLMatchView::PexprDedupInput(pexprGlobal))
		{
			eres = GPOS_FAILED;
		}
		for (CDSLRule *rule : {prule, typed})
		{
			if (ScalarGlobal == test)
			{
				continue;
			}
			CDSLMatcher matcher(mp, rule);
			CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
			const CDSLOp *popSource = rule->PfragSrc()->PopRoot();
			if (!matcher.FMatch(popSource, pexprGlobal, pmodel) ||
				pexprExpected !=
					pmodel->PexprTable((*(*popSource)[0]->Pdrgpsym())[0]))
			{
				eres = GPOS_FAILED;
			}
			pmodel->Release();
		}
		if (ScalarGlobal == test)
		{
			pdrgpcrGlobal->Release();
		}
		pdrgpcrGroup->Release();
		pexprGlobal->Release();
		pexprLocal->Release();
		pexprGet->Release();
	}
	typed->Release();
	prule->Release();
	return eres;
}

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
