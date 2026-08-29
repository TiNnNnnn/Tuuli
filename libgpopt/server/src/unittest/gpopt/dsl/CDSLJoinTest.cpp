//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLJoinTest.cpp
//
//	@doc:
//		Implementation of the Join three-stage tests (see header). Builds a live
//		CLogicalInnerJoin via the fixture, drives match -> instantiate, and asserts
//		join-key binding / predicate preservation / operator-identity gating and
//		the (FK-less) Reference rejection.
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLJoinTest.h"

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
#include "gpopt/operators/CLogicalConstTableGet.h"
#include "gpopt/operators/CLogicalLeftSemiApply.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarBoolOp.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

// identity InnerJoin rule: target reuses source's tables / join-key attrs.
#define GPOPT_DSL_JOIN_IDENTITY_RULE                              \
	"InnerJoin<a0 a1>(Input<t0>,Input<t1>)|"                      \
	"InnerJoin<a2 a3>(Input<t4>,Input<t5>)|"                      \
	"TableEq(t4,t0);TableEq(t5,t1);AttrsEq(a2,a0);AttrsEq(a3,a1)"

// a Reference-guarded InnerJoin rule: fires only if t0.a0 -> t1.a1 is an FK.
#define GPOPT_DSL_JOIN_REFERENCE_RULE                             \
	"InnerJoin<a0 a1>(Input<t0>,Input<t1>)|"                      \
	"InnerJoin<a2 a3>(Input<t4>,Input<t5>)|"                      \
	"TableEq(t4,t0);TableEq(t5,t1);AttrsEq(a2,a0);AttrsEq(a3,a1);" \
	"Reference(t0,a0,t1,a1)"

#define GPOPT_DSL_NESTED_JOIN_IDENTITY_RULE                              \
	"InnerJoin<a0 a1>(InnerJoin<a2 a3>(Input<t0>,Input<t1>),Input<t2>)|" \
	"InnerJoin<a4 a5>(InnerJoin<a6 a7>(Input<t3>,Input<t4>),Input<t5>)|" \
	"TableEq(t3,t0);TableEq(t4,t1);TableEq(t5,t2);"                       \
	"AttrsEq(a4,a0);AttrsEq(a5,a1);AttrsEq(a6,a2);AttrsEq(a7,a3)"

#define GPOPT_DSL_JOIN_OUTPUT_COMMUTE_RULE                               \
	"InnerJoin<a0 a1 a2 s0>(Input<t0>,Input<t1>)|"                       \
	"InnerJoin<a3 a4 a5 s1>(Input<t2>,Input<t3>)|"                       \
	"TableEq(t2,t1);TableEq(t3,t0);AttrsEq(a3,a1);AttrsEq(a4,a0);"       \
	"AttrsEq(a5,a2);SchemaEq(s1,s0)"

#define GPOPT_DSL_JOIN_RESIDUAL_IDENTITY_RULE                            \
	"InnerJoin<a0 a1 p0 a2 a3>(Input<t0>,Input<t1>)|"                    \
	"InnerJoin<a4 a5 p1 a6 a7>(Input<t2>,Input<t3>)|"                    \
	"AttrsSub(a0,t0);AttrsSub(a1,t1);AttrsSub(a2,t0);AttrsSub(a3,t1);"   \
	"TableEq(t2,t0);TableEq(t3,t1);AttrsEq(a4,a0);AttrsEq(a5,a1);"       \
	"PredicateEq(p1,p0);AttrsEq(a6,a2);AttrsEq(a7,a3)"

#define GPOPT_DSL_JOIN_PREDICATE_IDENTITY_RULE                         \
	"InnerJoin<p0 a0 a1>(Input<t0>,Input<t1>)|"                        \
		"InnerJoin<p1 a2 a3>(Input<t2>,Input<t3>)|"                        \
		"AttrsSub(a0,t0);AttrsSub(a1,t1);TableEq(t2,t0);TableEq(t3,t1);"  \
		"PredicateEq(p1,p0);AttrsEq(a2,a0);AttrsEmpty(a1);AttrsEmpty(a3)"

#define GPOPT_DSL_FALSE_LEFT_JOIN_EMPTY_RULE                           \
	"LeftJoin<p0 a0 a1>(Input<t0>,Input<t1>)|"                         \
	"LeftJoin<p1 a2 a3>(Input<t2>,Empty<t3>)|"                         \
	"AttrsSub(a0,t0);AttrsSub(a1,t1);PredicateFalse(p0);"              \
	"TableEq(t2,t0);TableEq(t3,t1);PredicateEq(p1,p0);"                \
	"AttrsEq(a2,a0);AttrsEq(a3,a1)"

