//---------------------------------------------------------------------------
// Tests for generic ANY / ALL DSL matching and instantiation.
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLQuantifiedTest.h"

#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CLogicalLeftAntiSemiApplyNotIn.h"
#include "gpopt/operators/CLogicalLeftAntiSemiCorrelatedApplyNotIn.h"
#include "gpopt/operators/CLogicalLeftSemiCorrelatedApplyIn.h"
#include "gpopt/operators/CLogicalMaxOneRow.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarSubquery.h"
#include "gpopt/operators/CScalarSubqueryAll.h"
#include "gpopt/operators/CScalarSubqueryAny.h"
#include "naucrates/md/IMDTypeBool.h"
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

#define GPOPT_DSL_EXPRESSION_DEFINED_ANY_RULE                            \
	"Filter<p0 a0>(Input<t0>)|Any<p1 a1>(Input<t1>,Input<t2>)|"        \
	"TableEq(t1,t0);PredicateAny(p0,p1,a1,t2)"

#define GPOPT_DSL_EXPRESSION_DEFINED_ALL_RULE                            \
	"Filter<p0 a0>(Input<t0>)|All<p1 a1>(Input<t1>,Input<t2>)|"        \
	"TableEq(t1,t0);PredicateAll(p0,p1,a1,t2)"

#define GPOPT_DSL_EXPRESSION_DEFINED_PROJECT_ANY_RULE                       \
	"Compute<e0 a0 s0>(Input<t0>)|"                                         \
	"Compute<e1 a1 s1>(LeftApply<p0 a2 a3 a4>(Input<t1>,"                   \
	"Compute<e2 a5 s2>(Input<t2>)))|TableEq(t1,t0);SchemaEq(s1,s0);"         \
	"ExprListAny(e0,e1,e2,a5,s2,p0,a2,a3,a4,a6,t2)"

#define GPOPT_DSL_EXPRESSION_DEFINED_PROJECT_ALL_RULE                       \
	"Compute<e0 a0 s0>(Input<t0>)|"                                         \
	"Compute<e1 a1 s1>(LeftApply<p0 a2 a3 a4>(Input<t1>,"                   \
	"Compute<e2 a5 s2>(Input<t2>)))|TableEq(t1,t0);SchemaEq(s1,s0);"         \
	"ExprListAll(e0,e1,e2,a5,s2,p0,a2,a3,a4,a6,t2)"

#define GPOPT_DSL_EXPRESSION_DEFINED_SCALAR_RULE                         \
	"Filter<p0 a0>(Input<t0>)|InnerApply<p1 a1 a2 a3>(Input<t1>,"      \
	"Input<t2>)|TableEq(t1,t0);"                                        \
	"PredicateScalarSubquery(p0,p1,a1,a2,a3,t2)"

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

CExpression *
PexprProjectScalar(CMemoryPool *mp, CExpression *pexprChild,
				   CColRef *pcrOutput, CExpression *pexprScalar)
{
	CExpressionArray *pdrgpexprElems = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexprElems->Append(CUtils::PexprScalarProjectElement(
		mp, pcrOutput, pexprScalar));
	CExpression *pexprList = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprElems);
	pexprChild->AddRef();
	return GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalProject(mp), pexprChild, pexprList);
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
			CDSLQuantifiedTest::EresUnittest_ConstantOuterDependencies),
		GPOS_UNITTEST_FUNC(
			CDSLQuantifiedTest::EresUnittest_ExpressionDefinedQuantified),
		GPOS_UNITTEST_FUNC(CDSLQuantifiedTest::
			EresUnittest_ExpressionDefinedProjectQuantified),
		GPOS_UNITTEST_FUNC(
			CDSLQuantifiedTest::EresUnittest_ExpressionDefinedScalarSubquery)};
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

