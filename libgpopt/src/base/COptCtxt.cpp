//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		COptCtxt.cpp
//
//	@doc:
//		Implementation of optimizer context
//---------------------------------------------------------------------------

#include "gpopt/base/COptCtxt.h"

#include <utility>

#include "gpos/base.h"
#include "gpos/common/CAutoP.h"
#include "gpos/error/CAutoTrace.h"

#include "gpopt/base/CDefaultComparator.h"
#include "gpopt/cost/ICostModel.h"
#include "gpopt/eval/IConstExprEvaluator.h"
#include "gpopt/dsl/CDSLPolicy.h"
#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/dsl/CDSLStatsExperiment.h"
#include "gpopt/exception.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/optimizer/COptimizerConfig.h"
#include "gpopt/search/CGroupExpression.h"
#include "naucrates/statistics/IStatistics.h"
#include "naucrates/traceflags/traceflags.h"


using namespace gpopt;
using namespace gpnaucrates;

// value of the first value part id
ULONG COptCtxt::m_ulFirstValidPartId = 1;

//---------------------------------------------------------------------------
//	@function:
//		COptCtxt::COptCtxt
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
COptCtxt::COptCtxt(CMemoryPool *mp, CColumnFactory *col_factory,
				   CMDAccessor *md_accessor, IConstExprEvaluator *pceeval,
				   COptimizerConfig *optimizer_config)
	: CTaskLocalStorageObject(CTaskLocalStorage::EtlsidxOptCtxt),
	  m_mp(mp),
	  m_pcf(col_factory),
	  m_pmda(md_accessor),
	  m_pceeval(pceeval),
	  m_pcomp(GPOS_NEW(m_mp) CDefaultComparator(pceeval)),
	  m_auPartId(m_ulFirstValidPartId),
	  m_pcteinfo(nullptr),
	  m_pdrgpcrSystemCols(nullptr),
	  m_optimizer_config(optimizer_config),
	  m_fDMLQuery(false),
	  m_has_master_only_tables(false),
	  m_has_replicated_tables(false),
	  m_scanid_to_part_map(nullptr),
	  m_selector_id_counter(0),
	  m_dsl_trace_events(nullptr),
	  m_dsl_rule_trace_counters(nullptr),
	  m_ulDSLBindingCalls(0),
	  m_ulDSLBindingBuildUs(0),
	  m_ulDSLBindingsBuilt(0),
	  m_ulDSLCandidateCalls(0),
	  m_ulDSLCandidateLookupUs(0),
	  m_ulDSLCandidatesFound(0),
	  m_ulDSLGeneratedAlternatives(0),
	  m_dsl_generated_alternatives_by_rule(nullptr),
	  m_pdslPolicySnapshot(nullptr),
	  m_pdslStatsExperimentSnapshot(nullptr)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != col_factory);
	GPOS_ASSERT(nullptr != md_accessor);
	GPOS_ASSERT(nullptr != pceeval);
	GPOS_ASSERT(nullptr != m_pcomp);
	GPOS_ASSERT(nullptr != optimizer_config);
	GPOS_ASSERT(nullptr != optimizer_config->GetCostModel());

	m_pcteinfo = GPOS_NEW(m_mp) CCTEInfo(m_mp);
	m_cost_model = optimizer_config->GetCostModel();
	m_direct_dispatchable_filters = GPOS_NEW(mp) CExpressionArray(mp);
	m_scanid_to_part_map = GPOS_NEW(m_mp) UlongToBitSetMap(m_mp);
	m_part_selector_info = GPOS_NEW(m_mp) SPartSelectorInfo(m_mp);
	m_dsl_trace_events = GPOS_NEW(m_mp) CBitSet(m_mp);
	m_dsl_rule_trace_counters =
		GPOS_NEW(m_mp) UlongToDSLRuleTraceCountersMap(m_mp);
	m_dsl_generated_alternatives_by_rule =
		GPOS_NEW(m_mp) UlongToUlongMap(m_mp);

	CDSLRuleEngine *pengine = CDSLRuleEngine::Instance();
	GPOS_ASSERT(nullptr != pengine);
	CWStringDynamic strPolicyErrors(m_mp);
	CDSLPolicy *ppolicy = nullptr;
	const CHAR *szPolicyPath =
		m_optimizer_config->GetHint()->SzDSLRulePolicyPath();
	if (nullptr != szPolicyPath && '\0' != szPolicyPath[0])
	{
		ppolicy = CDSLPolicyLoader::PpolicyLoadFile(
			m_mp, szPolicyPath, &strPolicyErrors);
		if (nullptr == ppolicy)
		{
			GPOS_RAISE(CException::ExmaInvalid, CException::ExmiInvalid,
					   strPolicyErrors.GetBuffer());
		}
	}
	m_pdslPolicySnapshot = CDSLPolicySnapshot::PsnapshotCompile(
		m_mp, pengine->PdrgpruleAll(), ppolicy, &strPolicyErrors);
	CRefCount::SafeRelease(ppolicy);
	if (nullptr == m_pdslPolicySnapshot)
	{
		GPOS_RAISE(CException::ExmaInvalid, CException::ExmiInvalid,
				   strPolicyErrors.GetBuffer());
	}
}


