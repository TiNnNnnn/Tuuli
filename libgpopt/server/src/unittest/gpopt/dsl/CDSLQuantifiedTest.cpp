//---------------------------------------------------------------------------
// Tests for generic ANY / ALL DSL matching and instantiation.
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLQuantifiedTest.h"

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
#include "gpopt/operators/CLogicalLeftAntiSemiApplyNotIn.h"
#include "gpopt/operators/CLogicalLeftAntiSemiCorrelatedApplyNotIn.h"
#include "gpopt/operators/CLogicalLeftSemiCorrelatedApplyIn.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CScalarSubqueryAll.h"
#include "gpopt/operators/CScalarSubqueryAny.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

#define GPOPT_DSL_ANY_DISTINCT_DROP_RULE                                  \
	"Any<p0 a0>(Input<t0>,Proj*<a1 s0>(Input<t1>))|"                    \
	"Any<p1 a2>(Input<t2>,Proj<a3 s1>(Input<t3>))|"                     \
	"AttrsSub(a0,t0);AttrsSub(a1,t1);TableEq(t2,t0);TableEq(t3,t1);"    \
	"PredicateEq(p1,p0);AttrsEq(a2,a0);AttrsEq(a3,a1);SchemaEq(s1,s0)"

#define GPOPT_DSL_ALL_DISTINCT_DROP_RULE                                  \
	"All<p0 a0>(Input<t0>,Proj*<a1 s0>(Input<t1>))|"                    \
	"All<p1 a2>(Input<t2>,Proj<a3 s1>(Input<t3>))|"                     \
	"AttrsSub(a0,t0);AttrsSub(a1,t1);TableEq(t2,t0);TableEq(t3,t1);"    \
	"PredicateEq(p1,p0);AttrsEq(a2,a0);AttrsEq(a3,a1);SchemaEq(s1,s0)"

namespace
{
CDSLRule *
PruleParse(CMemoryPool *mp, const CHAR *szRule)
{
	CWStringDynamic strErr(mp);
	return CDSLRuleParser::PdslruleParse(mp, szRule, "EQ", &strErr);
}

CExpression *
PexprQuantified(CMemoryPool *mp, CDSLTestFixture &fix, BOOL fAll,
				CExpression *pexprInner, CColRef *pcrOuter,
				CColRef *pcrInner)
{
	CExpression *pexprEq = fix.PexprEqPred(pcrOuter, pcrInner);
	CScalarCmp *popEq = CScalarCmp::PopConvert(pexprEq->Pop());
	IMDId *pmdidEq = popEq->MdIdOp();
	pmdidEq->AddRef();
	CWStringConst *pstrEq =
		GPOS_NEW(mp) CWStringConst(mp, popEq->Pstr()->GetBuffer());
	pexprEq->Release();
	COperator *pop = fAll
		? static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryAll(
			  mp, pmdidEq, pstrEq, pcrInner))
		: static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryAny(
			  mp, pmdidEq, pstrEq, pcrInner));
	return GPOS_NEW(mp) CExpression(
		mp, pop, pexprInner, CUtils::PexprScalarIdent(mp, pcrOuter));
}

CExpression *
PexprPreUnnest(CMemoryPool *mp, CDSLTestFixture &fix, BOOL fAll,
			   CExpression **ppexprInnerGet)
{
	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet(fAll ? "all_outer" : "any_outer", 2,
							   &pdrgpcrOuter);
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprInnerGet =
		fix.PexprLogicalGet(fAll ? "all_inner" : "any_inner", 2,
							   &pdrgpcrInner);
	*ppexprInnerGet = pexprInnerGet;
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrInner)[0]);
	CExpression *pexprDistinct =
		fix.PexprLogicalGbAgg(pexprInnerGet, pdrgpcrGroup);
	pdrgpcrGroup->Release();
	CExpression *pexprSubquery = PexprQuantified(
		mp, fix, fAll, pexprDistinct, (*pdrgpcrOuter)[0],
		(*pdrgpcrInner)[0]);
	return GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), pexprOuter, pexprSubquery);
}

