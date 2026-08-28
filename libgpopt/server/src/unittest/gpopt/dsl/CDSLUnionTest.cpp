//---------------------------------------------------------------------------
//	MONSOON DSL Union tests
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLUnionTest.h"

#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/operators/CLogicalGet.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalSetOp.h"
#include "gpopt/operators/CLogicalUnion.h"
#include "gpopt/operators/CLogicalUnionAll.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

#define GPOPT_DSL_UNION_IDENTITY_RULE                                  \
	"Union(Input<t0>,Input<t1>)|Union(Input<t2>,Input<t3>)|"            \
	"TableEq(t2,t0);TableEq(t3,t1)"

#define GPOPT_DSL_UNION_SWAP_RULE                                      \
	"Union(Input<t0>,Input<t1>)|Union(Input<t2>,Input<t3>)|"            \
	"TableEq(t2,t1);TableEq(t3,t0)"

#define GPOPT_DSL_UNION_DISTINCT_RULE                                  \
	"Union*(Input<t0>,Input<t1>)|Union*(Input<t2>,Input<t3>)|"          \
	"TableEq(t2,t0);TableEq(t3,t1)"

#define GPOPT_DSL_UNION_OUTPUT_DEDUP_RULE                              \
	"Union*<a0 s0>(Input<t0>,Input<t1>)|"                               \
	"Proj*<a1 s1>(Union<a2 s2>(Input<t2>,Input<t3>))|"                  \
	"TableEq(t2,t0);TableEq(t3,t1);AttrsEq(a1,a0);SchemaEq(s1,s0);"    \
	"AttrsEq(a2,a0);SchemaEq(s2,s0)"

#define GPOPT_DSL_JOIN_UNION_DISTRIBUTION_RULE                         \
	"InnerJoin<a0 a1 a2 s0>(Union(Input<t0>,Input<t1>),Input<t2>)|"    \
	"Union<a7 s1>(InnerJoin<a3 a4>(Input<t3>,Input<t4>),"              \
	"InnerJoin<a5 a6>(Input<t5>,Input<t6>))|"                          \
	"AttrsSub(a0,t0);AttrsSub(a1,t2);TableEq(t3,t0);TableEq(t5,t1);"   \
	"TableEq(t4,t2);TableEq(t6,t2);AttrsEq(a3,a0);AttrsEq(a5,a0);"     \
	"AttrsEq(a4,a1);AttrsEq(a6,a1);AttrsEq(a7,a2);SchemaEq(s1,s0)"

#define GPOPT_DSL_LEFT_JOIN_UNION_DISTRIBUTION_RULE                    \
	"LeftJoin<a0 a1 a2 s0>(Union(Input<t0>,Input<t1>),Input<t2>)|"     \
	"Union<a7 s1>(LeftJoin<a3 a4>(Input<t3>,Input<t4>),"               \
	"LeftJoin<a5 a6>(Input<t5>,Input<t6>))|"                           \
	"AttrsSub(a0,t0);AttrsSub(a1,t2);TableEq(t3,t0);TableEq(t5,t1);"   \
	"TableEq(t4,t2);TableEq(t6,t2);AttrsEq(a3,a0);AttrsEq(a5,a0);"     \
	"AttrsEq(a4,a1);AttrsEq(a6,a1);AttrsEq(a7,a2);SchemaEq(s1,s0)"

// MONSOON/dataset/rules/rules.els.reduced.txt:1215, unchanged.
#define GPOPT_DSL_UNION_CORPUS_PROJ_RULE                               \
	"Union(Proj<a0 s0>(Input<t0>),Proj<a1 s1>(Input<t1>))|"            \
	"Union(Proj<a2 s2>(Input<t2>),Proj<a3 s3>(Input<t3>))|"            \
	"AttrsSub(a0,t0);AttrsSub(a1,t1);TableEq(t2,t0);TableEq(t3,t1);"   \
	"AttrsEq(a2,a0);AttrsEq(a3,a1);SchemaEq(s2,s0);SchemaEq(s3,s1)"