#define GPOPT_DSL_SEMI_JOIN_IDENTITY_RULE                             \
	"SemiJoin<p0 a0 a1>(Input<t0>,Input<t1>)|"                        \
	"SemiJoin<p1 a2 a3>(Input<t2>,Input<t3>)|"                        \
	"TableEq(t2,t0);TableEq(t3,t1);PredicateEq(p1,p0);"               \
	"AttrsEq(a2,a0);AttrsEq(a3,a1)"

#define GPOPT_DSL_UNCORRELATED_SEMI_APPLY_RULE                        \
	"SemiApply<p0 a0 a1 a2>(Input<t0>,Input<t1>)|"                    \
	"SemiJoin<p1 a3 a4>(Input<t2>,Input<t3>)|"                        \
	"TableEq(t2,t0);TableEq(t3,t1);PredicateEq(p1,p0);"               \
	"AttrsEq(a3,a0);AttrsEq(a4,a1);AttrsEmpty(a2);"                   \
	"ErrorFree(p0);ErrorFree(p1)"

#define GPOPT_DSL_BUILD_UNCORRELATED_SEMI_APPLY_RULE                  \
	"SemiJoin<p0 a0 a1>(Input<t0>,Input<t1>)|"                        \
	"SemiApply<p1 a2 a3 a4>(Input<t2>,Input<t3>)|"                    \
	"TableEq(t2,t0);TableEq(t3,t1);PredicateEq(p1,p0);"               \
	"AttrsEq(a2,a0);AttrsEq(a3,a1);AttrsEmpty(a4);"                   \
	"ErrorFree(p0);ErrorFree(p1)"

#define GPOPT_DSL_SEMI_APPLY_FILTER_TO_JOIN_RULE                         \
	"SemiApply<p0 a0 a1 a2>(Input<t0>,Filter<p1 a3 a4>(Input<t1>))|"    \
	"SemiJoin<p2 a5 a6>(Input<t2>,Input<t3>)|"                           \
	"TableEq(t2,t0);TableEq(t3,t1);PredicateAnd(p2,p0,p1);"              \
	"AttrsEq(a5,a0);AttrsEq(a5,a4);AttrsEq(a6,a3)"

static CDSLRule *
PdslruleParseLocal(CMemoryPool *mp, const CHAR *sz_dsl)
{
	CWStringDynamic strErr(mp);
	return CDSLRuleParser::PdslruleParse(mp, sz_dsl, "EQ" /*verdict*/, &strErr);
}