GPOS_RESULT
EresPreUnnest(BOOL fAll)
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CExpression *pexprInnerGet = nullptr;
	CExpression *pexprSource =
		PexprPreUnnest(mp, fix, fAll, &pexprInnerGet);
	CDSLRule *prule = PruleParse(
		mp, fAll ? GPOPT_DSL_ALL_DISTINCT_DROP_RULE
				 : GPOPT_DSL_ANY_DISTINCT_DROP_RULE);
	GPOS_ASSERT(nullptr != prule);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
								 pmodel));
	CDSLConstraintChecker checker(mp);
	GPOS_ASSERT(checker.FCheck(prule, pmodel));
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget =
		instantiator.PexprInstantiate(prule, pmodel);
	GPOS_ASSERT(nullptr != pexprTarget);
	GPOS_ASSERT((fAll ? COperator::EopLogicalLeftAntiSemiApplyNotIn
					   : COperator::EopLogicalLeftSemiApplyIn) ==
				pexprTarget->Pop()->Eopid());
	GPOS_ASSERT((*pexprTarget)[1] == pexprInnerGet);
	GPOS_ASSERT(COperator::EopScalarCmp == (*pexprTarget)[2]->Pop()->Eopid());
	GPOS_ASSERT((fAll ? IMDType::EcmptNEq : IMDType::EcmptEq) ==
		CScalarCmp::PopConvert((*pexprTarget)[2]->Pop())->ParseCmpType());

	pexprTarget->Release();
	pmodel->Release();
	prule->Release();
	pexprSource->Release();
	pexprInnerGet->Release();
	return GPOS_OK;
}
}  // namespace

GPOS_RESULT
CDSLQuantifiedTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(
			CDSLQuantifiedTest::EresUnittest_PreUnnestAnyDistinctDrop),
		GPOS_UNITTEST_FUNC(
			CDSLQuantifiedTest::EresUnittest_PreUnnestAllDistinctDrop),
		GPOS_UNITTEST_FUNC(
			CDSLQuantifiedTest::EresUnittest_PostUnnestAllRestoresPredicate),
		GPOS_UNITTEST_FUNC(CDSLQuantifiedTest::
			EresUnittest_PostUnnestCorrelatedPreservesCarrier),
		GPOS_UNITTEST_FUNC(CDSLQuantifiedTest::EresUnittest_PolarityIsolation),
		GPOS_UNITTEST_FUNC(
			CDSLQuantifiedTest::EresUnittest_ConstantOuterDependencies)};
	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLQuantifiedTest::EresUnittest_PreUnnestAnyDistinctDrop()
{
	return EresPreUnnest(false);
}

GPOS_RESULT
CDSLQuantifiedTest::EresUnittest_PreUnnestAllDistinctDrop()
{
	return EresPreUnnest(true);
}