//---------------------------------------------------------------------------
//	@function:
//		COptCtxt::~COptCtxt
//
//	@doc:
//		dtor
//		Does not de-allocate memory pool!
//
//---------------------------------------------------------------------------
COptCtxt::~COptCtxt()
{
	GPOS_DELETE(m_pcf);
	GPOS_DELETE(m_pcomp);
	m_pceeval->Release();
	m_pcteinfo->Release();
	m_optimizer_config->Release();
	CRefCount::SafeRelease(m_pdrgpcrSystemCols);
	CRefCount::SafeRelease(m_direct_dispatchable_filters);
	m_scanid_to_part_map->Release();
	m_part_selector_info->Release();
	m_dsl_trace_events->Release();
	m_dsl_rule_trace_counters->Release();
	m_dsl_generated_alternatives_by_rule->Release();
	GPOS_DELETE(m_pdslPolicySnapshot);
	GPOS_DELETE(m_pdslStatsExperimentSnapshot);
}

void
COptCtxt::InitializeDSLStatsExperiment(const CExpression *root)
{
	GPOS_ASSERT(nullptr == m_pdslStatsExperimentSnapshot);
	const CHAR *path =
		m_optimizer_config->GetHint()->SzDSLStatsExperimentPath();
	if (nullptr == path || '\0' == path[0])
	{
		return;
	}
	CWStringDynamic errors(m_mp);
	m_pdslStatsExperimentSnapshot =
		CDSLStatsExperimentSnapshot::PsnapshotLoadFile(m_mp, path, root, &errors);
	if (nullptr == m_pdslStatsExperimentSnapshot)
	{
		GPOS_RAISE(CException::ExmaInvalid, CException::ExmiInvalid,
				   errors.GetBuffer());
	}
}

void
COptCtxt::RegisterDSLStatsExperimentGroup(const COperator *pop, CGroup *group)
{
	if (nullptr == m_pdslStatsExperimentSnapshot)
	{
		return;
	}
	const SDSLStatsExperimentTarget *target =
		m_pdslStatsExperimentSnapshot->Ptarget(pop);
	if (nullptr != target)
	{
		m_dsl_stats_group_targets[group] = target;
	}
}