// build InnerJoin(Get t0[2], Get t1[2], t0.c0 = t1.c0). Returns the two Gets and
// the join; caller owns them.
static void
BuildInnerJoinEqui(CDSLTestFixture &fix, CExpression **ppLeft,
				   CExpression **ppRight, CExpression **ppJoin)
{
	CColRefArray *pdrgpcrLeft = nullptr;
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprLeft = fix.PexprLogicalGet("t0", 2, &pdrgpcrLeft);
	CExpression *pexprRight = fix.PexprLogicalGet("t1", 2, &pdrgpcrRight);

	CExpression *pexprPred =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprPred);
	pexprPred->Release();

	*ppLeft = pexprLeft;
	*ppRight = pexprRight;
	*ppJoin = pexprJoin;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinTest::EresUnittest
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLJoinTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CDSLJoinTest::EresUnittest_MatchBindsJoinKeys),
		GPOS_UNITTEST_FUNC(CDSLJoinTest::EresUnittest_InstantiatePreservesJoin),
		GPOS_UNITTEST_FUNC(
			CDSLJoinTest::EresUnittest_ExtendedOutputPreservesCommutedJoin),
		GPOS_UNITTEST_FUNC(
			CDSLJoinTest::EresUnittest_NestedJoinPredicatesStayLocal),
		GPOS_UNITTEST_FUNC(CDSLJoinTest::EresUnittest_NonEquiPredicateResidual),
		GPOS_UNITTEST_FUNC(CDSLJoinTest::EresUnittest_PredicateOnlyJoin),
		GPOS_UNITTEST_FUNC(
			CDSLJoinTest::EresUnittest_ExplicitSemiJoinBindsCompletePredicate),
		GPOS_UNITTEST_FUNC(
			CDSLJoinTest::EresUnittest_UncorrelatedSemiApplyBuildsSemiJoin),
		GPOS_UNITTEST_FUNC(
			CDSLJoinTest::EresUnittest_SemiJoinBuildsUncorrelatedSemiApply),
		GPOS_UNITTEST_FUNC(
			CDSLJoinTest::EresUnittest_PredicateAndBuildsSemiJoinCondition),
		GPOS_UNITTEST_FUNC(
			CDSLJoinTest::EresUnittest_FalseLeftJoinBuildsEmptyInput),
		GPOS_UNITTEST_FUNC(CDSLJoinTest::EresUnittest_NoFireOnWrongRoot),
		GPOS_UNITTEST_FUNC(CDSLJoinTest::EresUnittest_ReferenceRejectsWithoutFK),
		GPOS_UNITTEST_FUNC(
			CDSLJoinTest::EresUnittest_ReferenceAcceptsReflexiveBaseColumn),
		GPOS_UNITTEST_FUNC(
			CDSLJoinTest::EresUnittest_ReferenceRejectsFilteredReflexiveTarget),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLJoinTest::EresUnittest_PredicateAndBuildsSemiJoinCondition()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_SEMI_APPLY_FILTER_TO_JOIN_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrOuter = nullptr;
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("predicate_and_outer", 2, &pdrgpcrOuter);
	CExpression *pexprInner =
		fix.PexprLogicalGet("predicate_and_inner", 2, &pdrgpcrInner);
	CExpression *pexprApplyPred = fix.PexprPredAtom((*pdrgpcrOuter)[1]);
	CExpression *pexprFilterPred =
		fix.PexprEqPred((*pdrgpcrOuter)[1], (*pdrgpcrInner)[0]);
	CExpression *pexprFilteredInner =
		fix.PexprLogicalSelect(pexprInner, pexprFilterPred);
	pexprOuter->AddRef();
	pexprFilteredInner->AddRef();
	pexprApplyPred->AddRef();
	CExpression *pexprApply = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalLeftSemiApply(mp), pexprOuter,
		pexprFilteredInner, pexprApplyPred);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprApply, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		CExpressionArray *pdrgpexprConjuncts = nullptr;
		if (nullptr != pexprTarget)
		{
			pdrgpexprConjuncts =
				CPredicateUtils::PdrgpexprConjuncts(mp, (*pexprTarget)[2]);
		}
		if (nullptr == pexprTarget ||
			COperator::EopLogicalLeftSemiJoin !=
				pexprTarget->Pop()->Eopid() ||
			nullptr == pdrgpexprConjuncts || 2 != pdrgpexprConjuncts->Size() ||
			!(*pdrgpexprConjuncts)[0]->Matches(pexprApplyPred) ||
			!(*pdrgpexprConjuncts)[1]->Matches(pexprFilterPred))
		{
			eres = GPOS_FAILED;
		}
		CRefCount::SafeRelease(pdrgpexprConjuncts);
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprApply->Release();
	pexprFilteredInner->Release();
	pexprApplyPred->Release();
	pexprFilterPred->Release();
	pexprOuter->Release();
	pexprInner->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLJoinTest::EresUnittest_SemiJoinBuildsUncorrelatedSemiApply()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp, GPOPT_DSL_BUILD_UNCORRELATED_SEMI_APPLY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprLeft = fix.PexprLogicalGet("apply_target_left", 1);
	CExpression *pexprRight = fix.PexprLogicalGet("apply_target_right", 1);
	CExpression *pexprTrue = CUtils::PexprScalarConstBool(mp, true);
	pexprLeft->AddRef();
	pexprRight->AddRef();
	pexprTrue->AddRef();
	CExpression *pexprSemiJoin = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalLeftSemiJoin(mp), pexprLeft, pexprRight,
		pexprTrue);
	pexprTrue->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSemiJoin,
						pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTarget ||
			COperator::EopLogicalLeftSemiApply !=
				pexprTarget->Pop()->Eopid() ||
			0 != (*pexprTarget)[1]->DeriveOuterReferences()->Size() ||
			!(*pexprTarget)[2]->Matches((*pexprSemiJoin)[2]))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprSemiJoin->Release();
	pexprLeft->Release();
	pexprRight->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLJoinTest::EresUnittest_UncorrelatedSemiApplyBuildsSemiJoin()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_UNCORRELATED_SEMI_APPLY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrOuter = nullptr;
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("apply_outer", 2, &pdrgpcrOuter);
	CExpression *pexprInner =
		fix.PexprLogicalGet("apply_inner", 2, &pdrgpcrInner);
	pexprOuter->AddRef();
	pexprInner->AddRef();
	CExpression *pexprApply =
		CUtils::PexprLogicalApply<CLogicalLeftSemiApply>(
			mp, pexprOuter, pexprInner, (*pdrgpcrInner)[0],
			COperator::EopScalarSubqueryExists);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprApply, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLSymbolArray *pdrgpsym =
			prule->PfragSrc()->PopRoot()->Pdrgpsym();
		CColRefArray *pdrgpcrCorrelations =
			pmodel->PdrgpcrAttrs((*pdrgpsym)[3]);
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pdrgpcrCorrelations ||
			0 != pdrgpcrCorrelations->Size() || nullptr == pexprTarget ||
			COperator::EopLogicalLeftSemiJoin !=
				pexprTarget->Pop()->Eopid() ||
			!(*pexprTarget)[2]->Matches((*pexprApply)[2]))
		{
			eres = GPOS_FAILED;
		}
	}

	// The same template must expose, and therefore reject, an actual reference
	// from the inner subtree to the current outer input.
	CExpression *pexprCorrelation =
		fix.PexprEqPred((*pdrgpcrOuter)[0], (*pdrgpcrInner)[0]);
	CExpression *pexprCorrelatedInner =
		fix.PexprLogicalSelect(pexprInner, pexprCorrelation);
	pexprCorrelation->Release();
	pexprOuter->AddRef();
	pexprCorrelatedInner->AddRef();
	CExpression *pexprCorrelatedApply =
		CUtils::PexprLogicalApply<CLogicalLeftSemiApply>(
			mp, pexprOuter, pexprCorrelatedInner, (*pdrgpcrInner)[0],
			COperator::EopScalarSubqueryExists);
	CDSLModel *pmodelCorrelated = GPOS_NEW(mp) CDSLModel(mp);
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprCorrelatedApply,
						pmodelCorrelated) ||
		checker.FCheck(prule, pmodelCorrelated))
	{
		eres = GPOS_FAILED;
	}

	pmodelCorrelated->Release();
	pexprCorrelatedApply->Release();
	pexprCorrelatedInner->Release();
	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprApply->Release();
	pexprOuter->Release();
	pexprInner->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLJoinTest::EresUnittest_ExplicitSemiJoinBindsCompletePredicate()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_SEMI_JOIN_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrLeft = nullptr;
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprLeft = fix.PexprLogicalGet("t0", 2, &pdrgpcrLeft);
	CExpression *pexprRight = fix.PexprLogicalGet("t1", 2, &pdrgpcrRight);
	CExpression *pexprPred =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	pexprLeft->AddRef();
	pexprRight->AddRef();
	pexprPred->AddRef();
	CExpression *pexprSemiJoin = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalLeftSemiJoin(mp), pexprLeft, pexprRight,
		pexprPred);
	pexprPred->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSemiJoin,
						pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLSymbolArray *pdrgpsym =
			prule->PfragSrc()->PopRoot()->Pdrgpsym();
		CExpression *pexprBound = pmodel->PexprPred((*pdrgpsym)[0]);
		CColRefArray *pdrgpcrBoundLeft =
			pmodel->PdrgpcrAttrs((*pdrgpsym)[1]);
		CColRefArray *pdrgpcrBoundRight =
			pmodel->PdrgpcrAttrs((*pdrgpsym)[2]);
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprBound || nullptr == pdrgpcrBoundLeft ||
			1 != pdrgpcrBoundLeft->Size() || nullptr == pdrgpcrBoundRight ||
			1 != pdrgpcrBoundRight->Size() || nullptr == pexprTarget ||
			COperator::EopLogicalLeftSemiJoin !=
				pexprTarget->Pop()->Eopid() ||
			!(*pexprTarget)[2]->Matches((*pexprSemiJoin)[2]))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprSemiJoin->Release();
	pexprLeft->Release();
	pexprRight->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLJoinTest::EresUnittest_FalseLeftJoinBuildsEmptyInput()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_FALSE_LEFT_JOIN_EMPTY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprLeft = fix.PexprLogicalGet("t0", 2);
	CExpression *pexprRight = fix.PexprLogicalGet("t1", 2);
	CExpression *pexprFalse = CUtils::PexprScalarConstBool(mp, false);
	CExpression *pexprJoin =
		fix.PexprLogicalLeftOuterJoin(pexprLeft, pexprRight, pexprFalse);
	pexprFalse->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprJoin, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTarget ||
			COperator::EopLogicalLeftOuterJoin != pexprTarget->Pop()->Eopid() ||
			COperator::EopLogicalConstTableGet !=
				(*pexprTarget)[1]->Pop()->Eopid() ||
			0 != CLogicalConstTableGet::PopConvert((*pexprTarget)[1]->Pop())
					->Pdrgpdrgpdatum()
					->Size() ||
			!pexprRight->DeriveOutputColumns()->Equals(
				(*pexprTarget)[1]->DeriveOutputColumns()) ||
			!CUtils::FScalarConstFalse((*pexprTarget)[2]))
		{
			eres = GPOS_FAILED;
		}
	}

	// The same structure with TRUE must be rejected by PredicateFalse.
	CExpression *pexprTrue = CUtils::PexprScalarConstBool(mp, true);
	CExpression *pexprTrueJoin =
		fix.PexprLogicalLeftOuterJoin(pexprLeft, pexprRight, pexprTrue);
	pexprTrue->Release();
	CDSLModel *pmodelTrue = GPOS_NEW(mp) CDSLModel(mp);
	if (matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprTrueJoin,
					   pmodelTrue) &&
		checker.FCheck(prule, pmodelTrue))
	{
		eres = GPOS_FAILED;
	}

	pmodelTrue->Release();
	pexprTrueJoin->Release();
	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprJoin->Release();
	pexprLeft->Release();
	pexprRight->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLJoinTest::EresUnittest_PredicateOnlyJoin()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_JOIN_PREDICATE_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrLeft = nullptr;
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprLeft = fix.PexprLogicalGet("t0", 2, &pdrgpcrLeft);
	CExpression *pexprRight = fix.PexprLogicalGet("t1", 2, &pdrgpcrRight);
	CExpression *pexprPred = fix.PexprPredAtom((*pdrgpcrLeft)[1]);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprPred);
	pexprPred->Release();

	CDSLMatcher matcher(mp);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	CDSLConstraintChecker checker(mp);
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprJoin, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLSymbolArray *pdrgpsym =
			prule->PfragSrc()->PopRoot()->Pdrgpsym();
		CExpression *pexprBound = pmodel->PexprPred((*pdrgpsym)[0]);
		CColRefArray *pdrgpcrBoundLeft =
			pmodel->PdrgpcrAttrs((*pdrgpsym)[1]);
		CColRefArray *pdrgpcrBoundRight =
			pmodel->PdrgpcrAttrs((*pdrgpsym)[2]);
		CDSLInstantiator inst(mp);
		pexprTarget = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprBound || nullptr == pdrgpcrBoundLeft ||
			1 != pdrgpcrBoundLeft->Size() || nullptr == pdrgpcrBoundRight ||
			0 != pdrgpcrBoundRight->Size() || nullptr == pexprTarget ||
			!(*pexprTarget)[2]->Matches((*pexprJoin)[2]))
		{
			eres = GPOS_FAILED;
		}
	}

	// The same three-symbol template must not absorb an equality join; keyed
	// conditions belong to the existing two/five/seven-symbol forms.
	CExpression *pexprEq =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprEquiJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprEq);
	pexprEq->Release();
	CDSLModel *pmodelEqui = GPOS_NEW(mp) CDSLModel(mp);
	if (matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprEquiJoin,
						   pmodelEqui))
	{
		 eres = GPOS_FAILED;
	}

	// A non-equality predicate that depends on the right input still matches the
	// predicate-only shape, but must fail the generic AttrsEmpty applicability
	// guard.
	CExpression *pexprRightPred = fix.PexprPredAtom((*pdrgpcrRight)[1]);
	CExpression *pexprRightJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprRightPred);
	pexprRightPred->Release();
	CDSLModel *pmodelRight = GPOS_NEW(mp) CDSLModel(mp);
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprRightJoin,
							pmodelRight) ||
		checker.FCheck(prule, pmodelRight))
	{
		eres = GPOS_FAILED;
	}

	pmodelRight->Release();
	pexprRightJoin->Release();
	pmodelEqui->Release();
	pexprEquiJoin->Release();
	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprLeft->Release();
	pexprRight->Release();
	pexprJoin->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinTest::EresUnittest_MatchBindsJoinKeys
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLJoinTest::EresUnittest_MatchBindsJoinKeys()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_JOIN_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprLeft = nullptr;
	CExpression *pexprRight = nullptr;
	CExpression *pexprJoin = nullptr;
	BuildInnerJoinEqui(fix, &pexprLeft, &pexprRight, &pexprJoin);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprJoin, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		// <a0> left key (1 col), <a1> right key (1 col), t0/t1 bound, predicate
		// recorded on the model.
		CDSLSymbolArray *pdrgpsym = prule->PfragSrc()->PopRoot()->Pdrgpsym();
		const CDSLSymbol *psymLeft = (*pdrgpsym)[0];
		const CDSLSymbol *psymRight = (*pdrgpsym)[1];
		CColRefArray *pdrgpcrL = pmodel->PdrgpcrAttrs(psymLeft);
		CColRefArray *pdrgpcrR = pmodel->PdrgpcrAttrs(psymRight);
		if (nullptr == pdrgpcrL || 1 != pdrgpcrL->Size() ||
			nullptr == pdrgpcrR || 1 != pdrgpcrR->Size() ||
			nullptr == pmodel->PexprJoinPred(psymLeft, psymRight))
		{
			eres = GPOS_FAILED;
		}
	}

	pmodel->Release();
	pexprLeft->Release();
	pexprRight->Release();
	pexprJoin->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinTest::EresUnittest_InstantiatePreservesJoin
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLJoinTest::EresUnittest_InstantiatePreservesJoin()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_JOIN_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprLeft = nullptr;
	CExpression *pexprRight = nullptr;
	CExpression *pexprJoin = nullptr;
	BuildInnerJoinEqui(fix, &pexprLeft, &pexprRight, &pexprJoin);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CExpression *pexprTgt = nullptr;

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprJoin, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTgt = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt ||
			COperator::EopLogicalInnerJoin != pexprTgt->Pop()->Eopid())
		{
			eres = GPOS_FAILED;
		}
		else if (!pexprJoin->DeriveOutputColumns()->Equals(
					 pexprTgt->DeriveOutputColumns()))
		{
			// output-column invariant
			eres = GPOS_FAILED;
		}
		else if ((*pexprTgt)[0] != pexprLeft || (*pexprTgt)[1] != pexprRight)
		{
			// both relational children are the grafted subtrees (pointer identity)
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprLeft->Release();
	pexprRight->Release();
	pexprJoin->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLJoinTest::EresUnittest_ExtendedOutputPreservesCommutedJoin()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_JOIN_OUTPUT_COMMUTE_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprLeft = nullptr;
	CExpression *pexprRight = nullptr;
	CExpression *pexprJoin = nullptr;
	BuildInnerJoinEqui(fix, &pexprLeft, &pexprRight, &pexprJoin);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprJoin, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLSymbolArray *pdrgpsym =
			prule->PfragSrc()->PopRoot()->Pdrgpsym();
		CColRefArray *pdrgpcrOutput =
			pmodel->PdrgpcrAttrs((*pdrgpsym)[2]);
		CColRefArray *pdrgpcrSchema =
			pmodel->PdrgpcrSchema((*pdrgpsym)[3]);
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		GPOS_UNITTEST_ASSERT(nullptr != pdrgpcrOutput);
		GPOS_UNITTEST_ASSERT(nullptr != pdrgpcrSchema);
		GPOS_UNITTEST_ASSERT(
			CColRef::Equals(pdrgpcrOutput, pdrgpcrSchema));
		GPOS_UNITTEST_ASSERT(nullptr != pexprTarget);
		GPOS_UNITTEST_ASSERT((*pexprTarget)[0] == pexprRight);
		GPOS_UNITTEST_ASSERT((*pexprTarget)[1] == pexprLeft);
		GPOS_UNITTEST_ASSERT(pexprTarget->DeriveOutputColumns()->Equals(
			pexprJoin->DeriveOutputColumns()));
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprLeft->Release();
	pexprRight->Release();
	pexprJoin->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLJoinTest::EresUnittest_NestedJoinPredicatesStayLocal()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_NESTED_JOIN_IDENTITY_RULE);
	GPOS_ASSERT(nullptr != prule);

	CColRefArray *pdrgpcr0 = nullptr;
	CColRefArray *pdrgpcr1 = nullptr;
	CColRefArray *pdrgpcr2 = nullptr;
	CExpression *pexpr0 = fix.PexprLogicalGet("nested_t0", 2, &pdrgpcr0);
	CExpression *pexpr1 = fix.PexprLogicalGet("nested_t1", 2, &pdrgpcr1);
	CExpression *pexpr2 = fix.PexprLogicalGet("nested_t2", 2, &pdrgpcr2);
	CExpression *pexprInnerPred =
		fix.PexprEqPred((*pdrgpcr0)[0], (*pdrgpcr1)[0]);
	CExpression *pexprInner =
		fix.PexprLogicalInnerJoin(pexpr0, pexpr1, pexprInnerPred);
	CExpression *pexprOuterPred =
		fix.PexprEqPred((*pdrgpcr0)[1], (*pdrgpcr2)[1]);
	CExpression *pexprSource =
		fix.PexprLogicalInnerJoin(pexprInner, pexpr2, pexprOuterPred);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
							   pmodel));
	CDSLConstraintChecker checker(mp);
	GPOS_ASSERT(checker.FCheck(prule, pmodel));
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
	GPOS_ASSERT(nullptr != pexprTarget);
	GPOS_ASSERT(COperator::EopLogicalInnerJoin ==
				pexprTarget->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalInnerJoin ==
				(*pexprTarget)[0]->Pop()->Eopid());
	GPOS_ASSERT((*pexprTarget)[2]->Matches(pexprOuterPred));
	GPOS_ASSERT((*(*pexprTarget)[0])[2]->Matches(pexprInnerPred));
	GPOS_ASSERT(!pexprOuterPred->Matches(pexprInnerPred));

	pexprTarget->Release();
	pmodel->Release();
	pexprSource->Release();
	pexprOuterPred->Release();
	pexprInner->Release();
	pexprInnerPred->Release();
	pexpr0->Release();
	pexpr1->Release();
	pexpr2->Release();
	prule->Release();
	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinTest::EresUnittest_NonEquiPredicateResidual
