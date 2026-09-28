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
#include "gpopt/dsl/CDSLQuantifiedMatcher.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CLogicalLeftAntiSemiApplyNotIn.h"
#include "gpopt/operators/CLogicalLeftAntiSemiCorrelatedApplyNotIn.h"
#include "gpopt/operators/CLogicalLeftSemiCorrelatedApplyIn.h"
#include "gpopt/operators/CLogicalLeftSemiApplyIn.h"
#include "gpopt/operators/CLogicalMaxOneRow.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CScalarFunc.h"
#include "gpopt/operators/CScalarBoolOp.h"
#include "gpopt/operators/CScalarIf.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarSubquery.h"
#include "gpopt/operators/CScalarSubqueryAll.h"
#include "gpopt/operators/CScalarSubqueryAny.h"
#include "gpopt/operators/CScalarSubqueryExists.h"
#include "gpopt/operators/CScalarSubqueryNotExists.h"
#include "naucrates/md/IMDTypeBool.h"
#include "naucrates/md/IMDTypeInt4.h"
#include "naucrates/md/CMDTypeInt4GPDB.h"
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
EresComparisonOutcomeDomain()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	BOOL ok = true;
	for (ULONG trial = 0; trial < 3; ++trial)
	{
		CColRefArray *cols = nullptr;
		CExpression *input = fix.PexprLogicalGet("comparison_domain", 1, &cols);
		CExpression *left = CUtils::PexprScalarIdent(mp, (*cols)[0]);
		if (1 == trial)
		{
			// Unknown error behavior in an argument must remain admissible:
			// Compare preserves argument failures, unlike failures in its head.
			left->Release();
			left = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarFunc(mp,
				GPOS_NEW(mp) CMDIdGPDB(IMDId::EmdidGeneral, 100300),
				GPOS_NEW(mp) CMDIdGPDB(IMDId::EmdidGeneral, GPDB_INT4_OID),
				default_type_modifier, GPOS_NEW(mp) CWStringConst(GPOS_WSZ_LIT("nullary")), 0, false));
		}
		CExpression *comparison = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarCmp(mp,
			GPOS_NEW(mp) CMDIdGPDB(IMDId::EmdidGeneral, 2 == trial ? 100400 : GPDB_INT4_EQ_OP),
			GPOS_NEW(mp) CWStringConst(GPOS_WSZ_LIT("=")), IMDType::EcmptEq), left,
			CUtils::PexprScalarConstInt4(mp, 7));
		CExpression *source = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CLogicalSelect(mp), input, comparison);
		for (BOOL call : {false, true})
		{
			const std::string expression = call ? "ValueBool(Call(h0,v0))" : "Compare(c0,v0)";
			CDSLRule *rule = PruleParse(mp, ("Filter<" + expression + " a0>(Input<t0>)|"
				"Filter<Not(Not(" + expression + ")) a1>(Input<t1>)|t1 := t0;a1 := a0").c_str());
			if (nullptr == rule) { source->Release(); return GPOS_FAILED; }
			CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
			const BOOL matched = CDSLMatcher(mp, rule).FMatch(rule->PfragSrc()->PopRoot(), source, model);
			ok &= matched == (call || 2 != trial);
			if (matched)
			{
				ok &= CDSLConstraintChecker(mp).FCheck(rule, model);
				CExpression *target = CDSLInstantiator(mp).PexprInstantiate(rule, model);
				ok &= nullptr != target &&
					CDSLMatchView::FSameCapturedExpression(comparison, (*(*(*target)[1])[0])[0]);
				CRefCount::SafeRelease(target);
			}
			model->Release(); rule->Release();
		}
		std::string exported, error;
		ok &= CDSLPlanTemplate::FSlice(mp, source, "r", {"r/0"}, &exported, &error) &&
			exported.find("ValueBool(Call(") != std::string::npos;
		source->Release();
	}
	return ok ? GPOS_OK : GPOS_FAILED;
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
EresRelationalToScalarQuantifier()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	for (BOOL all : {false, true})
	for (BOOL correlated : {false, true})
	for (BOOL post_apply : {false, true})
	for (ULONG invalid = 0; invalid < (post_apply ? 3 : 2); ++invalid)
	{
		const std::string quant = all ? "All" : "Any";
		const std::string text = quant + "<p0 a0>(Input<t0>,Input<t1>)|Filter<" + quant +
			"(c0,Args(n0,Args()),a2,t3) a1>(Input<t2>)|"
			"Compare(c0,v0) := p0;Args(n0,v1) := v0;Args(n1,v2) := v1;"
			"Column(a2) := n1;Args() := v2;t2 := t0;t3 := t1;a1 := a0";
		CDSLRule *rule = PruleParse(mp, text.c_str());
		GPOS_ASSERT(nullptr != rule);
		CColRefArray *outer_cols = nullptr, *inner_cols = nullptr;
		CExpression *outer = fix.PexprLogicalGet("scalar_outer", 1, &outer_cols);
		CExpression *inner = fix.PexprLogicalGet("scalar_inner", 2, &inner_cols);
		if (correlated)
			inner = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalSelect(mp), inner,
				fix.PexprEqPred((*inner_cols)[0], (*outer_cols)[0]));
		// Invalid cases: select an outer-only column, or let Apply metadata
		// name a different inner column from its comparison's right operand.
		CColRef *selected = invalid == 1 ? (*outer_cols)[0] : (*inner_cols)[1];
		CColRef *carrier_output = invalid == 2 ? (*inner_cols)[0] : selected;
		CExpression *source = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CLogicalSelect(mp), outer,
			PexprQuantified(mp, fix, all, inner, (*outer_cols)[0], selected));
		CExpression *expected = source;
		expected->AddRef();
		if (post_apply)
		{
			CExpression *comparison = CDSLQuantifiedMatcher::PexprComparison(mp, (*source)[1]);
			if (all && !correlated)
			{
				CExpression *inverse = CDSLMatchView::PexprInverseComparison(mp, comparison);
				comparison->Release();
				comparison = inverse;
			}
			outer->AddRef();
			inner->AddRef();
			source->Release();
			if (all)
				source = correlated
					? CUtils::PexprLogicalApply<CLogicalLeftAntiSemiCorrelatedApplyNotIn>(mp, outer, inner,
						carrier_output, COperator::EopScalarSubqueryAll, comparison)
					: CUtils::PexprLogicalApply<CLogicalLeftAntiSemiApplyNotIn>(mp, outer, inner,
						carrier_output, COperator::EopScalarSubqueryAll, comparison);
			else
				source = correlated
					? CUtils::PexprLogicalApply<CLogicalLeftSemiCorrelatedApplyIn>(mp, outer, inner,
						carrier_output, COperator::EopScalarSubqueryAny, comparison)
					: CUtils::PexprLogicalApply<CLogicalLeftSemiApplyIn>(mp, outer, inner,
						carrier_output, COperator::EopScalarSubqueryAny, comparison);
		}
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher matcher(mp, rule);
		const BOOL matched = matcher.FMatch(rule->PfragSrc()->PopRoot(), source, model);
		GPOS_ASSERT(matched == (invalid == 0));
		if (invalid != 0)
		{
			model->Release();
			source->Release();
			expected->Release();
			rule->Release();
			continue;
		}
		CDSLConstraintChecker checker(mp);
		GPOS_ASSERT(checker.FCheck(rule, model));
		CDSLInstantiator inst(mp);
		CExpression *target = inst.PexprInstantiate(rule, model);
		GPOS_ASSERT(nullptr != target);
		GPOS_ASSERT(CDSLMatchView::FSameCapturedExpression(expected, target));
		target->Release();
		std::string bad_text = text;
		bad_text.replace(bad_text.find("Args() := v2"), 12,
			"Args(n2,v3) := v2;Args() := v3");
		CDSLRule *bad_rule = PruleParse(mp, bad_text.c_str());
		GPOS_ASSERT(nullptr != bad_rule);
		CDSLModel *bad_model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher bad_matcher(mp, bad_rule);
		GPOS_ASSERT(!bad_matcher.FMatch(bad_rule->PfragSrc()->PopRoot(), source, bad_model));
		bad_model->Release();
		bad_rule->Release();
		model->Release();
		source->Release();
		expected->Release();
		rule->Release();
	}
	return GPOS_OK;
}

