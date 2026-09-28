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
#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLExpressionDefinitions.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLPlanTemplate.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CLogicalLeftAntiSemiApply.h"
#include "gpopt/operators/CLogicalLeftSemiApply.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarSubqueryExists.h"
#include "gpopt/operators/CScalarSubqueryNotExists.h"
#include "gpopt/operators/CScalarSubquery.h"
#include "gpopt/operators/CScalarNullTest.h"
#include "gpopt/operators/CScalarBoolOp.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

#define GPOPT_DSL_CORPUS_EXISTS_AGG_PROJ_RULE                              \
	"Exists(Agg<a0 a1 f0 s0 p0>(Input<t0>),Proj<a2 s1>(Input<t1>))|"     \
	"Exists(Agg<a3 a4 f1 s2 p1>(Input<t2>),Proj<a5 s3>(Input<t3>))|"     \
	"AttrsSub(a0,t0);AttrsSub(a1,t0);AttrsSub(a2,t1);"                    \
	"TableEq(t2,t0);TableEq(t3,t1);AttrsEq(a3,a0);AttrsEq(a4,a1);"       \
	"AttrsEq(a5,a2);PredicateEq(p1,p0);SchemaEq(s2,s0);"                 \
	"SchemaEq(s3,s1);FuncEq(f1,f0)"

#define GPOPT_DSL_NOT_EXISTS_DISTINCT_DROP_RULE                            \
	"NotExists(Input<t0>,Proj*<a0 s0>(Input<t1>))|"                      \
	"NotExists(Input<t2>,Proj<a1 s1>(Input<t3>))|"                       \
	"AttrsSub(a0,t1);TableEq(t2,t0);TableEq(t3,t1);"                     \
	"AttrsEq(a1,a0);SchemaEq(s1,s0)"

#define GPOPT_DSL_PREDICATE_EXISTS_IDENTITY_RULE                         \
	"Exists<p0 a0 a1>(Input<t0>,Input<t1>)|"                            \
	"Exists<p1 a2 a3>(Input<t2>,Input<t3>)|"                            \
	"AttrsSub(a0,t0);AttrsSub(a1,t1);TableEq(t2,t0);TableEq(t3,t1);"   \
	"PredicateEq(p1,p0);AttrsEq(a2,a0);AttrsEq(a3,a1)"

#define GPOPT_DSL_EXPRESSION_DEFINED_EXISTS_RULE                         \
	"Filter<p0 a0>(Input<t0>)|Exists(Input<t1>,Input<t2>)|"             \
	"TableEq(t1,t0);PredicateExists(p0,t2)"

#define GPOPT_DSL_EXPRESSION_DEFINED_NOT_EXISTS_RULE                    \
	"Filter<p0 a0>(Input<t0>)|NotExists(Input<t1>,Input<t2>)|"         \
	"TableEq(t1,t0);PredicateNotExists(p0,t2)"

static GPOS_RESULT
EresSafeFilterMerge()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	BOOL ok = true;
	for (ULONG kind = 0; kind < 6; kind++)
	for (BOOL stale : {false, true})
	{
		const BOOL correlated = 1 == kind;
		const std::string text = std::string(
			"Filter<p0 a0 a1>(Filter<p1 a2 a3>(Input<t0>))|Filter<p2 a4 a5>(Input<t1>)|"
			"t1 := t0;p2 := And(p0,p1);") +
			(stale ? "a4 := a0;" : "AttrsUnion(a4,a0,a2);") + "AttrsUnion(a5,a1,a3);" +
			"AttrsSub(a0,t0);AttrsSub(a2,t0);Deterministic(p0);Deterministic(p1);ErrorFree(p0);ErrorFree(p1)";
		CWStringDynamic error(mp);
		CDSLRule *rule = CDSLRuleParser::PdslruleParse(mp, text.c_str(), "EQ", &error);
		GPOS_UNITTEST_ASSERT(nullptr != rule);
		CColRefArray *cols = nullptr, *outerCols = nullptr;
		CExpression *input = fix.PexprLogicalGet("merge_input", 2, &cols);
		CExpression *outer = fix.PexprLogicalGet("merge_outer", 1, &outerCols);
		CExpression *first = fix.PexprEqPred((*cols)[0], (*cols)[0]);
		CExpression *second = fix.PexprEqPred((*cols)[1], correlated ? (*outerCols)[0] : (*cols)[1]);
		if (2 <= kind)
		{
			second->Release();
			CExpression *on = fix.PexprEqPred((*outerCols)[0], (*cols)[1]);
			CExpression *query = fix.PexprLogicalSelect(outer, on);
			on->Release();
			if (4 == kind)
				query = GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CLogicalLimit(mp, GPOS_NEW(mp) COrderSpec(mp), true, true, false),
					query, CUtils::PexprScalarConstInt8(mp, 0), CUtils::PexprScalarConstInt8(mp, 1));
			COperator *op = 5 == kind ? static_cast<COperator *>(GPOS_NEW(mp) CScalarSubquery(mp, (*outerCols)[0], false, false))
				: 3 == kind ? static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryNotExists(mp))
				: GPOS_NEW(mp) CScalarSubqueryExists(mp);
			second = GPOS_NEW(mp) CExpression(mp, op, query);
			if (5 == kind)
				second = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarNullTest(mp), second);
		}
		CExpression *inner = fix.PexprLogicalSelect(input, second);
		CExpression *source = fix.PexprLogicalSelect(inner, first);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		const BOOL captured = CDSLMatcher(mp, rule).FMatch(rule->PfragSrc()->PopRoot(), source, model);
		const BOOL matched = captured && CDSLConstraintChecker(mp).FCheck(rule, model);
		CDSLInstantiator inst(mp);
		CExpression *target = matched ? inst.PexprInstantiate(rule, model) : nullptr;
		if (!captured || matched != (kind < 4) || ((stale || 4 <= kind) != (nullptr == target)))
			GPOS_TRACE_FORMAT("Filter merge correlated=%d stale=%d matched=%d built=%d", correlated, stale, matched, nullptr != target);
		// Capturing a query is not evidence that reordering it is safe.
		ok &= captured && matched == (kind < 4) && ((stale || 4 <= kind) == (nullptr == target));
		if (2 <= kind && kind < 4 && !stale)
		{
			// The typed rule must retain the legacy rule's safe subquery domain.
			CDSLRule *legacy = CDSLRuleParser::PdslruleParse(mp,
				"Filter<p0 a0 a1>(Filter<p1 a2 a3>(Input<t0>))|Filter<p2 a4 a5>(Input<t1>)|"
				"TableEq(t1,t0);PredicateAnd(p2,p0,p1);AttrsUnion(a4,a0,a2);AttrsUnion(a5,a1,a3);"
				"AttrsSub(a0,t0);AttrsSub(a2,t0);Deterministic(p0);Deterministic(p1);Deterministic(p2);"
				"ErrorFree(p0);ErrorFree(p1);ErrorFree(p2)", "EQ", &error);
			GPOS_UNITTEST_ASSERT(nullptr != legacy);
			CDSLModel *legacyModel = GPOS_NEW(mp) CDSLModel(mp);
			CDSLInstantiator legacyInst(mp);
			const BOOL legacyMatched = CDSLMatcher(mp, legacy).FMatch(legacy->PfragSrc()->PopRoot(), source, legacyModel) &&
				CDSLConstraintChecker(mp).FCheck(legacy, legacyModel);
			CExpression *legacyTarget = legacyMatched ? legacyInst.PexprInstantiate(legacy, legacyModel) : nullptr;
			ok &= nullptr != legacyTarget && nullptr != target && target->Matches(legacyTarget);
			CRefCount::SafeRelease(legacyTarget);
			legacyModel->Release(); legacy->Release();
		}
		if (nullptr != target)
		{
			CExpression *predicate = (*target)[1];
			ok &= COperator::EopLogicalSelect == target->Pop()->Eopid() && (*target)[0] == input &&
				CPredicateUtils::FAnd(predicate) && 2 == predicate->Arity() &&
				(*predicate)[0]->Matches(first) && (*predicate)[1]->Matches(second) &&
				target->DeriveOuterReferences()->Equals(source->DeriveOuterReferences());
		}
		CRefCount::SafeRelease(target);
		model->Release(); source->Release(); inner->Release(); first->Release(); second->Release();
		input->Release(); outer->Release(); rule->Release();
	}
	return ok ? GPOS_OK : GPOS_FAILED;
}

