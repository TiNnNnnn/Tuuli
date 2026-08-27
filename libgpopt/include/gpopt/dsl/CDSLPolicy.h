//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLPolicy.h
//
//	@doc:
//		External scheduling policy and the immutable per-query snapshot compiled
//		from it.  Policy is keyed by canonical rule identity, never by private
//		rule-file line number.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLPolicy_H
#define GPOPT_CDSLPolicy_H

#include <unordered_map>
#include <vector>

#include "gpos/base.h"
#include "gpos/common/CRefCount.h"
#include "gpos/string/CWStringDynamic.h"

#include "gpopt/dsl/CDSLRuleLoader.h"

namespace gpopt
{
using namespace gpos;

enum EDslRulePlacement
{
	EdslplacementRBO,
	EdslplacementCBO,
	EdslplacementAuto,
	EdslplacementRejected
};

enum EDslRulePhase
{
	EdslphaseNormalize,
	EdslphasePreJoin,
	EdslphaseCleanup,
	EdslphaseExplore
};

enum EDslRuleEffect
{
	EdsleffectChangesJoinGraph,
	EdsleffectPreservesJoinGraph,
	EdsleffectJoinReordering
};

enum EDslRuleOrder
{
	EdslorderTopDown,
	EdslorderBottomUp
};

struct SDSLRulePolicy
{
	CHAR m_szRuleIdentity[17];
	BOOL m_fEnabled;
	EDslRulePlacement m_edslplacement;
	EDslRulePhase m_edslphase;
	EDslRuleEffect m_edsleffect;
	INT m_iPriority;
	EDslRuleOrder m_edslorder;
	BOOL m_fFixpoint;
	ULONG m_ulBudgetPerNode;
	ULONG m_ulBudgetPerRule;
	ULONG m_ulBudgetPerQuery;

	SDSLRulePolicy();
};

// Parsed policy document. Entries retain file order only for diagnostics;
// scheduling order is compiled explicitly by CDSLPolicySnapshot.
class CDSLPolicy : public CRefCount
{
private:
	std::vector<SDSLRulePolicy> m_entries;

public:
	void Append(const SDSLRulePolicy &entry) { m_entries.push_back(entry); }
	const std::vector<SDSLRulePolicy> &Entries() const { return m_entries; }
};

class CDSLPolicyLoader
{
public:
	// Strict YAML-subset parser. A null return means the whole document is
	// invalid; partial policy is never silently activated.
	static CDSLPolicy *PpolicyLoadBuffer(CMemoryPool *mp, const CHAR *szContent,
									  CWStringDynamic *pstrErrors);
	static CDSLPolicy *PpolicyLoadFile(CMemoryPool *mp, const CHAR *szPath,
									CWStringDynamic *pstrErrors);
};

// Immutable query-local binding from every loaded rule pointer to its fully
// resolved policy. Missing policy entries deliberately preserve legacy CBO
// behavior. The global rule library is never mutated.
class CDSLPolicySnapshot
{
private:
	std::vector<SDSLRulePolicy> m_policies;
	std::unordered_map<const CDSLRule *, ULONG> m_rule_to_policy;
	std::vector<const CDSLRule *> m_rbo_rules[3];
	std::vector<const CDSLRule *> m_cbo_rules;
	BOOL m_fExplicitPolicy;

	CDSLPolicySnapshot() : m_fExplicitPolicy(false) {}

public:
	CDSLPolicySnapshot(const CDSLPolicySnapshot &) = delete;

	static CDSLPolicySnapshot *PsnapshotCompile(
		CMemoryPool *mp, const CDSLRuleArray *pdrgRules,
		const CDSLPolicy *ppolicy, CWStringDynamic *pstrErrors);

	const SDSLRulePolicy &Policy(const CDSLRule *prule) const;
	const SDSLRulePolicy *Ppolicy(const CDSLRule *prule) const;
	BOOL FExplicitPolicy() const { return m_fExplicitPolicy; }
	ULONG UlRules() const { return (ULONG) m_policies.size(); }
	const std::vector<const CDSLRule *> &RboRules(EDslRulePhase phase) const;
	const std::vector<const CDSLRule *> &CboRules() const { return m_cbo_rules; }
};

const CHAR *SzDSLPlacement(EDslRulePlacement placement);
const CHAR *SzDSLPhase(EDslRulePhase phase);
const CHAR *SzDSLEffect(EDslRuleEffect effect);
const CHAR *SzDSLOrder(EDslRuleOrder order);

}  // namespace gpopt

#endif  // !GPOPT_CDSLPolicy_H
