//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLExistsTest.cpp
//
//	@doc:
//		Three-stage test using an unmodified real rule from
//		MONSOON/dataset/rules/rules.els.reduced.txt (line 486).
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLExistsTest.h"

#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CLogicalLeftSemiApply.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

#define GPOPT_DSL_CORPUS_EXISTS_AGG_PROJ_RULE                              \
	"Exists(Agg<a0 a1 f0 s0 p0>(Input<t0>),Proj<a2 s1>(Input<t1>))|"     \
	"Exists(Agg<a3 a4 f1 s2 p1>(Input<t2>),Proj<a5 s3>(Input<t3>))|"     \
	"AttrsSub(a0,t0);AttrsSub(a1,t0);AttrsSub(a2,t1);"                    \
	"TableEq(t2,t0);TableEq(t3,t1);AttrsEq(a3,a0);AttrsEq(a4,a1);"       \
	"AttrsEq(a5,a2);PredicateEq(p1,p0);SchemaEq(s2,s0);"                 \
	"SchemaEq(s3,s1);FuncEq(f1,f0)"

GPOS_RESULT
CDSLExistsTest::EresUnittest()
{
	CUnittest rgut[] = {GPOS_UNITTEST_FUNC(
		CDSLExistsTest::EresUnittest_CorpusAggProjRoundTrip)};
	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLExistsTest::EresUnittest_CorpusAggProjRoundTrip()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	// Outer child: genuine Global GbAgg, group c0 and MAX(c1).
	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuterGet =
		fix.PexprLogicalGet("exists_outer", 2, &pdrgpcrOuter);
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrOuter)[0]);
	CColRef *pcrMax = fix.PcrCreateInt4("exists_max");
	CExpression *pexprAgg = fix.PexprLogicalGbAgg(
		pexprOuterGet, pdrgpcrGroup, pcrMax, (*pdrgpcrOuter)[1]);
	pdrgpcrGroup->Release();

	// Inner child: Proj, wrapped in the exact LIMIT 1 inserted by ORCA for an
	// uncorrelated EXISTS.
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprInnerGet =
		fix.PexprLogicalGet("exists_inner", 2, &pdrgpcrInner);
	CColRefArray *pdrgpcrProjected = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrProjected->Append((*pdrgpcrInner)[0]);
	CExpression *pexprProject =
		fix.PexprLogicalProject(pexprInnerGet, pdrgpcrProjected);
	pdrgpcrProjected->Release();
	CColRef *pcrExistsCheck =
		pexprProject->DeriveOutputColumns()->PcrFirst();
	CExpression *pexprLimit = CUtils::PexprLimit(mp, pexprProject, 0, 1);
	CExpression *pexprSource =
		CUtils::PexprLogicalApply<CLogicalLeftSemiApply>(
			mp, pexprAgg, pexprLimit, pcrExistsCheck,
			COperator::EopScalarSubqueryExists);

	CWStringDynamic strErr(mp);
	CDSLRule *prule = CDSLRuleParser::PdslruleParse(
		mp, GPOPT_DSL_CORPUS_EXISTS_AGG_PROJ_RULE, "EQ", &strErr);
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(COperator::EopLogicalLeftSemiApply ==
				prule->EopidSrcRoot());

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
							   pmodel));
	CDSLConstraintChecker checker(mp);
	GPOS_ASSERT(checker.FCheck(prule, pmodel));

	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget =
		instantiator.PexprInstantiate(prule, pmodel);
	GPOS_ASSERT(nullptr != pexprTarget);
	GPOS_ASSERT(COperator::EopLogicalLeftSemiApply ==
				pexprTarget->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalGbAgg == (*pexprTarget)[0]->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalLimit == (*pexprTarget)[1]->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalProject ==
				(*(*pexprTarget)[1])[0]->Pop()->Eopid());
	GPOS_ASSERT(CUtils::FScalarConstTrue((*pexprTarget)[2]));
	GPOS_ASSERT(COperator::EopScalarSubqueryExists ==
				dynamic_cast<CLogicalApply *>(pexprTarget->Pop())
					->EopidOriginSubq());
	GPOS_ASSERT(pexprSource->DeriveOutputColumns()->Equals(
		pexprTarget->DeriveOutputColumns()));

	pexprTarget->Release();
	pmodel->Release();
	prule->Release();
	pexprSource->Release();
	pexprOuterGet->Release();
	pexprInnerGet->Release();
	return GPOS_OK;
}

// EOF
