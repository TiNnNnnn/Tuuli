//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLPolicy.cpp
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLPolicy.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace gpopt;

SDSLRulePolicy::SDSLRulePolicy()
	: m_szRuleIdentity{0},
	  m_fEnabled(true),
	  m_edslplacement(EdslplacementCBO),
	  m_edslphase(EdslphaseExplore),
	  m_edsleffect(EdsleffectPreservesJoinGraph),
	  m_iPriority(0),
	  m_edslorder(EdslorderBottomUp),
	  m_fFixpoint(false),
	  m_ulBudgetPerNode(0),
	  m_ulBudgetPerRule(0),
	  m_ulBudgetPerQuery(0)
{
}

namespace
{
std::string
Trim(const std::string &value)
{
	const size_t begin = value.find_first_not_of(" \r\n");
	if (std::string::npos == begin)
	{
		return "";
	}
	const size_t end = value.find_last_not_of(" \r\n");
	return value.substr(begin, end - begin + 1);
}

std::string
Unquote(const std::string &value)
{
	if (2 <= value.size() &&
		(('"' == value.front() && '"' == value.back()) ||
		 ('\'' == value.front() && '\'' == value.back())))
	{
		return value.substr(1, value.size() - 2);
	}
	return value;
}

void
AppendError(CWStringDynamic *pstrErrors, ULONG ulLine,
			const std::string &message)
{
	if (nullptr == pstrErrors)
	{
		return;
	}
	pstrErrors->AppendFormat(GPOS_WSZ_LIT("line %u: "), ulLine);
	pstrErrors->AppendCharArray(message.c_str());
	pstrErrors->AppendCharArray("\n");
}

BOOL
FParseBool(const std::string &value, BOOL *result)
{
	if ("true" == value)
	{
		*result = true;
		return true;
	}
	if ("false" == value)
	{
		*result = false;
		return true;
	}
	return false;
}

BOOL
FParseUnsigned(const std::string &value, ULONG *result)
{
	if (value.empty())
	{
		return false;
	}
	ULLONG number = 0;
	for (CHAR ch : value)
	{
		if (!std::isdigit((unsigned char) ch))
		{
			return false;
		}
		number = number * 10 + (ULLONG) (ch - '0');
		if (number > (ULLONG) gpos::ulong_max)
		{
			return false;
		}
	}
	*result = (ULONG) number;
	return true;
}

BOOL
FParseSigned(const std::string &value, INT *result)
{
	if (value.empty())
	{
		return false;
	}
	size_t pos = ('-' == value[0] || '+' == value[0]) ? 1 : 0;
	if (pos == value.size())
	{
		return false;
	}
	LINT number = 0;
	for (; pos < value.size(); ++pos)
	{
		if (!std::isdigit((unsigned char) value[pos]))
		{
			return false;
		}
		number = number * 10 + (value[pos] - '0');
		if (number > (LINT) gpos::int_max + 1)
		{
			return false;
		}
	}
	if ('-' == value[0])
	{
		number = -number;
	}
	if (number < gpos::int_min || number > gpos::int_max)
	{
		return false;
	}
	*result = (INT) number;
	return true;
}

BOOL
FParseIdentity(std::string value, CHAR result[17])
{
	value = Unquote(value);
	if ("*" == value)
	{
		result[0] = '*';
		result[1] = '\0';
		return true;
	}
	if (16 != value.size())
	{
		return false;
	}
	for (size_t index = 0; index < value.size(); ++index)
	{
		const unsigned char ch = (unsigned char) value[index];
		if (!std::isxdigit(ch))
		{
			return false;
		}
		result[index] = (CHAR) std::tolower(ch);
	}
	result[16] = '\0';
	return true;
}

BOOL
FParseKeyValue(const std::string &text, std::string *key,
			   std::string *value)
{
	const size_t colon = text.find(':');
	if (std::string::npos == colon)
	{
		return false;
	}
	*key = Trim(text.substr(0, colon));
	*value = Trim(text.substr(colon + 1));
	return !key->empty();
}

BOOL
FApplyField(SDSLRulePolicy *entry, const std::string &key,
			const std::string &rawValue, BOOL fBudget, ULONG ulLine,
			CWStringDynamic *pstrErrors)
{
	const std::string value = Unquote(rawValue);
	if (fBudget)
	{
		ULONG *target = nullptr;
		if ("per_node" == key)
		{
			target = &entry->m_ulBudgetPerNode;
		}
		else if ("per_rule" == key)
		{
			target = &entry->m_ulBudgetPerRule;
		}
		else if ("per_query" == key)
		{
			target = &entry->m_ulBudgetPerQuery;
		}
		if (nullptr == target || !FParseUnsigned(value, target))
		{
			AppendError(pstrErrors, ulLine,
						"invalid budget field or non-negative integer");
			return false;
		}
		return true;
	}

	if ("enabled" == key)
	{
		if (!FParseBool(value, &entry->m_fEnabled))
		{
			AppendError(pstrErrors, ulLine, "enabled must be true or false");
			return false;
		}
	}
	else if ("placement" == key)
	{
		if ("rbo" == value)
			entry->m_edslplacement = EdslplacementRBO;
		else if ("cbo" == value)
			entry->m_edslplacement = EdslplacementCBO;
		else if ("auto" == value)
			entry->m_edslplacement = EdslplacementAuto;
		else
		{
			AppendError(pstrErrors, ulLine, "placement must be rbo, cbo, or auto");
			return false;
		}
	}
	else if ("phase" == key)
	{
		if ("normalize" == value)
			entry->m_edslphase = EdslphaseNormalize;
		else if ("pre_join" == value)
			entry->m_edslphase = EdslphasePreJoin;
		else if ("cleanup" == value)
			entry->m_edslphase = EdslphaseCleanup;
		else if ("explore" == value)
			entry->m_edslphase = EdslphaseExplore;
		else
		{
			AppendError(pstrErrors, ulLine, "invalid phase");
			return false;
		}
	}
	else if ("effect" == key)
	{
		if ("changes_join_graph" == value)
			entry->m_edsleffect = EdsleffectChangesJoinGraph;
		else if ("preserves_join_graph" == value)
			entry->m_edsleffect = EdsleffectPreservesJoinGraph;
		else if ("join_reordering" == value)
			entry->m_edsleffect = EdsleffectJoinReordering;
		else
		{
			AppendError(pstrErrors, ulLine, "invalid effect");
			return false;
		}
	}
	else if ("priority" == key)
	{
		if (!FParseSigned(value, &entry->m_iPriority))
		{
			AppendError(pstrErrors, ulLine, "priority must be a 32-bit integer");
			return false;
		}
	}
	else if ("order" == key)
	{
		if ("top_down" == value)
			entry->m_edslorder = EdslorderTopDown;
		else if ("bottom_up" == value)
			entry->m_edslorder = EdslorderBottomUp;
		else
		{
			AppendError(pstrErrors, ulLine, "order must be top_down or bottom_up");
			return false;
		}
	}
	else if ("fixpoint" == key)
	{
		if (!FParseBool(value, &entry->m_fFixpoint))
		{
			AppendError(pstrErrors, ulLine, "fixpoint must be true or false");
			return false;
		}
	}
	else
	{
		AppendError(pstrErrors, ulLine, "unknown policy field: " + key);
		return false;
	}
	return true;
}

SDSLRulePolicy
DefaultPolicy(const CDSLRule *prule)
{
	SDSLRulePolicy policy;
	std::memcpy(policy.m_szRuleIdentity, prule->SzIdentity(), 17);
	return policy;
}
}  // namespace