namespace
{
std::string
JsonEscape(const std::string &value)
{
	std::string escaped;
	for (CHAR ch : value)
	{
		switch (ch)
		{
			case '"':
			case '\\':
				escaped.push_back('\\');
				escaped.push_back(ch);
				break;
			case '\n':
				escaped.append("\\n");
				break;
			case '\r':
				escaped.append("\\r");
				break;
			case '\t':
				escaped.append("\\t");
				break;
			case '\b':
				escaped.append("\\b");
				break;
			case '\f':
				escaped.append("\\f");
				break;
			default:
				escaped.push_back(ch);
		}
	}
	return escaped;
}

IStatistics *
PstatsScaleForExperiment(CMemoryPool *mp, IStatistics *stats,
						 const SDSLStatsExperimentTarget *target,
						 const CHAR *experiment, const CHAR *site,
						 BOOL trace_event)
{
	if (nullptr == target)
	{
		return nullptr;
	}
	if (target->m_inject && stats->Rows() == CDouble(target->m_rows))
	{
		return nullptr;
	}
	if (trace_event && GPOS_FTRACE(EopttracePrintDSLRule))
	{
		CAutoTrace trace(mp);
		const std::string escaped_experiment = JsonEscape(experiment);
		const std::string escaped_relations = JsonEscape(target->m_relations);
		const std::string escaped_fingerprint =
			JsonEscape(target->m_fingerprint);
		const std::string escaped_operator = JsonEscape(target->m_operator);
		trace.Os() << "DSL_TRACE {\"kind\":\""
				   << (target->m_inject ? "stats_injection"
									: "stats_observation")
				   << "\","
				   << "\"experiment\":\"" << escaped_experiment.c_str()
				   << "\",\"fingerprint\":\""
				   << escaped_fingerprint.c_str() << "\",\"operator\":\""
				   << escaped_operator.c_str() << "\"";
		if (!target->m_relations.empty())
		{
			trace.Os() << ",\"relations\":\""
					   << escaped_relations.c_str() << "\"";
		}
		trace.Os() << ","
				   << "\"site\":\"" << site << "\","
				   << "\"native_rows\":" << stats->Rows().Get();
		if (target->m_inject)
		{
			trace.Os() << ",\"rows\":" << target->m_rows;
		}
		trace.Os() << "}" << std::endl;
	}
	if (!target->m_inject)
	{
		return nullptr;
	}
	return stats->ScaleStats(mp, CDouble(target->m_rows) / stats->Rows());
}
}  // namespace

IStatistics *
COptCtxt::PstatsApplyDSLExperiment(CMemoryPool *mp, const CExpression *expr,
										IStatistics *stats)
{
	return nullptr == m_pdslStatsExperimentSnapshot
		? nullptr
		: PstatsScaleForExperiment(
			  mp, stats, m_pdslStatsExperimentSnapshot->Ptarget(expr),
			  m_pdslStatsExperimentSnapshot->SzId(), "expression", true);
}

IStatistics *
COptCtxt::PstatsApplyDSLExperiment(CMemoryPool *mp, const CGroup *group,
										IStatistics *stats)
{
	const auto found = m_dsl_stats_group_targets.find(group);
	return m_dsl_stats_group_targets.end() == found
		? nullptr
		: PstatsScaleForExperiment(
			  mp, stats, found->second,
			  m_pdslStatsExperimentSnapshot->SzId(), "memo_group",
			  m_dsl_stats_traced_groups.insert(group).second);
}

void
COptCtxt::TraceDSLExperimentOutcome(
	DOUBLE optimizer_cost, ULONG selected_plan_nodes,
	ULONG selected_plan_cbo_dsl_nodes, ULONG memo_groups,
	ULONG memo_group_expressions, ULONG optimization_ms,
	ULLONG optimizer_memory_bytes) const
{
	if (nullptr == m_pdslStatsExperimentSnapshot ||
		!GPOS_FTRACE(EopttracePrintDSLRule))
	{
		return;
	}

	const std::string experiment =
		JsonEscape(m_pdslStatsExperimentSnapshot->SzId());
	CAutoTrace trace(m_mp);
	trace.Os() << "DSL_TRACE {\"kind\":\"experiment_outcome\","
				   << "\"engine\":\"pgorca\",\"experiment\":\""
				   << experiment.c_str() << "\",\"optimizer_cost\":"
				   << optimizer_cost << ",\"selected_plan_nodes\":"
				   << selected_plan_nodes
				   << ",\"selected_plan_cbo_dsl_nodes\":"
				   << selected_plan_cbo_dsl_nodes
				   << ",\"selected_plan_has_cbo_dsl_provenance\":"
				   << (0 < selected_plan_cbo_dsl_nodes ? "true" : "false")
				   << ",\"memo_groups\":" << memo_groups
				   << ",\"memo_group_expressions\":"
				   << memo_group_expressions
				   << ",\"cbo_generated_dsl_alternatives\":"
				   << m_ulDSLGeneratedAlternatives
				   << ",\"optimization_ms\":" << optimization_ms
				   << ",\"optimizer_memory_bytes\":"
				   << optimizer_memory_bytes << "}" << std::endl;
}


