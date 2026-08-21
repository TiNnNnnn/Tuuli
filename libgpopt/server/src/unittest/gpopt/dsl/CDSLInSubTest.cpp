//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	Uses MONSOON/dataset/rules/fewshot_curated.txt:4 verbatim.
//--------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLInSubTest.h"

#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CLogicalLeftSemiApplyIn.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CScalarSubqueryAny.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

#define GPOPT_DSL_CORPUS_INSUB_ELIM_RULE                                  \
	"InSubFilter<a1>(Input<t0>,Proj<a0 s0>(Input<t1>))|Input<t2>|"      \
	"TableEq(t0,t1);AttrsEq(a0,a1);AttrsSub(a0,t1);AttrsSub(a1,t0);"    \
	"NotNull(t1,a0);NotNull(t0,a1);TableEq(t2,t0)"

#define GPOPT_DSL_INSUB_IDENTITY_RULE                                     \
	"InSubFilter<a0>(Input<t0>,Input<t1>)|"                              \
	"InSubFilter<a1>(Input<t2>,Input<t3>)|"                              \
	"TableEq(t2,t0);TableEq(t3,t1);AttrsEq(a1,a0)"

#define GPOPT_DSL_REPEATED_IN_ELIM_RULE                                  \
	"InSubFilter<a1>(InSubFilter<a0>(Input<t0>,Input<t1>),Input<t2>)|"  \
	"InSubFilter<a2>(Input<t3>,Input<t4>)|"                             \
	"TableEq(t1,t2);AttrsEq(a0,a1);AttrsSub(a0,t0);AttrsSub(a1,t0);"   \
	"TableEq(t3,t0);TableEq(t4,t1);AttrsEq(a2,a0)"

namespace
{
CExpression *
PexprScalarAny(CMemoryPool *mp, CDSLTestFixture &fix,
			   CExpression *pexprInner, CColRef *pcrOuter, CColRef *pcrInner)
{
	CExpression *pexprEq = fix.PexprEqPred(pcrOuter, pcrInner);
	CScalarCmp *popEq = CScalarCmp::PopConvert(pexprEq->Pop());
	IMDId *pmdidEq = popEq->MdIdOp();
	pmdidEq->AddRef();
	CWStringConst *pstrEq =
		GPOS_NEW(mp) CWStringConst(mp, popEq->Pstr()->GetBuffer());
	pexprEq->Release();

	CExpression *pexprOuterScalar = CUtils::PexprScalarIdent(mp, pcrOuter);
	return GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarSubqueryAny(mp, pmdidEq, pstrEq, pcrInner),
		pexprInner, pexprOuterScalar);
}

CDSLRule *
PruleParse(CMemoryPool *mp, const CHAR *szRule)
{
	CWStringDynamic strErr(mp);
	return CDSLRuleParser::PdslruleParse(mp, szRule, "EQ", &strErr);
}
}  // namespace