//
//	@doc:
//		Join predicate = (t0.c0 = t1.c0) AND IsNull(t0.c1). The equi conjunct binds
//		as a key; the non-equi atom is preserved as residual; instantiation still
//		produces a join whose output columns match the source.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLJoinTest::EresUnittest_NonEquiPredicateResidual()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_JOIN_RESIDUAL_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrLeft = nullptr;
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprLeft = fix.PexprLogicalGet("t0", 2, &pdrgpcrLeft);
	CExpression *pexprRight = fix.PexprLogicalGet("t1", 2, &pdrgpcrRight);

	// (t0.c0 = t1.c0) AND IsNull(t0.c1)
	CExpression *pexprEq =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprAtom = fix.PexprPredAtom((*pdrgpcrLeft)[1]);
	CExpressionArray *pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexpr->Append(pexprEq);
	pdrgpexpr->Append(pexprAtom);
	CExpression *pexprPred = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopAnd),
		pdrgpexpr);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprPred);
	pexprPred->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CExpression *pexprTgt = nullptr;

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprJoin, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		// One equi key each side; the residual expression and its dependencies
		// are first-class bindings rather than an unstructured leftover.
		CDSLSymbolArray *pdrgpsym = prule->PfragSrc()->PopRoot()->Pdrgpsym();
		CColRefArray *pdrgpcrL = pmodel->PdrgpcrAttrs((*pdrgpsym)[0]);
		CExpression *pexprResidual = pmodel->PexprPred((*pdrgpsym)[2]);
		CColRefArray *pdrgpcrResidualL =
			pmodel->PdrgpcrAttrs((*pdrgpsym)[3]);
		CColRefArray *pdrgpcrResidualR =
			pmodel->PdrgpcrAttrs((*pdrgpsym)[4]);
		if (nullptr == pdrgpcrL || 1 != pdrgpcrL->Size() ||
			nullptr == pexprResidual ||
			nullptr == pdrgpcrResidualL || 1 != pdrgpcrResidualL->Size() ||
			nullptr == pdrgpcrResidualR || 0 != pdrgpcrResidualR->Size() ||
			!pexprResidual->Matches(pexprAtom))
		{
			eres = GPOS_FAILED;
		}
		else
		{
			CDSLInstantiator inst(mp);
			pexprTgt = inst.PexprInstantiate(prule, pmodel);
			if (nullptr == pexprTgt ||
				!(*pexprTgt)[2]->Matches((*pexprJoin)[2]) ||
				!pexprJoin->DeriveOutputColumns()->Equals(
					pexprTgt->DeriveOutputColumns()))
			{
				eres = GPOS_FAILED;
			}
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprLeft->Release();
	pexprRight->Release();
	pexprJoin->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinTest::EresUnittest_NoFireOnWrongRoot
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLJoinTest::EresUnittest_NoFireOnWrongRoot()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_JOIN_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 2, &pdrgpcrOut);
	CExpression *pexprPred = fix.PexprPredAtom((*pdrgpcrOut)[0]);
	CExpression *pexprSelect = fix.PexprLogicalSelect(pexprGet, pexprPred);
	pexprPred->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);

	// a join-rooted rule must NOT match a Select.
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

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinTest::EresUnittest_ReferenceRejectsWithoutFK
//
//	@doc:
//		The match binds fine, but FCheckConstraints must REJECT because the
//		programmatic fixture carries no FK metadata (FCheckReference cannot confirm
//		the FK). Live FK verification is a base-C concern (doc M2 §C).
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLJoinTest::EresUnittest_ReferenceRejectsWithoutFK()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_JOIN_REFERENCE_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprLeft = nullptr;
	CExpression *pexprRight = nullptr;
	CExpression *pexprJoin = nullptr;
	BuildInnerJoinEqui(fix, &pexprLeft, &pexprRight, &pexprJoin);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprJoin, pmodel))
	{
		// the structural match should still succeed
		eres = GPOS_FAILED;
	}
	else if (checker.FCheck(prule, pmodel))
	{
		// but the Reference constraint must gate the fire (no FK => reject)
		eres = GPOS_FAILED;
	}

	pmodel->Release();
	pexprLeft->Release();
	pexprRight->Release();
	pexprJoin->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinTest::EresUnittest_ReferenceAcceptsReflexiveBaseColumn