static GPOS_RESULT
EresNestedFilterSplit()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	BOOL ok = true;
	const CHAR *patterns[] = {"And(p0,And(p1,p2))", "And(And(p0,p1),p2)",
		"And(And(p0,p1),And(p2,p3))", "And(p0,And(p1,p2))"};
	for (ULONG shape = 0; shape < GPOS_ARRAY_SIZE(patterns); shape++)
	{
		const ULONG count = 2 == shape ? 4 : 3;
		std::string targetText = "Input<t1>", bindings = "t1 := t0";
		for (ULONG i = count; i-- > 0;)
		{
			const std::string p = "p" + std::to_string(i), n = "n" + std::to_string(i);
			const std::string output = "p" + std::to_string(i + 4), attrs = "a" + std::to_string(i + 1);
			targetText = "Filter<" + output + " " + attrs + ">(" + targetText + ")";
			bindings += ";" + output + " := " + p + ";" + n + " := BoolValue(" + p + ");" +
				attrs + " := ScalarDeps(" + n + ");ErrorFree(" + p + ");Deterministic(" + p + ")";
		}
		const std::string text = std::string("Filter<") + patterns[shape] +
			" a0>(Input<t0>)|" + targetText + "|" + bindings;
		CWStringDynamic error(mp);
		CDSLRule *rule = CDSLRuleParser::PdslruleParse(mp, text.c_str(), "EQ", &error);
		GPOS_UNITTEST_ASSERT(nullptr != rule);
		CColRefArray *cols = nullptr;
		CExpression *input = fix.PexprLogicalGet("nested_filter", count, &cols);
		CExpressionArray *atoms = GPOS_NEW(mp) CExpressionArray(mp);
		for (ULONG i = 0; i < count; i++) atoms->Append(fix.PexprEqPred((*cols)[i], (*cols)[i]));
		auto conjunction = [&](CExpression *left, CExpression *right) {
			left->AddRef(); right->AddRef();
			return GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopAnd), left, right);
		};
		CExpression *predicate = nullptr;
		if (3 == shape)
		{
			// No implicit associativity bridge: a flat native AND is a different
			// source tree, even when its leaves happen to satisfy safety premises.
			atoms->AddRef();
			predicate = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopAnd), atoms);
		}
		else
		{
			CExpression *pair = conjunction((*atoms)[0 == shape ? 1 : 0], (*atoms)[0 == shape ? 2 : 1]);
			CExpression *tail = 2 == shape ? conjunction((*atoms)[2], (*atoms)[3]) : nullptr;
			predicate = 0 == shape ? conjunction((*atoms)[0], pair) :
				conjunction(pair, nullptr == tail ? (*atoms)[2] : tail);
			pair->Release(); CRefCount::SafeRelease(tail);
		}
		CExpression *source = fix.PexprLogicalSelect(input, predicate);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher matcher(mp, rule);
		const BOOL matched = matcher.FMatch(rule->PfragSrc()->PopRoot(), source, model);
		ok &= matched == (3 != shape);
		if (matched)
		{
			ok &= CDSLConstraintChecker(mp).FCheck(rule, model);
			CDSLInstantiator inst(mp);
			CExpression *target = inst.PexprInstantiate(rule, model), *current = target;
			for (ULONG i = 0; i < count && nullptr != current; i++)
			{
				if (COperator::EopLogicalSelect != current->Pop()->Eopid()) { ok = false; break; }
				ok &= (*current)[1]->Matches((*atoms)[i]);
				current = (*current)[0];
			}
			ok &= current == input;
			CRefCount::SafeRelease(target);
		}
		model->Release(); source->Release(); predicate->Release(); atoms->Release(); input->Release(); rule->Release();
	}
	return ok ? GPOS_OK : GPOS_FAILED;
}