GPOS_RESULT
CDSLInSubTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(
			CDSLInSubTest::EresUnittest_PreApplyCorpusElimination),
		GPOS_UNITTEST_FUNC(
			CDSLInSubTest::EresUnittest_PostApplyCorpusElimination),
		GPOS_UNITTEST_FUNC(
			CDSLInSubTest::EresUnittest_PreApplyRepeatedInElimination),
		GPOS_UNITTEST_FUNC(
			CDSLInSubTest::EresUnittest_PostApplyRepeatedInElimination),
		GPOS_UNITTEST_FUNC(
			CDSLInSubTest::EresUnittest_RejectsDifferentTable),
		GPOS_UNITTEST_FUNC(CDSLInSubTest::EresUnittest_PostApplyIdentity)};
	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLInSubTest::EresUnittest_PostApplyCorpusElimination()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CTableDescriptor *ptabdesc =
		fix.PtabdescCreate("insub_apply_same", 2, 0, false);
	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuterGet =
		fix.PexprLogicalGet(ptabdesc, "insub_apply_same_outer", &pdrgpcrOuter);
	CExpression *pexprResidual = fix.PexprPredAtom((*pdrgpcrOuter)[1]);
	CExpression *pexprOuter =
		fix.PexprLogicalSelect(pexprOuterGet, pexprResidual);
	pexprOuterGet->Release();
	pexprResidual->Release();
	ptabdesc->AddRef();
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprInner =
		fix.PexprLogicalGet(ptabdesc, "insub_apply_same_inner", &pdrgpcrInner);
	CExpression *pexprPred =
		fix.PexprEqPred((*pdrgpcrOuter)[0], (*pdrgpcrInner)[0]);
	CExpression *pexprSource =
		CUtils::PexprLogicalApply<CLogicalLeftSemiApplyIn>(
			mp, pexprOuter, pexprInner, (*pdrgpcrInner)[0],
			COperator::EopScalarSubqueryAny, pexprPred);

	CDSLRule *prule = PruleParse(mp, GPOPT_DSL_CORPUS_INSUB_ELIM_RULE);
	GPOS_ASSERT(nullptr != prule);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
							   pmodel));
	CDSLConstraintChecker checker(mp);
	GPOS_ASSERT(checker.FCheck(prule, pmodel));
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
	GPOS_ASSERT(nullptr != pexprTarget);
	GPOS_ASSERT(COperator::EopLogicalSelect == pexprTarget->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalGet == (*pexprTarget)[0]->Pop()->Eopid());

	pexprTarget->Release();
	pmodel->Release();
	prule->Release();
	pexprSource->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLInSubTest::EresUnittest_PostApplyRepeatedInElimination()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("repeat_apply_outer", 2, &pdrgpcrOuter, 0);
	CTableDescriptor *ptabdescInner =
		fix.PtabdescCreate("repeat_apply_inner", 2, 0, false);
	CColRefArray *pdrgpcrInner0 = nullptr;
	CExpression *pexprInner0 = fix.PexprLogicalGet(
		ptabdescInner, "repeat_apply_inner_0", &pdrgpcrInner0);
	ptabdescInner->AddRef();
	CColRefArray *pdrgpcrInner1 = nullptr;
	CExpression *pexprInner1 = fix.PexprLogicalGet(
		ptabdescInner, "repeat_apply_inner_1", &pdrgpcrInner1);
	CExpression *pexprPred0 =
		fix.PexprEqPred((*pdrgpcrOuter)[0], (*pdrgpcrInner0)[0]);
	CExpression *pexprApply0 =
		CUtils::PexprLogicalApply<CLogicalLeftSemiApplyIn>(
			mp, pexprOuter, pexprInner0, (*pdrgpcrInner0)[0],
			COperator::EopScalarSubqueryAny, pexprPred0);
	CExpression *pexprPred1 =
		fix.PexprEqPred((*pdrgpcrOuter)[0], (*pdrgpcrInner1)[0]);
	CExpression *pexprSource =
		CUtils::PexprLogicalApply<CLogicalLeftSemiApplyIn>(
			mp, pexprApply0, pexprInner1, (*pdrgpcrInner1)[0],
			COperator::EopScalarSubqueryAny, pexprPred1);

	CDSLRule *prule = PruleParse(mp, GPOPT_DSL_REPEATED_IN_ELIM_RULE);
	GPOS_ASSERT(nullptr != prule);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
							   pmodel));
	CDSLConstraintChecker checker(mp);
	GPOS_ASSERT(checker.FCheck(prule, pmodel));
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
	GPOS_ASSERT(nullptr != pexprTarget);
	GPOS_ASSERT(COperator::EopLogicalLeftSemiApplyIn ==
				pexprTarget->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalGet == (*pexprTarget)[0]->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopScalarCmp == (*pexprTarget)[2]->Pop()->Eopid());

	pexprTarget->Release();
	pmodel->Release();
	prule->Release();
	pexprSource->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLInSubTest::EresUnittest_PreApplyRepeatedInElimination()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("repeat_outer", 2, &pdrgpcrOuter, 0);
	CTableDescriptor *ptabdescInner =
		fix.PtabdescCreate("repeat_inner", 2, 0, false);
	CColRefArray *pdrgpcrInner0 = nullptr;
	CExpression *pexprInner0 =
		fix.PexprLogicalGet(ptabdescInner, "repeat_inner_0", &pdrgpcrInner0);
	ptabdescInner->AddRef();
	CColRefArray *pdrgpcrInner1 = nullptr;
	CExpression *pexprInner1 =
		fix.PexprLogicalGet(ptabdescInner, "repeat_inner_1", &pdrgpcrInner1);

	pexprInner0->AddRef();
	CExpression *pexprAny0 = PexprScalarAny(
		mp, fix, pexprInner0, (*pdrgpcrOuter)[0], (*pdrgpcrInner0)[0]);
	pexprInner1->AddRef();
	CExpression *pexprAny1 = PexprScalarAny(
		mp, fix, pexprInner1, (*pdrgpcrOuter)[0], (*pdrgpcrInner1)[0]);
	CExpressionArray *pdrgpexprConj = GPOS_NEW(mp) CExpressionArray(mp);
	pexprAny0->AddRef();
	pdrgpexprConj->Append(pexprAny0);
	pexprAny1->AddRef();
	pdrgpexprConj->Append(pexprAny1);
	CExpression *pexprWhere =
		CPredicateUtils::PexprConjunction(mp, pdrgpexprConj);
	CExpression *pexprSource = fix.PexprLogicalSelect(pexprOuter, pexprWhere);
	pexprOuter->Release();
	pexprWhere->Release();
	pexprAny0->Release();
	pexprAny1->Release();

	CDSLRule *prule = PruleParse(mp, GPOPT_DSL_REPEATED_IN_ELIM_RULE);
	GPOS_ASSERT(nullptr != prule);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
							   pmodel));
	CDSLConstraintChecker checker(mp);
	GPOS_ASSERT(checker.FCheck(prule, pmodel));
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
	GPOS_ASSERT(nullptr != pexprTarget);
	GPOS_ASSERT(COperator::EopLogicalLeftSemiApplyIn ==
				pexprTarget->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopScalarCmp == (*pexprTarget)[2]->Pop()->Eopid());
	GPOS_ASSERT(pexprSource->DeriveOutputColumns()->Equals(
		pexprTarget->DeriveOutputColumns()));

	pexprTarget->Release();
	pmodel->Release();
	prule->Release();
	pexprSource->Release();
	pexprInner0->Release();
	pexprInner1->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLInSubTest::EresUnittest_PreApplyCorpusElimination()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CTableDescriptor *ptabdesc =
		fix.PtabdescCreate("insub_same", 2, 0 /*key*/, false /*nullable*/);
	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet(ptabdesc, "insub_outer", &pdrgpcrOuter);
	ptabdesc->AddRef();
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprInnerGet =
		fix.PexprLogicalGet(ptabdesc, "insub_inner", &pdrgpcrInner);
	// The live PostgreSQL->ORCA tree folds the rule's pass-through Proj away.
	// Keep a local reference while ScalarSubqueryAny owns the expression.
	pexprInnerGet->AddRef();
	CExpression *pexprAny = PexprScalarAny(
		mp, fix, pexprInnerGet, (*pdrgpcrOuter)[0], (*pdrgpcrInner)[0]);
	// A sibling predicate is deliberately present: eliminating the IN must not
	// eliminate the rest of the WHERE clause with it.
	CExpressionArray *pdrgpexprConj = GPOS_NEW(mp) CExpressionArray(mp);
	pexprAny->AddRef();
	pdrgpexprConj->Append(pexprAny);
	pdrgpexprConj->Append(
		fix.PexprEqPred((*pdrgpcrOuter)[1], (*pdrgpcrOuter)[0]));
	CExpression *pexprWhere =
		CPredicateUtils::PexprConjunction(mp, pdrgpexprConj);
	CExpression *pexprSource = fix.PexprLogicalSelect(pexprOuter, pexprWhere);
	pexprOuter->Release();
	pexprAny->Release();
	pexprWhere->Release();

	CDSLRule *prule = PruleParse(mp, GPOPT_DSL_CORPUS_INSUB_ELIM_RULE);
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(COperator::EopLogicalLeftSemiApplyIn ==
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
	GPOS_ASSERT(COperator::EopLogicalSelect == pexprTarget->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopScalarCmp == (*pexprTarget)[1]->Pop()->Eopid());
	GPOS_ASSERT(pexprSource->DeriveOutputColumns()->Equals(
		pexprTarget->DeriveOutputColumns()));

	pexprTarget->Release();
	pmodel->Release();
	prule->Release();
	pexprSource->Release();
	pexprInnerGet->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLInSubTest::EresUnittest_RejectsDifferentTable()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("insub_outer_table", 2, &pdrgpcrOuter, 0);
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprInnerGet =
		fix.PexprLogicalGet("insub_inner_table", 2, &pdrgpcrInner, 0);
	CColRefArray *pdrgpcrProjected = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrProjected->Append((*pdrgpcrInner)[0]);
	CExpression *pexprProject =
		fix.PexprLogicalProject(pexprInnerGet, pdrgpcrProjected);
	pdrgpcrProjected->Release();
	CExpression *pexprAny = PexprScalarAny(
		mp, fix, pexprProject, (*pdrgpcrOuter)[0], (*pdrgpcrInner)[0]);
	CExpression *pexprSource = fix.PexprLogicalSelect(pexprOuter, pexprAny);
	pexprOuter->Release();
	pexprAny->Release();

	CDSLRule *prule = PruleParse(mp, GPOPT_DSL_CORPUS_INSUB_ELIM_RULE);
	GPOS_ASSERT(nullptr != prule);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
							   pmodel));
	CDSLConstraintChecker checker(mp);
	GPOS_ASSERT(!checker.FCheck(prule, pmodel));

	pmodel->Release();
	prule->Release();
	pexprSource->Release();
	pexprInnerGet->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLInSubTest::EresUnittest_PostApplyIdentity()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("insub_apply_outer", 2, &pdrgpcrOuter);
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprInner =
		fix.PexprLogicalGet("insub_apply_inner", 2, &pdrgpcrInner);
	CExpression *pexprPred =
		fix.PexprEqPred((*pdrgpcrOuter)[0], (*pdrgpcrInner)[0]);
	CExpression *pexprSource =
		CUtils::PexprLogicalApply<CLogicalLeftSemiApplyIn>(
			mp, pexprOuter, pexprInner, (*pdrgpcrInner)[0],
			COperator::EopScalarSubqueryAny, pexprPred);

	CDSLRule *prule = PruleParse(mp, GPOPT_DSL_INSUB_IDENTITY_RULE);
	GPOS_ASSERT(nullptr != prule);
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
	GPOS_ASSERT(COperator::EopLogicalLeftSemiApplyIn ==
				pexprTarget->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopScalarSubqueryAny ==
				CLogicalApply::PopConvert(pexprTarget->Pop())
					->EopidOriginSubq());
	GPOS_ASSERT(COperator::EopScalarCmp == (*pexprTarget)[2]->Pop()->Eopid());

	pexprTarget->Release();
	pmodel->Release();
	prule->Release();
	pexprSource->Release();
	return GPOS_OK;
}

// EOF
