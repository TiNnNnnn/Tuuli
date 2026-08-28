//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLPolicyTest.h"

#include <cstring>
#include <string>

#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/dsl/CDSLPolicy.h"
#include "gpopt/dsl/CDSLRewriteProgram.h"
#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

namespace
{
CDSLRule *
Parse(CMemoryPool *mp, const CHAR *text, const CHAR *verdict = "EQ")
{
	CWStringDynamic errors(mp);
	return CDSLRuleParser::PdslruleParse(mp, text, verdict, &errors);
}

CDSLPolicy *
LoadPolicyForRule(CMemoryPool *mp, const CDSLRule *rule,
				  CWStringDynamic *errors)
{
	CWStringDynamic text(mp);
	text.AppendCharArray("- rule: ");
	text.AppendCharArray(rule->SzIdentity());
	text.AppendCharArray(
		"\n  enabled: true\n  placement: auto\n  phase: explore\n"
		"  effect: changes_join_graph\n  priority: 42\n"
		"  order: top_down\n  fixpoint: true\n  budget:\n"
		"    per_node: 2\n    per_rule: 7\n    per_query: 11\n");
	// Policy text is ASCII; CWStringDynamic is converted explicitly to avoid
	// locale-dependent wide-to-multibyte conversion in the test.
	CHAR *buffer = GPOS_NEW_ARRAY(mp, CHAR, text.Length() + 1);
	for (ULONG index = 0; index < text.Length(); ++index)
		buffer[index] = (CHAR) text.GetBuffer()[index];
	buffer[text.Length()] = '\0';
	CDSLPolicy *policy =
		CDSLPolicyLoader::PpolicyLoadBuffer(mp, buffer, errors);
	GPOS_DELETE_ARRAY(buffer);
	return policy;
}

CDSLPolicy *
LoadRboPolicy(CMemoryPool *mp, const CDSLRule *rule, const CHAR *order,
			  BOOL fixpoint, ULONG perRule, CWStringDynamic *errors)
{
	std::string text("- rule: ");
	text.append(rule->SzIdentity());
	text.append(
		"\n  enabled: true\n  placement: rbo\n  phase: cleanup\n"
		"  effect: preserves_join_graph\n  priority: 100\n  order: ");
	text.append(order);
	text.append("\n  fixpoint: ");
	text.append(fixpoint ? "true" : "false");
	text.append(
		"\n  budget:\n    per_node: 0\n    per_rule: ");
	text.append(std::to_string(perRule));
	text.append("\n    per_query: 0\n");
	return CDSLPolicyLoader::PpolicyLoadBuffer(mp, text.c_str(), errors);
}
}  // namespace

GPOS_RESULT
CDSLPolicyTest::EresUnittest()
{
	CUnittest tests[] = {
		GPOS_UNITTEST_FUNC(CDSLPolicyTest::EresUnittest_CanonicalIdentity),
		GPOS_UNITTEST_FUNC(CDSLPolicyTest::EresUnittest_StrictLoader),
		GPOS_UNITTEST_FUNC(
			CDSLPolicyTest::EresUnittest_SnapshotDefaultsAndAuto),
		GPOS_UNITTEST_FUNC(CDSLPolicyTest::EresUnittest_WildcardDefaults),
		GPOS_UNITTEST_FUNC(CDSLPolicyTest::EresUnittest_RewriteProgram),
	};
	return CUnittest::EresExecute(tests, GPOS_ARRAY_SIZE(tests));
}

