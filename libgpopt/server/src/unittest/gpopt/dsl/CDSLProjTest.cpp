//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLProjTest.cpp
//
//	@doc:
//		Implementation of the Proj three-stage tests (see header). Builds a live
//		CLogicalProject via the fixture, drives match -> instantiate, and asserts
//		binding / output-column invariant / operator-identity gating.
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLProjTest.h"

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
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CScalarBoolOp.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarSubquery.h"
#include "gpopt/operators/CScalarSubqueryExists.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

// identity Proj rule: target reuses source's table / attrs / schema bindings.
#define GPOPT_DSL_PROJ_IDENTITY_RULE                            \
	"Proj<a0 s0>(Input<t0>)|Proj<a1 s1>(Input<t1>)|"            \
	"TableEq(t1,t0);AttrsEq(a1,a0);SchemaEq(s1,s0)"

#define GPOPT_DSL_PROJ_DEDUP_CHAIN_RULE                                  \
	"Proj<a1 s1>(Proj*<a0 s0>(Input<t0>))|"                              \
	"Proj*<a2 s2>(Input<t1>)|"                                           \
	"AttrsEq(a0,a1);AttrsSub(a0,t0);AttrsSub(a1,s0);"                    \
	"TableEq(t1,t0);AttrsEq(a2,a0);SchemaEq(s2,s1)"

#define GPOPT_DSL_PROJ_REBIND_RULE                                      \
	"Proj<a2 s0>(InnerJoin<a0 a1>(Input<t0>,Input<t1>))|"               \
	"Proj<a5 s1>(InnerJoin<a3 a4>(Input<t2>,Input<t3>))|"               \
	"AttrsEq(a0,a2);AttrsSub(a0,t0);AttrsSub(a1,t1);AttrsSub(a2,t0);"   \
	"TableEq(t2,t0);TableEq(t3,t1);AttrsEq(a3,a0);AttrsEq(a4,a1);"      \
	"AttrsEq(a5,a1);SchemaEq(s1,s0)"

#define GPOPT_DSL_ROOT_DEDUP_DROP_RULE                                  \
	"Proj*<a0 s0>(Input<t0>)|Proj<a1 s1>(Input<t1>)|"                    \
	"AttrsSub(a0,t0);Unique(t0,a0);TableEq(t1,t0);AttrsEq(a1,a0);"       \
	"SchemaEq(s1,s0)"

#define GPOPT_DSL_COLLAPSE_DEDUP_RULE                                  \
	"Proj*<a1 s1>(Proj*<a0 s0>(Input<t0>))|"                            \
	"Proj*<a2 s2>(Input<t1>)|"                                         \
	"AttrsSub(a0,t0);AttrsSub(a1,s0);TableEq(t1,t0);"                   \
	"AttrsEq(a2,a1);SchemaEq(s2,s1)"

#define GPOPT_DSL_COLLAPSE_IDENTITY_PROJECT_RULE                        \
	"Proj<a0 s0>(Proj<a1 s1>(Input<t0>))|"                              \
	"Proj<a2 s2>(Input<t1>)|"                                           \
	"TableEq(t1,t0);AttrsEq(a0,a1);AttrsEq(a2,a1);"                     \
	"AttrsSub(a0,s1);AttrsSub(a1,t0);SchemaEq(s2,s0);"                  \
	"ErrorFree(a1);Deterministic(a1)"

#define GPOPT_DSL_COMPUTE_IDENTITY_RULE                                  \
	"Compute<e0 a0 s0>(Input<t0>)|Compute<e1 a1 s1>(Input<t1>)|"          \
	"TableEq(t1,t0);ExprListEq(e1,e0);AttrsEq(a1,a0);SchemaEq(s1,s0);"    \
	"ErrorFree(e0);Deterministic(e0)"

#define GPOPT_DSL_EXPRESSION_DEFINED_PROJECT_SUBQUERY_RULE                 \
	"Compute<e0 a0 s0>(Input<t0>)|"                                       \
	"Compute<e1 a1 s1>(LeftApply<p0 a2 a3 a4>(Input<t1>,Input<t2>))|"     \
	"TableEq(t1,t0);SchemaEq(s1,s0);"                                      \
	"ExprListScalarSubquery(e0,e1,p0,a2,a3,a4,a5,t2)"

#define GPOPT_DSL_EXPRESSION_DEFINED_PROJECT_SUBQUERY_CHAIN_RULE                 \
	"Compute<e0 a0 s0>(Input<t0>)|"                                               \
	"Compute<e2 a1 s1>(LeftApply<p1 a6 a7 a8>("                                  \
	"LeftApply<p0 a2 a3 a4>(Input<t1>,Input<t2>),"                               \
	"Limit<n0 n1>(Compute<e3 a9 s2>(Input<t3>))))|"                               \
	"TableEq(t1,t0);SchemaEq(s1,s0);"                                              \
	"ExprListScalarSubquery(e0,e1,p0,a2,a3,a4,a5,t2);"                            \
	"ExprListExists(e1,e2,e3,a9,s2,p1,a6,a7,a8,a10,t3);"                         \
	"ScalarOne(n0);ScalarZero(n1)"

#define GPOPT_DSL_COLLAPSE_INDEPENDENT_COMPUTE_RULE                       \
	"Compute<e0 a0 s0>(Compute<e1 a1 s1>(Input<t0>))|"                    \
	"Compute<e2 a2 s2>(Input<t1>)|TableEq(t1,t0);"                        \
	"DepsDisjoint(e0,s1);ExprConcat(e2,e0,e1)"

#define GPOPT_DSL_SPLIT_COMPUTE_RULE                                      \
	"Compute<e0 a0 s0>(Compute<e1 a1 s1>(Input<t0>))|"                    \
	"Compute<e3 a3 s3>(Compute<e2 a2 s2>(Input<t1>))|"                    \
	"TableEq(t1,t0);ExprSplit(e2,e3,e0,e1)"

#define GPOPT_DSL_COMPUTE_FILTER_COMMUTE_RULE                              \
	"Compute<e0 a0 s0>(Filter<p0 a1 a2>(Input<t0>))|"                      \
	"Filter<p1 a3 a4>(Compute<e1 a5 s1>(Input<t1>))|"                      \
	"TableEq(t1,t0);ExprListEq(e1,e0);AttrsEq(a5,a0);SchemaEq(s1,s0);"     \
	"PredicateEq(p1,p0);AttrsEq(a3,a1);AttrsEq(a4,a2);"                    \
	"DepsDisjoint(p0,s0);ErrorFree(e0);Deterministic(e0)"