static GPOS_RESULT
EresIndependentFilterDependencies()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	BOOL ok = true;
	for (ULONG kind = 0; kind < 3; kind++)
	for (ULONG deps = 0; deps < (2 == kind ? 3 : 4); deps++)
	{
		// Correct independent dependencies, then stale combined and swapped
		// dependencies, plus an invalid Column(two-column vector) constructor.
		// None may bypass exact target validation or fall back to inferred cols.
		const std::string text = std::string(
			"Filter<And(p0,p1) a0>(Input<t0>)|Filter<p2 a1>(Filter<p3 a2>(Input<t1>))|") +
			(0 == kind ? "" : "Exists(t2) := p1;") +
			"t1 := t0;p2 := p0;p3 := p1;n1 := BoolValue(p1);" +
			(3 == deps ? "n0 := Column(a0);" : "n0 := BoolValue(p0);") +
			(0 == deps || 3 == deps ? "a1 := ScalarDeps(n0);a2 := ScalarDeps(n1);" :
			 1 == deps ? "a1 := a0;a2 := a0;" : "a1 := ScalarDeps(n1);a2 := ScalarDeps(n0);") +
			"ErrorFree(p0);ErrorFree(p1);Deterministic(p0);Deterministic(p1)";
		CWStringDynamic error(mp);
		CDSLRule *rule = CDSLRuleParser::PdslruleParse(mp, text.c_str(), "EQ", &error);
		GPOS_UNITTEST_ASSERT(nullptr != rule);
		CColRefArray *lc = nullptr, *rc = nullptr;
		CExpression *left = fix.PexprLogicalGet("deps_left", 2, &lc);
		CExpression *right = fix.PexprLogicalGet("deps_right", 1, &rc);
		CExpression *first = fix.PexprEqPred((*lc)[0], (*lc)[0]);
		CExpression *second = nullptr;
		if (0 == kind)
			second = fix.PexprEqPred((*lc)[1], (*lc)[1]);
		else
		{
			CExpression *on = fix.PexprEqPred((*rc)[0], 1 == kind ? (*lc)[1] : (*rc)[0]);
			CExpression *input = fix.PexprLogicalSelect(right, on);
			on->Release();
			second = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarSubqueryExists(mp), input);
		}
		first->AddRef(); second->AddRef();
		CExpression *both = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopAnd), first, second);
		CExpression *source = fix.PexprLogicalSelect(left, both);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher matcher(mp, rule);
		const BOOL matched = matcher.FMatch(rule->PfragSrc()->PopRoot(), source, model) &&
			CDSLConstraintChecker(mp).FCheck(rule, model);
		CDSLInstantiator inst(mp);
		CExpression *target = matched ? inst.PexprInstantiate(rule, model) : nullptr;
		ok &= matched && ((0 == deps) == (nullptr != target));
		if (nullptr != target)
			ok &= COperator::EopLogicalSelect == target->Pop()->Eopid() &&
				COperator::EopLogicalSelect == (*target)[0]->Pop()->Eopid() &&
				(*(*target)[0])[0] == left && (*target)[1]->Matches(first) &&
				(*(*target)[0])[1]->Matches(second);
		CRefCount::SafeRelease(target);
		model->Release(); source->Release(); both->Release(); first->Release(); second->Release();
		left->Release(); right->Release(); rule->Release();
	}
	return ok ? GPOS_OK : GPOS_FAILED;
}