//---------------------------------------------------------------------------
//	@function:
//		COptCtxt::RecordDSLRuleTrace
//
//	@doc:
//		Record an uncompacted DSL rule attempt for query-level diagnostics
//---------------------------------------------------------------------------
void
COptCtxt::RecordDSLRuleTrace(ULONG ulRuleId, ULONG ulStage,
						 ULONG ulBoundSymbols)
{
	GPOS_ASSERT(ulStage < 7);
	SDSLRuleTraceCounters *pcounters =
		m_dsl_rule_trace_counters->Find(&ulRuleId);
	if (nullptr == pcounters)
	{
		pcounters = GPOS_NEW(m_mp) SDSLRuleTraceCounters();
		(void) m_dsl_rule_trace_counters->Insert(
			GPOS_NEW(m_mp) ULONG(ulRuleId), pcounters);
	}
	pcounters->m_stage_attempts[ulStage]++;
	pcounters->m_bound_symbols += ulBoundSymbols;
}

void
COptCtxt::RecordDSLRuleTiming(ULONG ulRuleId, ULONG ulMatchUs,
						  ULONG ulConstraintUs, ULONG ulInstantiateUs)
{
	SDSLRuleTraceCounters *pcounters =
		m_dsl_rule_trace_counters->Find(&ulRuleId);
	if (nullptr == pcounters)
	{
		pcounters = GPOS_NEW(m_mp) SDSLRuleTraceCounters();
		(void) m_dsl_rule_trace_counters->Insert(
			GPOS_NEW(m_mp) ULONG(ulRuleId), pcounters);
	}
	pcounters->m_match_us += ulMatchUs;
	pcounters->m_constraint_us += ulConstraintUs;
	pcounters->m_instantiate_us += ulInstantiateUs;
}

void
COptCtxt::RecordDSLBindingTiming(ULONG ulElapsedUs, ULONG ulBindings)
{
	++m_ulDSLBindingCalls;
	m_ulDSLBindingBuildUs += ulElapsedUs;
	m_ulDSLBindingsBuilt += ulBindings;
}

void
COptCtxt::RecordDSLCandidateTiming(ULONG ulElapsedUs, ULONG ulCandidates)
{
	++m_ulDSLCandidateCalls;
	m_ulDSLCandidateLookupUs += ulElapsedUs;
	m_ulDSLCandidatesFound += ulCandidates;
}

void
COptCtxt::RegisterDSLPendingAlternative(const CExpression *pexpr,
										const CDSLRule *prule,
										const CDSLTargetInputOriginArray &inputOrigins)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != prule);
	m_dsl_pending_alternative_rules[pexpr] = {prule, inputOrigins};
}

const CDSLRule *
COptCtxt::PdslruleTakePendingAlternative(
	const CExpression *pexpr, CDSLTargetInputOriginArray *inputOrigins)
{
	GPOS_ASSERT(nullptr != inputOrigins);
	auto found = m_dsl_pending_alternative_rules.find(pexpr);
	if (m_dsl_pending_alternative_rules.end() == found)
		return nullptr;
	const CDSLRule *prule = found->second.m_prule;
	*inputOrigins = std::move(found->second.m_input_origins);
	m_dsl_pending_alternative_rules.erase(found);
	return prule;
}

void
COptCtxt::RegisterDSLGroupExpressionOrigin(const CGroupExpression *pgexpr,
										 const CDSLRule *prule,
										 const CHAR *szTargetPath,
										 const CHAR *szRelation)
{
	GPOS_ASSERT(nullptr != pgexpr);
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(nullptr != szTargetPath);
	GPOS_ASSERT(nullptr != szRelation);
	m_dsl_group_expression_origins[pgexpr] = {prule, szTargetPath, szRelation};
}