#define GPOPT_DSL_TYPED_NULL_COMPUTE_RULE                                  \
	"Proj<a0 s0>(Input<t0>)|Compute<e0 a1 s1>(Input<t1>)|"                 \
	"TableEq(t1,t0);ExprNulls(e0,a0,a2);AttrsEmpty(a1);"                  \
	"SchemaFromAttrs(s1,a2)"

static CDSLRule *
PdslruleParseLocal(CMemoryPool *mp, const CHAR *sz_dsl)
{
	CWStringDynamic strErr(mp);
	return CDSLRuleParser::PdslruleParse(mp, sz_dsl, "EQ" /*verdict*/, &strErr);
}

// build Project(Get t0[ulCols], projlist over the first ulProj output columns).
static void
BuildProjectOverGet(CDSLTestFixture &fix, ULONG ulCols, ULONG ulProj,
					CExpression **ppGet, CExpression **ppProject,
					CColRefArray **ppdrgpcrOut)
{
	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", ulCols, &pdrgpcrOut);

	GPOS_ASSERT(ulProj <= ulCols);
	CColRefArray *pdrgpcrProj = GPOS_NEW(fix.Pmp()) CColRefArray(fix.Pmp());
	for (ULONG ul = 0; ul < ulProj; ul++)
	{
		pdrgpcrProj->Append((*pdrgpcrOut)[ul]);
	}
	CExpression *pexprProject = fix.PexprLogicalProject(pexprGet, pdrgpcrProj);
	pdrgpcrProj->Release();

	*ppGet = pexprGet;
	*ppProject = pexprProject;
	*ppdrgpcrOut = pdrgpcrOut;
}

