//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLRewriteProgram.cpp
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLRewriteProgram.h"

#include "gpopt/base/COptCtxt.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

CDSLRewriteProgram::CDSLRewriteProgram(
	CMemoryPool *mp, const CDSLRuleEngine *pengine,
	const CDSLPolicySnapshot *psnapshot, const CColRefSet *pcrsRequired,
	ULONG ulHardSteps,
	ULONG ulHardAddedNodes)
	: m_mp(mp),
	  m_pengine(pengine),
	  m_psnapshot(psnapshot),
	  m_pcrsRequired(pcrsRequired),
	  m_ulApplications(0),
	  m_ulApplicableAlternatives(0),
	  m_ulAddedNodes(0),
	  m_ulHardSteps(ulHardSteps),
	  m_ulHardAddedNodes(ulHardAddedNodes),
	  m_fHardBudgetExhausted(false)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pengine);
	GPOS_ASSERT(nullptr != psnapshot);
}

CDSLRewriteProgram::~CDSLRewriteProgram()
{
	for (SFiredEntry &entry : m_fired)
		entry.m_pexprSource->Release();
	for (auto &lineage : m_lineages)
	{
		for (CExpression *pexpr : lineage.second)
			pexpr->Release();
	}
}

std::string
CDSLRewriteProgram::StrPath(const Path &path)
{
	std::string result("r");
	for (ULONG child : path)
	{
		result.append("/");
		result.append(std::to_string(child));
	}
	return result;
}

std::string
CDSLRewriteProgram::StrNodeRuleKey(const Path &path, const CDSLRule *prule)
{
	std::string result = StrPath(path);
	result.append("@");
	result.append(prule->SzIdentity());
	return result;
}

CExpression *
CDSLRewriteProgram::PexprResolve(CExpression *pexprRoot, const Path &path)
{
	CExpression *pexpr = pexprRoot;
	for (ULONG child : path)
	{
		if (nullptr == pexpr || child >= pexpr->Arity())
			return nullptr;
		pexpr = (*pexpr)[child];
	}
	return pexpr;
}

CExpression *
CDSLRewriteProgram::PexprReplace(CExpression *pexprRoot, const Path &path,
								 CExpression *pexprReplacement) const
{
	GPOS_ASSERT(nullptr != pexprRoot);
	GPOS_ASSERT(nullptr != pexprReplacement);
	if (path.empty())
		return pexprReplacement;

	std::vector<CExpression *> ancestors;
	ancestors.reserve(path.size());
	CExpression *pexpr = pexprRoot;
	for (ULONG child : path)
	{
		if (child >= pexpr->Arity())
		{
			pexprReplacement->Release();
			return nullptr;
		}
		ancestors.push_back(pexpr);
		pexpr = (*pexpr)[child];
	}

	CExpression *pexprChild = pexprReplacement;
	for (ULONG depth = (ULONG) path.size(); 0 < depth; --depth)
	{
		CExpression *pexprParent = ancestors[depth - 1];
		const ULONG replacedChild = path[depth - 1];
		CExpressionArray *children =
			GPOS_NEW(m_mp) CExpressionArray(m_mp, pexprParent->Arity());
		for (ULONG child = 0; child < pexprParent->Arity(); ++child)
		{
			if (child == replacedChild)
			{
				children->Append(pexprChild);
			}
			else
			{
				(*pexprParent)[child]->AddRef();
				children->Append((*pexprParent)[child]);
			}
		}
		pexprParent->Pop()->AddRef();
		pexprChild = GPOS_NEW(m_mp)
			CExpression(m_mp, pexprParent->Pop(), children);
	}
	return pexprChild;
}

ULONG
CDSLRewriteProgram::UlNodes(CExpression *pexpr)
{
	ULONG count = 0;
	std::vector<CExpression *> stack;
	stack.push_back(pexpr);
	while (!stack.empty())
	{
		CExpression *current = stack.back();
		stack.pop_back();
		++count;
		for (ULONG child = 0; child < current->Arity(); ++child)
			stack.push_back((*current)[child]);
	}
	return count;
}

