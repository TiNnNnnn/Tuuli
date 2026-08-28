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
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CScalarBoolOp.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"
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
			CDSLProjTest::EresUnittest_NestedProjStarConsumesGeneratedDedup),
		GPOS_UNITTEST_FUNC(CDSLProjTest::EresUnittest_NoFireOnWrongRoot),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
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
