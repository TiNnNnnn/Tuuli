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
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLPlanTemplate.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CLogicalLeftAntiSemiApplyNotIn.h"
#include "gpopt/operators/CLogicalLeftAntiSemiCorrelatedApplyNotIn.h"
#include "gpopt/operators/CLogicalLeftSemiCorrelatedApplyIn.h"
#include "gpopt/operators/CLogicalMaxOneRow.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CScalarBoolOp.h"
#include "gpopt/operators/CScalarIf.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarSubquery.h"
#include "gpopt/operators/CScalarSubqueryAll.h"
#include "gpopt/operators/CScalarSubqueryAny.h"
#include "naucrates/md/IMDTypeBool.h"
#include "naucrates/md/IMDTypeInt4.h"
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
	std::string sourceTemplate;
	std::string templateError;
	GPOS_ASSERT(CDSLPlanTemplate::FSlice(mp, pexprSource, "r",
		{"r/0", "r/1"}, &sourceTemplate, &templateError));
	GPOS_ASSERT(sourceTemplate == (fAll
		? "All<p0 a0>(Input<t0>,Input<t1>)"
		: "Any<p0 a0>(Input<t0>,Input<t1>)"));
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

static GPOS_RESULT
EresSharedComparisonHead()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	for (BOOL first_all : {false, true})
	for (BOOL shared_output : {false, true})
	{
		const std::string first = first_all ? "All" : "Any";
		const std::string second = first_all ? "Any" : "All";
		const std::string predicate = "And(" + first + "(c0,v0,a8,t1)," + second +
			"(c0,v1," + (shared_output ? "a8" : "a9") + ",t2))";
		CDSLRule *rule = PruleParse(mp, ("Filter<" + predicate + " a0>(Input<t0>)|Filter<Not(Not(" +
			predicate + ")) a1>(Input<t3>)|t3 := t0;a1 := a0").c_str());
		GPOS_ASSERT(nullptr != rule);
		for (ULONG trial = 0; trial < 6; ++trial)
		{
			CColRefArray *outer_cols = nullptr, *inner_cols = nullptr;
			CExpression *outer = fix.PexprLogicalGet("shared_head_outer", 1, &outer_cols);
			CExpression *query = fix.PexprLogicalGet("shared_head_inner", 2, &inner_cols);
			query->AddRef();
			CExpression *left = PexprQuantified(mp, fix, first_all, query, (*outer_cols)[0], (*inner_cols)[0]);
			CExpression *right_query = trial == 5
				? GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalSelect(mp), query,
					fix.PexprEqPred((*outer_cols)[0], (*inner_cols)[1])) : query;
			IMDId *id = (*inner_cols)[0]->RetrieveType()->GetMdidForCmpType(
				trial == 2 ? IMDType::EcmptNEq : IMDType::EcmptEq);
			id->AddRef();
			auto *name = GPOS_NEW(mp) CWStringConst(mp, trial == 2 ? GPOS_WSZ_LIT("<>") : GPOS_WSZ_LIT("="));
			const CColRef *selected = (*inner_cols)[trial == 1 ? 1 : 0];
			const BOOL right_all = trial == 4 ? first_all : !first_all;
			COperator *op = right_all
				? static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryAll(mp, id, name, selected))
				: static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryAny(mp, id, name, selected));
			CExpression *right = GPOS_NEW(mp) CExpression(mp, op, right_query,
				trial == 3 ? CUtils::PexprScalarConstBool(mp, true) : CUtils::PexprScalarIdent(mp, (*outer_cols)[0]));
			CExpression *source = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalSelect(mp), outer,
				GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopAnd), left, right));
			CDSLMatcher matcher(mp, rule);
			CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
			const BOOL matched = matcher.FMatch(rule->PfragSrc()->PopRoot(), source, model);
			GPOS_ASSERT(matched == (trial == 0 || (trial == 1 && !shared_output) || trial == 5));
			if (matched)
			{
				CDSLConstraintChecker checker(mp);
				GPOS_ASSERT(checker.FCheck(rule, model));
				CDSLInstantiator inst(mp);
				CExpression *target = inst.PexprInstantiate(rule, model);
				GPOS_ASSERT(nullptr != target);
				// Construction reuses c0 captured from the other quantifier, but
				// must restore each template quantifier and its own complete input.
				CExpression *conjunction = (*(*(*target)[1])[0])[0];
				GPOS_ASSERT(CDSLMatchView::FSameCapturedExpression((*source)[1], conjunction));
				target->Release();
			}
			model->Release();
			source->Release();
		}
		rule->Release();
	}
	return GPOS_OK;
}