static CExpression *
PexprProjectWithScalar(CMemoryPool *mp, CExpression *pexprChild,
					   CColRef *pcrOutput, CExpression *pexprScalar)
{
	CExpressionArray *pdrgpexprElems = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexprElems->Append(GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectElement(mp, pcrOutput), pexprScalar));
	CExpression *pexprList = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprElems);
	pexprChild->AddRef();
	return GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalProject(mp), pexprChild, pexprList);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLProjTest::EresUnittest
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLProjTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(
			CDSLProjTest::EresUnittest_MatchBindsProjectedColumns),
		GPOS_UNITTEST_FUNC(CDSLProjTest::EresUnittest_InstantiatePreservesOutput),
		GPOS_UNITTEST_FUNC(
			CDSLProjTest::EresUnittest_InstantiateRebindsTargetAttrs),
		GPOS_UNITTEST_FUNC(
			CDSLProjTest::EresUnittest_JoinKeySubsetFollowsAttrsEq),
		GPOS_UNITTEST_FUNC(
			CDSLProjTest::EresUnittest_PreservesHiddenLimitShell),
		GPOS_UNITTEST_FUNC(
			CDSLProjTest::EresUnittest_TrivialSelectContinuesDedupChain),
		GPOS_UNITTEST_FUNC(
			CDSLProjTest::EresUnittest_DroppedDedupFeedsParentProject),
		GPOS_UNITTEST_FUNC(
			CDSLProjTest::EresUnittest_NestedProjStarConsumesGeneratedDedup),
		GPOS_UNITTEST_FUNC(
			CDSLProjTest::EresUnittest_CollapseGbAggRuleBoundaries),
		GPOS_UNITTEST_FUNC(
			CDSLProjTest::EresUnittest_CollapseIdentityProject),
		GPOS_UNITTEST_FUNC(
			CDSLProjTest::EresUnittest_ComputeExactRoundTrip),
		GPOS_UNITTEST_FUNC(
			CDSLProjTest::EresUnittest_ExpressionDefinedScalarSubquery),
		GPOS_UNITTEST_FUNC(
			CDSLProjTest::EresUnittest_ExpressionDefinedSubqueryChain),
		GPOS_UNITTEST_FUNC(
			CDSLProjTest::EresUnittest_ComputeFilterCommutesWithCorrelatedPredicate),
		GPOS_UNITTEST_FUNC(
			CDSLProjTest::EresUnittest_CollapseIndependentCompute),
		GPOS_UNITTEST_FUNC(
			CDSLProjTest::EresUnittest_SplitPartiallyIndependentCompute),
		GPOS_UNITTEST_FUNC(
			CDSLProjTest::EresUnittest_ConstructTypedNullExpressions),
		GPOS_UNITTEST_FUNC(CDSLProjTest::EresUnittest_NoFireOnWrongRoot),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLProjTest::EresUnittest_ExpressionDefinedScalarSubquery()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp, GPOPT_DSL_EXPRESSION_DEFINED_PROJECT_SUBQUERY_RULE);
	GPOS_ASSERT(nullptr != prule);

	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("project_scalar_outer", 1, &pdrgpcrOuter);
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprInner =
		fix.PexprLogicalGet("project_scalar_inner", 1, &pdrgpcrInner);
	CExpression *pexprSubquery = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarSubquery(
			mp, (*pdrgpcrInner)[0], false, false),
		pexprInner);
	CColRef *pcrOutput = fix.PcrCreateInt4("project_scalar_value");
	CExpression *pexprSource = PexprProjectWithScalar(
		mp, pexprOuter, pcrOutput, pexprSubquery);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	const BOOL fMatched = matcher.FMatch(
		prule->PfragSrc()->PopRoot(), pexprSource, pmodel);
	const BOOL fChecked = fMatched && checker.FCheck(prule, pmodel);
	if (!fChecked)
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		CExpression *pexprApply = nullptr == pexprTarget
			? nullptr
			: (*pexprTarget)[0];
		if (nullptr == pexprApply ||
			COperator::EopLogicalProject != pexprTarget->Pop()->Eopid() ||
			COperator::EopLogicalLeftOuterApply != pexprApply->Pop()->Eopid() ||
			COperator::EopLogicalMaxOneRow != (*pexprApply)[1]->Pop()->Eopid() ||
			COperator::EopScalarSubquery !=
				CLogicalApply::PopConvert(pexprApply->Pop())->EopidOriginSubq() ||
			(*pexprTarget)[1]->DeriveHasSubquery() ||
			!pexprTarget->DeriveOutputColumns()->ContainsAll(
				pexprSource->DeriveOutputColumns()))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprSource->Release();
	pexprOuter->Release();

	CColRefArray *pdrgpcrGenerated = nullptr;
	CExpression *pexprGeneratedOuter =
		fix.PexprLogicalGet("project_generated_outer", 1);
	CExpression *pexprGeneratedInner = fix.PexprLogicalGet(
		"project_generated_inner", 1, &pdrgpcrGenerated);
	CExpression *pexprGeneratedSource = PexprProjectWithScalar(
		mp, pexprGeneratedOuter, fix.PcrCreateInt4("generated_value"),
		GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarSubquery(
				mp, (*pdrgpcrGenerated)[0], false, true),
			pexprGeneratedInner));
	CDSLModel *pmodelGenerated = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcherGenerated(mp, prule);
	GPOS_ASSERT(matcherGenerated.FMatch(
		prule->PfragSrc()->PopRoot(), pexprGeneratedSource, pmodelGenerated));
	GPOS_ASSERT(!checker.FCheck(prule, pmodelGenerated));
	pmodelGenerated->Release();
	pexprGeneratedSource->Release();
	pexprGeneratedOuter->Release();

	CColRefArray *pdrgpcrReturnedOuter = nullptr;
	CExpression *pexprReturnedOuter = fix.PexprLogicalGet(
		"project_returned_outer", 1, &pdrgpcrReturnedOuter);
	CExpression *pexprReturnedInner =
		fix.PexprLogicalGet("project_returned_inner", 1);
	CExpression *pexprReturnedSource = PexprProjectWithScalar(
		mp, pexprReturnedOuter, fix.PcrCreateInt4("returned_value"),
		GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarSubquery(
					mp, (*pdrgpcrReturnedOuter)[0], false, false),
			pexprReturnedInner));
	CDSLModel *pmodelReturned = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcherReturned(mp, prule);
	GPOS_ASSERT(matcherReturned.FMatch(
		prule->PfragSrc()->PopRoot(), pexprReturnedSource, pmodelReturned));
	GPOS_ASSERT(!checker.FCheck(prule, pmodelReturned));
	pmodelReturned->Release();
	pexprReturnedSource->Release();
	pexprReturnedOuter->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLProjTest::EresUnittest_ExpressionDefinedSubqueryChain()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp, GPOPT_DSL_EXPRESSION_DEFINED_PROJECT_SUBQUERY_CHAIN_RULE);
	GPOS_ASSERT(nullptr != prule);

	CExpression *pexprOuter = fix.PexprLogicalGet("project_chain_outer", 1);
	CColRefArray *pdrgpcrScalar = nullptr;
	CExpression *pexprScalarInner =
		fix.PexprLogicalGet("project_chain_scalar", 1, &pdrgpcrScalar);
	CExpression *pexprExistsInner =
		fix.PexprLogicalGet("project_chain_exists", 1);
	CExpressionArray *pdrgpexprElems = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexprElems->Append(GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectElement(
			mp, fix.PcrCreateInt4("project_chain_scalar_value")),
		GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarSubquery(
				mp, (*pdrgpcrScalar)[0], false, false),
			pexprScalarInner)));
	pdrgpexprElems->Append(GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectElement(
			mp, fix.PcrCreateInt4("project_chain_exists_value")),
		GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarSubqueryExists(mp), pexprExistsInner)));
	CExpression *pexprSource = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalProject(mp), pexprOuter,
		GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprElems));

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
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
		CExpression *pexprOuterApply = nullptr == pexprTarget
			? nullptr
			: (*pexprTarget)[0];
		if (nullptr == pexprOuterApply ||
			COperator::EopLogicalLeftOuterApply !=
				pexprOuterApply->Pop()->Eopid() ||
			COperator::EopLogicalLeftOuterApply !=
				(*pexprOuterApply)[0]->Pop()->Eopid() ||
			(*pexprTarget)[1]->DeriveHasSubquery())
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprSource->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLProjTest::EresUnittest_CollapseIdentityProject()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp, GPOPT_DSL_COLLAPSE_IDENTITY_PROJECT_RULE);
	GPOS_ASSERT(nullptr != prule);

	CColRefArray *pdrgpcrInput = nullptr;
	CExpression *pexprGet =
		fix.PexprLogicalGet("collapse_project", 1, &pdrgpcrInput);
	CColRef *pcrInner = fix.PcrCreateInt4("inner_alias");
	CColRef *pcrOuter = fix.PcrCreateInt4("outer_alias");
	CExpression *pexprInner = PexprProjectWithScalar(
		mp, pexprGet, pcrInner,
		GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarIdent(mp, (*pdrgpcrInput)[0])));
	CExpression *pexprOuter = PexprProjectWithScalar(
		mp, pexprInner, pcrOuter,
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarIdent(mp, pcrInner)));

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprOuter, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTarget ||
			COperator::EopLogicalProject != pexprTarget->Pop()->Eopid() ||
			COperator::EopLogicalGet != (*pexprTarget)[0]->Pop()->Eopid() ||
			1 != (*pexprTarget)[1]->Arity())
		{
			eres = GPOS_FAILED;
		}
		else
		{
			CExpression *pexprElem = (*(*pexprTarget)[1])[0];
			if (pcrOuter !=
					CScalarProjectElement::PopConvert(pexprElem->Pop())->Pcr() ||
				COperator::EopScalarIdent != (*pexprElem)[0]->Pop()->Eopid() ||
				(*pdrgpcrInput)[0] !=
					CScalarIdent::PopConvert((*pexprElem)[0]->Pop())->Pcr())
			{
				eres = GPOS_FAILED;
			}
		}
	}

	// ErrorFree admits structural boolean/null-test composition and ORCA's
	// explicitly recognized built-in equality operators.
	CDSLRule *pruleSafety = PdslruleParseLocal(
		mp, "Proj<a0 s0>(Input<t0>)|Proj<a1 s1>(Input<t1>)|"
			"TableEq(t1,t0);AttrsEq(a1,a0);SchemaEq(s1,s0);"
			"ErrorFree(a0);Deterministic(a0)");
	CColRef *pcrUnsafe = fix.PcrCreateInt4("unsafe_expr");
	CExpression *pexprUnsafe = PexprProjectWithScalar(
		mp, pexprGet, pcrUnsafe,
		fix.PexprEqPred((*pdrgpcrInput)[0], (*pdrgpcrInput)[0]));
	CDSLModel *pmodelUnsafe = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcherUnsafe(mp, pruleSafety);
	if (!matcherUnsafe.FMatch(pruleSafety->PfragSrc()->PopRoot(),
							  pexprUnsafe, pmodelUnsafe) ||
		!checker.FCheck(pruleSafety, pmodelUnsafe))
	{
		eres = GPOS_FAILED;
	}

	pmodelUnsafe->Release();
	pexprUnsafe->Release();
	pruleSafety->Release();
	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprOuter->Release();
	pexprInner->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLProjTest::EresUnittest_ComputeExactRoundTrip()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_COMPUTE_IDENTITY_RULE);
	GPOS_ASSERT(nullptr != prule);

	CColRefArray *pdrgpcrInput = nullptr;
	CExpression *pexprGet =
		fix.PexprLogicalGet("compute", 2, &pdrgpcrInput);
	CColRef *pcrDefined = fix.PcrCreateInt4("computed");
	CExpression *pexprProject = PexprProjectWithScalar(
		mp, pexprGet, pcrDefined,
		GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarIdent(mp, (*pdrgpcrInput)[0])));

	const CDSLOp *popSource = prule->PfragSrc()->PopRoot();
	CDSLSymbolArray *pdrgpsym = popSource->Pdrgpsym();
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(popSource, pexprProject, pmodel) ||
		!checker.FCheck(prule, pmodel) ||
		pmodel->PexprExpr((*pdrgpsym)[0]) != (*pexprProject)[1] ||
		1 != pmodel->PdrgpcrAttrs((*pdrgpsym)[1])->Size() ||
		(*pdrgpcrInput)[0] !=
			(*pmodel->PdrgpcrAttrs((*pdrgpsym)[1]))[0] ||
		1 != pmodel->PdrgpcrSchema((*pdrgpsym)[2])->Size() ||
		pcrDefined != (*pmodel->PdrgpcrSchema((*pdrgpsym)[2]))[0])
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTarget ||
			COperator::EopLogicalProject != pexprTarget->Pop()->Eopid() ||
			(*pexprTarget)[0] != pexprGet ||
			!(*pexprTarget)[1]->Matches((*pexprProject)[1]) ||
			!pexprTarget->DeriveOutputColumns()->Equals(
				pexprProject->DeriveOutputColumns()))
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
CDSLProjTest::EresUnittest_ComputeFilterCommutesWithCorrelatedPredicate()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp, GPOPT_DSL_COMPUTE_FILTER_COMMUTE_RULE);
	GPOS_ASSERT(nullptr != prule);

	CColRefArray *pdrgpcrOuter = nullptr;
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("commute_outer", 1, &pdrgpcrOuter);
	CExpression *pexprInner =
		fix.PexprLogicalGet("commute_inner", 1, &pdrgpcrInner);
	CExpression *pexprPred = fix.PexprPredAtom((*pdrgpcrOuter)[0]);
	CExpression *pexprSelect =
		fix.PexprLogicalSelect(pexprInner, pexprPred);
	CColRef *pcrDefined = fix.PcrCreateInt4("commuted_constant");
	CExpression *pexprProject = PexprProjectWithScalar(
		mp, pexprSelect, pcrDefined, CUtils::PexprScalarConstInt4(mp, 1));

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
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
		if (nullptr == pexprTarget ||
			COperator::EopLogicalSelect != pexprTarget->Pop()->Eopid() ||
			COperator::EopLogicalProject != (*pexprTarget)[0]->Pop()->Eopid() ||
			COperator::EopLogicalGet != (*(*pexprTarget)[0])[0]->Pop()->Eopid() ||
			!(*pexprTarget)[1]->Matches(pexprPred) ||
			!pexprTarget->DeriveOutputColumns()->Equals(
				pexprProject->DeriveOutputColumns()))
		{
			eres = GPOS_FAILED;
		}
	}

	// The same structural rule must reject a predicate that consumes a column
	// defined by the Compute layer: moving it below that definition is ill-scoped.
	CExpression *pexprDefinedPred = fix.PexprPredAtom(pcrDefined);
	CExpression *pexprInvalidSelect =
		fix.PexprLogicalSelect(pexprInner, pexprDefinedPred);
	CExpression *pexprInvalidProject = PexprProjectWithScalar(
		mp, pexprInvalidSelect, pcrDefined,
		CUtils::PexprScalarConstInt4(mp, 1));
	CDSLModel *pmodelInvalid = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcherInvalid(mp, prule);
	if (!matcherInvalid.FMatch(prule->PfragSrc()->PopRoot(),
								 pexprInvalidProject, pmodelInvalid) ||
		checker.FCheck(prule, pmodelInvalid))
	{
		eres = GPOS_FAILED;
	}
	pmodelInvalid->Release();
	pexprInvalidProject->Release();
	pexprInvalidSelect->Release();
	pexprDefinedPred->Release();

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprProject->Release();
	pexprSelect->Release();
	pexprPred->Release();
	pexprInner->Release();
	pexprOuter->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLProjTest::EresUnittest_CollapseIndependentCompute()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp, GPOPT_DSL_COLLAPSE_INDEPENDENT_COMPUTE_RULE);
	GPOS_ASSERT(nullptr != prule);

	CColRefArray *pdrgpcrInput = nullptr;
	CExpression *pexprGet =
		fix.PexprLogicalGet("collapse_compute", 2, &pdrgpcrInput);
	CColRef *pcrInner = fix.PcrCreateInt4("inner_compute");
	CColRef *pcrOuter = fix.PcrCreateInt4("outer_compute");
	CExpression *pexprInner = PexprProjectWithScalar(
		mp, pexprGet, pcrInner,
		GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarIdent(mp, (*pdrgpcrInput)[0])));
	CExpression *pexprOuter = PexprProjectWithScalar(
		mp, pexprInner, pcrOuter,
		GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarIdent(mp, (*pdrgpcrInput)[1])));

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprOuter, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTarget ||
			COperator::EopLogicalProject != pexprTarget->Pop()->Eopid() ||
			COperator::EopLogicalGet != (*pexprTarget)[0]->Pop()->Eopid() ||
			2 != (*pexprTarget)[1]->Arity() ||
			pcrOuter != CScalarProjectElement::PopConvert(
				(*(*pexprTarget)[1])[0]->Pop())->Pcr() ||
			pcrInner != CScalarProjectElement::PopConvert(
				(*(*pexprTarget)[1])[1]->Pop())->Pcr() ||
			!pexprTarget->DeriveOutputColumns()->Equals(
				pexprOuter->DeriveOutputColumns()))
		{
			eres = GPOS_FAILED;
		}
	}

	// The same generic rule must reject a parent expression that consumes the
	// inner Compute's definition; flattening would create an invalid same-list
	// dependency rather than an equivalent LET chain.
	CExpression *pexprDependent = PexprProjectWithScalar(
		mp, pexprInner, pcrOuter,
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarIdent(mp, pcrInner)));
	CDSLModel *pmodelDependent = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcherDependent(mp, prule);
	if (!matcherDependent.FMatch(prule->PfragSrc()->PopRoot(),
								pexprDependent, pmodelDependent) ||
		checker.FCheck(prule, pmodelDependent))
	{
		eres = GPOS_FAILED;
	}

	pmodelDependent->Release();
	pexprDependent->Release();
	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprOuter->Release();
	pexprInner->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLProjTest::EresUnittest_SplitPartiallyIndependentCompute()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_SPLIT_COMPUTE_RULE);
	GPOS_ASSERT(nullptr != prule);

	CColRefArray *pdrgpcrInput = nullptr;
	CExpression *pexprGet =
		fix.PexprLogicalGet("split_compute", 2, &pdrgpcrInput);
	CColRef *pcrInner = fix.PcrCreateInt4("split_inner");
	CColRef *pcrMoved = fix.PcrCreateInt4("split_moved");
	CColRef *pcrResidual = fix.PcrCreateInt4("split_residual");
	CExpression *pexprInner = PexprProjectWithScalar(
		mp, pexprGet, pcrInner,
		GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarIdent(mp, (*pdrgpcrInput)[0])));

	CExpressionArray *pdrgpexprOuterElems =
		GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexprOuterElems->Append(GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectElement(mp, pcrMoved),
		GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarIdent(mp, (*pdrgpcrInput)[1]))));
	pdrgpexprOuterElems->Append(GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectElement(mp, pcrResidual),
		GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarIdent(mp, pcrInner))));
	CExpression *pexprOuterList = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprOuterElems);
	pexprInner->AddRef();
	CExpression *pexprOuter = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalProject(mp), pexprInner, pexprOuterList);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprOuter, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTarget ||
			COperator::EopLogicalProject != pexprTarget->Pop()->Eopid() ||
			1 != (*pexprTarget)[1]->Arity() ||
			pcrResidual != CScalarProjectElement::PopConvert(
				(*(*pexprTarget)[1])[0]->Pop())->Pcr() ||
			COperator::EopLogicalProject != (*pexprTarget)[0]->Pop()->Eopid() ||
			2 != (*(*pexprTarget)[0])[1]->Arity() ||
			pcrMoved != CScalarProjectElement::PopConvert(
				(*(*(*pexprTarget)[0])[1])[0]->Pop())->Pcr() ||
			pcrInner != CScalarProjectElement::PopConvert(
				(*(*(*pexprTarget)[0])[1])[1]->Pop())->Pcr() ||
			COperator::EopLogicalGet != (*(*pexprTarget)[0])[0]->Pop()->Eopid() ||
			!pexprTarget->DeriveOutputColumns()->Equals(
				pexprOuter->DeriveOutputColumns()))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprOuter->Release();
	pexprInner->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLProjTest::EresUnittest_ConstructTypedNullExpressions()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_TYPED_NULL_COMPUTE_RULE);
	GPOS_ASSERT(nullptr != prule);

	CExpression *pexprGet = nullptr;
	CExpression *pexprProject = nullptr;
	CColRefArray *pdrgpcrInput = nullptr;
	BuildProjectOverGet(
		fix, 2, 1, &pexprGet, &pexprProject, &pdrgpcrInput);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprProject, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		CExpression *pexprList = nullptr == pexprTarget ? nullptr : (*pexprTarget)[1];
		CExpression *pexprElem = nullptr == pexprList || 1 != pexprList->Arity()
			? nullptr
			: (*pexprList)[0];
		if (nullptr == pexprElem ||
			COperator::EopLogicalProject != pexprTarget->Pop()->Eopid() ||
			COperator::EopScalarProjectElement != pexprElem->Pop()->Eopid() ||
			COperator::EopScalarConst != (*pexprElem)[0]->Pop()->Eopid() ||
			!CScalarConst::PopConvert((*pexprElem)[0]->Pop())->GetDatum()->IsNull() ||
			!(*pdrgpcrInput)[0]->RetrieveType()->MDId()->Equals(
				CScalarProjectElement::PopConvert(pexprElem->Pop())
					->Pcr()->RetrieveType()->MDId()))
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