// MONSOON/dataset/rules/rules.els.reduced.txt:70, unchanged.
#define GPOPT_DSL_UNION_CORPUS_DEDUP_RULE                              \
	"Union(Proj*<a0 s0>(Input<t0>),Proj*<a1 s1>(Input<t1>))|"          \
	"Union(Proj*<a2 s2>(Input<t2>),Proj*<a3 s3>(Input<t3>))|"          \
	"AttrsSub(a0,t0);AttrsSub(a1,t1);TableEq(t2,t0);TableEq(t3,t1);"   \
	"AttrsEq(a2,a0);AttrsEq(a3,a1);SchemaEq(s2,s0);SchemaEq(s3,s1)"

static CDSLRule *
PdslruleParseLocal(CMemoryPool *mp, const CHAR *szDsl)
{
	CWStringDynamic strErr(mp);
	return CDSLRuleParser::PdslruleParse(mp, szDsl, "EQ", &strErr);
}

static CColRefArray *
PdrgpcrCopy(CMemoryPool *mp, CColRefArray *pdrgpcr)
{
	CColRefArray *pdrgpcrCopy = GPOS_NEW(mp) CColRefArray(mp);
	for (ULONG ul = 0; ul < pdrgpcr->Size(); ul++)
	{
		pdrgpcrCopy->Append((*pdrgpcr)[ul]);
	}
	return pdrgpcrCopy;
}

static BOOL
FOutputContains(CExpression *pexpr, CColRefArray *pdrgpcr)
{
	CColRefSet *pcrsOutput = pexpr->DeriveOutputColumns();
	for (ULONG ul = 0; ul < pdrgpcr->Size(); ul++)
	{
		if (!pcrsOutput->FMember((*pdrgpcr)[ul]))
		{
			return false;
		}
	}
	return true;
}

static CExpression *
PexprSetOp(CMemoryPool *mp, BOOL fDistinct, CExpression *pexprLeft,
			   CColRefArray *pdrgpcrLeft, CExpression *pexprRight,
			   CColRefArray *pdrgpcrRight)
{
	GPOS_ASSERT(pdrgpcrLeft->Size() == pdrgpcrRight->Size());
	CColRefArray *pdrgpcrOutput = PdrgpcrCopy(mp, pdrgpcrLeft);
	CColRef2dArray *pdrgpdrgpcrInput = GPOS_NEW(mp) CColRef2dArray(mp);
	pdrgpdrgpcrInput->Append(PdrgpcrCopy(mp, pdrgpcrLeft));
	pdrgpdrgpcrInput->Append(PdrgpcrCopy(mp, pdrgpcrRight));
	CExpressionArray *pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	pexprLeft->AddRef();
	pexprRight->AddRef();
	pdrgpexpr->Append(pexprLeft);
	pdrgpexpr->Append(pexprRight);
	COperator *pop = fDistinct
		? static_cast<COperator *>(GPOS_NEW(mp) CLogicalUnion(
			  mp, pdrgpcrOutput, pdrgpdrgpcrInput))
		: static_cast<COperator *>(GPOS_NEW(mp) CLogicalUnionAll(
			  mp, pdrgpcrOutput, pdrgpdrgpcrInput));
	return GPOS_NEW(mp) CExpression(mp, pop, pdrgpexpr);
}

static void
BuildTwoGetUnion(CDSLTestFixture &fix, BOOL fDistinct,
				 CExpression **ppLeft, CExpression **ppRight,
				 CExpression **ppUnion)
{
	CColRefArray *pdrgpcrLeft = nullptr;
	CColRefArray *pdrgpcrRight = nullptr;
	*ppLeft = fix.PexprLogicalGet("union_left", 2, &pdrgpcrLeft);
	*ppRight = fix.PexprLogicalGet("union_right", 2, &pdrgpcrRight);
	*ppUnion = PexprSetOp(fix.Pmp(), fDistinct, *ppLeft, pdrgpcrLeft,
						  *ppRight, pdrgpcrRight);
}