static GPOS_RESULT
EresCapturedComparison()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	GPOS_ASSERT(nullptr == PruleParse(mp,
		"Filter<Compare(c0,v0) a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
		"t1 := t0;a1 := a0"));
	CDSLRule *captured = PruleParse(mp,
		"Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
		"Compare(c0,v0) := p0;t1 := t0;a1 := a0;p1 := Compare(c0,v0)");
	GPOS_ASSERT(nullptr != captured);
	CColRefArray *compare_cols = nullptr;
	CExpression *compare_input = fix.PexprLogicalGet("captured_compare", 2, &compare_cols);
	CExpression *compare_pred = fix.PexprEqPred((*compare_cols)[0], (*compare_cols)[1]);
	CExpression *compare_source = GPOS_NEW(mp) CExpression(mp,
		GPOS_NEW(mp) CLogicalSelect(mp), compare_input, compare_pred);
	CDSLModel *compare_model = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher compare_matcher(mp, captured);
	GPOS_ASSERT(compare_matcher.FMatch(captured->PfragSrc()->PopRoot(), compare_source, compare_model));
	CDSLConstraintChecker compare_checker(mp);
	GPOS_ASSERT(compare_checker.FCheck(captured, compare_model));
	CDSLInstantiator compare_inst(mp);
	CExpression *compare_target = compare_inst.PexprInstantiate(captured, compare_model);
	GPOS_ASSERT(nullptr != compare_target);
	GPOS_ASSERT(CDSLMatchView::FSameCapturedExpression(
		(*compare_source)[1], (*compare_target)[1]));
	compare_target->Release();
	compare_model->Release();
	compare_source->Release();
	captured->Release();
	for (BOOL all : {false, true})
	{
		const std::string source_rule = "Filter<" + std::string(all ? "All" : "Any") +
			"(c0,Args(n0,Args()),a1,t1) a0>(Input<t0>)|";
		const std::string target_rule =
			"SemiJoin<p1 a2 a3>(Input<t2>,Input<t3>)|"
			"t2 := t0;t3 := t1;a2 := a0;a3 := a1;"
			"p1 := Compare(c0,v1);v1 := Args(n0,v2);"
			"v2 := Args(n1,v3);n1 := Column(a1);v3 := Args()";
		CDSLRule *rule = PruleParse(mp, (source_rule + target_rule).c_str());
		GPOS_ASSERT(nullptr != rule);
		CColRefArray *outer_cols = nullptr, *inner_cols = nullptr;
		CExpression *outer = fix.PexprLogicalGet("compare_outer", 1, &outer_cols);
		CExpression *inner = fix.PexprLogicalGet("compare_inner", 1, &inner_cols);
		CExpression *quantified = PexprQuantified(mp, fix, all, inner,
			(*outer_cols)[0], (*inner_cols)[0]);
		CExpression *source = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CLogicalSelect(mp), outer, quantified);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher matcher(mp, rule);
		GPOS_ASSERT(matcher.FMatch(rule->PfragSrc()->PopRoot(), source, model));
		CDSLConstraintChecker checker(mp);
		GPOS_ASSERT(checker.FCheck(rule, model));
		CDSLInstantiator inst(mp);
		CExpression *target = inst.PexprInstantiate(rule, model);
		GPOS_ASSERT(nullptr != target);
		CExpression *comparison = (*target)[2];
		GPOS_ASSERT(COperator::EopScalarCmp == comparison->Pop()->Eopid());
		GPOS_ASSERT(CScalarCmp::PopConvert(comparison->Pop())->MdIdOp()->Equals(
			CScalarSubqueryQuantified::PopConvert(quantified->Pop())->MdIdOp()));
		GPOS_ASSERT(CDSLMatchView::FSameCapturedExpression((*comparison)[0], (*quantified)[1]));
		GPOS_ASSERT((*comparison)[1]->DeriveUsedColumns()->FMember((*inner_cols)[0]));
		target->Release();
		const std::string too_many = source_rule + target_rule.substr(0,
			target_rule.find("v3 := Args()")) + "v3 := Args(n1,v4);v4 := Args()";
		CDSLRule *bad_rule = PruleParse(mp, too_many.c_str());
		GPOS_ASSERT(nullptr != bad_rule);
		CDSLModel *bad_model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher bad_matcher(mp, bad_rule);
		GPOS_ASSERT(bad_matcher.FMatch(bad_rule->PfragSrc()->PopRoot(), source, bad_model));
		GPOS_ASSERT(nullptr == inst.PexprInstantiate(bad_rule, bad_model));
		bad_model->Release();
		bad_rule->Release();
		model->Release();
		source->Release();
		rule->Release();
	}
	return GPOS_OK;
}