static GPOS_RESULT
EresExplicitExistentialApply()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	BOOL ok = true;
	for (BOOL negated : {false, true})
	for (BOOL correlated : {false, true})
	{
		const std::string kind = negated ? "NotExists" : "Exists";
		const std::string apply = negated ? "AntiApply" : "SemiApply";
		const std::string text = correlated
			? kind + "(Input<t0>,Filter<p0 a0 a1>(Filter<p1 a2 a3>(Input<t1>)))|" +
			  apply + "<p2 a4 a5 a6>(Input<t2>,Filter<p3 a7 a8>(Input<t3>))|"
			  "t2 := t0;t3 := t1;p2 := p0;p3 := p1;a4 := a1;a5 := a0;a6 := a3;a7 := a2;a8 := a3"
			: kind + "(Input<t0>,Filter<p0 a0 a1>(Input<t1>))|" +
			  apply + "<p1 a2 a3 a4>(Input<t2>,Input<t3>)|"
			  "t2 := t0;t3 := t1;p1 := p0;a2 := a1;a3 := a0;a4 := a1;AttrsEmpty(a1)";
		CWStringDynamic error(mp);
		CDSLRule *rule = CDSLRuleParser::PdslruleParse(mp, text.c_str(), "EQ", &error);
		GPOS_UNITTEST_ASSERT(nullptr != rule);
		CColRefArray *lc = nullptr, *rc = nullptr;
		CExpression *left = fix.PexprLogicalGet("explicit_apply_left", 2, &lc);
		CExpression *right = fix.PexprLogicalGet("explicit_apply_right", 2, &rc);
		CExpression *on = fix.PexprEqPred((*rc)[0], correlated ? (*lc)[0] : (*rc)[1]);
		CExpression *residual = fix.PexprEqPred((*rc)[1], (*lc)[1]);
		CExpression *input = correlated ? fix.PexprLogicalSelect(right, residual) : right;
		if (!correlated) input->AddRef();
		CExpression *filtered = fix.PexprLogicalSelect(input, on);
		COperator *op = negated
			? static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryNotExists(mp))
			: GPOS_NEW(mp) CScalarSubqueryExists(mp);
		CExpression *predicate = GPOS_NEW(mp) CExpression(mp, op, filtered);
		CExpression *source = fix.PexprLogicalSelect(left, predicate);
		CDSLMatcher matcher(mp, rule);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLConstraintChecker checker(mp);
		CDSLInstantiator inst(mp);
		const BOOL matched = matcher.FMatch(rule->PfragSrc()->PopRoot(), source, model) &&
			checker.FCheck(rule, model);
		CExpression *target = matched ? inst.PexprInstantiate(rule, model) : nullptr;
		if (nullptr == target)
		{
			GPOS_TRACE_FORMAT("Explicit Apply negated=%d correlated=%d matched=%d", negated, correlated, matched);
			ok = false;
		}
		else
		{
			CLogicalApply *built = CLogicalApply::PopConvert(target->Pop());
			auto check = [&](BOOL valid, const CHAR *step) {
				if (!valid) GPOS_TRACE_FORMAT("Explicit Apply negated=%d correlated=%d check=%s", negated, correlated, step);
				ok &= valid;
			};
			check((negated ? COperator::EopLogicalLeftAntiSemiApply :
				COperator::EopLogicalLeftSemiApply) == built->Eopid() &&
				(nullptr == built->PdrgPcrInner() || 0 == built->PdrgPcrInner()->Size()) &&
				COperator::EopSentinel == built->EopidOriginSubq() &&
				(*target)[0] == left && (*target)[2]->Matches(on) &&
				source->DeriveOutputColumns()->Equals(target->DeriveOutputColumns()), "operator/left/ON/output");
			check(correlated ? (*(*target)[1])[0] == right && (*(*target)[1])[1]->Matches(residual)
				: (*target)[1] == right, "right input");
			// Ordinary existential Apply has no scalar result metadata to remap.
			UlongToColRefMap *mapping = GPOS_NEW(mp) UlongToColRefMap(mp);
			CExpression *copy = target->PexprCopyWithRemappedColumns(mp, mapping, false);
			check(copy->Matches(target), "copy");
			copy->Release();
			mapping->Release();
			// A newly built ordinary Apply remains consumable by another typed
			// rule; missing scalar-result metadata must not break a rule chain.
			const std::string nextText = apply + "<p0 a0 a1 a2>(Input<t0>,Input<t1>)|" +
				apply + "<Not(Not(p0)) a3 a4 a5>(Input<t2>,Input<t3>)|"
				"t2 := t0;t3 := t1;a3 := a0;a4 := a1;a5 := a2";
			CDSLRule *next = CDSLRuleParser::PdslruleParse(mp, nextText.c_str(), "EQ", &error);
			GPOS_UNITTEST_ASSERT(nullptr != next);
			CDSLModel *nextModel = GPOS_NEW(mp) CDSLModel(mp);
			CDSLMatcher nextMatcher(mp, next);
			CDSLInstantiator nextInst(mp);
			const BOOL nextMatched = nextMatcher.FMatch(next->PfragSrc()->PopRoot(), target, nextModel) &&
				checker.FCheck(next, nextModel);
			CExpression *nextTarget = nextMatched ? nextInst.PexprInstantiate(next, nextModel) : nullptr;
			check(nullptr != nextTarget && nextTarget->Pop()->Matches(target->Pop()), "second rule");
			CRefCount::SafeRelease(nextTarget);
			nextModel->Release(); next->Release();
		}
		CRefCount::SafeRelease(target);
		model->Release(); source->Release(); predicate->Release(); input->Release();
		residual->Release(); on->Release(); right->Release(); left->Release(); rule->Release();
	}
	return ok ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLExistsTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(EresSafeFilterMerge),
		GPOS_UNITTEST_FUNC(EresNestedFilterSplit),
		GPOS_UNITTEST_FUNC(EresIndependentFilterDependencies),
		GPOS_UNITTEST_FUNC(EresExplicitExistentialApply),
		GPOS_UNITTEST_FUNC(
			CDSLExistsTest::EresUnittest_CorpusAggProjRoundTrip),
		GPOS_UNITTEST_FUNC(
			CDSLExistsTest::EresUnittest_PreApplyCorpusAggProjRoundTrip),
		GPOS_UNITTEST_FUNC(
			CDSLExistsTest::EresUnittest_PreApplyPreservesResidual),
		GPOS_UNITTEST_FUNC(
			CDSLExistsTest::EresUnittest_PreApplyNotExistsDistinctDrop),
		GPOS_UNITTEST_FUNC(
			CDSLExistsTest::EresUnittest_PostApplyNotExistsDistinctDrop),
		GPOS_UNITTEST_FUNC(
			CDSLExistsTest::EresUnittest_ExistsPolarityIsolation),
		GPOS_UNITTEST_FUNC(
			CDSLExistsTest::EresUnittest_PredicateSemiJoinRoundTrip),
		GPOS_UNITTEST_FUNC(
			CDSLExistsTest::EresUnittest_ExpressionDefinedExistence),
		GPOS_UNITTEST_FUNC(CDSLExistsTest::EresUnittest_TypedScalarExists)};
	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLExistsTest::EresUnittest_TypedScalarExists()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CWStringDynamic error(mp);
	CDSLRule *rule = CDSLRuleParser::PdslruleParse(mp,
		"Filter<Not(Not(Exists(t1))) a0>(Input<t0>)|"
		"Filter<Exists(t3) a1>(Input<t2>)|t2 := t0;t3 := t1;a1 := a0", "EQ", &error);
	GPOS_ASSERT(nullptr != rule);
	for (ULONG correlated = 0; correlated < 2; ++correlated)
	{
		CColRefArray *outer_cols = nullptr, *inner_cols = nullptr;
		CExpression *outer = fix.PexprLogicalGet("typed_exists_outer", 2, &outer_cols);
		CExpression *inner = fix.PexprLogicalGet("typed_exists_inner", 2, &inner_cols);
		// The TABLE operand can be a query with outer references, not just a Get.
		CExpression *query = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalSelect(mp),
			inner, fix.PexprEqPred((*inner_cols)[0],
				correlated ? (*outer_cols)[0] : (*inner_cols)[1]));
		CExpression *exists = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CScalarSubqueryExists(mp), query);
		CExpression *source = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalSelect(mp),
			outer, CUtils::PexprNegate(mp, CUtils::PexprNegate(mp, exists)));
		CDSLMatcher matcher(mp, rule);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		GPOS_ASSERT(matcher.FMatch(rule->PfragSrc()->PopRoot(), source, model));
		CDSLConstraintChecker checker(mp);
		GPOS_ASSERT(checker.FCheck(rule, model));
		CDSLInstantiator inst(mp);
		CExpression *target = inst.PexprInstantiate(rule, model);
		GPOS_ASSERT(nullptr != target && COperator::EopLogicalSelect == target->Pop()->Eopid());
		GPOS_ASSERT(COperator::EopScalarSubqueryExists == (*target)[1]->Pop()->Eopid());
		GPOS_ASSERT((*(*target)[1])[0] == query);
		GPOS_ASSERT(source->DeriveOutputColumns()->Equals(target->DeriveOutputColumns()));
		// The same typed TABLE capture must also feed a relational target Input.
		// Both polarities retain the complete correlated query, not a synthetic Get.
		for (ULONG variant = 0; variant < 4; ++variant)
		{
			const BOOL negated = 0 != variant % 2;
			const BOOL nested = 2 <= variant;
			const CHAR *text = nested ? (negated
				? "Filter<Not(Not(Not(Exists(t1)))) a0>(Input<t0>)|"
				  "NotExists(Input<t2>,Input<t3>)|t2 := t0;t3 := t1"
				: "Filter<Not(Not(Exists(t1))) a0>(Input<t0>)|"
				  "Exists(Input<t2>,Input<t3>)|t2 := t0;t3 := t1") : (negated
				? "Filter<Not(Exists(t1)) a0>(Input<t0>)|"
				  "NotExists(Input<t2>,Input<t3>)|t2 := t0;t3 := t1"
				: "Filter<Exists(t1) a0>(Input<t0>)|"
				  "Exists(Input<t2>,Input<t3>)|t2 := t0;t3 := t1");
			CDSLRule *bridge = CDSLRuleParser::PdslruleParse(mp, text, "EQ", &error);
			GPOS_ASSERT(nullptr != bridge);
			outer->AddRef();
			exists->AddRef();
			CExpression *predicate = nested
				? CUtils::PexprNegate(mp, CUtils::PexprNegate(mp, exists)) : exists;
			if (negated) predicate = CUtils::PexprNegate(mp, predicate);
			CExpression *bridge_source = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CLogicalSelect(mp), outer, predicate);
			CDSLModel *bridge_model = GPOS_NEW(mp) CDSLModel(mp);
			CDSLMatcher bridge_matcher(mp, bridge);
			GPOS_ASSERT(bridge_matcher.FMatch(bridge->PfragSrc()->PopRoot(), bridge_source, bridge_model));
			GPOS_ASSERT(checker.FCheck(bridge, bridge_model));
			CDSLInstantiator bridge_inst(mp);
			CExpression *bridge_target = bridge_inst.PexprInstantiate(bridge, bridge_model);
			GPOS_ASSERT(nullptr != bridge_target);
			GPOS_ASSERT(COperator::EopLogicalSelect == bridge_target->Pop()->Eopid());
			GPOS_ASSERT((negated ? COperator::EopScalarSubqueryNotExists
				: COperator::EopScalarSubqueryExists) == (*bridge_target)[1]->Pop()->Eopid());
			GPOS_ASSERT((*bridge_target)[0] == outer && (*(*bridge_target)[1])[0] == query);
			GPOS_ASSERT(source->DeriveOutputColumns()->Equals(bridge_target->DeriveOutputColumns()));
			GPOS_ASSERT(checker.FCheck(bridge, bridge_model));
			// Compact native NOT EXISTS and explicit NOT(EXISTS) feed the same
			// typed query capture. This is representation matching, not unnesting.
			if (negated && !nested)
			{
				outer->AddRef();
				query->AddRef();
				CExpression *compact = GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CLogicalSelect(mp), outer,
					GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarSubqueryNotExists(mp), query));
				CDSLModel *compact_model = GPOS_NEW(mp) CDSLModel(mp);
				GPOS_ASSERT(bridge_matcher.FMatch(bridge->PfragSrc()->PopRoot(), compact, compact_model));
				GPOS_ASSERT(checker.FCheck(bridge, compact_model));
				CDSLInstantiator compact_inst(mp);
				CExpression *compact_target = compact_inst.PexprInstantiate(bridge, compact_model);
				GPOS_ASSERT(nullptr != compact_target &&
					COperator::EopLogicalSelect == compact_target->Pop()->Eopid() &&
					(*(*compact_target)[1])[0] == query);
				compact_target->Release();
				compact_model->Release();
				compact->Release();
			}
			bridge_target->Release();
			bridge_model->Release();
			bridge_source->Release();
			bridge->Release();
		}
		std::string exported, export_error;
		GPOS_ASSERT(CDSLPlanTemplate::FSlice(mp, source, "r", {"r/0"}, &exported, &export_error));
		GPOS_ASSERT(std::string::npos != exported.find("Not(Not(Exists("));
		// Negated/native non-subquery inputs must not masquerade as Exists.
		const CDSLSymbol *exists_symbol = nullptr;
		for (ULONG i = 0; i < rule->Pexprdefs()->UlDefinitions(); ++i)
		{
			const auto *def = rule->Pexprdefs()->PdefAt(i);
			if (EdslexprExists == def->Edslexpr() && CDSLExpressionDefinitions::EMatch == def->Binding())
				exists_symbol = def->PsymOutput();
		}
		GPOS_ASSERT(nullptr != exists_symbol);
		CDSLModel *conflict = GPOS_NEW(mp) CDSLModel(mp);
		conflict->FBind(rule->Pexprdefs()->Pdef(exists_symbol)->PsymOperand(0), outer);
		GPOS_ASSERT(!matcher.FMatchExpression(exists_symbol, exists, conflict));
		conflict->Release();
		CDSLRule *opaque = CDSLRuleParser::PdslruleParse(mp,
			"Filter<p0 a0>(Input<t0>)|Filter<Not(Not(p0)) a1>(Input<t1>)|"
			"t1 := t0;a1 := a0", "EQ", &error);
		GPOS_ASSERT(nullptr != opaque);
		CDSLModel *opaque_model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher opaque_matcher(mp, opaque);
		GPOS_ASSERT(opaque_matcher.FMatch(opaque->PfragSrc()->PopRoot(), source, opaque_model));
		CDSLInstantiator opaque_inst(mp);
		CExpression *opaque_target = opaque_inst.PexprInstantiate(opaque, opaque_model);
		GPOS_ASSERT(nullptr != opaque_target && COperator::EopLogicalSelect == opaque_target->Pop()->Eopid());
		// Adding double negation preserves the complete captured predicate, not
		// merely the subquery's base relation or an implicit Apply conversion.
		GPOS_ASSERT((*(*(*opaque_target)[1])[0])[0] == (*source)[1]);
		opaque_target->Release();
		opaque_model->Release();
		opaque->Release();
		query->AddRef();
		CExpression *wrong[] = {
			CUtils::PexprScalarConstBool(mp, false),
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarSubqueryNotExists(mp), query)};
		for (CExpression *value : wrong)
		{
			CDSLModel *rejected = GPOS_NEW(mp) CDSLModel(mp);
			GPOS_ASSERT(!matcher.FMatchExpression(exists_symbol, value, rejected));
			rejected->Release();
			if (COperator::EopScalarSubqueryNotExists == value->Pop()->Eopid())
			{
				CDSLRule *negated = CDSLRuleParser::PdslruleParse(mp,
					"Filter<Not(Exists(t1)) a0>(Input<t0>)|"
					"Filter<Not(Not(Not(Exists(t1)))) a1>(Input<t2>)|t2 := t0;a1 := a0", "EQ", &error);
				GPOS_ASSERT(nullptr != negated);
				outer->AddRef();
				value->AddRef();
				CExpression *negative_source = GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CLogicalSelect(mp), outer, value);
				CDSLModel *negative_model = GPOS_NEW(mp) CDSLModel(mp);
				CDSLMatcher negative_matcher(mp, negated);
				GPOS_ASSERT(negative_matcher.FMatch(negated->PfragSrc()->PopRoot(), negative_source, negative_model));
				CDSLInstantiator negative_inst(mp);
				CExpression *negative_target = negative_inst.PexprInstantiate(negated, negative_model);
				GPOS_ASSERT(nullptr != negative_target);
				CExpression *predicate = (*negative_target)[1];
				for (ULONG i = 0; i < 3; ++i)
				{
					GPOS_ASSERT(CUtils::FScalarBoolOp(predicate, CScalarBoolOp::EboolopNot));
					predicate = (*predicate)[0];
				}
				GPOS_ASSERT(COperator::EopScalarSubqueryExists == predicate->Pop()->Eopid() && (*predicate)[0] == query);
				negative_target->Release();
				negative_model->Release();
				negative_source->Release();
				negated->Release();
			}
			value->Release();
		}
		target->Release();
		model->Release();
		source->Release();
	}
	rule->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLExistsTest::EresUnittest_ExpressionDefinedExistence()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	for (ULONG ul = 0; ul < 2; ul++)
	{
		const BOOL fNegated = 1 == ul;
		CExpression *pexprOuter = fix.PexprLogicalGet(
			fNegated ? "expression_not_exists_outer" : "expression_exists_outer",
			2);
		CExpression *pexprInner = fix.PexprLogicalGet(
			fNegated ? "expression_not_exists_inner" : "expression_exists_inner",
			2);
		COperator *popScalar =
			fNegated
				? static_cast<COperator *>(
					  GPOS_NEW(mp) CScalarSubqueryNotExists(mp))
				: static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryExists(mp));
		CExpression *pexprPredicate =
			GPOS_NEW(mp) CExpression(mp, popScalar, pexprInner);
		CExpression *pexprSource = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CLogicalSelect(mp), pexprOuter, pexprPredicate);

		CWStringDynamic strErr(mp);
		CDSLRule *prule = CDSLRuleParser::PdslruleParse(
			mp, fNegated ? GPOPT_DSL_EXPRESSION_DEFINED_NOT_EXISTS_RULE
							 : GPOPT_DSL_EXPRESSION_DEFINED_EXISTS_RULE,
			"EQ", &strErr);
		GPOS_ASSERT(nullptr != prule);
		CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher matcher(mp);
		GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
								   pmodel));
		CDSLConstraintChecker checker(mp);
		const CDSLConstraint *extraction = (*prule->Pdrgpcon())[1];
		const CDSLSymbol *predicate = (*extraction->Pdrgpsym())[0];
		const CDSLSymbol *input = (*extraction->Pdrgpsym())[1];
		// The legacy check produces a binding, not just a boolean result.
		// Missing/non-subquery captures, opposite polarity and a conflicting
		// prebound input must fail at this constraint without inventing an input.
		for (ULONG shape = 0; shape < 5; ++shape)
		{
			CDSLModel *rejected = GPOS_NEW(mp) CDSLModel(mp);
			if (1 == shape)
			{
				CExpression *value = CUtils::PexprScalarConstBool(mp, true);
				rejected->FBind(predicate, value);
				value->Release();
			}
			else if (2 == shape)
			{
				pexprInner->AddRef();
				COperator *opposite = fNegated
					? static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryExists(mp))
					: static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryNotExists(mp));
				CExpression *value = GPOS_NEW(mp) CExpression(mp, opposite, pexprInner);
				rejected->FBind(predicate, value);
				value->Release();
			}
			else if (3 == shape)
			{
				rejected->FBind(predicate, pexprPredicate);
				rejected->FBind(input, pexprOuter);
			}
			else if (4 == shape)
			{
				COperator *op = fNegated
					? static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryNotExists(mp))
					: static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryExists(mp));
				CExpression *value = GPOS_NEW(mp) CExpression(mp, op,
					CUtils::PexprScalarConstBool(mp, true));
				rejected->FBind(predicate, value);
				value->Release();
			}
			const CDSLConstraint *failed = nullptr;
			ULONG index = gpos::ulong_max;
			GPOS_ASSERT(!checker.FCheck(prule, rejected, &failed, &index));
			GPOS_ASSERT(extraction == failed && 1 == index);
			GPOS_ASSERT(rejected->PexprTable(input) == (3 == shape ? pexprOuter : nullptr));
			rejected->Release();
		}
		GPOS_ASSERT(checker.FCheck(prule, pmodel));
		GPOS_ASSERT(pmodel->PexprTable(input) == pexprInner);

		CDSLInstantiator instantiator(mp);
		CExpression *pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		GPOS_ASSERT(nullptr != pexprTarget);
		GPOS_ASSERT((fNegated ? COperator::EopLogicalLeftAntiSemiApply
							  : COperator::EopLogicalLeftSemiApply) ==
					pexprTarget->Pop()->Eopid());
		GPOS_ASSERT((fNegated ? COperator::EopScalarSubqueryNotExists
							  : COperator::EopScalarSubqueryExists) ==
					dynamic_cast<CLogicalApply *>(pexprTarget->Pop())
						->EopidOriginSubq());
		GPOS_ASSERT(pexprSource->DeriveOutputColumns()->Equals(
			pexprTarget->DeriveOutputColumns()));

		pexprTarget->Release();
		pmodel->Release();
		prule->Release();
		pexprSource->Release();
	}
	return GPOS_OK;
}