GPOS_RESULT
CDSLUnionTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CDSLUnionTest::EresUnittest_MatchAndDistinctGate),
		GPOS_UNITTEST_FUNC(
			CDSLUnionTest::EresUnittest_NarySetOpUsesAssociativeView),
		GPOS_UNITTEST_FUNC(
			CDSLUnionTest::EresUnittest_InstantiatePreservesColumnMaps),
		GPOS_UNITTEST_FUNC(
			CDSLUnionTest::EresUnittest_OutputBindingBuildsFullRowDedup),
		GPOS_UNITTEST_FUNC(CDSLUnionTest::EresUnittest_SwapsBranchesByConstraints),
		GPOS_UNITTEST_FUNC(
			CDSLUnionTest::EresUnittest_RejectsRemapAcrossOptimizerGbAgg),
		GPOS_UNITTEST_FUNC(CDSLUnionTest::EresUnittest_CorpusTwoProjects),
		GPOS_UNITTEST_FUNC(
			CDSLUnionTest::EresUnittest_CorpusNestedDistinctProjects),
		GPOS_UNITTEST_FUNC(
			CDSLUnionTest::EresUnittest_JoinDistributionBuildsFreshBranches),
		GPOS_UNITTEST_FUNC(
			CDSLUnionTest::EresUnittest_LeftJoinDistributionBuildsFreshBranches),
		GPOS_UNITTEST_FUNC(
			CDSLUnionTest::EresUnittest_JoinDistributionRejectsDistinctUnion),
	};
	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLUnionTest::EresUnittest_JoinDistributionRejectsDistinctUnion()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_JOIN_UNION_DISTRIBUTION_RULE);
	CColRefArray *pdrgpcrLeft0 = nullptr, *pdrgpcrLeft1 = nullptr,
		*pdrgpcrOther = nullptr;
	CExpression *pexprLeft0 =
		fix.PexprLogicalGet("distinct_dist_l0", 2, &pdrgpcrLeft0);
	CExpression *pexprLeft1 =
		fix.PexprLogicalGet("distinct_dist_l1", 2, &pdrgpcrLeft1);
	CExpression *pexprOther =
		fix.PexprLogicalGet("distinct_dist_r", 2, &pdrgpcrOther);
	CExpression *pexprUnion = PexprSetOp(
		mp, true, pexprLeft0, pdrgpcrLeft0, pexprLeft1, pdrgpcrLeft1);
	CExpression *pexprPred =
		fix.PexprEqPred((*pdrgpcrLeft0)[0], (*pdrgpcrOther)[0]);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprUnion, pexprOther, pexprPred);
	pexprPred->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	const BOOL fMatched = nullptr != prule &&
		matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprJoin, pmodel);

	pmodel->Release();
	pexprJoin->Release();
	pexprUnion->Release();
	pexprOther->Release();
	pexprLeft0->Release();
	pexprLeft1->Release();
	CRefCount::SafeRelease(prule);
	return fMatched ? GPOS_FAILED : GPOS_OK;
}