CDSLPolicy *
CDSLPolicyLoader::PpolicyLoadBuffer(CMemoryPool *mp, const CHAR *szContent,
									CWStringDynamic *pstrErrors)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != szContent);
	CDSLPolicy *policy = GPOS_NEW(mp) CDSLPolicy();
	SDSLRulePolicy current;
	BOOL fHaveEntry = false;
	BOOL fInBudget = false;
	BOOL fValid = true;
	size_t fieldIndent = 0;
	std::unordered_set<std::string> identities;
	std::unordered_set<std::string> fields;

	auto finishEntry = [&]() {
		if (!fHaveEntry)
			return;
		if ('\0' == current.m_szRuleIdentity[0])
		{
			fValid = false;
			return;
		}
		const std::string identity(current.m_szRuleIdentity);
		if (!identities.insert(identity).second)
		{
			fValid = false;
			if (nullptr != pstrErrors)
			{
				pstrErrors->AppendCharArray("duplicate policy rule identity: ");
				pstrErrors->AppendCharArray(identity.c_str());
				pstrErrors->AppendCharArray("\n");
			}
			return;
		}
		if (current.m_fEnabled && EdslplacementRBO == current.m_edslplacement &&
			(fields.end() == fields.find("phase") ||
			 EdslphaseExplore == current.m_edslphase))
		{
			fValid = false;
			if (nullptr != pstrErrors)
				pstrErrors->AppendCharArray(
					"an RBO rule requires normalize, pre_join, or cleanup phase\n");
			return;
		}
		if (current.m_fEnabled && EdslplacementCBO == current.m_edslplacement &&
			EdslphaseExplore != current.m_edslphase)
		{
			fValid = false;
			if (nullptr != pstrErrors)
				pstrErrors->AppendCharArray(
					"a CBO rule must use the explore phase\n");
			return;
		}
		policy->Append(current);
	};

	std::istringstream input(szContent);
	std::string raw;
	ULONG ulLine = 0;
	while (std::getline(input, raw))
	{
		++ulLine;
		if (std::string::npos != raw.find('\t'))
		{
			AppendError(pstrErrors, ulLine, "tabs are not allowed in policy indentation");
			fValid = false;
			continue;
		}
		const size_t comment = raw.find('#');
		if (std::string::npos != comment)
			raw.erase(comment);
		if (Trim(raw).empty())
			continue;

		const size_t indent = raw.find_first_not_of(' ');
		std::string text = raw.substr(indent);
		BOOL fListStart = false;
		if (0 == text.rfind("- ", 0))
		{
			fListStart = true;
			text = Trim(text.substr(2));
		}
		std::string key;
		std::string value;
		if (!FParseKeyValue(text, &key, &value))
		{
			AppendError(pstrErrors, ulLine, "expected key: value");
			fValid = false;
			continue;
		}

		if ("rule" == key)
		{
			finishEntry();
			current = SDSLRulePolicy();
			fHaveEntry = true;
			fInBudget = false;
			fieldIndent = indent + (fListStart ? 2 : 0);
			fields.clear();
			if (!FParseIdentity(value, current.m_szRuleIdentity))
			{
				AppendError(pstrErrors, ulLine,
							"rule must be '*' or a 16-digit hexadecimal canonical identity");
				fValid = false;
			}
			fields.insert("rule");
			continue;
		}
		if (!fHaveEntry || fListStart)
		{
			AppendError(pstrErrors, ulLine, "each policy entry must start with rule");
			fValid = false;
			continue;
		}

		if ("budget" == key)
		{
			if (indent != fieldIndent || !value.empty() ||
				!fields.insert("budget").second)
			{
				AppendError(pstrErrors, ulLine,
							"budget must be a unique, correctly indented mapping");
				fValid = false;
			}
			fInBudget = true;
			continue;
		}
		const BOOL fBudgetName =
			("per_node" == key || "per_rule" == key || "per_query" == key);
		const BOOL fBudgetField =
			fInBudget && fBudgetName && indent == fieldIndent + 2;
		if ((fBudgetName && !fBudgetField) ||
			(!fBudgetName && indent != fieldIndent))
		{
			AppendError(pstrErrors, ulLine, "invalid policy indentation");
			fValid = false;
			continue;
		}
		const std::string fieldKey = fBudgetField ? "budget." + key : key;
		if (!fields.insert(fieldKey).second)
		{
			AppendError(pstrErrors, ulLine, "duplicate policy field: " + fieldKey);
			fValid = false;
			continue;
		}
		if (!fBudgetField)
			fInBudget = false;
		if (!FApplyField(&current, key, value, fBudgetField, ulLine, pstrErrors))
			fValid = false;
	}
	finishEntry();
	if (!fValid)
	{
		policy->Release();
		return nullptr;
	}
	return policy;
}