//---------------------------------------------------------------------------
//	@function:
//		CDSLProjTest::EresUnittest_CollapseGbAggRuleBoundaries
//
//	@doc:
//		The proved nested-Proj* rule consumes both equal and strict grouping
//		subsets, but never treats aggregate-bearing or Local GbAgg nodes as pure
//		deduplication. These are the semantic guards of CXformCollapseGbAgg.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLProjTest::EresUnittest_CollapseGbAggRuleBoundaries()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_COLLAPSE_DEDUP_RULE);
	GPOS_ASSERT(nullptr != prule);

	CColRefArray *pdrgpcrInput = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet(
		"collapse_dedup", 3, &pdrgpcrInput, gpos::ulong_max);
	CColRefArray *pdrgpcrInner = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrInner->Append((*pdrgpcrInput)[0]);
	pdrgpcrInner->Append((*pdrgpcrInput)[1]);
	CColRefArray *pdrgpcrOuter = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrOuter->Append((*pdrgpcrInput)[0]);

	CExpression *pexprInner =
		fix.PexprLogicalGbAgg(pexprGet, pdrgpcrInner);
	CExpression *pexprOuter =
		fix.PexprLogicalGbAgg(pexprInner, pdrgpcrOuter);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprOuter, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTarget ||
			COperator::EopLogicalGbAgg != pexprTarget->Pop()->Eopid() ||
			COperator::EopLogicalGbAgg == (*pexprTarget)[0]->Pop()->Eopid() ||
			!pexprOuter->DeriveOutputColumns()->Equals(
				pexprTarget->DeriveOutputColumns()))
		{
			eres = GPOS_FAILED;
		}
	}

	// An aggregate function on the inner GbAgg invalidates its Proj* view.
	CColRef *pcrAgg = fix.PcrCreateInt4("collapse_max");
	CExpression *pexprInnerAgg = fix.PexprLogicalGbAgg(
		pexprGet, pdrgpcrInner, pcrAgg, (*pdrgpcrInput)[2]);
	CExpression *pexprOuterOverAgg =
		fix.PexprLogicalGbAgg(pexprInnerAgg, pdrgpcrOuter);
	CDSLModel *pmodelAgg = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcherAgg(mp, prule);
	if (matcherAgg.FMatch(prule->PfragSrc()->PopRoot(), pexprOuterOverAgg,
						pmodelAgg))
	{
		eres = GPOS_FAILED;
	}

	// A Local aggregate is an implementation stage, not a user-level Proj*.
	CExpressionArray *pdrgpexprEmpty = GPOS_NEW(mp) CExpressionArray(mp);
	CExpression *pexprEmptyList = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprEmpty);
	pdrgpcrInner->AddRef();
	pexprGet->AddRef();
	CExpression *pexprInnerLocal = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CLogicalGbAgg(
			mp, pdrgpcrInner, COperator::EgbaggtypeLocal),
		pexprGet, pexprEmptyList);
	CExpression *pexprOuterOverLocal =
		fix.PexprLogicalGbAgg(pexprInnerLocal, pdrgpcrOuter);
	CDSLModel *pmodelLocal = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcherLocal(mp, prule);
	if (matcherLocal.FMatch(prule->PfragSrc()->PopRoot(),
						  pexprOuterOverLocal, pmodelLocal))
	{
		eres = GPOS_FAILED;
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodelLocal->Release();
	pmodelAgg->Release();
	pmodel->Release();
	pexprOuterOverLocal->Release();
	pexprInnerLocal->Release();
	pexprOuterOverAgg->Release();
	pexprInnerAgg->Release();
	pexprOuter->Release();
	pexprInner->Release();
	pexprGet->Release();
	pdrgpcrOuter->Release();
	pdrgpcrInner->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLProjTest::EresUnittest_JoinKeySubsetFollowsAttrsEq()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_PROJ_REBIND_RULE);
	GPOS_ASSERT(nullptr != prule);

	CColRefArray *pdrgpcrLeft = nullptr;
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprLeft =
		fix.PexprLogicalGet("subset_left", 2, &pdrgpcrLeft);
	CExpression *pexprRight =
		fix.PexprLogicalGet("subset_right", 1, &pdrgpcrRight);
	CExpressionArray *pdrgpexprEq = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexprEq->Append(
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]));
	pdrgpexprEq->Append(
		fix.PexprEqPred((*pdrgpcrLeft)[1], (*pdrgpcrRight)[0]));
	CExpression *pexprPred = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopAnd),
		pdrgpexprEq);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprPred);

	CColRefArray *pdrgpcrProject = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrProject->Append((*pdrgpcrLeft)[0]);
	CExpression *pexprProject =
		fix.PexprLogicalProject(pexprJoin, pdrgpcrProject);
	pdrgpcrProject->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprProject, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTarget ||
			COperator::EopLogicalProject != pexprTarget->Pop()->Eopid() ||
			!(*(*pexprTarget)[0])[2]->Matches(pexprPred))
		{
			eres = GPOS_FAILED;
		}
		else
		{
			CExpression *pexprTargetElem = (*(*pexprTarget)[1])[0];
			CScalarIdent *popTargetIdent =
				CScalarIdent::PopConvert((*pexprTargetElem)[0]->Pop());
			if ((*pdrgpcrRight)[0] != popTargetIdent->Pcr())
			{
				eres = GPOS_FAILED;
			}
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprProject->Release();
	pexprJoin->Release();
	pexprPred->Release();
	pexprLeft->Release();
	pexprRight->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLProjTest::EresUnittest_PreservesHiddenLimitShell()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_PROJ_REBIND_RULE);
	GPOS_ASSERT(nullptr != prule);

	CColRefArray *pdrgpcrLeft = nullptr;
	CExpression *pexprLeft =
		fix.PexprLogicalGet("limit_rebind_left", 2, &pdrgpcrLeft);
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprRight =
		fix.PexprLogicalGet("limit_rebind_right", 2, &pdrgpcrRight);
	CExpression *pexprPred =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprPred);
	pexprPred->Release();

	pexprJoin->AddRef();
	CExpression *pexprLimit = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CLogicalLimit(
			mp, GPOS_NEW(mp) COrderSpec(mp), true /*global*/,
			true /*has count*/, false /*top DML*/),
		pexprJoin, CUtils::PexprScalarConstInt8(mp, 0),
		CUtils::PexprScalarConstInt8(mp, 10));
	CColRefArray *pdrgpcrProject = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrProject->Append((*pdrgpcrLeft)[0]);
	CExpression *pexprProject =
		fix.PexprLogicalProject(pexprLimit, pdrgpcrProject);
	pdrgpcrProject->Release();
	pexprLimit->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprProject, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTarget ||
			COperator::EopLogicalProject != pexprTarget->Pop()->Eopid() ||
			COperator::EopLogicalLimit != (*pexprTarget)[0]->Pop()->Eopid() ||
			COperator::EopLogicalInnerJoin !=
				(*(*pexprTarget)[0])[0]->Pop()->Eopid() ||
			!pexprProject->DeriveOutputColumns()->Equals(
				pexprTarget->DeriveOutputColumns()))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprProject->Release();
	pexprJoin->Release();
	pexprLeft->Release();
	pexprRight->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLProjTest::EresUnittest_NestedProjStarConsumesGeneratedDedup