GPOS_RESULT
CDSLUnionTest::EresUnittest_LeftJoinDistributionBuildsFreshBranches()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp, GPOPT_DSL_LEFT_JOIN_UNION_DISTRIBUTION_RULE);
	CColRefArray *pdrgpcrLeft0 = nullptr, *pdrgpcrLeft1 = nullptr,
		*pdrgpcrOther = nullptr;
	CExpression *pexprLeft0 =
		fix.PexprLogicalGet("loj_dist_l0", 2, &pdrgpcrLeft0);
	CExpression *pexprLeft1 =
		fix.PexprLogicalGet("loj_dist_l1", 2, &pdrgpcrLeft1);
	CExpression *pexprOther =
		fix.PexprLogicalGet("loj_dist_r", 2, &pdrgpcrOther);
	CExpression *pexprUnion = PexprSetOp(
		mp, false, pexprLeft0, pdrgpcrLeft0, pexprLeft1, pdrgpcrLeft1);
	CExpression *pexprPred =
		fix.PexprEqPred((*pdrgpcrLeft0)[0], (*pdrgpcrOther)[0]);
	CExpression *pexprJoin =
		fix.PexprLogicalLeftOuterJoin(pexprUnion, pexprOther, pexprPred);
	pexprPred->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_FAILED;
	if (nullptr != prule &&
		matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprJoin, pmodel) &&
		checker.FCheck(prule, pmodel))
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr != pexprTarget &&
			COperator::EopLogicalUnionAll == pexprTarget->Pop()->Eopid() &&
			2 == pexprTarget->Arity() &&
			COperator::EopLogicalLeftOuterJoin ==
				(*pexprTarget)[0]->Pop()->Eopid() &&
			COperator::EopLogicalLeftOuterJoin ==
				(*pexprTarget)[1]->Pop()->Eopid())
		{
			CExpression *pexprJoin0 = (*pexprTarget)[0];
			CExpression *pexprJoin1 = (*pexprTarget)[1];
			CLogicalSetOp *popTarget =
				CLogicalSetOp::PopConvert(pexprTarget->Pop());
			CColRefArray *pdrgpcrSourceOutput =
				pexprJoin->DeriveOutputColumns()->Pdrgpcr(mp);
			const BOOL fPredicatesBound =
				pexprJoin0->DeriveOutputColumns()->ContainsAll(
					(*pexprJoin0)[2]->DeriveUsedColumns()) &&
				pexprJoin1->DeriveOutputColumns()->ContainsAll(
					(*pexprJoin1)[2]->DeriveUsedColumns());
			const BOOL fFreshRepeatedInput =
				!(*pexprJoin1)[1]->DeriveOutputColumns()->FMember(
					(*pdrgpcrOther)[0]) &&
				!(*pexprJoin1)[1]->DeriveOutputColumns()->FMember(
					(*pdrgpcrOther)[1]);
			eres = CColRef::Equals(popTarget->PdrgpcrOutput(),
								 pdrgpcrSourceOutput) &&
					   fPredicatesBound && fFreshRepeatedInput
				? GPOS_OK
				: GPOS_FAILED;
			pdrgpcrSourceOutput->Release();
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprJoin->Release();
	pexprUnion->Release();
	pexprOther->Release();
	pexprLeft0->Release();
	pexprLeft1->Release();
	CRefCount::SafeRelease(prule);
	return eres;
}