void
COptCtxt::TraceDSLCBOEdge(const CDSLRule *prule,
							 const CExpression *pexprSource) const
{
	if (!GPOS_FTRACE(EopttracePrintDSLRule) || nullptr == pexprSource->Pgexpr())
		return;
	auto found = m_dsl_group_expression_origins.find(pexprSource->Pgexpr());
	if (m_dsl_group_expression_origins.end() == found)
		return;

	const CGroupExpression *pgexpr = pexprSource->Pgexpr();
	CAutoTrace trace(m_mp);
	trace.Os() << "DSL_TRACE {\"kind\":\"rule_edge\",\"engine\":\"pgorca\","
				   << "\"scheduler\":\"cbo\",\"src_rule\":\""
				   << found->second.m_prule->SzIdentity()
				   << "\",\"dst_rule\":\"" << prule->SzIdentity()
				   << "\",\"target_path\":\""
				   << found->second.m_target_path.c_str()
				   << "\",\"src_target_path\":\""
				   << found->second.m_target_path.c_str()
				   << "\",\"dst_source_path\":\"r"
				   << "\",\"path_kind\":\"instantiated_expression\","
					  "\"binding_group\":" << pgexpr->Pgroup()->Id()
				   << ",\"binding_group_expression\":" << pgexpr->Id()
				   << ",\"evidence\":\"runtime_observed\",\"relation\":\""
				   << found->second.m_relation.c_str() << "\"}"
				   << std::endl;
}

void
COptCtxt::RecordDSLRuleBudgetSkip(ULONG ulRuleId)
{
	const ULONG ulBudgetSkippedStage = 6;
	if (FMarkDSLTraceEvent(ulRuleId, ulBudgetSkippedStage))
	{
		RecordDSLRuleTrace(ulRuleId, ulBudgetSkippedStage, 0);
	}
}

BOOL
COptCtxt::FDSLAlternativeBudgetExhausted(ULONG ulRuleId,
									 const SDSLRulePolicy *policy,
									 ULONG ulNodeId)
{
	const ULONG ulMax =
		m_optimizer_config->GetHint()->UlDSLRuleMaxAlternatives();
	if ((0 != ulMax && m_ulDSLGeneratedAlternatives >= ulMax) ||
		(nullptr != policy && 0 != policy->m_ulBudgetPerQuery &&
		 m_ulDSLGeneratedAlternatives >= policy->m_ulBudgetPerQuery))
	{
		return true;
	}

	const ULONG ulMaxPerRule =
		m_optimizer_config->GetHint()->UlDSLRuleMaxAlternativesPerRule();
	ULONG *pulGenerated =
		m_dsl_generated_alternatives_by_rule->Find(&ulRuleId);
	if (nullptr != pulGenerated &&
		((0 != ulMaxPerRule && *pulGenerated >= ulMaxPerRule) ||
		 (nullptr != policy && 0 != policy->m_ulBudgetPerRule &&
		  *pulGenerated >= policy->m_ulBudgetPerRule)))
	{
		return true;
	}
	if (nullptr != policy && 0 != policy->m_ulBudgetPerNode &&
		gpos::ulong_max != ulNodeId)
	{
		auto rule = m_dsl_generated_alternatives_by_node_rule.find(ulRuleId);
		if (m_dsl_generated_alternatives_by_node_rule.end() != rule)
		{
			auto node = rule->second.find(ulNodeId);
			if (rule->second.end() != node &&
				node->second >= policy->m_ulBudgetPerNode)
			{
				return true;
			}
		}
	}
	return false;
}