//
//	@doc:
//		Two aliases of one base relation satisfy Reference(t0,a0,t1,a1) when
//		the bound attribute vectors name the same column.  No FK metadata is
//		needed: inclusion of a relation's column in itself is reflexive.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLJoinTest::EresUnittest_ReferenceAcceptsReflexiveBaseColumn()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_JOIN_REFERENCE_RULE);
	GPOS_ASSERT(nullptr != prule);

	CTableDescriptor *ptabdesc =
		fix.PtabdescCreate("self_reference", 2, gpos::ulong_max, false);
	CColRefArray *pdrgpcrLeft = nullptr;
	CExpression *pexprLeft =
		fix.PexprLogicalGet(ptabdesc, "self_reference_left", &pdrgpcrLeft);
	ptabdesc->AddRef();
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprRight =
		fix.PexprLogicalGet(ptabdesc, "self_reference_right", &pdrgpcrRight);
	CExpression *pexprPred =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprPred);
	pexprPred->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	GPOS_RESULT eres =
		matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprJoin, pmodel) &&
				checker.FCheck(prule, pmodel)
			? GPOS_OK
			: GPOS_FAILED;

	pmodel->Release();
	pexprLeft->Release();
	pexprRight->Release();
	pexprJoin->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinTest::EresUnittest_ReferenceRejectsFilteredReflexiveTarget