CDSLPolicy *
CDSLPolicyLoader::PpolicyLoadFile(CMemoryPool *mp, const CHAR *szPath,
								  CWStringDynamic *pstrErrors)
{
	GPOS_ASSERT(nullptr != szPath);
	std::ifstream input(szPath);
	if (!input.is_open())
	{
		if (nullptr != pstrErrors)
		{
			pstrErrors->AppendCharArray("cannot open DSL policy file: ");
			pstrErrors->AppendCharArray(szPath);
		}
		return nullptr;
	}
	std::ostringstream content;
	content << input.rdbuf();
	return PpolicyLoadBuffer(mp, content.str().c_str(), pstrErrors);
}

CDSLPolicySnapshot *
CDSLPolicySnapshot::PsnapshotCompile(CMemoryPool *mp,
									 const CDSLRuleArray *pdrgRules,
									 const CDSLPolicy *ppolicy,
									 CWStringDynamic *pstrErrors)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pdrgRules);
	CDSLPolicySnapshot *snapshot = GPOS_NEW(mp) CDSLPolicySnapshot();
	snapshot->m_fExplicitPolicy = nullptr != ppolicy;
	std::unordered_map<std::string, const SDSLRulePolicy *> configured;
	const SDSLRulePolicy *defaultEntry = nullptr;
	if (nullptr != ppolicy)
	{
		for (const SDSLRulePolicy &entry : ppolicy->Entries())
		{
			if (0 == std::strcmp("*", entry.m_szRuleIdentity))
				defaultEntry = &entry;
			else
				configured.emplace(entry.m_szRuleIdentity, &entry);
		}
	}

	std::unordered_set<std::string> loaded;
	for (ULONG index = 0; index < pdrgRules->Size(); ++index)
	{
		const CDSLRule *rule = (*pdrgRules)[index];
		const std::string identity(rule->SzIdentity());
		loaded.insert(identity);
		SDSLRulePolicy resolved = nullptr == defaultEntry
			? DefaultPolicy(rule)
			: *defaultEntry;
		std::memcpy(resolved.m_szRuleIdentity, rule->SzIdentity(), 17);
		auto configuredIt = configured.find(identity);
		if (configured.end() != configuredIt)
		{
			resolved = *configuredIt->second;
			if (EdslplacementAuto == resolved.m_edslplacement)
			{
				if (EdsleffectChangesJoinGraph == resolved.m_edsleffect)
				{
					resolved.m_edslplacement = EdslplacementRBO;
					if (EdslphaseExplore == resolved.m_edslphase)
						resolved.m_edslphase = EdslphasePreJoin;
				}
				else if (EdsleffectPreservesJoinGraph == resolved.m_edsleffect)
				{
					resolved.m_edslplacement = EdslplacementCBO;
					resolved.m_edslphase = EdslphaseExplore;
				}
				else
				{
					resolved.m_edslplacement = EdslplacementRejected;
				}
			}
			if (EdsleffectJoinReordering == resolved.m_edsleffect)
				resolved.m_edslplacement = EdslplacementRejected;
		}
		const ULONG policyIndex = (ULONG) snapshot->m_policies.size();
		snapshot->m_policies.push_back(resolved);
		snapshot->m_rule_to_policy.emplace(rule, policyIndex);
	}

	BOOL fValid = true;
	for (const auto &configuredEntry : configured)
	{
		if (loaded.end() == loaded.find(configuredEntry.first))
		{
			fValid = false;
			if (nullptr != pstrErrors)
			{
				pstrErrors->AppendCharArray("policy references unknown rule identity: ");
				pstrErrors->AppendCharArray(configuredEntry.first.c_str());
				pstrErrors->AppendCharArray("\n");
			}
		}
	}
	if (!fValid)
	{
		GPOS_DELETE(snapshot);
		return nullptr;
	}

	for (ULONG index = 0; index < pdrgRules->Size(); ++index)
	{
		const CDSLRule *rule = (*pdrgRules)[index];
		const SDSLRulePolicy &resolved = snapshot->Policy(rule);
		if (!resolved.m_fEnabled)
			continue;
		if (EdslplacementCBO == resolved.m_edslplacement)
		{
			snapshot->m_cbo_rules.push_back(rule);
		}
		else if (EdslplacementRBO == resolved.m_edslplacement &&
				 EdslphaseExplore != resolved.m_edslphase)
		{
			snapshot->m_rbo_rules[(ULONG) resolved.m_edslphase].push_back(rule);
		}
	}
	for (ULONG phase = 0; phase < 3; ++phase)
	{
		auto &rules = snapshot->m_rbo_rules[phase];
		// Preserve rule-library order for equal priorities. WeTune's rule bank is
		// insertion ordered, so this also makes a wildcard BottomUp-RBO policy a
		// useful differential oracle while explicit priority remains authoritative.
		std::stable_sort(rules.begin(), rules.end(),
			[snapshot](const CDSLRule *left, const CDSLRule *right) {
				const SDSLRulePolicy &leftPolicy = snapshot->Policy(left);
				const SDSLRulePolicy &rightPolicy = snapshot->Policy(right);
				return leftPolicy.m_iPriority > rightPolicy.m_iPriority;
			});
	}
	return snapshot;
}