BOOL
COptCtxt::FReserveDSLAlternative(ULONG ulRuleId,
							 const SDSLRulePolicy *policy,
							 ULONG ulNodeId)
{
	if (FDSLAlternativeBudgetExhausted(ulRuleId, policy, ulNodeId))
	{
		return false;
	}

	ULONG *pulGenerated =
		m_dsl_generated_alternatives_by_rule->Find(&ulRuleId);
	if (nullptr == pulGenerated)
	{
		(void) m_dsl_generated_alternatives_by_rule->Insert(
			GPOS_NEW(m_mp) ULONG(ulRuleId), GPOS_NEW(m_mp) ULONG(1));
	}
	else
	{
		(*pulGenerated)++;
	}
	m_ulDSLGeneratedAlternatives++;
	if (nullptr != policy && 0 != policy->m_ulBudgetPerNode &&
		gpos::ulong_max != ulNodeId)
	{
		++m_dsl_generated_alternatives_by_node_rule[ulRuleId][ulNodeId];
	}
	return true;
}


//---------------------------------------------------------------------------
//	@function:
//		COptCtxt::PoctxtCreate
//
//	@doc:
//		Factory method for optimizer context
//
//---------------------------------------------------------------------------
COptCtxt *
COptCtxt::PoctxtCreate(CMemoryPool *mp, CMDAccessor *md_accessor,
					   IConstExprEvaluator *pceeval,
					   COptimizerConfig *optimizer_config)
{
	GPOS_ASSERT(nullptr != optimizer_config);

	// CONSIDER:  - 1/5/09; allocate column factory out of given mem pool
	// instead of having it create its own;
	CColumnFactory *col_factory = GPOS_NEW(mp) CColumnFactory;

	COptCtxt *poctxt = nullptr;
	{
		// safe handling of column factory; since it owns a pool that would be
		// leaked if below allocation fails
		CAutoP<CColumnFactory> a_pcf;
		a_pcf = col_factory;
		a_pcf.Value()->Initialize();

		poctxt = GPOS_NEW(mp)
			COptCtxt(mp, col_factory, md_accessor, pceeval, optimizer_config);

		// detach safety
		(void) a_pcf.Reset();
	}
	return poctxt;
}


//---------------------------------------------------------------------------
//	@function:
//		COptCtxt::FAllEnforcersEnabled
//
//	@doc:
//		Return true if all enforcers are enabled
//
//---------------------------------------------------------------------------
BOOL
COptCtxt::FAllEnforcersEnabled()
{
	BOOL fEnforcerDisabled =
		GPOS_FTRACE(EopttraceDisableMotions) ||
		GPOS_FTRACE(EopttraceDisableMotionBroadcast) ||
		GPOS_FTRACE(EopttraceDisableMotionGather) ||
		GPOS_FTRACE(EopttraceDisableMotionHashDistribute) ||
		GPOS_FTRACE(EopttraceDisableMotionRandom) ||
		GPOS_FTRACE(EopttraceDisableMotionRountedDistribute) ||
		GPOS_FTRACE(EopttraceDisableSort) ||
		GPOS_FTRACE(EopttraceDisableSpool) ||
		GPOS_FTRACE(EopttraceDisablePartPropagation);

	return !fEnforcerDisabled;
}

void
COptCtxt::AddPartForScanId(ULONG scanid, ULONG index)
{
	CBitSet *parts = m_scanid_to_part_map->Find(&scanid);
	if (nullptr == parts)
	{
		parts = GPOS_NEW(m_mp) CBitSet(m_mp);
		m_scanid_to_part_map->Insert(GPOS_NEW(m_mp) ULONG(scanid), parts);
	}
	parts->ExchangeSet(index);
}

const SPartSelectorInfoEntry *
COptCtxt::GetPartSelectorInfo(ULONG selector_id) const
{
	return m_part_selector_info->Find(&selector_id);
}

BOOL
COptCtxt::AddPartSelectorInfo(ULONG selector_id, SPartSelectorInfoEntry *entry)
{
	ULONG *key = GPOS_NEW(m_mp) ULONG(selector_id);

	/*
	 * Selector ids come from a monotone counter and are therefore unique,
	 * so skip the duplicate-key scan: enforcers can be appended a huge
	 * number of times (e.g. many-branch UNION ALL under a partition-key
	 * join), and Insert()'s linear probe would make registration quadratic.
	 */
	m_part_selector_info->InsertUnique(key, entry);
	return true;
}