//
//	@doc:
//		The reflexive shortcut must not treat a filtered alias as the complete
//		referred relation: the filter may have removed a value from the inclusion
//		dependency's target domain.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLJoinTest::EresUnittest_ReferenceRejectsFilteredReflexiveTarget()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_JOIN_REFERENCE_RULE);
	GPOS_ASSERT(nullptr != prule);

	CTableDescriptor *ptabdesc =
		fix.PtabdescCreate("filtered_self_reference", 2, gpos::ulong_max, false);
	CColRefArray *pdrgpcrLeft = nullptr;
	CExpression *pexprLeft = fix.PexprLogicalGet(
		ptabdesc, "filtered_self_reference_left", &pdrgpcrLeft);
	ptabdesc->AddRef();
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprRightGet = fix.PexprLogicalGet(
		ptabdesc, "filtered_self_reference_right", &pdrgpcrRight);
	CExpression *pexprFilterPred = fix.PexprPredAtom((*pdrgpcrRight)[1]);
	CExpression *pexprRight =
		fix.PexprLogicalSelect(pexprRightGet, pexprFilterPred);
	pexprRightGet->Release();
	pexprFilterPred->Release();
	CExpression *pexprJoinPred =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprJoinPred);
	pexprJoinPred->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprJoin, pmodel) ||
		checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}

	pmodel->Release();
	pexprLeft->Release();
	pexprRight->Release();
	pexprJoin->Release();
	prule->Release();
	return eres;
}

// EOF