static GPOS_RESULT
EresQuantifiedInnerFilterRewrite()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	for (BOOL all : {false, true})
	{
		const std::string quant = all ? "All" : "Any";
		const std::string text = quant +
			"<p0 a0>(Input<t0>,Filter<Not(Not(p1)) a1>(Input<t1>))|" + quant +
			"<p2 a2>(Input<t2>,Filter<p3 a3>(Input<t3>))|"
			"t2 := t0;t3 := t1;p2 := p0;a2 := a0;p3 := p1;a3 := a1";
		CDSLRule *rule = PruleParse(mp, text.c_str());
		GPOS_ASSERT(nullptr != rule);
		CColRefArray *outer_cols = nullptr, *inner_cols = nullptr;
		CExpression *outer = fix.PexprLogicalGet("inner_rewrite_outer", 1, &outer_cols);
		CExpression *inner = fix.PexprLogicalGet("inner_rewrite_inner", 2, &inner_cols);
		CExpression *predicate = fix.PexprEqPred((*inner_cols)[0], (*inner_cols)[1]);
		CExpression *filtered = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalSelect(mp),
			inner, CUtils::PexprNegate(mp, CUtils::PexprNegate(mp, predicate)));
		CExpression *quantified = PexprQuantified(mp, fix, all, filtered,
			(*outer_cols)[0], (*inner_cols)[0]);
		CExpression *source = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CLogicalSelect(mp), outer, quantified);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher matcher(mp, rule);
		GPOS_ASSERT(matcher.FMatch(rule->PfragSrc()->PopRoot(), source, model));
		CDSLConstraintChecker checker(mp);
		GPOS_ASSERT(checker.FCheck(rule, model));
		CDSLInstantiator inst(mp);
		CExpression *target = inst.PexprInstantiate(rule, model);
		GPOS_ASSERT(nullptr != target);
		GPOS_ASSERT(target->Pop()->Eopid() == (all
			? COperator::EopLogicalLeftAntiSemiApplyNotIn
			: COperator::EopLogicalLeftSemiApplyIn));
		GPOS_ASSERT((*(*target)[1])[0] == inner);
		GPOS_ASSERT(CDSLMatchView::FSameCapturedExpression(predicate, (*(*target)[1])[1]));
		GPOS_ASSERT((*CLogicalApply::PopConvert(target->Pop())->PdrgPcrInner())[0] == (*inner_cols)[0]);
		target->Release();
		model->Release();
		source->Release();
		rule->Release();
	}
	return GPOS_OK;
}