GPOS_RESULT
CDSLUnionTest::EresUnittest_JoinDistributionBuildsFreshBranches()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_JOIN_UNION_DISTRIBUTION_RULE);
	CColRefArray *pdrgpcrLeft0 = nullptr, *pdrgpcrLeft1 = nullptr,
		*pdrgpcrOther = nullptr;
	CExpression *pexprLeft0 =
		fix.PexprLogicalGet("dist_l0", 2, &pdrgpcrLeft0);
	CExpression *pexprLeft1 =
		fix.PexprLogicalGet("dist_l1", 2, &pdrgpcrLeft1);
	CExpression *pexprOther =
		fix.PexprLogicalGet("dist_r", 2, &pdrgpcrOther);
	CExpression *pexprUnion = PexprSetOp(
		mp, false, pexprLeft0, pdrgpcrLeft0, pexprLeft1, pdrgpcrLeft1);
	CExpression *pexprPred =
		fix.PexprEqPred((*pdrgpcrLeft0)[0], (*pdrgpcrOther)[0]);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprUnion, pexprOther, pexprPred);
	pexprPred->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_FAILED;
	GPOS_UNITTEST_ASSERT(nullptr != prule);
	GPOS_UNITTEST_ASSERT(
		matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprJoin, pmodel));
	GPOS_UNITTEST_ASSERT(checker.FCheck(prule, pmodel));
	if (nullptr != prule)
	{
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		GPOS_UNITTEST_ASSERT(nullptr != pexprTarget);
		if (nullptr != pexprTarget &&
			COperator::EopLogicalUnionAll == pexprTarget->Pop()->Eopid() &&
			2 == pexprTarget->Arity() &&
			COperator::EopLogicalInnerJoin ==
				(*pexprTarget)[0]->Pop()->Eopid() &&
			COperator::EopLogicalInnerJoin ==
				(*pexprTarget)[1]->Pop()->Eopid())
		{
			CExpression *pexprJoin0 = (*pexprTarget)[0];
			CExpression *pexprJoin1 = (*pexprTarget)[1];
			CLogicalSetOp *popTarget =
				CLogicalSetOp::PopConvert(pexprTarget->Pop());
			CColRefArray *pdrgpcrSourceOutput =
				pexprJoin->DeriveOutputColumns()->Pdrgpcr(mp);
			const BOOL fPredicatesBound =
				pexprJoin0->DeriveOutputColumns()->ContainsAll(
					(*pexprJoin0)[2]->DeriveUsedColumns()) &&
				pexprJoin1->DeriveOutputColumns()->ContainsAll(
					(*pexprJoin1)[2]->DeriveUsedColumns());
			const BOOL fFreshRepeatedInput =
				!(*pexprJoin1)[1]->DeriveOutputColumns()->FMember(
					(*pdrgpcrOther)[0]) &&
				!(*pexprJoin1)[1]->DeriveOutputColumns()->FMember(
					(*pdrgpcrOther)[1]);
			eres = CColRef::Equals(popTarget->PdrgpcrOutput(),
								 pdrgpcrSourceOutput) &&
				   fPredicatesBound && fFreshRepeatedInput
				? GPOS_OK
				: GPOS_FAILED;
			pdrgpcrSourceOutput->Release();
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprJoin->Release();
	pexprUnion->Release();
	pexprOther->Release();
	pexprLeft0->Release();
	pexprLeft1->Release();
	CRefCount::SafeRelease(prule);
	return eres;
}

GPOS_RESULT
CDSLUnionTest::EresUnittest_MatchAndDistinctGate()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *pruleAll =
		PdslruleParseLocal(mp, GPOPT_DSL_UNION_IDENTITY_RULE);
	CDSLRule *pruleDistinct =
		PdslruleParseLocal(mp, GPOPT_DSL_UNION_DISTINCT_RULE);
	CExpression *pexprLeft = nullptr, *pexprRight = nullptr, *pexprAll = nullptr;
	BuildTwoGetUnion(fix, false, &pexprLeft, &pexprRight, &pexprAll);
	CDSLMatcher matcher(mp);
	CDSLModel *pmodelAll = GPOS_NEW(mp) CDSLModel(mp);
	CDSLModel *pmodelWrong = GPOS_NEW(mp) CDSLModel(mp);
	GPOS_RESULT eres =
		(nullptr != pruleAll && nullptr != pruleDistinct &&
		 matcher.FMatch(pruleAll->PfragSrc()->PopRoot(), pexprAll, pmodelAll) &&
		 1 == pmodelAll->PdrgpexprUnionBindings()->Size() &&
		 !matcher.FMatch(pruleDistinct->PfragSrc()->PopRoot(), pexprAll,
						 pmodelWrong))
		? GPOS_OK
		: GPOS_FAILED;
	pmodelWrong->Release();
	pmodelAll->Release();
	pexprAll->Release();
	pexprLeft->Release();
	pexprRight->Release();
	CRefCount::SafeRelease(pruleAll);
	CRefCount::SafeRelease(pruleDistinct);
	return eres;
}