const CHAR *
CDSLRewriteProgram::SzSafetyFailure(const SDSLRulePolicy &policy,
									CExpression *pexprSource,
									CExpression *pexprTarget) const
{
	if (!pexprSource->Pop()->FLogical() || !pexprTarget->Pop()->FLogical())
		return "non_logical_boundary";
	if (!pexprSource->DeriveOuterReferences()->ContainsAll(
			pexprTarget->DeriveOuterReferences()))
		return "outer_references_expanded";
	COptCtxt *poctxt = COptCtxt::PoctxtFromTLS();
	if (nullptr != poctxt && poctxt->FDMLQuery())
		return "dml_boundary";
	if (nullptr != poctxt && poctxt->HasVolatileFunc())
		return "volatile_query";
	if (EdsleffectChangesJoinGraph == policy.m_edsleffect &&
		EdslphasePreJoin != policy.m_edslphase)
		return "join_graph_change_outside_pre_join";
	return nullptr;
}

const CHAR *
CDSLRewriteProgram::SzRootSafetyFailure(CExpression *pexprSourceRoot,
									CExpression *pexprTargetRoot) const
{
	if (nullptr != m_pcrsRequired &&
		!pexprTargetRoot->DeriveOutputColumns()->ContainsAll(m_pcrsRequired))
		return "required_output_columns_removed";
	if (!pexprSourceRoot->DeriveOuterReferences()->ContainsAll(
			pexprTargetRoot->DeriveOuterReferences()))
		return "root_outer_references_expanded";
	return nullptr;
}

BOOL
CDSLRewriteProgram::FFired(const CDSLRule *prule, ULONG ulFingerprint,
							 CExpression *pexprSource) const
{
	for (const SFiredEntry &entry : m_fired)
	{
		if (entry.m_prule == prule && entry.m_ulFingerprint == ulFingerprint &&
			entry.m_pexprSource->Matches(pexprSource))
			return true;
	}
	return false;
}

void
CDSLRewriteProgram::RecordFired(const CDSLRule *prule, ULONG ulFingerprint,
								CExpression *pexprSource)
{
	pexprSource->AddRef();
	m_fired.emplace_back(prule, ulFingerprint, pexprSource);
}

BOOL
CDSLRewriteProgram::FLineageContains(const Path &path,
									 CExpression *pexpr) const
{
	auto found = m_lineages.find(StrPath(path));
	if (m_lineages.end() == found)
		return false;
	const ULONG fingerprint = CExpression::HashValue(pexpr);
	for (CExpression *previous : found->second)
	{
		if (CExpression::HashValue(previous) == fingerprint &&
			previous->Matches(pexpr))
			return true;
	}
	return false;
}

void
CDSLRewriteProgram::RecordLineage(const Path &path, CExpression *pexpr)
{
	if (FLineageContains(path, pexpr))
		return;
	pexpr->AddRef();
	m_lineages[StrPath(path)].push_back(pexpr);
}

BOOL
CDSLRewriteProgram::FCandidate(const CDSLRuleArray *pdrgCandidates,
								   const CDSLRule *prule) const
{
	// Programmatically constructed rule arrays (unit tests and future catalog
	// snapshots) need no process-global trie membership. Production rules always
	// have a non-zero engine id and therefore use the trie exclusively.
	if (0 == m_pengine->UlRuleId(prule))
		return true;
	for (ULONG candidate = 0; candidate < pdrgCandidates->Size(); ++candidate)
	{
		if ((*pdrgCandidates)[candidate] == prule)
			return true;
	}
	return false;
}

BOOL
CDSLRewriteProgram::FBudgetAvailable(const Path &path,
									 const CDSLRule *prule,
									 const SDSLRulePolicy &policy) const
{
	if (m_fHardBudgetExhausted ||
		(0 != m_ulHardSteps && m_ulApplications >= m_ulHardSteps))
		return false;
	auto ruleFound = m_ruleApplications.find(prule);
	const ULONG ruleApplications = m_ruleApplications.end() == ruleFound
		? 0
		: ruleFound->second;
	if (0 != policy.m_ulBudgetPerRule &&
		ruleApplications >= policy.m_ulBudgetPerRule)
		return false;
	if (0 != policy.m_ulBudgetPerQuery &&
		m_ulApplications >= policy.m_ulBudgetPerQuery)
		return false;
	auto nodeFound = m_nodeRuleApplications.find(StrNodeRuleKey(path, prule));
	const ULONG nodeApplications = m_nodeRuleApplications.end() == nodeFound
		? 0
		: nodeFound->second;
	return 0 == policy.m_ulBudgetPerNode ||
		nodeApplications < policy.m_ulBudgetPerNode;
}