static GPOS_RESULT
EresConstructedQuantifiedPredicate()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	for (BOOL all : {false, true})
	{
		const std::string quant = all ? "All" : "Any";
		const std::string rule_text =
			"Filter<" + quant + "(c0,Args(n0,Args()),a2,t1) a0>(Input<t0>)|" +
			quant + "<p1 a3>(Input<t2>,Input<t3>)|" +
			"Column(a1) := n0;t2 := t0;t3 := t1;a3 := a1;"
			"n1 := Column(a2);v3 := Args();v2 := Args(n1,v3);"
			"v1 := Args(n0,v2);p1 := Compare(c0,v1)";
		CDSLRule *rule = PruleParse(mp, rule_text.c_str());
		GPOS_ASSERT(nullptr != rule);
		CColRefArray *outer_cols = nullptr, *inner_cols = nullptr;
		CExpression *outer = fix.PexprLogicalGet("built_quant_outer", 1, &outer_cols);
		CExpression *inner = fix.PexprLogicalGet("built_quant_inner", 1, &inner_cols);
		CExpression *quantified = PexprQuantified(mp, fix, all, inner,
			(*outer_cols)[0], (*inner_cols)[0]);
		CExpression *source = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CLogicalSelect(mp), outer, quantified);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher matcher(mp, rule);
		GPOS_ASSERT(matcher.FMatch(rule->PfragSrc()->PopRoot(), source, model));
		CDSLConstraintChecker checker(mp);
		GPOS_ASSERT(checker.FCheck(rule, model));
		CDSLInstantiator inst(mp);
		CExpression *target = inst.PexprInstantiate(rule, model);
		GPOS_ASSERT(nullptr != target);
		GPOS_ASSERT(target->Pop()->Eopid() == (all
			? COperator::EopLogicalLeftAntiSemiApplyNotIn
			: COperator::EopLogicalLeftSemiApplyIn));
		target->Release();
		std::string invalid = rule_text;
		invalid.replace(invalid.find("a3 := a1"), 8, "a3 := a2");
		CDSLRule *bad_rule = PruleParse(mp, invalid.c_str());
		GPOS_ASSERT(nullptr != bad_rule);
		CDSLModel *bad_model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher bad_matcher(mp, bad_rule);
		GPOS_ASSERT(bad_matcher.FMatch(bad_rule->PfragSrc()->PopRoot(), source, bad_model));
		GPOS_ASSERT(checker.FCheck(bad_rule, bad_model));
		CDSLInstantiator bad_inst(mp);
		GPOS_ASSERT(nullptr == bad_inst.PexprInstantiate(bad_rule, bad_model));
		bad_model->Release();
		bad_rule->Release();
		model->Release();
		source->Release();
		rule->Release();
	}
	return GPOS_OK;
}