//
//	@doc:
//		A complete Global dedup with PdrgpcrMinimal is a valid nested Proj* view,
//		but must not be eligible for root-level dedup deletion.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLProjTest::EresUnittest_NestedProjStarConsumesGeneratedDedup()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *pruleNested =
		PdslruleParseLocal(mp, GPOPT_DSL_PROJ_DEDUP_CHAIN_RULE);
	CDSLRule *pruleRoot =
		PdslruleParseLocal(mp, GPOPT_DSL_ROOT_DEDUP_DROP_RULE);
	GPOS_ASSERT(nullptr != pruleNested && nullptr != pruleRoot);

	CColRefArray *pdrgpcrInput = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet(
		"generated_dedup", 2, &pdrgpcrInput, gpos::ulong_max);
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrInput)[0]);

	CExpressionArray *pdrgpexprEmpty = GPOS_NEW(mp) CExpressionArray(mp);
	CExpression *pexprEmptyList = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprEmpty);
	pdrgpcrGroup->AddRef();
	pdrgpcrGroup->AddRef();
	pexprGet->AddRef();
	CExpression *pexprGeneratedDedup = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CLogicalGbAgg(
			mp, pdrgpcrGroup, pdrgpcrGroup,
			COperator::EgbaggtypeGlobal),
		pexprGet, pexprEmptyList);
	CExpression *pexprProject =
		fix.PexprLogicalProject(pexprGeneratedDedup, pdrgpcrGroup);

	CDSLModel *pmodelNested = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcherNested(mp, pruleNested);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcherNested.FMatch(
			pruleNested->PfragSrc()->PopRoot(), pexprProject, pmodelNested) ||
		!checker.FCheck(pruleNested, pmodelNested))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(pruleNested, pmodelNested);
		if (nullptr == pexprTarget ||
			COperator::EopLogicalGbAgg != pexprTarget->Pop()->Eopid() ||
			!pexprProject->DeriveOutputColumns()->Equals(
				pexprTarget->DeriveOutputColumns()))
		{
			eres = GPOS_FAILED;
		}
	}

	// The same provenance-marked aggregate cannot be deleted by a rule whose
	// source root itself is Proj*.
	CDSLModel *pmodelRoot = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcherRoot(mp, pruleRoot);
	if (matcherRoot.FMatch(
			pruleRoot->PfragSrc()->PopRoot(), pexprGeneratedDedup, pmodelRoot))
	{
		eres = GPOS_FAILED;
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodelRoot->Release();
	pmodelNested->Release();
	pexprProject->Release();
	pexprGeneratedDedup->Release();
	pexprGet->Release();
	pdrgpcrGroup->Release();
	pruleRoot->Release();
	pruleNested->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLProjTest::EresUnittest_InstantiateRebindsTargetAttrs
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLProjTest::EresUnittest_InstantiateRebindsTargetAttrs()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_PROJ_REBIND_RULE);
	GPOS_ASSERT(nullptr != prule);

	CColRefArray *pdrgpcrLeft = nullptr;
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprLeft =
		fix.PexprLogicalGet("rebind_left", 2, &pdrgpcrLeft);
	CExpression *pexprRight =
		fix.PexprLogicalGet("rebind_right", 2, &pdrgpcrRight);
	CExpression *pexprPred =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprPred);
	pexprPred->Release();

	CColRef *pcrOutput = fix.PcrCreateInt4("projected_key");
	CExpression *pexprSourceScalar = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarIdent(mp, (*pdrgpcrLeft)[0]));
	CExpression *pexprSourceElem = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectElement(mp, pcrOutput),
		pexprSourceScalar);
	CExpressionArray *pdrgpexprElems = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexprElems->Append(pexprSourceElem);
	CExpression *pexprSourceList = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprElems);
	pexprJoin->AddRef();
	CExpression *pexprProject = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalProject(mp), pexprJoin, pexprSourceList);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprProject, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTarget ||
			COperator::EopLogicalProject != pexprTarget->Pop()->Eopid() ||
			!pexprProject->DeriveOutputColumns()->Equals(
				pexprTarget->DeriveOutputColumns()))
		{
			eres = GPOS_FAILED;
		}
		else
		{
			CExpression *pexprTargetElem = (*(*pexprTarget)[1])[0];
			CScalarProjectElement *popTargetElem =
				CScalarProjectElement::PopConvert(pexprTargetElem->Pop());
			CScalarIdent *popTargetIdent =
				CScalarIdent::PopConvert((*pexprTargetElem)[0]->Pop());
			if (pcrOutput != popTargetElem->Pcr() ||
				(*pdrgpcrRight)[0] != popTargetIdent->Pcr())
			{
				eres = GPOS_FAILED;
			}
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprProject->Release();
	pexprJoin->Release();
	pexprLeft->Release();
	pexprRight->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLProjTest::EresUnittest_MatchBindsProjectedColumns
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLProjTest::EresUnittest_MatchBindsProjectedColumns()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_PROJ_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprProject = nullptr;
	CColRefArray *pdrgpcrOut = nullptr;
	BuildProjectOverGet(fix, 3 /*cols*/, 2 /*proj*/, &pexprGet, &pexprProject,
						&pdrgpcrOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprProject, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		// <a0> must be bound to the 2 projected columns; <t0> to the Get subtree;
		// the project list must have been recorded on the model.
		const CDSLSymbol *psymAttrs =
			prule->PfragSrc()->PopRoot()->Pdrgpsym()->operator[](0);
		const CDSLSymbol *psymSchema =
			prule->PfragSrc()->PopRoot()->Pdrgpsym()->operator[](1);
		CColRefArray *pdrgpcrBound = pmodel->PdrgpcrAttrs(psymAttrs);
		if (nullptr == pdrgpcrBound || 2 != pdrgpcrBound->Size() ||
			nullptr == pmodel->PexprProjList(psymSchema))
		{
			eres = GPOS_FAILED;
		}
	}

	pmodel->Release();
	pexprGet->Release();
	pexprProject->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLProjTest::EresUnittest_InstantiatePreservesOutput
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLProjTest::EresUnittest_InstantiatePreservesOutput()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_PROJ_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprProject = nullptr;
	CColRefArray *pdrgpcrOut = nullptr;
	BuildProjectOverGet(fix, 3 /*cols*/, 2 /*proj*/, &pexprGet, &pexprProject,
						&pdrgpcrOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CExpression *pexprTgt = nullptr;

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprProject, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTgt = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt ||
			COperator::EopLogicalProject != pexprTgt->Pop()->Eopid())
		{
			eres = GPOS_FAILED;
		}
		else if (!pexprProject->DeriveOutputColumns()->Equals(
					 pexprTgt->DeriveOutputColumns()))
		{
			// output-column invariant
			eres = GPOS_FAILED;
		}
		else if ((*pexprTgt)[0] != pexprGet)
		{
			// relational child is the grafted Get subtree (pointer identity)
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprGet->Release();
	pexprProject->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLProjTest::EresUnittest_TrivialSelectContinuesDedupChain
//
//	@doc:
//		A previous Proj* -> Proj rewrite is inserted into the memo as
//		Select(pure-dedup, TRUE). Match that safe identity-Proj view and prove the
//		next Proj(Proj*) -> Proj* rule can instantiate a valid single dedup.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLProjTest::EresUnittest_TrivialSelectContinuesDedupChain()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_PROJ_DEDUP_CHAIN_RULE);
	GPOS_ASSERT(nullptr != prule);

	CColRefArray *pdrgpcrInput = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet(
		"dedup_chain", 2, &pdrgpcrInput, gpos::ulong_max);
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrInput)[0]);
	CExpression *pexprDedup =
		fix.PexprLogicalGbAgg(pexprGet, pdrgpcrGroup);
	pdrgpcrGroup->Release();
	CExpression *pexprTrue = CUtils::PexprScalarConstBool(mp, true);
	CExpression *pexprMarker =
		fix.PexprLogicalSelect(pexprDedup, pexprTrue);
	pexprTrue->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprMarker, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTarget ||
			COperator::EopLogicalGbAgg != pexprTarget->Pop()->Eopid() ||
			!pexprMarker->DeriveOutputColumns()->Equals(
				pexprTarget->DeriveOutputColumns()))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprGet->Release();
	pexprDedup->Release();
	pexprMarker->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLProjTest::EresUnittest_DroppedDedupFeedsParentProject
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLProjTest::EresUnittest_DroppedDedupFeedsParentProject()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *pruleNested =
		PdslruleParseLocal(mp, GPOPT_DSL_PROJ_DEDUP_CHAIN_RULE);
	CDSLRule *pruleRoot =
		PdslruleParseLocal(mp, GPOPT_DSL_ROOT_DEDUP_DROP_RULE);
	GPOS_ASSERT(nullptr != pruleNested && nullptr != pruleRoot);

	CColRefArray *pdrgpcrUnique = nullptr;
	CExpression *pexprUnique =
		fix.PexprLogicalGet("dedup_marker_unique", 2, &pdrgpcrUnique, 0);
	CExpression *pexprTrue = CUtils::PexprScalarConstBool(mp, true);
	CExpression *pexprMarker =
		fix.PexprLogicalSelect(pexprUnique, pexprTrue);
	pexprTrue->Release();
	CExpression *pexprProject =
		fix.PexprLogicalProject(pexprMarker, pdrgpcrUnique);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, pruleNested);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(pruleNested->PfragSrc()->PopRoot(), pexprProject,
						pmodel) ||
		!checker.FCheck(pruleNested, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(pruleNested, pmodel);
		if (nullptr == pexprTarget ||
			COperator::EopLogicalGbAgg != pexprTarget->Pop()->Eopid() ||
			!pexprProject->DeriveOutputColumns()->Equals(
				pexprTarget->DeriveOutputColumns()))
		{
			eres = GPOS_FAILED;
		}
	}

	// The marker is a nested dependency carrier, not a new source-root Proj*;
	// otherwise a dedup-drop rule could repeatedly rewrite its own result.
	CDSLModel *pmodelRoot = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcherRoot(mp, pruleRoot);
	if (matcherRoot.FMatch(pruleRoot->PfragSrc()->PopRoot(), pexprMarker,
						pmodelRoot))
	{
		eres = GPOS_FAILED;
	}

	// TRUE alone is insufficient: without a key the child can contain duplicate
	// full rows, so it cannot represent an already-eliminated Proj*.
	CColRefArray *pdrgpcrNonUnique = nullptr;
	CExpression *pexprNonUnique = fix.PexprLogicalGet(
		"dedup_marker_non_unique", 2, &pdrgpcrNonUnique, gpos::ulong_max);
	pexprTrue = CUtils::PexprScalarConstBool(mp, true);
	CExpression *pexprUnsafeMarker =
		fix.PexprLogicalSelect(pexprNonUnique, pexprTrue);
	pexprTrue->Release();
	CExpression *pexprUnsafeProject =
		fix.PexprLogicalProject(pexprUnsafeMarker, pdrgpcrNonUnique);
	CDSLModel *pmodelUnsafe = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcherUnsafe(mp, pruleNested);
	if (matcherUnsafe.FMatch(pruleNested->PfragSrc()->PopRoot(),
						   pexprUnsafeProject, pmodelUnsafe))
	{
		eres = GPOS_FAILED;
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodelUnsafe->Release();
	pmodelRoot->Release();
	pmodel->Release();
	pexprUnsafeProject->Release();
	pexprUnsafeMarker->Release();
	pexprNonUnique->Release();
	pexprProject->Release();
	pexprMarker->Release();
	pexprUnique->Release();
	pruleRoot->Release();
	pruleNested->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLProjTest::EresUnittest_NoFireOnWrongRoot
//
//	@doc:
//		A Proj-rooted rule must NOT match a Select (operator-identity gate). Build
//		a Select and confirm the Proj source template fails to match.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLProjTest::EresUnittest_NoFireOnWrongRoot()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_PROJ_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 2 /*cols*/, &pdrgpcrOut);
	CExpression *pexprPred = fix.PexprPredAtom((*pdrgpcrOut)[0]);
	CExpression *pexprSelect = fix.PexprLogicalSelect(pexprGet, pexprPred);
	pexprPred->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);

	// must NOT match a Select
	GPOS_RESULT eres =
		matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSelect, pmodel)
			? GPOS_FAILED
			: GPOS_OK;

	pmodel->Release();
	pexprGet->Release();
	pexprSelect->Release();
	prule->Release();
	return eres;
}

// EOF
