//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLRewriteProgram.h
//
//	@doc:
//		Deterministic, memo-free scheduler for rules placed in DSL RBO phases.
//		It rewrites CExpression directly with copy-on-write ancestor rebuilding;
//		all matching, constraints and target construction remain in the shared
//		CDSLRuleEngine evaluation kernel.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLRewriteProgram_H
#define GPOPT_CDSLRewriteProgram_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "gpos/base.h"

#include "gpopt/dsl/CDSLPolicy.h"
#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

class CDSLRewriteProgram
{
private:
	using Path = std::vector<ULONG>;

	struct SWorkItem
	{
		Path m_path;
		BOOL m_fChildrenVisited;

		SWorkItem(const Path &path, BOOL fChildrenVisited)
			: m_path(path), m_fChildrenVisited(fChildrenVisited)
		{
		}
	};

	struct SFiredEntry
	{
		const CDSLRule *m_prule;
		ULONG m_ulFingerprint;
		CExpression *m_pexprSource;

		SFiredEntry(const CDSLRule *prule, ULONG ulFingerprint,
					CExpression *pexprSource)
			: m_prule(prule),
			  m_ulFingerprint(ulFingerprint),
			  m_pexprSource(pexprSource)
		{
		}
	};

	CMemoryPool *m_mp;
	const CDSLRuleEngine *m_pengine;
	const CDSLPolicySnapshot *m_psnapshot;
	std::vector<SFiredEntry> m_fired;
	std::unordered_map<std::string, std::vector<CExpression *> > m_lineages;
	std::unordered_map<const CDSLRule *, ULONG> m_ruleApplications;
	std::unordered_map<std::string, ULONG> m_nodeRuleApplications;
	std::unordered_set<std::string> m_nonFixpointApplications;
	ULONG m_ulApplications;
	ULONG m_ulAddedNodes;
	ULONG m_ulHardSteps;
	ULONG m_ulHardAddedNodes;
	BOOL m_fHardBudgetExhausted;

	static std::string StrPath(const Path &path);
	static std::string StrNodeRuleKey(const Path &path,
									 const CDSLRule *prule);
	static CExpression *PexprResolve(CExpression *pexprRoot,
									 const Path &path);
	CExpression *PexprReplace(CExpression *pexprRoot, const Path &path,
							  CExpression *pexprReplacement) const;
	static ULONG UlNodes(CExpression *pexpr);
	const CHAR *SzSafetyFailure(const SDSLRulePolicy &policy,
								 CExpression *pexprSource,
								 CExpression *pexprTarget) const;
	BOOL FFired(const CDSLRule *prule, ULONG ulFingerprint,
				CExpression *pexprSource) const;
	void RecordFired(const CDSLRule *prule, ULONG ulFingerprint,
					 CExpression *pexprSource);
	BOOL FLineageContains(const Path &path, CExpression *pexpr) const;
	void RecordLineage(const Path &path, CExpression *pexpr);
	BOOL FCandidate(const CDSLRuleArray *pdrgCandidates,
					const CDSLRule *prule) const;
	BOOL FBudgetAvailable(const Path &path, const CDSLRule *prule,
					  const SDSLRulePolicy &policy) const;
	void ReserveBudget(const Path &path, const CDSLRule *prule,
					   const SDSLRulePolicy &policy, ULONG ulAddedNodes);

	// Try candidates of one traversal cohort at a node. On success ownership of
	// one target reference is returned through ppexprTarget.
	BOOL FApplyAtNode(EDslRulePhase phase, EDslRuleOrder order,
					  const Path &path, CExpression *pexprSource,
					  CExpression **ppexprTarget);
	BOOL FRunTopDown(EDslRulePhase phase, CExpression **ppexprRoot);
	BOOL FRunBottomUp(EDslRulePhase phase, CExpression **ppexprRoot);
	void RunPhase(EDslRulePhase phase, CExpression **ppexprRoot);

public:
	CDSLRewriteProgram(const CDSLRewriteProgram &) = delete;

	CDSLRewriteProgram(CMemoryPool *mp, const CDSLRuleEngine *pengine,
					   const CDSLPolicySnapshot *psnapshot,
					   ULONG ulHardSteps = 10000,
					   ULONG ulHardAddedNodes = 100000);
	~CDSLRewriteProgram();

	// Caller retains pexpr and owns the returned reference. With no RBO rules
	// this is an exact AddRef pass-through.
	CExpression *PexprRewrite(CExpression *pexpr);

	ULONG UlApplications() const { return m_ulApplications; }
	ULONG UlAddedNodes() const { return m_ulAddedNodes; }
	BOOL FHardBudgetExhausted() const { return m_fHardBudgetExhausted; }
};
}  // namespace gpopt

#endif  // !GPOPT_CDSLRewriteProgram_H