const SDSLRulePolicy &
CDSLPolicySnapshot::Policy(const CDSLRule *prule) const
{
	const SDSLRulePolicy *policy = Ppolicy(prule);
	GPOS_ASSERT(nullptr != policy);
	return *policy;
}

const SDSLRulePolicy *
CDSLPolicySnapshot::Ppolicy(const CDSLRule *prule) const
{
	auto found = m_rule_to_policy.find(prule);
	return m_rule_to_policy.end() == found ? nullptr
		: &m_policies[found->second];
}

const std::vector<const CDSLRule *> &
CDSLPolicySnapshot::RboRules(EDslRulePhase phase) const
{
	GPOS_ASSERT(EdslphaseExplore != phase);
	return m_rbo_rules[(ULONG) phase];
}

namespace
{
BOOL
FContainsOperator(const CDSLOp *pop, EDslOpKind edslop)
{
	if (pop->Edslop() == edslop)
		return true;
	for (ULONG child = 0; child < pop->UlChildren(); ++child)
	{
		if (FContainsOperator((*pop)[child], edslop))
			return true;
	}
	return false;
}
}  // namespace

BOOL
CDSLPolicySnapshot::FHasCBOSourceOperator(EDslOpKind edslop) const
{
	for (const CDSLRule *rule : m_cbo_rules)
	{
		if (FContainsOperator(rule->PfragSrc()->PopRoot(), edslop))
			return true;
	}
	return false;
}