static GPOS_RESULT
EresSharedScalarAndQuantifiedComparison()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	for (BOOL all : {false, true})
	for (BOOL scalar_first : {false, true})
	for (BOOL same_head : {false, true})
	{
		const std::string quant = std::string(all ? "All" : "Any") + "(c0,v0,a2,t1)";
		const std::string predicate = scalar_first ? "And(p0," + quant + ")"
			: "And(" + quant + ",p0)";
		const std::string text = "Filter<" + predicate + " a0>(Input<t0>)|"
			"Filter<Not(Not(" + predicate + ")) a1>(Input<t2>)|"
			"Compare(c0,v1) := p0;t2 := t0;a1 := a0";
		CDSLRule *rule = PruleParse(mp, text.c_str());
		GPOS_ASSERT(nullptr != rule);
		CColRefArray *outer_cols = nullptr, *inner_cols = nullptr;
		CExpression *outer = fix.PexprLogicalGet("shared_scalar_outer", 2, &outer_cols);
		CExpression *inner = fix.PexprLogicalGet("shared_scalar_inner", 1, &inner_cols);
		CExpression *scalar = fix.PexprEqPred((*outer_cols)[0], (*outer_cols)[1]);
		if (!same_head)
		{
			CExpression *different = CDSLMatchView::PexprInverseComparison(mp, scalar);
			scalar->Release();
			scalar = different;
		}
		CExpression *quantified = PexprQuantified(mp, fix, all, inner,
			(*outer_cols)[0], (*inner_cols)[0]);
		CExpression *source = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CLogicalSelect(mp), outer, GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopAnd),
				scalar_first ? scalar : quantified, scalar_first ? quantified : scalar));
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher matcher(mp, rule);
		const BOOL matched = matcher.FMatch(rule->PfragSrc()->PopRoot(), source, model);
		GPOS_ASSERT(matched == same_head);
		if (matched)
		{
			CDSLConstraintChecker checker(mp);
			GPOS_ASSERT(checker.FCheck(rule, model));
			CDSLInstantiator inst(mp);
			CExpression *target = inst.PexprInstantiate(rule, model);
			GPOS_ASSERT(nullptr != target);
			GPOS_ASSERT(CDSLMatchView::FSameCapturedExpression((*source)[1], (*(*(*target)[1])[0])[0]));
			target->Release();
		}
		model->Release();
		source->Release();
		rule->Release();
	}
	return GPOS_OK;
}