GPOS_RESULT
CDSLQuantifiedTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(EresSharedComparisonHead),
		GPOS_UNITTEST_FUNC(EresCapturedComparison),
		GPOS_UNITTEST_FUNC(EresQuantifiedInnerFilterRewrite),
		GPOS_UNITTEST_FUNC(EresConstructedQuantifiedPredicate),
		GPOS_UNITTEST_FUNC(CDSLQuantifiedTest::EresUnittest_TypedQuantifiedBindings),
		GPOS_UNITTEST_FUNC(CDSLQuantifiedTest::EresUnittest_TypedScalarSubqueryBindings),
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
		GPOS_UNITTEST_FUNC(
			CDSLQuantifiedTest::EresUnittest_PredicateRemapPreservesInput),
		GPOS_UNITTEST_FUNC(CDSLQuantifiedTest::
			EresUnittest_ExpressionDefinedProjectQuantified),
		GPOS_UNITTEST_FUNC(
			CDSLQuantifiedTest::EresUnittest_ExpressionDefinedScalarSubquery)};
	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLQuantifiedTest::EresUnittest_TypedQuantifiedBindings()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	for (BOOL all : {false, true})
	{
		const std::string quant = all ? "All" : "Any";
		const std::string source_pattern = "Filter<Not(Not(" + quant +
			"(c0,Args(n0,Args()),a8,t1))) a0>(Input<t0>)|";
		const std::string target_pattern = "Filter<" + quant +
			"(c1,Args(n0,Args()),a8,t3) a1>(Input<t2>)|t2 := t0;t3 := t1;a1 := a0;c1 := c0";
		CDSLRule *rule = PruleParse(mp, (source_pattern + target_pattern).c_str());
		GPOS_ASSERT(nullptr != rule);
		for (ULONG trial = 0; trial < 4; ++trial)
		{
			CColRefArray *outer_cols = nullptr, *inner_cols = nullptr;
			CExpression *outer = fix.PexprLogicalGet("typed_quant_outer", 1, &outer_cols);
			CExpression *query = fix.PexprLogicalGet("typed_quant_inner", trial == 2 ? 2 : 1, &inner_cols);
			if (trial == 1)
				query = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalSelect(mp), query,
					fix.PexprEqPred((*outer_cols)[0], (*inner_cols)[0]));
			if (trial == 3)
				query = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalMaxOneRow(mp), query);
			CExpression *quantified = PexprQuantified(mp, fix, all, query, (*outer_cols)[0], (*inner_cols)[trial == 2 ? 1 : 0]);
			CExpression *source = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalSelect(mp), outer,
				CUtils::PexprNegate(mp, CUtils::PexprNegate(mp, quantified)));
			CDSLMatcher matcher(mp, rule);
			CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
			const BOOL matched = matcher.FMatch(rule->PfragSrc()->PopRoot(), source, model);
			GPOS_ASSERT(matched == (trial < 3));
			if (matched)
			{
				CDSLConstraintChecker checker(mp);
				GPOS_ASSERT(checker.FCheck(rule, model));
				CDSLInstantiator inst(mp);
				CExpression *target = inst.PexprInstantiate(rule, model);
				GPOS_ASSERT(nullptr != target && (*target)[1]->Pop() == quantified->Pop());
				GPOS_ASSERT((*(*target)[1])[0] == query && (*(*target)[1])[1] == (*quantified)[1]);
				// The target may retain extra columns, but may not lose Pcr().
				CExpressionArray *arguments = GPOS_NEW(mp) CExpressionArray(mp);
				(*quantified)[1]->AddRef();
				arguments->Append((*quantified)[1]);
				GPOS_ASSERT(!CDSLMatchView::FQuantifiedInputs(quantified, outer, arguments, CScalarSubqueryQuantified::PopConvert(quantified->Pop())->Pcr()));
				// Even an available output cannot be substituted at another type
				// or typmod into the source-resolved comparison signature.
				for (ULONG different = 0; different < 2; ++different)
				{
					const auto *type = different == 0 ? fix.Pmda()->PtMDType<IMDTypeBool>()
						: (*inner_cols)[0]->RetrieveType();
					CColRef *output = COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(type,
						different == 0 ? default_type_modifier : 42);
					CExpression *value = different == 0 ? CUtils::PexprScalarConstBool(mp, true)
						: CUtils::PexprScalarIdent(mp, (*inner_cols)[0]);
					CExpression *project = PexprProjectScalar(mp, query, output, value);
					GPOS_ASSERT(!CDSLMatchView::FQuantifiedInputs(quantified, project, arguments, output));
					project->Release();
				}
				arguments->Release();
				GPOS_ASSERT(source->DeriveOutputColumns()->Equals(target->DeriveOutputColumns()));
				std::string text, error;
				GPOS_ASSERT(CDSLPlanTemplate::FSlice(mp, source, "r", {"r/0"}, &text, &error));
				GPOS_ASSERT(text.find(quant + "(c0,Args(Column(a2),Args()),a3,t1)") != std::string::npos);
				target->Release();
			}
			model->Release();
			source->Release();
		}
		rule->Release();
	}
	return GPOS_OK;
}