const CHAR *
gpopt::SzDSLPlacement(EDslRulePlacement placement)
{
	switch (placement)
	{
		case EdslplacementRBO: return "rbo";
		case EdslplacementCBO: return "cbo";
		case EdslplacementAuto: return "auto";
		case EdslplacementRejected: return "rejected";
	}
	return "invalid";
}

const CHAR *
gpopt::SzDSLPhase(EDslRulePhase phase)
{
	switch (phase)
	{
		case EdslphaseNormalize: return "normalize";
		case EdslphasePreJoin: return "pre_join";
		case EdslphaseCleanup: return "cleanup";
		case EdslphaseExplore: return "explore";
	}
	return "invalid";
}

const CHAR *
gpopt::SzDSLEffect(EDslRuleEffect effect)
{
	switch (effect)
	{
		case EdsleffectChangesJoinGraph: return "changes_join_graph";
		case EdsleffectPreservesJoinGraph: return "preserves_join_graph";
		case EdsleffectJoinReordering: return "join_reordering";
	}
	return "invalid";
}

const CHAR *
gpopt::SzDSLOrder(EDslRuleOrder order)
{
	switch (order)
	{
		case EdslorderTopDown: return "top_down";
		case EdslorderBottomUp: return "bottom_up";
	}
	return "invalid";
}