GPOS_RESULT
CDSLUnionTest::EresUnittest_NarySetOpUsesAssociativeView()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_UNION_OUTPUT_DEDUP_RULE);
	CColRefArray *pdrgpcr0 = nullptr, *pdrgpcr1 = nullptr, *pdrgpcr2 = nullptr;
	CExpression *pexpr0 = fix.PexprLogicalGet("u0", 1, &pdrgpcr0);
	CExpression *pexpr1 = fix.PexprLogicalGet("u1", 1, &pdrgpcr1);
	CExpression *pexpr2 = fix.PexprLogicalGet("u2", 1, &pdrgpcr2);
	CColRef2dArray *pdrgpdrgpcr = GPOS_NEW(mp) CColRef2dArray(mp);
	pdrgpdrgpcr->Append(PdrgpcrCopy(mp, pdrgpcr0));
	pdrgpdrgpcr->Append(PdrgpcrCopy(mp, pdrgpcr1));
	pdrgpdrgpcr->Append(PdrgpcrCopy(mp, pdrgpcr2));
	CExpressionArray *pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	pexpr0->AddRef(); pdrgpexpr->Append(pexpr0);
	pexpr1->AddRef(); pdrgpexpr->Append(pexpr1);
	pexpr2->AddRef(); pdrgpexpr->Append(pexpr2);
	CExpression *pexprNary = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalUnion(
			mp, PdrgpcrCopy(mp, pdrgpcr0), pdrgpdrgpcr), pdrgpexpr);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_FAILED;
	if (nullptr != prule &&
		matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprNary, pmodel) &&
		checker.FCheck(prule, pmodel))
	{
		CDSLInstantiator inst(mp);
		pexprTarget = inst.PexprInstantiate(prule, pmodel);
		if (nullptr != pexprTarget &&
			COperator::EopLogicalGbAgg == pexprTarget->Pop()->Eopid() &&
			COperator::EopLogicalUnionAll ==
				(*pexprTarget)[0]->Pop()->Eopid() &&
			3 == (*pexprTarget)[0]->Arity())
		{
			CLogicalSetOp *popSource =
				CLogicalSetOp::PopConvert(pexprNary->Pop());
			CLogicalSetOp *popTarget = CLogicalSetOp::PopConvert(
				(*pexprTarget)[0]->Pop());
			eres = CColRef::Equals(popSource->PdrgpcrOutput(),
								 popTarget->PdrgpcrOutput()) &&
				CColRef::Equals((*popSource->PdrgpdrgpcrInput())[0],
							   (*popTarget->PdrgpdrgpcrInput())[0]) &&
				CColRef::Equals((*popSource->PdrgpdrgpcrInput())[1],
							   (*popTarget->PdrgpdrgpcrInput())[1]) &&
				CColRef::Equals((*popSource->PdrgpdrgpcrInput())[2],
							   (*popTarget->PdrgpdrgpcrInput())[2])
				? GPOS_OK
				: GPOS_FAILED;
		}
	}
	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprNary->Release();
	pexpr0->Release(); pexpr1->Release(); pexpr2->Release();
	CRefCount::SafeRelease(prule);
	return eres;
}

static GPOS_RESULT
EresInstantiateSimpleUnion(BOOL fSwap)
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp, fSwap ? GPOPT_DSL_UNION_SWAP_RULE
				  : GPOPT_DSL_UNION_IDENTITY_RULE);
	CExpression *pexprLeft = nullptr, *pexprRight = nullptr, *pexprSource = nullptr;
	BuildTwoGetUnion(fix, false, &pexprLeft, &pexprRight, &pexprSource);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_FAILED;
	if (nullptr != prule &&
		matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource, pmodel))
	{
		CDSLInstantiator inst(mp);
		pexprTarget = inst.PexprInstantiate(prule, pmodel);
		if (nullptr != pexprTarget &&
			COperator::EopLogicalUnionAll == pexprTarget->Pop()->Eopid())
		{
			CLogicalSetOp *popSource =
				CLogicalSetOp::PopConvert(pexprSource->Pop());
			CLogicalSetOp *popTarget =
				CLogicalSetOp::PopConvert(pexprTarget->Pop());
			eres = popTarget->PdrgpcrOutput()->Equals(
					   popSource->PdrgpcrOutput()) &&
				   (*popTarget->PdrgpdrgpcrInput())[0]->Equals(
					   (*popSource->PdrgpdrgpcrInput())[0]) &&
				   (*popTarget->PdrgpdrgpcrInput())[1]->Equals(
					   (*popSource->PdrgpdrgpcrInput())[1]) &&
				   FOutputContains(
					   (*pexprTarget)[0],
					   (*popTarget->PdrgpdrgpcrInput())[0]) &&
				   FOutputContains(
					   (*pexprTarget)[1],
					   (*popTarget->PdrgpdrgpcrInput())[1])
				? GPOS_OK : GPOS_FAILED;
			if (fSwap && GPOS_OK == eres)
			{
				eres = CLogicalGet::PopConvert((*pexprTarget)[0]->Pop())
						   ->Ptabdesc() ==
					   CLogicalGet::PopConvert(pexprRight->Pop())->Ptabdesc() &&
					   CLogicalGet::PopConvert((*pexprTarget)[1]->Pop())
						   ->Ptabdesc() ==
					   CLogicalGet::PopConvert(pexprLeft->Pop())->Ptabdesc()
					? GPOS_OK
					: GPOS_FAILED;
			}
		}
	}
	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprSource->Release(); pexprLeft->Release(); pexprRight->Release();
	CRefCount::SafeRelease(prule);
	return eres;
}