static GPOS_RESULT
EresQuantifiedInnerBooleanConstruction()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	for (BOOL all : {false, true})
	for (BOOL correlated : {false, true})
	{
		const std::string quant = all ? "All" : "Any";
		const std::string text = quant +
			"<p0 a0>(Input<t0>,Filter<Or(p1,p2) a1 a4>(Input<t1>))|" + quant +
			"<p3 a3>(Input<t2>,Filter<Not(And(Not(p4),Not(p5))) a6 a5>(Input<t3>))|"
			"t2 := t0;t3 := t1;p3 := p0;a3 := a0;p4 := p1;p5 := p2;a6 := a1;a5 := a4";
		CDSLRule *rule = PruleParse(mp, text.c_str());
		GPOS_ASSERT(nullptr != rule);
		CColRefArray *outer_cols = nullptr, *inner_cols = nullptr;
		CExpression *outer = fix.PexprLogicalGet("boolean_outer", 1, &outer_cols);
		CExpression *inner = fix.PexprLogicalGet("boolean_inner", 2, &inner_cols);
		CExpression *left = fix.PexprEqPred((*inner_cols)[0], (*inner_cols)[1]);
		CExpression *right = fix.PexprEqPred((*inner_cols)[1],
			correlated ? (*outer_cols)[0] : (*inner_cols)[0]);
		CExpression *filtered = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CLogicalSelect(mp), inner,
			GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopOr), left, right));
		CExpression *source = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CLogicalSelect(mp), outer,
			PexprQuantified(mp, fix, all, filtered, (*outer_cols)[0], (*inner_cols)[0]));
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher matcher(mp, rule);
		GPOS_ASSERT(matcher.FMatch(rule->PfragSrc()->PopRoot(), source, model));
		CDSLConstraintChecker checker(mp);
		GPOS_ASSERT(checker.FCheck(rule, model));
		CDSLInstantiator inst(mp);
		CExpression *target = inst.PexprInstantiate(rule, model);
		GPOS_ASSERT(nullptr != target);
		CLogicalApply *apply = CLogicalApply::PopConvert(target->Pop());
		GPOS_ASSERT(apply->FCorrelated() == correlated);
		GPOS_ASSERT(apply->EopidOriginSubq() == (all
			? COperator::EopScalarSubqueryAll : COperator::EopScalarSubqueryAny));
		GPOS_ASSERT((*apply->PdrgPcrInner())[0] == (*inner_cols)[0]);
		GPOS_ASSERT((*(*target)[1])[0] == inner);
		left->AddRef();
		right->AddRef();
		CExpression *expected = CUtils::PexprNegate(mp, GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopAnd),
			CUtils::PexprNegate(mp, left), CUtils::PexprNegate(mp, right)));
		GPOS_ASSERT(CDSLMatchView::FSameCapturedExpression(expected, (*(*target)[1])[1]));
		GPOS_ASSERT((*target)[1]->HasOuterRefs() == correlated);
		expected->Release();
		target->Release();
		model->Release();
		// Dependency partitions govern runtime applicability, not the captured
		// predicate's meaning. Reject incorrect local/outer metadata here.
		std::string bad_text = text;
		bad_text.replace(bad_text.find("a5 := a4"), 8, "a5 := a1");
		CDSLRule *bad_rule = PruleParse(mp, bad_text.c_str());
		GPOS_ASSERT(nullptr != bad_rule);
		CDSLModel *bad_model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher bad_matcher(mp, bad_rule);
		GPOS_ASSERT(bad_matcher.FMatch(bad_rule->PfragSrc()->PopRoot(), source, bad_model));
		GPOS_ASSERT(nullptr == inst.PexprInstantiate(bad_rule, bad_model));
		bad_model->Release();
		bad_rule->Release();
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