GPOS_RESULT
CDSLQuantifiedTest::EresUnittest_PostUnnestAllRestoresPredicate()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("post_all_outer", 1, &pdrgpcrOuter);
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprInnerGet =
		fix.PexprLogicalGet("post_all_inner", 1, &pdrgpcrInner);
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrInner)[0]);
	CExpression *pexprDistinct =
		fix.PexprLogicalGbAgg(pexprInnerGet, pdrgpcrGroup);
	pdrgpcrGroup->Release();
	CExpression *pexprViolation = CUtils::PexprScalarCmp(
		mp, CUtils::PexprScalarIdent(mp, (*pdrgpcrOuter)[0]),
		CUtils::PexprScalarIdent(mp, (*pdrgpcrInner)[0]),
		IMDType::EcmptNEq);
	CExpression *pexprSource =
		CUtils::PexprLogicalApply<CLogicalLeftAntiSemiApplyNotIn>(
			mp, pexprOuter, pexprDistinct, (*pdrgpcrInner)[0],
			COperator::EopScalarSubqueryAll, pexprViolation);
	CDSLRule *prule =
		PruleParse(mp, GPOPT_DSL_ALL_DISTINCT_DROP_RULE);
	GPOS_ASSERT(nullptr != prule);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
								 pmodel));
	CExpression *pexprBound = pmodel->PexprPred(
		(*prule->PfragSrc()->PopRoot()->Pdrgpsym())[0]);
	GPOS_ASSERT(nullptr != pexprBound);
	GPOS_ASSERT(IMDType::EcmptEq ==
		CScalarCmp::PopConvert(pexprBound->Pop())->ParseCmpType());
	CDSLConstraintChecker checker(mp);
	GPOS_ASSERT(checker.FCheck(prule, pmodel));
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget =
		instantiator.PexprInstantiate(prule, pmodel);
	GPOS_ASSERT(nullptr != pexprTarget);
	GPOS_ASSERT(COperator::EopLogicalLeftAntiSemiApplyNotIn ==
				pexprTarget->Pop()->Eopid());
	GPOS_ASSERT(IMDType::EcmptNEq ==
		CScalarCmp::PopConvert((*pexprTarget)[2]->Pop())->ParseCmpType());
	GPOS_ASSERT((*pexprTarget)[1] == pexprInnerGet);

	pexprTarget->Release();
	pmodel->Release();
	prule->Release();
	pexprSource->Release();
	pexprInnerGet->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLQuantifiedTest::EresUnittest_PostUnnestCorrelatedPreservesCarrier()
{
	for (ULONG ulAll = 0; ulAll < 2; ulAll++)
	{
		const BOOL fAll = 0 != ulAll;
		CAutoMemoryPool amp;
		CMemoryPool *mp = amp.Pmp();
		CDSLTestFixture fix(mp);
		CColRefArray *pdrgpcrOuter = nullptr;
		CExpression *pexprOuter = fix.PexprLogicalGet(
			fAll ? "corr_all_outer" : "corr_any_outer", 1,
			&pdrgpcrOuter);
		CColRefArray *pdrgpcrInner = nullptr;
		CExpression *pexprInnerGet = fix.PexprLogicalGet(
			fAll ? "corr_all_inner" : "corr_any_inner", 1,
			&pdrgpcrInner);
		CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
		pdrgpcrGroup->Append((*pdrgpcrInner)[0]);
		CExpression *pexprDistinct =
			fix.PexprLogicalGbAgg(pexprInnerGet, pdrgpcrGroup);
		pdrgpcrGroup->Release();
		CExpression *pexprPred = CUtils::PexprScalarCmp(
			mp, CUtils::PexprScalarIdent(mp, (*pdrgpcrOuter)[0]),
			CUtils::PexprScalarIdent(mp, (*pdrgpcrInner)[0]),
			fAll ? IMDType::EcmptNEq : IMDType::EcmptEq);
		CExpression *pexprSource = fAll
			? CUtils::PexprLogicalApply<
				  CLogicalLeftAntiSemiCorrelatedApplyNotIn>(
				  mp, pexprOuter, pexprDistinct, (*pdrgpcrInner)[0],
				  COperator::EopScalarSubqueryAll, pexprPred)
			: CUtils::PexprLogicalApply<CLogicalLeftSemiCorrelatedApplyIn>(
				  mp, pexprOuter, pexprDistinct, (*pdrgpcrInner)[0],
				  COperator::EopScalarSubqueryAny, pexprPred);
		CDSLRule *prule = PruleParse(
			mp, fAll ? GPOPT_DSL_ALL_DISTINCT_DROP_RULE
					 : GPOPT_DSL_ANY_DISTINCT_DROP_RULE);
		GPOS_ASSERT(nullptr != prule);
		CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher matcher(mp, prule);
		GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
									 pmodel));
		CDSLConstraintChecker checker(mp);
		GPOS_ASSERT(checker.FCheck(prule, pmodel));
		CDSLInstantiator instantiator(mp);
		CExpression *pexprTarget =
			instantiator.PexprInstantiate(prule, pmodel);
		GPOS_ASSERT(nullptr != pexprTarget);
		GPOS_ASSERT((fAll
					 ? COperator::EopLogicalLeftAntiSemiCorrelatedApplyNotIn
					 : COperator::EopLogicalLeftSemiCorrelatedApplyIn) ==
					pexprTarget->Pop()->Eopid());
		GPOS_ASSERT((*pexprTarget)[1] == pexprInnerGet);

		pexprTarget->Release();
		pmodel->Release();
		prule->Release();
		pexprSource->Release();
		pexprInnerGet->Release();
	}
	return GPOS_OK;
}