GPOS_RESULT
CDSLQuantifiedTest::EresUnittest_ExpressionDefinedQuantified()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	for (ULONG ul = 0; ul < 2; ul++)
	{
		const BOOL fAll = 0 < ul;
		CExpression *pexprInnerGet = nullptr;
		CExpression *pexprSource =
			PexprPreUnnest(mp, fix, fAll, &pexprInnerGet);
		CDSLRule *prule = PruleParse(
			mp, fAll ? GPOPT_DSL_EXPRESSION_DEFINED_ALL_RULE
					 : GPOPT_DSL_EXPRESSION_DEFINED_ANY_RULE);
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
		GPOS_ASSERT((fAll ? COperator::EopLogicalLeftAntiSemiApplyNotIn
						   : COperator::EopLogicalLeftSemiApplyIn) ==
					pexprTarget->Pop()->Eopid());
		GPOS_ASSERT((fAll ? COperator::EopScalarSubqueryAll
						   : COperator::EopScalarSubqueryAny) ==
					CLogicalApply::PopConvert(pexprTarget->Pop())->EopidOriginSubq());

		pexprTarget->Release();
		pmodel->Release();
		prule->Release();
		pexprSource->Release();
		pexprInnerGet->Release();
	}
	return GPOS_OK;
}

GPOS_RESULT
CDSLQuantifiedTest::EresUnittest_ExpressionDefinedProjectQuantified()
{
	for (ULONG ul = 0; ul < 2; ul++)
	{
		const BOOL fAll = 0 < ul;
		CAutoMemoryPool amp;
		CMemoryPool *mp = amp.Pmp();
		CDSLTestFixture fix(mp);
		CColRefArray *pdrgpcrOuter = nullptr;
		CExpression *pexprOuter = fix.PexprLogicalGet(
			fAll ? "project_all_outer" : "project_any_outer", 1,
			&pdrgpcrOuter);
		CColRefArray *pdrgpcrInner = nullptr;
		CExpression *pexprInner = fix.PexprLogicalGet(
			fAll ? "project_all_inner" : "project_any_inner", 1,
			&pdrgpcrInner);
		CExpression *pexprSubquery = PexprQuantified(
			mp, fix, fAll, pexprInner, (*pdrgpcrOuter)[0],
			(*pdrgpcrInner)[0]);
		const IMDTypeBool *pmdtypebool =
			COptCtxt::PoctxtFromTLS()->Pmda()->PtMDType<IMDTypeBool>();
		CColRef *pcrOutput = COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
			pmdtypebool, default_type_modifier);
		CExpression *pexprSource = PexprProjectScalar(
			mp, pexprOuter, pcrOutput, pexprSubquery);
		CDSLRule *prule = PruleParse(
			mp, fAll ? GPOPT_DSL_EXPRESSION_DEFINED_PROJECT_ALL_RULE
					 : GPOPT_DSL_EXPRESSION_DEFINED_PROJECT_ANY_RULE);
		GPOS_ASSERT(nullptr != prule);
		CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher matcher(mp);
		CDSLConstraintChecker checker(mp);
		GPOS_ASSERT(matcher.FMatch(
			prule->PfragSrc()->PopRoot(), pexprSource, pmodel));
		GPOS_ASSERT(checker.FCheck(prule, pmodel));
		CDSLInstantiator instantiator(mp);
		CExpression *pexprTarget =
			instantiator.PexprInstantiate(prule, pmodel);
		GPOS_ASSERT(nullptr != pexprTarget);
		CExpression *pexprApply = (*pexprTarget)[0];
		GPOS_ASSERT(COperator::EopLogicalProject ==
					pexprTarget->Pop()->Eopid());
		GPOS_ASSERT(COperator::EopLogicalLeftOuterCorrelatedApply ==
					pexprApply->Pop()->Eopid());
		GPOS_ASSERT((fAll ? COperator::EopScalarSubqueryAll
						   : COperator::EopScalarSubqueryAny) ==
			CLogicalApply::PopConvert(pexprApply->Pop())->EopidOriginSubq());
		GPOS_ASSERT(COperator::EopScalarCmp == (*pexprApply)[2]->Pop()->Eopid());
		GPOS_ASSERT(!(*pexprTarget)[1]->DeriveHasSubquery());

		pexprTarget->Release();
		pmodel->Release();
		prule->Release();
		pexprSource->Release();
		pexprOuter->Release();
	}

	// A quantified comparison may itself contain a quantified subquery. Lower
	// the deepest node first so the generated Apply predicate is subquery-free.
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("nested_quant_outer", 1, &pdrgpcrOuter);
	CColRefArray *pdrgpcrInnerAll = nullptr;
	CExpression *pexprInnerAllRel =
		fix.PexprLogicalGet("nested_all_inner", 1, &pdrgpcrInnerAll);
	CExpression *pexprInnerAll = PexprQuantified(
		mp, fix, true, pexprInnerAllRel, (*pdrgpcrOuter)[0],
		(*pdrgpcrInnerAll)[0]);
	CColRefArray *pdrgpcrOuterAny = nullptr;
	CExpression *pexprOuterAnyRel =
		fix.PexprLogicalGet("nested_any_inner", 1, &pdrgpcrOuterAny);
	CExpression *pexprEq =
		fix.PexprEqPred((*pdrgpcrOuter)[0], (*pdrgpcrOuterAny)[0]);
	CScalarCmp *popEq = CScalarCmp::PopConvert(pexprEq->Pop());
	IMDId *pmdidEq = popEq->MdIdOp();
	pmdidEq->AddRef();
	CWStringConst *pstrEq =
		GPOS_NEW(mp) CWStringConst(mp, popEq->Pstr()->GetBuffer());
	pexprEq->Release();
	CExpression *pexprOuterAny = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CScalarSubqueryAny(
			mp, pmdidEq, pstrEq, (*pdrgpcrOuterAny)[0]),
		pexprOuterAnyRel, pexprInnerAll);
	const IMDTypeBool *pmdtypebool =
		COptCtxt::PoctxtFromTLS()->Pmda()->PtMDType<IMDTypeBool>();
	CColRef *pcrOutput = COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
		pmdtypebool, default_type_modifier);
	CExpression *pexprSource =
		PexprProjectScalar(mp, pexprOuter, pcrOutput, pexprOuterAny);
	CDSLRule *prule =
		PruleParse(mp, GPOPT_DSL_EXPRESSION_DEFINED_PROJECT_ALL_RULE);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
								 pmodel));
	GPOS_ASSERT(checker.FCheck(prule, pmodel));
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
	GPOS_ASSERT(nullptr != pexprTarget);
	CExpression *pexprApply = (*pexprTarget)[0];
	GPOS_ASSERT(COperator::EopScalarSubqueryAll ==
		CLogicalApply::PopConvert(pexprApply->Pop())->EopidOriginSubq());
	GPOS_ASSERT(!(*pexprApply)[2]->DeriveHasSubquery());
	GPOS_ASSERT((*pexprTarget)[1]->DeriveHasSubquery());

	pexprTarget->Release();
	pmodel->Release();
	prule->Release();
	pexprSource->Release();
	pexprOuter->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLQuantifiedTest::EresUnittest_ExpressionDefinedScalarSubquery()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("scalar_outer", 1, &pdrgpcrOuter);
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprInner =
		fix.PexprLogicalGet("scalar_inner", 1, &pdrgpcrInner);
	CExpression *pexprSubquery = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarSubquery(
			mp, (*pdrgpcrInner)[0], false, false),
		pexprInner);
	CExpression *pexprPredicate = CUtils::PexprScalarCmp(
		mp, (*pdrgpcrOuter)[0], pexprSubquery, IMDType::EcmptEq);
	CExpression *pexprSource = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), pexprOuter, pexprPredicate);
	CDSLRule *prule =
		PruleParse(mp, GPOPT_DSL_EXPRESSION_DEFINED_SCALAR_RULE);
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
	GPOS_ASSERT(COperator::EopLogicalInnerApply ==
				pexprTarget->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopScalarSubquery ==
		CLogicalApply::PopConvert(pexprTarget->Pop())->EopidOriginSubq());
	GPOS_ASSERT(COperator::EopLogicalMaxOneRow ==
				(*pexprTarget)[1]->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopScalarCmp == (*pexprTarget)[2]->Pop()->Eopid());
	GPOS_ASSERT(!(*pexprTarget)[2]->DeriveHasSubquery());

	pexprTarget->Release();
	pmodel->Release();
	prule->Release();
	pexprSource->Release();
	return GPOS_OK;
}

// EOF