static GPOS_RESULT
EresSubqueryOutputBindings()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	BOOL ok = true;
	const CHAR *rules[] = {
		GPOPT_DSL_EXPRESSION_DEFINED_SCALAR_RULE,
		"Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(LeftApply<p2 a2 a3 a4>(Input<t1>,Input<t2>))|TableEq(t1,t0);"
		"ExprListScalarSubquery(p0,p1,p2,a2,a3,a4,a5,t2)",
		"Compute<e0 a0 s0>(Input<t0>)|Compute<e1 a1 s1>(LeftApply<p0 a2 a3 a4>(Input<t1>,Input<t2>))|TableEq(t1,t0);"
		"ExprListScalarSubquery(e0,e1,p0,a2,a3,a4,a5,t2)",
		"WindowRows<a0 o0 w0>(Input<t0>)|WindowRows<a1 o1 w1>(LeftApply<p0 a2 a3 a4>(Input<t1>,Input<t2>))|TableEq(t1,t0);"
		"ExprListScalarSubquery(w0,w1,p0,a2,a3,a4,a5,t2)",
		"Agg<a0 a1 f0 s0 p0>(Input<t0>)|Agg<a2 a3 f1 s1 p1>(LeftApply<p2 a4 a5 a6>(Input<t1>,Input<t2>))|TableEq(t1,t0);"
		"ExprListScalarSubquery(f0,f1,p2,a4,a5,a6,a7,t2)",
		"Compute<e0 a0 s0>(Input<t0>)|Compute<e1 a1 s1>(LeftApply<p0 a3 a4 a5>(Input<t1>,Compute<e2 a2 s2>(Input<t2>)))|TableEq(t1,t0);"
		"ExprListExists(e0,e1,e2,a2,s2,p0,a3,a4,a5,a6,t2)",
		"Compute<e0 a0 s0>(Input<t0>)|Compute<e1 a1 s1>(LeftApply<p0 a3 a4 a5>(Input<t1>,Compute<e2 a2 s2>(Input<t2>)))|TableEq(t1,t0);"
		"ExprListNotExists(e0,e1,e2,a2,s2,p0,a3,a4,a5,a6,t2)",
		"Compute<e0 a0 s0>(Input<t0>)|Compute<e1 a1 s1>(LeftApply<p0 a3 a4 a5>(Input<t1>,Compute<e2 a2 s2>(Input<t2>)))|TableEq(t1,t0);"
		"ExprListAny(e0,e1,e2,a2,s2,p0,a3,a4,a5,a6,t2)",
		"Compute<e0 a0 s0>(Input<t0>)|Compute<e1 a1 s1>(LeftApply<p0 a3 a4 a5>(Input<t1>,Compute<e2 a2 s2>(Input<t2>)))|TableEq(t1,t0);"
		"ExprListAll(e0,e1,e2,a2,s2,p0,a3,a4,a5,a6,t2)",
		GPOPT_DSL_EXPRESSION_DEFINED_ANY_RULE,
		GPOPT_DSL_EXPRESSION_DEFINED_ALL_RULE};
	// Constraint/container probes, not certified or registered rewrite rules.
	for (ULONG kind = 0; kind < GPOS_ARRAY_SIZE(rules); ++kind)
	{
		CWStringDynamic error(mp);
		CDSLRule *rule = CDSLRuleParser::PdslruleParse(mp, rules[kind], nullptr, &error);
		if (nullptr == rule) { ok = false; GPOS_TRACE(error.GetBuffer()); continue; }
		const auto *symbols = (*rule->Pdrgpcon())[1]->Pdrgpsym();
		const BOOL freshMarker = kind >= 5 && kind <= 8;
		CColRefArray *outerCols = nullptr, *innerCols = nullptr;
		CExpression *outer = fix.PexprLogicalGet("capture_outer", 2, &outerCols);
		CExpression *inner = fix.PexprLogicalGet("capture_inner", 1, &innerCols);
		CExpression *scalar = nullptr;
		if (kind < 5)
			scalar = CUtils::PexprScalarCmp(mp, (*outerCols)[0],
				GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarSubquery(
					mp, (*innerCols)[0], false, false), inner), IMDType::EcmptEq);
		else if (kind < 7)
			scalar = GPOS_NEW(mp) CExpression(mp, kind == 5
				? static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryExists(mp))
				: static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryNotExists(mp)), inner);
		else
			scalar = PexprQuantified(mp, fix, kind == 8 || kind == 10, inner, (*outerCols)[0], (*innerCols)[0]);
		CRefCount *source = scalar;
		if (EdslsymFunc == (*symbols)[0]->Esymkind())
		{
			CExpressionArray *items = GPOS_NEW(mp) CExpressionArray(mp);
			items->Append(scalar);
			source = items;
		}
		else if (EdslsymPred != (*symbols)[0]->Esymkind())
		{
			CColRef *output = COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
				fix.Pmda()->PtMDType<IMDTypeBool>(), default_type_modifier);
			source = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
				CUtils::PexprScalarProjectElement(mp, output, scalar));
		}
		CDSLModel *baseline = GPOS_NEW(mp) CDSLModel(mp);
		ok &= baseline->FBind((*symbols)[0], source);
		CDSLConstraintChecker checker(mp);
		const BOOL accepted = checker.FCheck(rule, baseline);
		ok &= accepted;
		if (!accepted) GPOS_TRACE_FORMAT("subquery probe baseline=%d", kind);
		if (accepted)
		{
			CRefCount *outputs[11] = {};
			for (ULONG slot = 1; slot < symbols->Size(); ++slot)
				outputs[slot] = baseline->PvalLookup((*symbols)[slot]);
			const BOOL repeated = checker.FCheck(rule, baseline);
			if (!repeated) GPOS_TRACE_FORMAT("subquery recheck rejected kind=%d", kind);
			ok &= repeated;
			for (ULONG slot = 1; slot < symbols->Size(); ++slot)
				ok &= outputs[slot] == baseline->PvalLookup((*symbols)[slot]);
		}
		if (accepted && freshMarker)
		{
			const auto *constraint = (*rule->Pdrgpcon())[1];
			CColRef *marker = baseline->PcrSubqueryMarker(constraint);
			ok &= nullptr != marker && marker == (*baseline->PdrgpcrSchema((*symbols)[4]))[0];
			// Reusing column identity must not bypass revalidation of outputs.
			CColRefArray *required = baseline->PdrgpcrAttrs((*symbols)[9]);
			required->Replace(0, (*outerCols)[1]);
			ok &= !checker.FCheck(rule, baseline) && baseline->PcrSubqueryMarker(constraint) == marker;
			required->Replace(0, marker);
			ok &= checker.FCheck(rule, baseline);
			// Copying the entire output group to another model supplies no evidence
			// that its marker was freshly generated for that match.
			CDSLModel *imported = GPOS_NEW(mp) CDSLModel(mp);
			for (ULONG slot = 0; slot < symbols->Size(); ++slot)
				ok &= imported->FBind((*symbols)[slot], baseline->PvalLookup((*symbols)[slot]));
			ok &= !checker.FCheck(rule, imported) && nullptr == imported->PcrSubqueryMarker(constraint);
			for (ULONG slot = 0; slot < symbols->Size(); ++slot)
				ok &= imported->PvalLookup((*symbols)[slot]) == baseline->PvalLookup((*symbols)[slot]);
			imported->Release();
			CDSLModel *independent = GPOS_NEW(mp) CDSLModel(mp);
			ok &= independent->FBind((*symbols)[0], source);
			ok &= checker.FCheck(rule, independent) && checker.FCheck(rule, independent);
			ok &= nullptr != independent->PcrSubqueryMarker(constraint) &&
				marker != independent->PcrSubqueryMarker(constraint);
			independent->Release();
		}
		for (ULONG slot = 1; accepted && slot < symbols->Size(); ++slot)
		{
			for (BOOL compatible : {false, true})
			{
				// Fresh-marker lowering is not equated by alpha-renaming.
				if (compatible && freshMarker) continue;
				CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
				ok &= model->FBind((*symbols)[0], source);
				CRefCount *capture = baseline->PvalLookup((*symbols)[slot]);
				if (compatible) capture->AddRef();
				else switch ((*symbols)[slot]->Esymkind())
				{
					case EdslsymAttrs:
					case EdslsymSchema:
					{
						CColRefArray *columns = GPOS_NEW(mp) CColRefArray(mp);
						columns->Append((*outerCols)[1]); capture = columns; break;
					}
					case EdslsymTable: capture = outer; capture->AddRef(); break;
					case EdslsymFunc: capture = GPOS_NEW(mp) CExpressionArray(mp); break;
					case EdslsymExpr:
					case EdslsymWindow:
						capture = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp)); break;
					default: capture = CUtils::PexprScalarConstBool(mp, false); break;
				}
				ok &= model->FBind((*symbols)[slot], capture);
				const BOOL checked = checker.FCheck(rule, model);
				if (checked != compatible)
					GPOS_TRACE_FORMAT("subquery output kind=%d slot=%d compatible=%d", kind, slot, compatible);
				ok &= checked == compatible && model->PvalLookup((*symbols)[slot]) == capture;
				if (checked)
				{
					ok &= checker.FCheck(rule, model);
					if (1 == kind) ok &= model->FDerivedBinding((*symbols)[1]);
				}
				else
				{
					ok &= !model->FDerivedBinding((*symbols)[1]);
					ok &= nullptr == model->PcrSubqueryMarker((*rule->Pdrgpcon())[1]);
					for (ULONG other = 1; other < symbols->Size(); ++other)
						if (other != slot) ok &= nullptr == model->PvalLookup((*symbols)[other]);
				}
				capture->Release(); model->Release();
			}
		}
		if (kind < 2)
		{
			// Repeated output symbols must agree with each other even when none
			// is already bound: two empty vectors agree; outer/inner keys do not.
			std::string text(rules[kind]);
			const std::string oldName = kind == 0 ? "a2" : "a3";
			const std::string newName = kind == 0 ? "a1" : "a2";
			for (size_t pos = text.find(oldName, text.rfind('|') + 1); pos != std::string::npos;
				 pos = text.find(oldName, pos + newName.size()))
				text.replace(pos, oldName.size(), newName);
			CWStringDynamic aliasError(mp);
			CDSLRule *alias = CDSLRuleParser::PdslruleParse(mp, text.c_str(), nullptr, &aliasError);
			ok &= nullptr != alias;
			if (nullptr == alias) GPOS_TRACE(aliasError.GetBuffer());
			if (nullptr != alias)
			{
				const auto *aliased = (*alias->Pdrgpcon())[1]->Pdrgpsym();
				CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
				ok &= model->FBind((*aliased)[0], source);
				const BOOL checked = checker.FCheck(alias, model);
				if (checked != (1 == kind)) GPOS_TRACE_FORMAT("aliased subquery output kind=%d accepted=%d", kind, checked);
				ok &= checked == (1 == kind);
				if (checked) ok &= checker.FCheck(alias, model);
				else for (ULONG slot = 1; slot < aliased->Size(); ++slot)
					ok &= nullptr == model->PvalLookup((*aliased)[slot]);
				model->Release(); alias->Release();
			}
		}
		if (kind >= 7)
		{
			// A correlated reference is not a selected output of the inner query.
			inner->AddRef();
			CExpression *invalid = PexprQuantified(mp, fix, kind == 8 || kind == 10,
				inner, (*outerCols)[0], (*outerCols)[1]);
			if (EdslsymExpr == (*symbols)[0]->Esymkind())
			{
				CColRef *output = COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
					fix.Pmda()->PtMDType<IMDTypeBool>(), default_type_modifier);
				invalid = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
					CUtils::PexprScalarProjectElement(mp, output, invalid));
			}
			CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
			ok &= model->FBind((*symbols)[0], invalid);
			const BOOL checked = checker.FCheck(rule, model);
			if (checked) GPOS_TRACE_FORMAT("invalid selected output accepted kind=%d", kind);
			ok &= !checked;
			for (ULONG slot = 1; slot < symbols->Size(); ++slot)
				ok &= nullptr == model->PvalLookup((*symbols)[slot]);
			invalid->Release(); model->Release();
		}
		baseline->Release(); source->Release(); outer->Release(); rule->Release();
	}
	return ok ? GPOS_OK : GPOS_FAILED;
}