GPOS_RESULT
CDSLUnionTest::EresUnittest_InstantiatePreservesColumnMaps()
{
	return EresInstantiateSimpleUnion(false);
}

GPOS_RESULT
CDSLUnionTest::EresUnittest_OutputBindingBuildsFullRowDedup()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_UNION_OUTPUT_DEDUP_RULE);
	CExpression *pexprLeft = nullptr, *pexprRight = nullptr,
				*pexprSource = nullptr;
	BuildTwoGetUnion(fix, true, &pexprLeft, &pexprRight, &pexprSource);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_FAILED;
	if (nullptr != prule &&
		matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource, pmodel) &&
		checker.FCheck(prule, pmodel))
	{
		CDSLSymbolArray *pdrgpsymSource =
			prule->PfragSrc()->PopRoot()->Pdrgpsym();
		CLogicalSetOp *popSource =
			CLogicalSetOp::PopConvert(pexprSource->Pop());
		const BOOL fBound = 2 == pdrgpsymSource->Size() &&
			pmodel->PdrgpcrAttrs((*pdrgpsymSource)[0]) ==
				popSource->PdrgpcrOutput() &&
			pmodel->PdrgpcrSchema((*pdrgpsymSource)[1]) ==
				popSource->PdrgpcrOutput();

		CDSLInstantiator inst(mp);
		pexprTarget = inst.PexprInstantiate(prule, pmodel);
		if (fBound && nullptr != pexprTarget &&
			COperator::EopLogicalGbAgg == pexprTarget->Pop()->Eopid() &&
			COperator::EopLogicalUnionAll == (*pexprTarget)[0]->Pop()->Eopid())
		{
			CLogicalGbAgg *popAgg =
				CLogicalGbAgg::PopConvert(pexprTarget->Pop());
			CLogicalSetOp *popUnionAll =
				CLogicalSetOp::PopConvert((*pexprTarget)[0]->Pop());
			eres = popAgg->Pdrgpcr()->Equals(popSource->PdrgpcrOutput()) &&
				popUnionAll->PdrgpcrOutput()->Equals(
					popSource->PdrgpcrOutput())
			? GPOS_OK
			: GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprSource->Release();
	pexprLeft->Release();
	pexprRight->Release();
	CRefCount::SafeRelease(prule);
	return eres;
}

GPOS_RESULT
CDSLUnionTest::EresUnittest_SwapsBranchesByConstraints()
{
	return EresInstantiateSimpleUnion(true);
}

GPOS_RESULT
CDSLUnionTest::EresUnittest_RejectsRemapAcrossOptimizerGbAgg()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_UNION_SWAP_RULE);
	CColRefArray *pdrgpcr0 = nullptr, *pdrgpcr1 = nullptr;
	CExpression *pexprGet0 = fix.PexprLogicalGet("ugb0", 1, &pdrgpcr0);
	CExpression *pexprGet1 = fix.PexprLogicalGet("ugb1", 1, &pdrgpcr1);

	CExpression *rgpexprGet[2] = {pexprGet0, pexprGet1};
	CColRefArray *rgpdrgpcr[2] = {pdrgpcr0, pdrgpcr1};
	CExpression *rgpexprAgg[2] = {nullptr, nullptr};
	for (ULONG ul = 0; ul < 2; ul++)
	{
		rgpdrgpcr[ul]->AddRef();
		rgpdrgpcr[ul]->AddRef();
		rgpexprGet[ul]->AddRef();
		rgpexprAgg[ul] = GPOS_NEW(mp) CExpression(
			mp,
			GPOS_NEW(mp) CLogicalGbAgg(
				mp, rgpdrgpcr[ul], rgpdrgpcr[ul],
				COperator::EgbaggtypeGlobal),
			rgpexprGet[ul],
			GPOS_NEW(mp) CExpression(
				mp, GPOS_NEW(mp) CScalarProjectList(mp),
				GPOS_NEW(mp) CExpressionArray(mp)));
	}

	CExpression *pexprSource = PexprSetOp(
		mp, false, rgpexprAgg[0], pdrgpcr0, rgpexprAgg[1], pdrgpcr1);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_FAILED;
	if (nullptr != prule &&
		matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource, pmodel))
	{
		CDSLInstantiator inst(mp);
		pexprTarget = inst.PexprInstantiate(prule, pmodel);
		eres = nullptr == pexprTarget ? GPOS_OK : GPOS_FAILED;
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprSource->Release();
	rgpexprAgg[0]->Release();
	rgpexprAgg[1]->Release();
	pexprGet0->Release();
	pexprGet1->Release();
	CRefCount::SafeRelease(prule);
	return eres;
}