GPOS_RESULT
CDSLQuantifiedTest::EresUnittest_TypedScalarSubqueryBindings()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *rule = PruleParse(mp,
		"Compute<Item(BoolValue(ValueBool(Subquery(a0,t1))),a1,e0) a2 s0>(Input<t0>)|"
		"Compute<Item(BoolValue(Not(Not(ValueBool(Subquery(a3,t3))))),a1,e0) a4 s1>(Input<t2>)|"
		"t2 := t0;t3 := t1;a3 := a0;a4 := a2;s1 := s0");
	GPOS_ASSERT(nullptr != rule);
	CDSLRule *call_rule = PruleParse(mp,
		"Compute<Item(Call(h0,Args(Subquery(a0,t1),Args(Case(p0,n0,n1),Args()))),a1,e0) a2 s0>(Input<t0>)|"
		"Compute<Item(Call(h0,Args(Subquery(a3,t3),Args(Case(Not(Not(p0)),n0,n1),Args()))),a1,e0) a4 s1>(Input<t2>)|"
		"t2 := t0;t3 := t1;a3 := a0;a4 := a2;s1 := s0");
	CDSLRule *opaque_rule = PruleParse(mp,
		"Compute<Item(Call(h0,v0),a1,e0) a2 s0>(Input<t0>)|"
		"Compute<Item(Call(h0,v0),a1,e0) a4 s1>(Input<t2>)|t2 := t0;a4 := a2;s1 := s0");
	GPOS_ASSERT(nullptr != call_rule && nullptr != opaque_rule);
	const auto *bool_type = COptCtxt::PoctxtFromTLS()->Pmda()->PtMDType<IMDTypeBool>();
	for (ULONG trial = 0; trial < 7; ++trial)
	{
		CColRefArray *outer_cols = nullptr, *inner_cols = nullptr;
		CExpression *outer = fix.PexprLogicalGet("typed_scalar_outer", 1, &outer_cols);
		CExpression *input = fix.PexprLogicalGet("typed_scalar_inner", 2, &inner_cols);
		CColRef *selected = COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(bool_type, default_type_modifier);
		CExpression *query = PexprProjectScalar(mp, input, selected,
			fix.PexprEqPred((*inner_cols)[0], trial == 1 ? (*outer_cols)[0] : (*inner_cols)[1]));
		input->Release();
		if (trial >= 5) selected = (*inner_cols)[1];
		if (trial == 2)
			query = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalMaxOneRow(mp), query);
		CExpression *subquery = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CScalarSubquery(mp, selected, trial == 3, trial == 4), query);
		CColRef *output = COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(bool_type, default_type_modifier);
		CExpression *scalar = subquery;
		if (trial >= 5)
		{
			IMDId *type = fix.Pmda()->PtMDType<IMDTypeInt4>()->MDId();
			type->AddRef();
			CExpression *argument = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarIf(mp, type),
				CUtils::PexprScalarConstBool(mp, true), CUtils::PexprScalarConstInt4(mp, 7),
				CUtils::PexprScalarConstInt4(mp, 9));
			scalar = CUtils::PexprScalarCmp(mp, subquery, argument, IMDType::EcmptEq);
		}
		CExpression *source = PexprProjectScalar(mp, outer, output, scalar);
		CDSLRule *active_rule = trial == 6 ? opaque_rule : trial == 5 ? call_rule : rule;
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher matcher(mp, active_rule);
		const BOOL matched = matcher.FMatch(active_rule->PfragSrc()->PopRoot(), source, model);
		GPOS_ASSERT(matched == (trial < 2 || trial == 5));
		if (matched)
		{
			CDSLConstraintChecker checker(mp);
			GPOS_ASSERT(checker.FCheck(active_rule, model));
			CDSLInstantiator inst(mp);
			CExpression *target = inst.PexprInstantiate(active_rule, model);
			GPOS_ASSERT(nullptr != target);
			CExpression *value = (*(*(*target)[1])[0])[0];
			CExpression *rebuilt = trial == 5 ? (*value)[0] : (*(*value)[0])[0];
			GPOS_ASSERT(rebuilt->Pop()->Matches(subquery->Pop()) && (*rebuilt)[0] == query);
			GPOS_ASSERT(source->DeriveOutputColumns()->Equals(target->DeriveOutputColumns()));
			std::string text, error;
			GPOS_ASSERT(CDSLPlanTemplate::FSlice(mp, source, "r", {"r/0"}, &text, &error));
			GPOS_ASSERT(text.find("Subquery(") != std::string::npos);
			target->Release();
		}
		model->Release();
		source->Release();
		outer->Release();
	}
	call_rule->Release();
	opaque_rule->Release();
	rule->Release();
	return GPOS_OK;
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
		// PredicateAny/All exposes the complete relational operand as Input.
		// Re-normalizing that input would detach a split aggregate from Memo.
		GPOS_ASSERT((*pexprTarget)[1] == (*(*pexprSource)[1])[0]);
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
CDSLQuantifiedTest::EresUnittest_PredicateRemapPreservesInput()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	for (BOOL all : {false, true})
	{
		CColRefArray *leftCols = nullptr, *rightCols = nullptr, *innerCols = nullptr;
		CExpression *left = fix.PexprLogicalGet("remap_left", 1, &leftCols);
		CExpression *right = fix.PexprLogicalGet("remap_right", 1, &rightCols);
		CExpression *inner = fix.PexprLogicalGet("remap_inner", 1, &innerCols);
		CExpression *eq = fix.PexprEqPred((*leftCols)[0], (*rightCols)[0]);
		CExpression *join = fix.PexprLogicalInnerJoin(left, right, eq);
		CExpression *predicate = PexprQuantified(
			mp, fix, all, inner, (*rightCols)[0], (*innerCols)[0]);
		CExpression *source = fix.PexprLogicalSelect(join, predicate);
		CDSLRule *rule = PruleParse(mp,
			"Filter<p0 a2>(InnerJoin<a0 a1>(Input<t0>,Input<t1>))|"
			"Filter<p1 a5>(InnerJoin<a3 a4>(Input<t2>,Input<t3>))|"
			"AttrsEq(a1,a2);AttrsSub(a0,t0);AttrsSub(a1,t1);"
			"AttrsSub(a2,t1);AttrsSub(a5,t0);TableEq(t2,t0);TableEq(t3,t1);"
			"AttrsEq(a3,a0);AttrsEq(a4,a1);AttrsEq(a5,a0);PredicateEq(p1,p0)");
		GPOS_ASSERT(nullptr != rule);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher matcher(mp, rule);
		GPOS_ASSERT(matcher.FMatch(rule->PfragSrc()->PopRoot(), source, model));
		CDSLConstraintChecker checker(mp);
		GPOS_ASSERT(checker.FCheck(rule, model));
		CDSLInstantiator instantiator(mp);
		CExpression *target = instantiator.PexprInstantiate(rule, model);
		GPOS_ASSERT(nullptr != target);
		GPOS_ASSERT((*(*target)[1])[0] == inner);
		GPOS_ASSERT((*(*target)[1])[1]->Matches((*eq)[0]));
		target->Release();
		model->Release();
		rule->Release();
		source->Release();
		predicate->Release();
		join->Release();
		eq->Release();
		left->Release();
		right->Release();
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

		// This carrier computes a boolean over all inner rows. Treating it as
		// an ordinary LeftApply would permit decorrelation to a row-wise LOJ.
		CDSLRule *pruleRowApply = PruleParse(mp,
			"LeftApply<p0 a0 a1 a2>(Input<t0>,Input<t1>)|"
			"LeftJoin<p1 a3 a4>(Input<t2>,Input<t3>)|"
			"TableEq(t2,t0);TableEq(t3,t1);PredicateEq(p1,p0);"
			"AttrsEq(a3,a0);AttrsEq(a4,a1);AttrsEmpty(a2)");
		GPOS_ASSERT(nullptr != pruleRowApply);
		CDSLModel *pmodelRowApply = GPOS_NEW(mp) CDSLModel(mp);
		GPOS_UNITTEST_ASSERT(!CDSLMatcher(mp, pruleRowApply).FMatch(
			pruleRowApply->PfragSrc()->PopRoot(), pexprApply, pmodelRowApply));
		pmodelRowApply->Release();
		pruleRowApply->Release();

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