GPOS_RESULT
CDSLPolicyTest::EresUnittest_CanonicalIdentity()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLRule *first = Parse(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|TableEq(t1,t0)", "EQ");
	CDSLRule *spaced = Parse(
		mp, "Filter<p0 a0>(Input<t0>) | Input<t1> | TableEq(t1, t0)",
		"UNKNOWN");
	CDSLRule *different = Parse(
		mp,
		"Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
		"TableEq(t1,t0);AttrsEq(a1,a0);PredicateEq(p1,p0)");
	const BOOL valid = nullptr != first && nullptr != spaced &&
		nullptr != different && 16 == std::strlen(first->SzIdentity()) &&
		0 == std::strcmp("dab83b15370d0c10", first->SzIdentity()) &&
		0 == std::strcmp(first->SzIdentity(), spaced->SzIdentity()) &&
		0 != std::strcmp(first->SzIdentity(), different->SzIdentity());
	CRefCount::SafeRelease(first);
	CRefCount::SafeRelease(spaced);
	CRefCount::SafeRelease(different);
	return valid ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLPolicyTest::EresUnittest_RewriteProgram()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fixture(mp);
	CDSLRuleEngine *engine = CDSLRuleEngine::Instance();
	CDSLRule *rule = Parse(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|TableEq(t1,t0)");
	if (nullptr == engine || nullptr == rule)
	{
		CRefCount::SafeRelease(rule);
		return GPOS_FAILED;
	}
	CDSLRuleArray *rules = GPOS_NEW(mp) CDSLRuleArray(mp);
	rules->Append(rule);
	CWStringDynamic errors(mp);

	auto buildNestedSelect = [&]() -> CExpression * {
		CColRefArray *columns = nullptr;
		CExpression *get = fixture.PexprLogicalGet("rbo_t", 2, &columns);
		CExpression *innerPredicate = fixture.PexprPredAtom((*columns)[0]);
		CExpression *inner =
			fixture.PexprLogicalSelect(get, innerPredicate);
		innerPredicate->Release();
		CExpression *outerPredicate = fixture.PexprPredAtom((*columns)[1]);
		CExpression *outer =
			fixture.PexprLogicalSelect(inner, outerPredicate);
		outerPredicate->Release();
		inner->Release();
		get->Release();
		return outer;
	};

	BOOL valid = true;
	for (const CHAR *order : {"top_down", "bottom_up"})
	{
		errors.Reset();
		CDSLPolicy *policy =
			LoadRboPolicy(mp, rule, order, true, 0, &errors);
		CDSLPolicySnapshot *snapshot = nullptr == policy
			? nullptr
			: CDSLPolicySnapshot::PsnapshotCompile(mp, rules, policy, &errors);
		CExpression *source = buildNestedSelect();
		if (nullptr == snapshot)
		{
			valid = false;
		}
		else
		{
			CDSLRewriteProgram program(mp, engine, snapshot);
			CExpression *result = program.PexprRewrite(source);
			valid = valid && COperator::EopLogicalGet == result->Pop()->Eopid() &&
				2 == program.UlApplications() &&
				result->DeriveOutputColumns()->Equals(
					source->DeriveOutputColumns());
			result->Release();
		}
		source->Release();
		GPOS_DELETE(snapshot);
		CRefCount::SafeRelease(policy);
	}

	// A per-rule budget of one must retain the second Select instead of silently
	// treating a skipped rewrite as a rejection.
	errors.Reset();
	CDSLPolicy *budgetPolicy =
		LoadRboPolicy(mp, rule, "top_down", true, 1, &errors);
	CDSLPolicySnapshot *budgetSnapshot = nullptr == budgetPolicy
		? nullptr
		: CDSLPolicySnapshot::PsnapshotCompile(
			mp, rules, budgetPolicy, &errors);
	CExpression *budgetSource = buildNestedSelect();
	if (nullptr == budgetSnapshot)
	{
		valid = false;
	}
	else
	{
		CDSLRewriteProgram program(mp, engine, budgetSnapshot);
		CExpression *result = program.PexprRewrite(budgetSource);
		valid = valid &&
			COperator::EopLogicalSelect == result->Pop()->Eopid() &&
			1 == program.UlApplications();
		result->Release();
	}
	budgetSource->Release();
	GPOS_DELETE(budgetSnapshot);
	CRefCount::SafeRelease(budgetPolicy);

	// InnerJoin commutativity recreates the exact ancestor expression on its
	// second application, so lineage detection must reject it before the hard
	// budget is involved.
	CDSLRule *swapRule = Parse(
		mp,
		"InnerJoin<a0 a1>(Input<t0>,Input<t1>)|"
		"InnerJoin<a3 a2>(Input<t3>,Input<t2>)|"
		"TableEq(t2,t0);TableEq(t3,t1);"
		"AttrsEq(a2,a0);AttrsEq(a3,a1)");
	if (nullptr == swapRule)
	{
		valid = false;
	}
	else
	{
		CDSLRuleArray *swapRules = GPOS_NEW(mp) CDSLRuleArray(mp);
		swapRules->Append(swapRule);
		errors.Reset();
		CDSLPolicy *swapPolicy =
			LoadRboPolicy(mp, swapRule, "top_down", true, 0, &errors);
		CDSLPolicySnapshot *swapSnapshot = nullptr == swapPolicy
			? nullptr
			: CDSLPolicySnapshot::PsnapshotCompile(
				mp, swapRules, swapPolicy, &errors);
		CColRefArray *leftColumns = nullptr;
		CColRefArray *rightColumns = nullptr;
		CExpression *left =
			fixture.PexprLogicalGet("swap_left", 2, &leftColumns);
		CExpression *right =
			fixture.PexprLogicalGet("swap_right", 2, &rightColumns);
		CExpression *joinPredicate = fixture.PexprEqPred(
			(*leftColumns)[0], (*rightColumns)[0]);
		CExpression *swapSource = fixture.PexprLogicalInnerJoin(
			left, right, joinPredicate);
		joinPredicate->Release();
		left->Release();
		right->Release();
		if (nullptr == swapSnapshot)
		{
			valid = false;
		}
		else
		{
			CDSLRewriteProgram program(mp, engine, swapSnapshot, nullptr, 8, 32);
			CExpression *result = program.PexprRewrite(swapSource);
			valid = valid && 1 == program.UlApplications() &&
				!program.FHardBudgetExhausted() &&
				!result->Matches(swapSource);
			result->Release();
		}
		swapSource->Release();
		GPOS_DELETE(swapSnapshot);
		CRefCount::SafeRelease(swapPolicy);
		swapRules->Release();
	}

	// Missing placement remains a true no-op and keeps object identity.
	CDSLPolicySnapshot *defaultSnapshot = CDSLPolicySnapshot::PsnapshotCompile(
		mp, rules, nullptr, &errors);
	CExpression *defaultSource = buildNestedSelect();
	if (nullptr == defaultSnapshot)
	{
		valid = false;
	}
	else
	{
		CDSLRewriteProgram program(mp, engine, defaultSnapshot);
		CExpression *result = program.PexprRewrite(defaultSource);
		valid = valid && result == defaultSource && 0 == program.UlApplications();
		result->Release();
	}
	defaultSource->Release();
	GPOS_DELETE(defaultSnapshot);
	rules->Release();
	return valid ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLPolicyTest::EresUnittest_StrictLoader()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CWStringDynamic errors(mp);
	const CHAR *invalidUnknown =
		"rule: 0123456789abcdef\nplacement: cbo\nunknown: value\n";
	CDSLPolicy *policy = CDSLPolicyLoader::PpolicyLoadBuffer(
		mp, invalidUnknown, &errors);
	if (nullptr != policy)
	{
		policy->Release();
		return GPOS_FAILED;
	}
	errors.Reset();
	const CHAR *invalidIndent =
		"- rule: 0123456789abcdef\n  budget:\n  per_node: 1\n";
	policy = CDSLPolicyLoader::PpolicyLoadBuffer(mp, invalidIndent, &errors);
	if (nullptr != policy)
	{
		policy->Release();
		return GPOS_FAILED;
	}
	errors.Reset();
	const CHAR *invalidRbo =
		"rule: 0123456789abcdef\nplacement: rbo\nphase: explore\n";
	policy = CDSLPolicyLoader::PpolicyLoadBuffer(mp, invalidRbo, &errors);
	if (nullptr != policy)
	{
		policy->Release();
		return GPOS_FAILED;
	}
	return GPOS_OK;
}

GPOS_RESULT
CDSLPolicyTest::EresUnittest_SnapshotDefaultsAndAuto()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLRule *first = Parse(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|TableEq(t1,t0)");
	CDSLRule *second = Parse(
		mp, "Proj<a0 s0>(Input<t0>)|Input<t1>|TableEq(t1,t0)");
	if (nullptr == first || nullptr == second)
	{
		CRefCount::SafeRelease(first);
		CRefCount::SafeRelease(second);
		return GPOS_FAILED;
	}
	CDSLRuleArray *rules = GPOS_NEW(mp) CDSLRuleArray(mp);
	rules->Append(first);
	rules->Append(second);
	CWStringDynamic errors(mp);
	CDSLPolicy *policy = LoadPolicyForRule(mp, first, &errors);
	CDSLPolicySnapshot *snapshot = nullptr == policy
		? nullptr
		: CDSLPolicySnapshot::PsnapshotCompile(mp, rules, policy, &errors);
	BOOL valid = nullptr != snapshot;
	if (valid)
	{
		const SDSLRulePolicy &configured = snapshot->Policy(first);
		const SDSLRulePolicy &fallback = snapshot->Policy(second);
		valid = EdslplacementRBO == configured.m_edslplacement &&
			EdslphasePreJoin == configured.m_edslphase &&
			42 == configured.m_iPriority && EdslorderTopDown == configured.m_edslorder &&
			configured.m_fFixpoint && 2 == configured.m_ulBudgetPerNode &&
			7 == configured.m_ulBudgetPerRule && 11 == configured.m_ulBudgetPerQuery &&
			EdslplacementCBO == fallback.m_edslplacement &&
			EdslphaseExplore == fallback.m_edslphase &&
			1 == snapshot->RboRules(EdslphasePreJoin).size() &&
			first == snapshot->RboRules(EdslphasePreJoin)[0] &&
			1 == snapshot->CboRules().size() && second == snapshot->CboRules()[0] &&
			!snapshot->FHasCBOSourceOperator(EdslopFilter) &&
			snapshot->FHasCBOSourceOperator(EdslopProj);
	}
	GPOS_DELETE(snapshot);
	CRefCount::SafeRelease(policy);
	rules->Release();
	return valid ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLPolicyTest::EresUnittest_WildcardDefaults()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLRule *first = Parse(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|TableEq(t1,t0)");
	CDSLRule *second = Parse(
		mp, "Proj<a0 s0>(Input<t0>)|Input<t1>|TableEq(t1,t0)");
	if (nullptr == first || nullptr == second)
	{
		CRefCount::SafeRelease(first);
		CRefCount::SafeRelease(second);
		return GPOS_FAILED;
	}

	CDSLRuleArray *rules = GPOS_NEW(mp) CDSLRuleArray(mp);
	rules->Append(first);
	rules->Append(second);
	std::string text(
		"- rule: '*'\n"
		"  enabled: true\n"
		"  placement: rbo\n"
		"  phase: pre_join\n"
		"  effect: changes_join_graph\n"
		"  priority: 0\n"
		"  order: bottom_up\n"
		"  fixpoint: true\n"
		"- rule: ");
	text.append(second->SzIdentity());
	text.append("\n  enabled: false\n");

	CWStringDynamic errors(mp);
	CDSLPolicy *policy =
		CDSLPolicyLoader::PpolicyLoadBuffer(mp, text.c_str(), &errors);
	CDSLPolicySnapshot *snapshot = nullptr == policy
		? nullptr
		: CDSLPolicySnapshot::PsnapshotCompile(mp, rules, policy, &errors);
	BOOL valid = nullptr != snapshot;
	if (valid)
	{
		const SDSLRulePolicy &wildcard = snapshot->Policy(first);
		const SDSLRulePolicy &disabled = snapshot->Policy(second);
		valid = snapshot->FExplicitPolicy() &&
			EdslplacementRBO == wildcard.m_edslplacement &&
			EdslphasePreJoin == wildcard.m_edslphase &&
			EdsleffectChangesJoinGraph == wildcard.m_edsleffect &&
			EdslorderBottomUp == wildcard.m_edslorder &&
			wildcard.m_fFixpoint && !disabled.m_fEnabled &&
			1 == snapshot->RboRules(EdslphasePreJoin).size() &&
			first == snapshot->RboRules(EdslphasePreJoin)[0] &&
			snapshot->CboRules().empty();
	}

	GPOS_DELETE(snapshot);
	CRefCount::SafeRelease(policy);
	rules->Release();
	return valid ? GPOS_OK : GPOS_FAILED;
}