GPOS_RESULT
CDSLQuantifiedTest::EresUnittest_PolarityIsolation()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CExpression *pexprInnerGet = nullptr;
	CExpression *pexprAny =
		PexprPreUnnest(mp, fix, false, &pexprInnerGet);
	CDSLRule *pruleAll =
		PruleParse(mp, GPOPT_DSL_ALL_DISTINCT_DROP_RULE);
	GPOS_ASSERT(nullptr != pruleAll);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, pruleAll);
	GPOS_ASSERT(!matcher.FMatch(pruleAll->PfragSrc()->PopRoot(), pexprAny,
								  pmodel));
	pmodel->Release();
	pruleAll->Release();
	pexprAny->Release();
	pexprInnerGet->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLQuantifiedTest::EresUnittest_ConstantOuterDependencies()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("constant_any_outer", 1, &pdrgpcrOuter);
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprInnerGet =
		fix.PexprLogicalGet("constant_any_inner", 1, &pdrgpcrInner);
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrInner)[0]);
	CExpression *pexprDistinct =
		fix.PexprLogicalGbAgg(pexprInnerGet, pdrgpcrGroup);
	pdrgpcrGroup->Release();

	// Reuse the fixture's equality metadata, but the actual outer operand is a
	// constant and therefore binds a legitimate empty dependency vector.
	CExpression *pexprEq =
		fix.PexprEqPred((*pdrgpcrOuter)[0], (*pdrgpcrInner)[0]);
	CScalarCmp *popEq = CScalarCmp::PopConvert(pexprEq->Pop());
	IMDId *pmdidEq = popEq->MdIdOp();
	pmdidEq->AddRef();
	CWStringConst *pstrEq =
		GPOS_NEW(mp) CWStringConst(mp, popEq->Pstr()->GetBuffer());
	pexprEq->Release();
	CExpression *pexprAny = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CScalarSubqueryAny(mp, pmdidEq, pstrEq,
										 (*pdrgpcrInner)[0]),
		pexprDistinct, CUtils::PexprScalarConstInt4(mp, 7));
	CExpression *pexprSource = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), pexprOuter, pexprAny);
	CDSLRule *prule =
		PruleParse(mp, GPOPT_DSL_ANY_DISTINCT_DROP_RULE);
	GPOS_ASSERT(nullptr != prule);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
								 pmodel));
	CColRefArray *pdrgpcrBound = pmodel->PdrgpcrAttrs(
		(*prule->PfragSrc()->PopRoot()->Pdrgpsym())[1]);
	GPOS_ASSERT(nullptr != pdrgpcrBound && 0 == pdrgpcrBound->Size());
	CDSLConstraintChecker checker(mp);
	GPOS_ASSERT(checker.FCheck(prule, pmodel));
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget =
		instantiator.PexprInstantiate(prule, pmodel);
	GPOS_ASSERT(nullptr != pexprTarget);
	GPOS_ASSERT(COperator::EopLogicalLeftSemiApplyIn ==
				pexprTarget->Pop()->Eopid());

	pexprTarget->Release();
	pmodel->Release();
	prule->Release();
	pexprSource->Release();
	pexprInnerGet->Release();
	return GPOS_OK;
}

// EOF