GPOS_RESULT
CDSLExistsTest::EresUnittest_PredicateSemiJoinRoundTrip()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CColRefArray *pdrgpcrOuter = nullptr;
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("predicate_exists_outer", 2, &pdrgpcrOuter);
	CExpression *pexprInner =
		fix.PexprLogicalGet("predicate_exists_inner", 2, &pdrgpcrInner);
	CExpression *pexprPred = fix.PexprPredAtom((*pdrgpcrOuter)[1]);
	CExpression *pexprSemiJoin =
		CUtils::PexprLogicalJoin<CLogicalLeftSemiJoin>(
			mp, pexprOuter, pexprInner, pexprPred);

	CWStringDynamic strErr(mp);
	CDSLRule *prule = CDSLRuleParser::PdslruleParse(
		mp, GPOPT_DSL_PREDICATE_EXISTS_IDENTITY_RULE, "EQ", &strErr);
	if (nullptr == prule)
	{
		pexprSemiJoin->Release();
		return GPOS_FAILED;
	}

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSemiJoin, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTarget = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTarget ||
			COperator::EopLogicalLeftSemiJoin != pexprTarget->Pop()->Eopid() ||
			!(*pexprTarget)[2]->Matches((*pexprSemiJoin)[2]))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	prule->Release();
	pexprSemiJoin->Release();
	return eres;
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
	// EXISTS does not observe target-list values, so an ordinary scalar Project
	// is removed before the Limit carrier is constructed.
	GPOS_ASSERT(COperator::EopLogicalGet ==
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

GPOS_RESULT
CDSLExistsTest::EresUnittest_PreApplyCorpusAggProjRoundTrip()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuterGet =
		fix.PexprLogicalGet("exists_preapply_outer", 2, &pdrgpcrOuter);
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrOuter)[0]);
	CColRef *pcrMax = fix.PcrCreateInt4("exists_preapply_max");
	CExpression *pexprAgg = fix.PexprLogicalGbAgg(
		pexprOuterGet, pdrgpcrGroup, pcrMax, (*pdrgpcrOuter)[1]);
	pdrgpcrGroup->Release();

	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprInnerGet =
		fix.PexprLogicalGet("exists_preapply_inner", 2, &pdrgpcrInner);
	CColRefArray *pdrgpcrProjected = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrProjected->Append((*pdrgpcrInner)[0]);
	CExpression *pexprProject =
		fix.PexprLogicalProject(pexprInnerGet, pdrgpcrProjected);
	pdrgpcrProjected->Release();

	CExpression *pexprScalarExists = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarSubqueryExists(mp), pexprProject);
	CExpression *pexprSource = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), pexprAgg, pexprScalarExists);

	CWStringDynamic strErr(mp);
	CDSLRule *prule = CDSLRuleParser::PdslruleParse(
		mp, GPOPT_DSL_CORPUS_EXISTS_AGG_PROJ_RULE, "EQ", &strErr);
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
	GPOS_ASSERT(COperator::EopLogicalLeftSemiApply ==
				pexprTarget->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalGbAgg == (*pexprTarget)[0]->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalLimit == (*pexprTarget)[1]->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalGet ==
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

GPOS_RESULT
CDSLExistsTest::EresUnittest_PreApplyPreservesResidual()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuterGet =
		fix.PexprLogicalGet("exists_residual_outer", 2, &pdrgpcrOuter);
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrOuter)[0]);
	CColRef *pcrMax = fix.PcrCreateInt4("exists_residual_max");
	CExpression *pexprAgg = fix.PexprLogicalGbAgg(
		pexprOuterGet, pdrgpcrGroup, pcrMax, (*pdrgpcrOuter)[1]);
	pdrgpcrGroup->Release();

	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprInnerGet =
		fix.PexprLogicalGet("exists_residual_inner", 2, &pdrgpcrInner);
	CColRefArray *pdrgpcrProjected = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrProjected->Append((*pdrgpcrInner)[0]);
	CExpression *pexprProject =
		fix.PexprLogicalProject(pexprInnerGet, pdrgpcrProjected);
	pdrgpcrProjected->Release();

	CExpression *pexprResidual = fix.PexprPredAtom(pcrMax);
	CExpression *pexprScalarExists = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarSubqueryExists(mp), pexprProject);
	CExpressionArray *pdrgpexprConj = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexprConj->Append(pexprResidual);
	pdrgpexprConj->Append(pexprScalarExists);
	CExpression *pexprPred =
		CPredicateUtils::PexprConjunction(mp, pdrgpexprConj);
	CExpression *pexprSource = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), pexprAgg, pexprPred);

	CWStringDynamic strErr(mp);
	CDSLRule *prule = CDSLRuleParser::PdslruleParse(
		mp, GPOPT_DSL_CORPUS_EXISTS_AGG_PROJ_RULE, "EQ", &strErr);
	GPOS_ASSERT(nullptr != prule);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
							   pmodel));
	// The sibling predicate is part of the matched Agg's HAVING binding,
	// so it survives target construction without an unscoped residual slot.

	CDSLConstraintChecker checker(mp);
	GPOS_ASSERT(checker.FCheck(prule, pmodel));
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget =
		instantiator.PexprInstantiate(prule, pmodel);
	GPOS_ASSERT(nullptr != pexprTarget);
	GPOS_ASSERT(COperator::EopLogicalLeftSemiApply ==
				pexprTarget->Pop()->Eopid());
	// Normalization pushes an outer-only sibling predicate below SemiApply. It
	// must remain attached to the outer Agg rather than being discarded.
	GPOS_ASSERT(COperator::EopLogicalSelect ==
				(*pexprTarget)[0]->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalGbAgg ==
				(*(*pexprTarget)[0])[0]->Pop()->Eopid());
	GPOS_ASSERT((*(*pexprTarget)[0])[1]->Matches(pexprResidual));
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