static GPOS_RESULT
EresMarkerSequenceReplay()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	BOOL ok = true;
	const CHAR *names[] = {"Exists", "NotExists", "Any", "All"};
	// The existing extraction order chooses the first occurrence at the same
	// depth, with these operator priorities. Exercise all ten ordered pairs.
	for (ULONG first = 0; first < 4; ++first)
	{
		for (ULONG second = first; second < 4; ++second)
		{
			const std::string text =
				"Compute<e0 a0 s0>(Input<t0>)|Compute<e2 a1 s1>("
				"LeftApply<p1 a6 a7 a8>(LeftApply<p0 a2 a3 a4>(Input<t1>,"
				"Compute<e3 a9 s2>(Input<t2>)),Compute<e4 a10 s3>(Input<t3>)))|"
				"TableEq(t1,t0);SchemaEq(s1,s0);ExprList" + std::string(names[first]) +
				"(e0,e1,e3,a9,s2,p0,a2,a3,a4,a11,t2);ExprList" + names[second] +
				"(e1,e2,e4,a10,s3,p1,a6,a7,a8,a12,t3)";
			// Output identity probes only, not registered equivalent rewrites.
			CWStringDynamic error(mp);
			CDSLRule *rule = CDSLRuleParser::PdslruleParse(mp, text.c_str(), nullptr, &error);
			if (nullptr == rule) { GPOS_TRACE(error.GetBuffer()); ok = false; continue; }
			CColRefArray *outerCols = nullptr;
			CExpression *outer = fix.PexprLogicalGet("marker_chain_outer", 1, &outerCols);
			CExpressionArray *items = GPOS_NEW(mp) CExpressionArray(mp);
			for (ULONG kind : {first, second})
			{
				CColRefArray *innerCols = nullptr;
				CExpression *inner = fix.PexprLogicalGet("marker_chain_inner", 1, &innerCols);
				CExpression *subquery = kind >= 2
					? PexprQuantified(mp, fix, kind == 3, inner, (*outerCols)[0], (*innerCols)[0])
					: GPOS_NEW(mp) CExpression(mp, kind == 0
						? static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryExists(mp))
						: static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryNotExists(mp)), inner);
				CColRef *output = COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
					fix.Pmda()->PtMDType<IMDTypeBool>(), default_type_modifier);
				items->Append(CUtils::PexprScalarProjectElement(mp, output, subquery));
			}
			CExpression *source = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp),
				outer, GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp), items));
			CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
			CDSLConstraintChecker checker(mp);
			const BOOL checked = CDSLMatcher(mp, rule).FMatch(rule->PfragSrc()->PopRoot(), source, model) &&
				checker.FCheck(rule, model);
			if (!checked) GPOS_TRACE_FORMAT("marker chain failed first=%d second=%d", first, second);
			ok &= checked;
			if (checked)
			{
				const auto *step1 = (*rule->Pdrgpcon())[2];
				const auto *step2 = (*rule->Pdrgpcon())[3];
				CColRef *marker1 = model->PcrSubqueryMarker(step1);
				CColRef *marker2 = model->PcrSubqueryMarker(step2);
				CExpression *intermediate = model->PexprExpr((*step1->Pdrgpsym())[1]);
				CExpression *result = model->PexprExpr((*step2->Pdrgpsym())[1]);
				ok &= nullptr != marker1 && nullptr != marker2 && marker1 != marker2 &&
					intermediate->DeriveHasSubquery() && !result->DeriveHasSubquery();
				for (ULONG replay = 0; replay < 2; ++replay)
					ok &= checker.FCheck(rule, model) && marker1 == model->PcrSubqueryMarker(step1) &&
						marker2 == model->PcrSubqueryMarker(step2) &&
						intermediate == model->PexprExpr((*step1->Pdrgpsym())[1]) &&
						result == model->PexprExpr((*step2->Pdrgpsym())[1]);
			}
			model->Release(); source->Release(); rule->Release();
		}
	}
	return ok ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLQuantifiedTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(EresSubqueryOutputBindings),
		GPOS_UNITTEST_FUNC(EresMarkerSequenceReplay),
		GPOS_UNITTEST_FUNC(EresSharedComparisonHead),
		GPOS_UNITTEST_FUNC(EresCapturedComparison),
		GPOS_UNITTEST_FUNC(EresComparisonOutcomeDomain),
		GPOS_UNITTEST_FUNC(EresQuantifiedInnerFilterRewrite),
		GPOS_UNITTEST_FUNC(EresRelationalToScalarQuantifier),
		GPOS_UNITTEST_FUNC(EresSharedScalarAndQuantifiedComparison),
		GPOS_UNITTEST_FUNC(EresQuantifiedInnerBooleanConstruction),
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
				GPOS_ASSERT(nullptr != target &&
					CDSLMatchView::FSameCapturedExpression((*target)[1], quantified));
				GPOS_ASSERT((*(*target)[1])[0] == query && (*(*target)[1])[1] == (*quantified)[1]);
				// The target may retain extra columns, but may not lose Pcr().
				CExpressionArray *arguments = GPOS_NEW(mp) CExpressionArray(mp);
				(*quantified)[1]->AddRef();
				arguments->Append((*quantified)[1]);
				CExpression *head = CDSLQuantifiedMatcher::PexprComparison(mp, quantified);
				GPOS_ASSERT(!CDSLMatchView::FQuantifiedInputs(quantified, outer, arguments, CScalarSubqueryQuantified::PopConvert(quantified->Pop())->Pcr()));
				GPOS_ASSERT(!CDSLMatchView::FQuantifiedInputs(head, outer, arguments, CScalarSubqueryQuantified::PopConvert(quantified->Pop())->Pcr()));
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
					GPOS_ASSERT(!CDSLMatchView::FQuantifiedInputs(head, project, arguments, output));
					project->Release();
				}
				arguments->Release();
				head->Release();
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
	BOOL ok = true;
	for (ULONG ul = 0; ul < 6; ul++)
	{
		const BOOL fAll = 0 != ul % 2;
		// Current native demand guard admits Get but not GbAgg or MaxOneRow.
		const BOOL demandSensitive = 2 <= ul;
		CExpression *pexprInnerGet = nullptr;
		CExpression *pexprSource =
			PexprPreUnnest(mp, fix, fAll, &pexprInnerGet);
		if (ul < 2 || ul >= 4)
		{
			CExpression *predicate = (*pexprSource)[1];
			CExpression *query = ul < 2 ? pexprInnerGet : (*predicate)[0];
			query->AddRef();
			if (ul >= 4)
				query = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalMaxOneRow(mp), query);
			predicate->Pop()->AddRef();
			(*predicate)[1]->AddRef();
			CExpression *wrapped = GPOS_NEW(mp) CExpression(mp, predicate->Pop(), query, (*predicate)[1]);
			(*pexprSource)[0]->AddRef();
			CExpression *source = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CLogicalSelect(mp), (*pexprSource)[0], wrapped);
			pexprSource->Release();
			pexprSource = source;
		}
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
		// Extraction must accept an existing identical capture, including a
		// separately allocated comparison/vector, and never overwrite a conflict.
		ok &= checker.FCheck(prule, pmodel);
		const CDSLSymbolArray *symbols = (*prule->Pdrgpcon())[1]->Pdrgpsym();
		for (ULONG variant = 0; variant < 6; ++variant)
		{
			CDSLModel *bound = GPOS_NEW(mp) CDSLModel(mp);
			ok &= matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource, bound);
			const ULONG slot = 1 + variant % 3;
			const BOOL compatible = variant < 3;
			CRefCount *capture = nullptr;
			if (1 == slot)
				capture = compatible
					? CDSLQuantifiedMatcher::PexprComparison(mp, (*pexprSource)[1])
					: CUtils::PexprScalarConstBool(mp, true);
			else if (2 == slot)
			{
				CColRefArray *columns = GPOS_NEW(mp) CColRefArray(mp);
				if (compatible)
					columns->AppendArray(pmodel->PdrgpcrAttrs((*symbols)[2]));
				capture = columns;
			}
			else
			{
				capture = compatible ? (*(*pexprSource)[1])[0] : (*pexprSource)[0];
				capture->AddRef();
			}
			ok &= bound->FBind((*symbols)[slot], capture);
			const BOOL accepted = checker.FCheck(prule, bound);
			if (accepted != compatible)
				GPOS_TRACE_FORMAT("quantified capture all=%d variant=%d accepted=%d", fAll, variant, accepted);
			ok &= accepted == compatible && bound->PvalLookup((*symbols)[slot]) == capture;
			if (accepted)
				ok &= checker.FCheck(prule, bound);
			else
				for (ULONG other = 1; other <= 3; ++other)
					if (other != slot) ok &= nullptr == bound->PvalLookup((*symbols)[other]);
			capture->Release();
			bound->Release();
		}
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

		// The typed bridge agrees on its admitted domain, but its native-demand
		// guard is narrower. Do not delete the compatibility rule on proof alone.
		const std::string kind = fAll ? "All" : "Any";
		const std::string text = "Filter<" + kind + "(c0,Args(n0,Args()),a2,t1) a0>(Input<t0>)|" +
			kind + "<p1 a3>(Input<t2>,Input<t3>)|t2 := t0;t3 := t1;a3 := ScalarDeps(n0);"
			"n1 := Column(a2);v3 := Args();v2 := Args(n1,v3);v1 := Args(n0,v2);p1 := Compare(c0,v1)";
		CDSLRule *typed = PruleParse(mp, text.c_str());
		GPOS_UNITTEST_ASSERT(nullptr != typed);
		CDSLModel *typedModel = GPOS_NEW(mp) CDSLModel(mp);
		const BOOL matched = CDSLMatcher(mp, typed).FMatch(typed->PfragSrc()->PopRoot(), pexprSource, typedModel);
		GPOS_UNITTEST_ASSERT(matched == !demandSensitive);
		if (matched)
		{
			GPOS_UNITTEST_ASSERT(checker.FCheck(typed, typedModel));
			CDSLInstantiator typedInst(mp);
			CExpression *target = typedInst.PexprInstantiate(typed, typedModel);
			GPOS_UNITTEST_ASSERT(nullptr != target && target->Matches(pexprTarget));
			GPOS_UNITTEST_ASSERT((*target)[1] == (*(*pexprSource)[1])[0]);
			target->Release();
		}
		typedModel->Release();
		typed->Release();

		pexprTarget->Release();
		pmodel->Release();
		prule->Release();
		pexprSource->Release();
		pexprInnerGet->Release();
	}
	return ok ? GPOS_OK : GPOS_FAILED;
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