void
CDSLRewriteProgram::ReserveBudget(const Path &path,
								  const CDSLRule *prule,
								  const SDSLRulePolicy &policy,
								  ULONG ulAddedNodes)
{
	++m_ulApplications;
	++m_ruleApplications[prule];
	++m_nodeRuleApplications[StrNodeRuleKey(path, prule)];
	m_ulAddedNodes += ulAddedNodes;
	if (!policy.m_fFixpoint)
		m_nonFixpointApplications.insert(StrNodeRuleKey(path, prule));
	if ((0 != m_ulHardSteps && m_ulApplications >= m_ulHardSteps) ||
		(0 != m_ulHardAddedNodes && m_ulAddedNodes >= m_ulHardAddedNodes))
		m_fHardBudgetExhausted = true;
}

void
CDSLRewriteProgram::ObserveReadyAlternatives(
	const Path &path, CExpression *pexprRoot, CExpression *pexprSource,
	const std::vector<const CDSLRule *> &ordered, ULONG ulFirst)
{
	// This is validation telemetry, not search. With tracing disabled the RBO
	// retains its first-applicable cost and evaluates no losing alternatives.
	if (!GPOS_FTRACE(EopttracePrintDSLRule))
	{
		return;
	}

	for (ULONG ul = ulFirst; ul < ordered.size(); ul++)
	{
		const CDSLRule *prule = ordered[ul];
		const SDSLRulePolicy &policy = m_psnapshot->Policy(prule);
		CDSLRewriteDecision *decision = m_pengine->PdecisionEvaluate(
			m_mp, prule, pexprSource, true /*fingerprint*/);
		if (EdsldecisionReady != decision->Status())
		{
			GPOS_DELETE(decision);
			continue;
		}

		CExpression *pexprTarget = decision->PexprTarget();
		if (nullptr != SzSafetyFailure(policy, pexprSource, pexprTarget))
		{
			GPOS_DELETE(decision);
			continue;
		}

		pexprTarget->AddRef();
		CExpression *pexprShadowRoot =
			PexprReplace(pexprRoot, path, pexprTarget);
		if (nullptr == pexprShadowRoot ||
			nullptr != SzRootSafetyFailure(pexprRoot, pexprShadowRoot))
		{
			CRefCount::SafeRelease(pexprShadowRoot);
			GPOS_DELETE(decision);
			continue;
		}

		CExpression *pexprShadowNode = PexprResolve(pexprShadowRoot, path);
		m_pengine->TraceRBOOutcome(
			m_mp, prule, &policy, decision, pexprSource, pexprShadowNode,
			"applicable_rbo", "source_replaced_by_prior_rule");
		m_ulApplicableAlternatives++;
		pexprShadowRoot->Release();
		GPOS_DELETE(decision);
	}
}

