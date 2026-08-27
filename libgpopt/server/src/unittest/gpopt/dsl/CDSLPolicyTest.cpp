//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLPolicyTest.h"

#include <cstring>

#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/dsl/CDSLPolicy.h"
#include "gpopt/dsl/CDSLRuleParser.h"

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
}  // namespace

GPOS_RESULT
CDSLPolicyTest::EresUnittest()
{
	CUnittest tests[] = {
		GPOS_UNITTEST_FUNC(CDSLPolicyTest::EresUnittest_CanonicalIdentity),
		GPOS_UNITTEST_FUNC(CDSLPolicyTest::EresUnittest_StrictLoader),
		GPOS_UNITTEST_FUNC(
			CDSLPolicyTest::EresUnittest_SnapshotDefaultsAndAuto),
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
			1 == snapshot->CboRules().size() && second == snapshot->CboRules()[0];
	}
	GPOS_DELETE(snapshot);
	CRefCount::SafeRelease(policy);
	rules->Release();
	return valid ? GPOS_OK : GPOS_FAILED;
}