GPOS_RESULT
CDSLExistsTest::EresUnittest_PreApplyNotExistsDistinctDrop()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("not_exists_pre_outer", 2, &pdrgpcrOuter);
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprInnerGet =
		fix.PexprLogicalGet("not_exists_pre_inner", 2, &pdrgpcrInner);
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrInner)[0]);
	CExpression *pexprDistinct =
		fix.PexprLogicalGbAgg(pexprInnerGet, pdrgpcrGroup);
	pdrgpcrGroup->Release();
	CExpression *pexprNotExists = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarSubqueryNotExists(mp), pexprDistinct);
	CExpression *pexprSource = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), pexprOuter, pexprNotExists);

	CWStringDynamic strErr(mp);
	CDSLRule *prule = CDSLRuleParser::PdslruleParse(
		mp, GPOPT_DSL_NOT_EXISTS_DISTINCT_DROP_RULE, "EQ", &strErr);
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(COperator::EopLogicalLeftAntiSemiApply ==
				prule->EopidSrcRoot());
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
							   pmodel));
	GPOS_ASSERT(pmodel->FDedupDrop());
	CDSLConstraintChecker checker(mp);
	GPOS_ASSERT(checker.FCheck(prule, pmodel));
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget =
		instantiator.PexprInstantiate(prule, pmodel);
	GPOS_ASSERT(nullptr != pexprTarget);
	GPOS_ASSERT(COperator::EopLogicalLeftAntiSemiApply ==
				pexprTarget->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalGet == (*pexprTarget)[1]->Pop()->Eopid());
	GPOS_ASSERT(CUtils::FScalarConstTrue((*pexprTarget)[2]));
	GPOS_ASSERT(COperator::EopScalarSubqueryNotExists ==
				CLogicalApply::PopConvert(pexprTarget->Pop())
					->EopidOriginSubq());

	pexprTarget->Release();
	pmodel->Release();
	prule->Release();
	pexprSource->Release();
	pexprInnerGet->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLExistsTest::EresUnittest_PostApplyNotExistsDistinctDrop()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("not_exists_apply_outer", 2, &pdrgpcrOuter);
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprInnerGet =
		fix.PexprLogicalGet("not_exists_apply_inner", 2, &pdrgpcrInner);
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrInner)[0]);
	CExpression *pexprDistinct =
		fix.PexprLogicalGbAgg(pexprInnerGet, pdrgpcrGroup);
	pdrgpcrGroup->Release();
	CExpression *pexprSource =
		CUtils::PexprLogicalApply<CLogicalLeftAntiSemiApply>(
			mp, pexprOuter, pexprDistinct, (*pdrgpcrInner)[0],
			COperator::EopScalarSubqueryNotExists);

	CWStringDynamic strErr(mp);
	CDSLRule *prule = CDSLRuleParser::PdslruleParse(
		mp, GPOPT_DSL_NOT_EXISTS_DISTINCT_DROP_RULE, "EQ", &strErr);
	GPOS_ASSERT(nullptr != prule);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
							   pmodel));
	GPOS_ASSERT(pmodel->FDedupDrop());
	CDSLConstraintChecker checker(mp);
	GPOS_ASSERT(checker.FCheck(prule, pmodel));
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget =
		instantiator.PexprInstantiate(prule, pmodel);
	GPOS_ASSERT(nullptr != pexprTarget);
	GPOS_ASSERT(COperator::EopLogicalLeftAntiSemiApply ==
				pexprTarget->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalGet == (*pexprTarget)[1]->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopScalarSubqueryNotExists ==
				CLogicalApply::PopConvert(pexprTarget->Pop())
					->EopidOriginSubq());

	pexprTarget->Release();
	pmodel->Release();
	prule->Release();
	pexprSource->Release();
	pexprInnerGet->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLExistsTest::EresUnittest_ExistsPolarityIsolation()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CExpression *pexprOuter = fix.PexprLogicalGet("polarity_outer", 1);
	CExpression *pexprInner = fix.PexprLogicalGet("polarity_inner", 1);
	CExpression *pexprScalarExists = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarSubqueryExists(mp), pexprInner);
	CExpression *pexprPositive = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), pexprOuter, pexprScalarExists);

	CWStringDynamic strErr(mp);
	CDSLRule *pruleNotExists = CDSLRuleParser::PdslruleParse(
		mp,
		"NotExists(Input<t0>,Input<t1>)|NotExists(Input<t2>,Input<t3>)|"
		"TableEq(t2,t0);TableEq(t3,t1)",
		"EQ", &strErr);
	GPOS_ASSERT(nullptr != pruleNotExists);
	CDSLModel *pmodelPositive = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcherNotExists(mp, pruleNotExists);
	GPOS_ASSERT(!matcherNotExists.FMatch(
		pruleNotExists->PfragSrc()->PopRoot(), pexprPositive, pmodelPositive));

	CExpression *pexprOuter2 = fix.PexprLogicalGet("polarity_outer2", 1);
	CExpression *pexprInner2 = fix.PexprLogicalGet("polarity_inner2", 1);
	CExpression *pexprScalarNotExists = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarSubqueryNotExists(mp), pexprInner2);
	CExpression *pexprNegative = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), pexprOuter2,
		pexprScalarNotExists);
	CWStringDynamic strErrExists(mp);
	CDSLRule *pruleExists = CDSLRuleParser::PdslruleParse(
		mp,
		"Exists(Input<t0>,Input<t1>)|Exists(Input<t2>,Input<t3>)|"
		"TableEq(t2,t0);TableEq(t3,t1)",
		"EQ", &strErrExists);
	GPOS_ASSERT(nullptr != pruleExists);
	CDSLModel *pmodelNegative = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcherExists(mp, pruleExists);
	GPOS_ASSERT(!matcherExists.FMatch(pruleExists->PfragSrc()->PopRoot(),
								   pexprNegative, pmodelNegative));

	pmodelNegative->Release();
	pruleExists->Release();
	pexprNegative->Release();
	pmodelPositive->Release();
	pruleNotExists->Release();
	pexprPositive->Release();
	return GPOS_OK;
}

// EOF