BOOL
CDSLRewriteProgram::FApplyAtNode(EDslRulePhase phase, EDslRuleOrder order,
								 const Path &path, CExpression *pexprRoot,
								 CExpression *pexprSource,
								 CExpression **ppexprNewRoot)
{
	GPOS_ASSERT(nullptr != pexprSource);
	GPOS_ASSERT(nullptr != pexprRoot);
	GPOS_ASSERT(nullptr != ppexprNewRoot);
	*ppexprNewRoot = nullptr;
	if (m_fHardBudgetExhausted || !pexprSource->Pop()->FLogical())
		return false;

	CDSLRuleArray *candidates = m_pengine->PdrgpruleCandidates(
		m_mp, pexprSource->Pop()->Eopid(), pexprSource,
		false /*fFilterCBO: this scheduler applies its own RBO snapshot*/);
	std::vector<const CDSLRule *> ordered;
	for (const CDSLRule *prule : m_psnapshot->RboRules(phase))
	{
		const SDSLRulePolicy &policy = m_psnapshot->Policy(prule);
		if (order == policy.m_edslorder && FCandidate(candidates, prule))
			ordered.push_back(prule);
	}

	for (ULONG ulRule = 0; ulRule < ordered.size(); ulRule++)
	{
		const CDSLRule *prule = ordered[ulRule];
		const SDSLRulePolicy &policy = m_psnapshot->Policy(prule);
		const std::string nodeRuleKey = StrNodeRuleKey(path, prule);
		if (!policy.m_fFixpoint &&
			m_nonFixpointApplications.end() !=
				m_nonFixpointApplications.find(nodeRuleKey))
			continue;
		const ULONG sourceFingerprint = CExpression::HashValue(pexprSource);
		if (FFired(prule, sourceFingerprint, pexprSource))
		{
			m_pengine->TraceRBOOutcome(m_mp, prule, &policy, nullptr, pexprSource,
									 nullptr, "fired_cache_skipped", nullptr);
			continue;
		}
		RecordFired(prule, sourceFingerprint, pexprSource);
		RecordLineage(path, pexprSource);
		CDSLRewriteDecision *decision = m_pengine->PdecisionEvaluate(
			m_mp, prule, pexprSource, true /*fingerprint*/);
		if (EdsldecisionReady != decision->Status())
		{
			const CHAR *status = "instantiate_rejected";
			if (EdsldecisionMatchRejected == decision->Status())
				status = "match_rejected";
			else if (EdsldecisionConstraintRejected == decision->Status())
				status = "constraint_rejected";
			else if (EdsldecisionDuplicate == decision->Status())
				status = "duplicate";
			m_pengine->TraceRBOOutcome(m_mp, prule, &policy, decision, pexprSource,
									 decision->PexprTarget(), status, nullptr);
			GPOS_DELETE(decision);
			continue;
		}

		CExpression *pexprTarget = decision->PexprTarget();
		const CHAR *safetyFailure =
			SzSafetyFailure(policy, pexprSource, pexprTarget);
		if (nullptr != safetyFailure)
		{
			m_pengine->TraceRBOOutcome(m_mp, prule, &policy, decision, pexprSource,
									 pexprTarget, "safety_rejected", safetyFailure);
			GPOS_DELETE(decision);
			continue;
		}
		if (FLineageContains(path, pexprTarget))
		{
			m_pengine->TraceRBOOutcome(m_mp, prule, &policy, decision, pexprSource,
									 pexprTarget, "cycle_rejected",
									 "target_seen_in_lineage");
			GPOS_DELETE(decision);
			continue;
		}
		const ULONG sourceNodes = UlNodes(pexprSource);
		const ULONG targetNodes = UlNodes(pexprTarget);
		const ULONG addedNodes = targetNodes > sourceNodes
			? targetNodes - sourceNodes
			: 0;
		if (!FBudgetAvailable(path, prule, policy) ||
			(0 != m_ulHardAddedNodes &&
			 m_ulAddedNodes + addedNodes > m_ulHardAddedNodes))
		{
			m_pengine->TraceRBOOutcome(m_mp, prule, &policy, decision, pexprSource,
									 pexprTarget, "budget_skipped", nullptr);
			GPOS_DELETE(decision);
			continue;
		}

		CExpression *pexprDetachedTarget = decision->PexprDetachTarget();
		CExpression *pexprNewRoot =
			PexprReplace(pexprRoot, path, pexprDetachedTarget);
		if (nullptr == pexprNewRoot)
		{
			m_pengine->TraceRBOOutcome(
				m_mp, prule, &policy, decision, pexprSource, nullptr,
				"safety_rejected", "replacement_path_invalid");
			GPOS_DELETE(decision);
			continue;
		}
		CExpression *pexprNewNode = PexprResolve(pexprNewRoot, path);
		const CHAR *rootSafetyFailure =
			SzRootSafetyFailure(pexprRoot, pexprNewRoot);
		if (nullptr != rootSafetyFailure)
		{
			m_pengine->TraceRBOOutcome(
				m_mp, prule, &policy, decision, pexprSource, pexprNewNode,
				"safety_rejected", rootSafetyFailure);
			pexprNewRoot->Release();
			GPOS_DELETE(decision);
			continue;
		}

		RecordLineage(path, pexprNewNode);
		ReserveBudget(path, prule, policy, addedNodes);
		m_pengine->TraceRBOOutcome(m_mp, prule, &policy, decision, pexprSource,
								 pexprNewNode, "applied_rbo",
								 "source_alternative_replaced");
		ObserveReadyAlternatives(path, pexprRoot, pexprSource, ordered,
							 ulRule + 1);
		*ppexprNewRoot = pexprNewRoot;
		GPOS_DELETE(decision);
		candidates->Release();
		return true;
	}
	candidates->Release();
	return false;
}