static GPOS_RESULT
EresCorpusProjectRule(BOOL fDistinctProjects)
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp, fDistinctProjects ? GPOPT_DSL_UNION_CORPUS_DEDUP_RULE
						 : GPOPT_DSL_UNION_CORPUS_PROJ_RULE);
	CColRefArray *pdrgpcr0 = nullptr, *pdrgpcr1 = nullptr;
	CExpression *pexprGet0 = fix.PexprLogicalGet("cp0", 1, &pdrgpcr0);
	CExpression *pexprGet1 = fix.PexprLogicalGet("cp1", 1, &pdrgpcr1);
	CExpression *pexprChild0 = fDistinctProjects
		? fix.PexprLogicalGbAgg(pexprGet0, pdrgpcr0)
		: fix.PexprLogicalProject(pexprGet0, pdrgpcr0);
	CExpression *pexprChild1 = fDistinctProjects
		? fix.PexprLogicalGbAgg(pexprGet1, pdrgpcr1)
		: fix.PexprLogicalProject(pexprGet1, pdrgpcr1);
	CExpression *pexprSource = PexprSetOp(
		mp, false, pexprChild0, pdrgpcr0, pexprChild1, pdrgpcr1);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_FAILED;
	if (nullptr != prule &&
		matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource, pmodel) &&
		checker.FCheck(prule, pmodel))
	{
		CDSLInstantiator inst(mp);
		pexprTarget = inst.PexprInstantiate(prule, pmodel);
		COperator::EOperatorId eopidChild = fDistinctProjects
			? COperator::EopLogicalGbAgg : COperator::EopLogicalProject;
		eres = nullptr != pexprTarget &&
			COperator::EopLogicalUnionAll == pexprTarget->Pop()->Eopid() &&
			eopidChild == (*pexprTarget)[0]->Pop()->Eopid() &&
			eopidChild == (*pexprTarget)[1]->Pop()->Eopid()
			? GPOS_OK : GPOS_FAILED;
		if (GPOS_OK == eres && !fDistinctProjects)
		{
			// The two independently matched project lists must not collapse to the
			// last one recorded in the model.
			eres = (*(*pexprTarget)[0])[1] == (*pexprChild0)[1] &&
					(*(*pexprTarget)[1])[1] == (*pexprChild1)[1]
				? GPOS_OK : GPOS_FAILED;
		}
	}
	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprSource->Release();
	pexprChild0->Release(); pexprChild1->Release();
	pexprGet0->Release(); pexprGet1->Release();
	CRefCount::SafeRelease(prule);
	return eres;
}

GPOS_RESULT
CDSLUnionTest::EresUnittest_CorpusTwoProjects()
{
	return EresCorpusProjectRule(false);
}

GPOS_RESULT
CDSLUnionTest::EresUnittest_CorpusNestedDistinctProjects()
{
	return EresCorpusProjectRule(true);
}