BOOL
CDSLRewriteProgram::FRunTopDown(EDslRulePhase phase,
								CExpression **ppexprRoot)
{
	BOOL changed = false;
	std::vector<SWorkItem> worklist;
	worklist.emplace_back(Path(), false);
	while (!worklist.empty() && !m_fHardBudgetExhausted)
	{
		GPOS_CHECK_ABORT;
		SWorkItem item = worklist.back();
		worklist.pop_back();
		CExpression *pexprNode = PexprResolve(*ppexprRoot, item.m_path);
		if (nullptr == pexprNode)
			continue;
		CExpression *pexprNewRoot = nullptr;
		if (FApplyAtNode(phase, EdslorderTopDown, item.m_path, *ppexprRoot,
						 pexprNode, &pexprNewRoot))
		{
			(*ppexprRoot)->Release();
			*ppexprRoot = pexprNewRoot;
			changed = true;
			worklist.emplace_back(item.m_path, false);
			continue;
		}
		for (ULONG child = pexprNode->Arity(); 0 < child; --child)
		{
			Path childPath = item.m_path;
			childPath.push_back(child - 1);
			worklist.emplace_back(childPath, false);
		}
	}
	return changed;
}

BOOL
CDSLRewriteProgram::FRunBottomUp(EDslRulePhase phase,
								 CExpression **ppexprRoot)
{
	BOOL changed = false;
	std::vector<SWorkItem> worklist;
	worklist.emplace_back(Path(), false);
	while (!worklist.empty() && !m_fHardBudgetExhausted)
	{
		GPOS_CHECK_ABORT;
		SWorkItem item = worklist.back();
		worklist.pop_back();
		CExpression *pexprNode = PexprResolve(*ppexprRoot, item.m_path);
		if (nullptr == pexprNode)
			continue;
		if (!item.m_fChildrenVisited)
		{
			worklist.emplace_back(item.m_path, true);
			for (ULONG child = pexprNode->Arity(); 0 < child; --child)
			{
				Path childPath = item.m_path;
				childPath.push_back(child - 1);
				worklist.emplace_back(childPath, false);
			}
			continue;
		}
		CExpression *pexprNewRoot = nullptr;
		if (!FApplyAtNode(phase, EdslorderBottomUp, item.m_path, *ppexprRoot,
						  pexprNode, &pexprNewRoot))
			continue;
		(*ppexprRoot)->Release();
		*ppexprRoot = pexprNewRoot;
		changed = true;
		// Revisit new children before retrying the replacement root.
		worklist.emplace_back(item.m_path, false);
	}
	return changed;
}

void
CDSLRewriteProgram::RunPhase(EDslRulePhase phase, CExpression **ppexprRoot)
{
	if (m_psnapshot->RboRules(phase).empty())
		return;
	BOOL changed = false;
	do
	{
		changed = FRunTopDown(phase, ppexprRoot);
		changed = FRunBottomUp(phase, ppexprRoot) || changed;
	} while (changed && !m_fHardBudgetExhausted);
}

CExpression *
CDSLRewriteProgram::PexprRewrite(CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);
	pexpr->AddRef();
	CExpression *pexprResult = pexpr;
	RunPhase(EdslphaseNormalize, &pexprResult);
	RunPhase(EdslphasePreJoin, &pexprResult);
	RunPhase(EdslphaseCleanup, &pexprResult);
	return pexprResult;
}
