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

#include <cstring>
#include <iomanip>
#include <map>
#include <sstream>
#include <utility>

#include "gpos/base.h"
#include "gpos/common/CAutoP.h"
#include "gpos/error/CAutoTrace.h"

#include "gpopt/base/CDefaultComparator.h"
#include "gpopt/base/CCostContext.h"
#include "gpopt/base/COptimizationContext.h"
#include "gpopt/base/CEnfdOrder.h"
#include "gpopt/base/CEnfdDistribution.h"
#include "gpopt/cost/ICostModel.h"
#include "gpopt/eval/IConstExprEvaluator.h"
#include "gpopt/dsl/CDSLPolicy.h"
#include "gpopt/dsl/CDSLRewriteDecision.h"
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

namespace
{
std::string JsonEscape(const std::string &value);
void EmitExperimentCandidate(CMemoryPool *mp, const CHAR *experiment,
							 const std::string &suffix);
}  // namespace

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
	  m_ulDSLExperimentSequence(0),
	  m_ulDSLExperimentCandidates(0),
	  m_ulDSLExperimentApplications(0),
	  m_ulDSLMemoVersion(0),
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
	for (const std::string &suffix : m_dsl_pending_experiment_candidates)
	{
		EmitExperimentCandidate(m_mp, m_pdslStatsExperimentSnapshot->SzId(),
							 suffix);
	}
	m_dsl_pending_experiment_candidates.clear();
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

void
EmitExperimentCandidate(CMemoryPool *mp, const CHAR *experiment,
						const std::string &suffix)
{
	CAutoTrace trace(mp);
	trace.Os() << "DSL_TRACE {\"kind\":\"rule_candidate\","
				   << "\"engine\":\"pgorca\",\"experiment\":\""
				   << JsonEscape(experiment).c_str() << "\"" << suffix.c_str()
				   << std::endl;
}

IStatistics *
PstatsScaleForExperiment(CMemoryPool *mp, IStatistics *stats,
						 const SDSLStatsExperimentTarget *target,
						 const CHAR *experiment, const CHAR *site,
						 BOOL trace_event,
						 const CGroup *group,
						 std::unordered_set<const SDSLStatsExperimentTarget *> *consumed)
{
	if (nullptr == target)
	{
		return nullptr;
	}
	if (target->m_inject && GPOS_FTRACE(EopttracePrintDSLRule))
	{
		consumed->insert(target);
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
		if (nullptr != group)
			trace.Os() << ",\"group\":" << group->Id();
		if (target->m_inject)
		{
			trace.Os() << ",\"rows\":" << target->m_rows
					   << ",\"changed\":"
					   << (stats->Rows() == CDouble(target->m_rows) ? "false" : "true");
		}
		trace.Os() << "}" << std::endl;
	}
	if (!target->m_inject || stats->Rows() == CDouble(target->m_rows))
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
			  m_pdslStatsExperimentSnapshot->SzId(), "expression", true,
			  nullptr == expr->Pgexpr() ? nullptr : expr->Pgexpr()->Pgroup(),
			  &m_dsl_stats_consumed_targets);
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
			  m_dsl_stats_traced_groups.insert(group).second,
			  group,
			  &m_dsl_stats_consumed_targets);
}

void
COptCtxt::TraceDSLStatsLifecycle(const CGroup *group, const IStatistics *stats)
{
	if (nullptr == m_pdslStatsExperimentSnapshot || !GPOS_FTRACE(EopttracePrintDSLRule) || group->FScalar())
		return;
	std::ostringstream out;
	out << std::setprecision(17);
	out << "DSL_TRACE {\"kind\":\"group_stats_lifecycle\",\"experiment\":\""
		<< JsonEscape(m_pdslStatsExperimentSnapshot->SzId())
		<< "\",\"sequence\":" << ++m_ulDSLStatsLifecycleEvents
		<< ",\"preceding_rule_candidates\":" << m_ulDSLExperimentCandidates
		<< ",\"preceding_cost_candidates\":" << m_ulDSLExperimentCostEvents
		<< ",\"memo_version\":" << m_ulDSLMemoVersion
		<< ",\"group\":" << group->Id()
		<< ",\"status\":\"" << (nullptr == stats ? "reset" : "available")
		<< "\",\"scope\":\"group_cache_after_write\",\"rows\":";
	if (nullptr != stats && std::isfinite(stats->Rows().Get()))
		out << stats->Rows().Get();
	else
		out << "null";
	out << ",\"empty\":";
	if (nullptr != stats)
		out << (stats->IsEmpty() ? "true" : "false");
	else
		out << "null";
	out << "}";
	CAutoTrace trace(m_mp);
	trace.Os() << out.str().c_str() << std::endl;
}

void
COptCtxt::TraceDSLExperimentCost(const CGroupExpression *expr,
	const COptimizationContext *context, ULONG request, const CHAR *status,
	CCostContext *cost)
{
	if (nullptr == m_pdslStatsExperimentSnapshot || !GPOS_FTRACE(EopttracePrintDSLRule))
		return;
	CAutoTrace trace(m_mp);
	auto &out = trace.Os();
	out << "DSL_TRACE {\"kind\":\"cost_candidate\",\"experiment\":\""
		<< JsonEscape(m_pdslStatsExperimentSnapshot->SzId()).c_str()
		<< "\",\"sequence\":" << ++m_ulDSLExperimentCostEvents
		<< ",\"preceding_rule_candidates\":" << m_ulDSLExperimentCandidates
		<< ",\"stats_lifecycle_sequence\":" << m_ulDSLStatsLifecycleEvents
		<< ",\"memo_version\":" << m_ulDSLMemoVersion
		<< ",\"group\":" << expr->Pgroup()->Id()
		<< ",\"group_expression\":" << expr->Id()
		<< ",\"operator\":\"" << expr->Pop()->SzId()
		<< "\",\"optimization_context\":" << context->Id()
		<< ",\"search_stage\":" << context->UlSearchStageIndex()
		<< ",\"optimization_request\":" << request
		<< ",\"status\":\"" << status << "\"";
	const auto *required = context->Prpp();
	out << ",\"required_columns\":" << required->PcrsRequired()->Size()
		<< ",\"required_order_columns\":" << required->Peo()->PosRequired()->UlSortColumns()
		<< ",\"required_order_matching\":" << (ULONG) required->Peo()->Eom()
		<< ",\"required_distribution_type\":" << (ULONG) required->Ped()->PdsRequired()->Edt();
	if (nullptr != context->PccBest())
		out << ",\"context_best_cost_at_event\":" << context->PccBest()->Cost().Get();
	const auto *origin = expr->PgexprOrigin();
	if (nullptr != origin)
		out << ",\"origin_group\":" << origin->Pgroup()->Id()
			<< ",\"origin_expression\":" << origin->Id();
	// Preserve intermediate logical lowering (for example Anchor -> Sequence).
	// These are existing Memo provenance links, not inferred rule dependencies.
	out << ",\"origin_chain\":[";
	for (const auto *ancestor = origin; nullptr != ancestor;
		 ancestor = ancestor->PgexprOrigin())
	{
		if (ancestor != origin) out << ",";
		out << "{\"group\":" << ancestor->Pgroup()->Id()
			<< ",\"group_expression\":" << ancestor->Id()
			<< ",\"operator\":\"" << ancestor->Pop()->SzId() << "\"}";
	}
	out << "]";
	if (nullptr != cost)
	{
		cost->SetDSLTraceCandidate(m_ulDSLExperimentCostEvents);
		// This is the evaluated candidate, before best-context insertion/selection.
		out << ",\"cost\":" << cost->Cost().Get()
			<< ",\"cost_kind\":\"" << (cost->FPruned() ? "lower_bound" : "computed") << "\"";
		const auto *stats = cost->Pstats();  // Access cached stats only.
		if (nullptr != stats && std::isfinite(stats->Rows().Get()))
			out << ",\"rows\":" << stats->Rows().Get();
		const auto *stats_expr = cost->PgexprForStats();
		if (nullptr != stats_expr)
			out << ",\"stats_group\":" << stats_expr->Pgroup()->Id()
				<< ",\"stats_expression\":" << stats_expr->Id();
		out << ",\"child_contexts\":[";
		const auto *children = cost->Pdrgpoc();
		for (ULONG i = 0; nullptr != children && i < children->Size(); ++i)
		{
			if (i > 0) out << ",";
			out << "{\"index\":" << i << ",\"group\":" << (*children)[i]->Pgroup()->Id()
				<< ",\"optimization_context\":" << (*children)[i]->Id();
			const auto *best = (*children)[i]->PccBest();
			out << ",\"cost_candidate_sequence\":" << (nullptr == best ? 0 : best->UlDSLTraceCandidate()) << "}";
		}
		out << "]";
	}
	out << "}" << std::endl;
}

void
COptCtxt::TraceDSLExperimentCostLifecycle(const CHAR *status, const CCostContext *candidate,
	const CCostContext *previous, const COptimizationContext *owner)
{
	if (nullptr == m_pdslStatsExperimentSnapshot || !GPOS_FTRACE(EopttracePrintDSLRule))
		return;
	CAutoTrace trace(m_mp);
	if (nullptr == owner) owner = candidate->Poc();
	trace.Os() << "DSL_TRACE {\"kind\":\"cost_lifecycle\",\"experiment\":\""
		<< JsonEscape(m_pdslStatsExperimentSnapshot->SzId()).c_str()
		<< "\",\"sequence\":" << ++m_ulDSLExperimentCostLifecycleEvents
		<< ",\"status\":\"" << status
		<< "\",\"candidate_sequence\":" << candidate->UlDSLTraceCandidate()
		<< ",\"previous_candidate_sequence\":" << (nullptr == previous ? 0 : previous->UlDSLTraceCandidate())
		<< ",\"group\":" << owner->Pgroup()->Id()
		<< ",\"optimization_context\":" << owner->Id()
		<< "}" << std::endl;
}

void
COptCtxt::TraceDSLExperimentSelectedCost(const CExpression *expr, ULONG node, ULONG parent)
{
	if (nullptr == m_pdslStatsExperimentSnapshot || !GPOS_FTRACE(EopttracePrintDSLRule)
		|| !expr->Pop()->FPhysical())
		return;
	CAutoTrace trace(m_mp);
	trace.Os() << "DSL_TRACE {\"kind\":\"cost_lifecycle\",\"experiment\":\""
		<< JsonEscape(m_pdslStatsExperimentSnapshot->SzId()).c_str()
		<< "\",\"sequence\":" << ++m_ulDSLExperimentCostLifecycleEvents
		<< ",\"status\":\"selected_plan\",\"candidate_sequence\":" << expr->UlDSLTraceCandidate()
		<< ",\"plan_node\":" << node << ",\"parent_plan_node\":" << parent
		<< ",\"operator\":\"" << expr->Pop()->SzId()
		<< "\",\"cost\":" << expr->Cost().Get() << "}" << std::endl;
}

void
COptCtxt::TraceDSLExperimentSearchCheck(const CHAR *check, const CHAR *status,
	const CGroupExpression *expr, const CReqdPropPlan *required, ULONG request,
	const CCostContext *incumbent, DOUBLE lower_bound, const CCostContext *child, ULONG child_index)
{
	if (nullptr == m_pdslStatsExperimentSnapshot || !GPOS_FTRACE(EopttracePrintDSLRule))
		return;
	CAutoTrace trace(m_mp);
	auto &out = trace.Os();
	out << "DSL_TRACE {\"kind\":\"search_check\",\"experiment\":\""
		<< JsonEscape(m_pdslStatsExperimentSnapshot->SzId()).c_str()
		<< "\",\"sequence\":" << ++m_ulDSLExperimentSearchChecks
		<< ",\"check\":\"" << check << "\",\"status\":\"" << status
		<< "\",\"group\":" << expr->Pgroup()->Id() << ",\"group_expression\":" << expr->Id()
		<< ",\"operator\":\"" << expr->Pop()->SzId()
		<< "\",\"preceding_cost_candidates\":" << m_ulDSLExperimentCostEvents
		<< ",\"preceding_rule_candidates\":" << m_ulDSLExperimentCandidates
		<< ",\"memo_version\":" << m_ulDSLMemoVersion
		<< ",\"required_columns\":" << required->PcrsRequired()->Size()
		<< ",\"required_order_columns\":" << required->Peo()->PosRequired()->UlSortColumns()
		<< ",\"required_distribution_type\":" << (ULONG) required->Ped()->PdsRequired()->Edt();
	if (request != gpos::ulong_max)
		out << ",\"optimization_request\":" << request;
	if (nullptr != incumbent)
		out << ",\"incumbent_candidate\":" << incumbent->UlDSLTraceCandidate()
			<< ",\"incumbent_context\":" << incumbent->Poc()->Id()
			<< ",\"incumbent_cost\":" << incumbent->Cost().Get();
	if (lower_bound >= 0)
		out << ",\"lower_bound\":" << lower_bound;
	if (nullptr != child)
		out << ",\"optimized_child_candidate\":" << child->UlDSLTraceCandidate()
			<< ",\"optimized_child_index\":" << child_index;
	out << "}" << std::endl;
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
				   << ",\"rule_candidates\":"
				   << m_ulDSLExperimentCandidates
				   << ",\"candidate_trace_version\":2"
				   << ",\"binding_edge_trace_version\":3,\"binding_origin_edges\":" << m_ulDSLBindingOriginEdges
				   << ",\"cost_trace_version\":1,\"cost_candidates\":" << m_ulDSLExperimentCostEvents
				   << ",\"stats_lifecycle_version\":1,\"stats_lifecycle_events\":" << m_ulDSLStatsLifecycleEvents
				   << ",\"cost_lifecycle_version\":1,\"cost_lifecycle_events\":" << m_ulDSLExperimentCostLifecycleEvents
				   << ",\"search_check_version\":1,\"search_checks\":" << m_ulDSLExperimentSearchChecks
				   << ",\"candidate_context_encoding\":\"dictionary_v1\""
				   << ",\"rule_applications\":"
				   << m_ulDSLExperimentApplications
				   << ",\"optimization_ms\":" << optimization_ms
				   << ",\"optimizer_memory_bytes\":"
				   << optimizer_memory_bytes
				   << ",\"selected_plan_cbo_dsl_rules\":{";
	BOOL first_rule = true;
	for (const auto &entry : std::map<std::string, ULONG>(
			 m_dsl_selected_plan_rules.begin(),
			 m_dsl_selected_plan_rules.end()))
	{
		if (!first_rule)
		{
			trace.Os() << ",";
		}
		first_rule = false;
		trace.Os() << "\"" << entry.first.c_str() << "\":" << entry.second;
	}
	trace.Os() << "},\"stats_targets\":[";
	BOOL first_target = true;
	for (const auto &target : m_pdslStatsExperimentSnapshot->Targets())
	{
		if (!target.m_inject)
		{
			continue;
		}
		if (!first_target)
		{
			trace.Os() << ",";
		}
		first_target = false;
		const BOOL expression = target.m_relations.empty();
		trace.Os() << "{\"selector_kind\":\""
				   << (expression ? "expression" : "relations")
				   << "\",\"selector\":\""
				   << JsonEscape(expression ? target.m_fingerprint : target.m_relations).c_str()
				   << "\",\"operator\":\"" << JsonEscape(target.m_operator).c_str()
				   << "\",\"fingerprint\":\"" << JsonEscape(target.m_fingerprint).c_str()
				   << "\",\"requested_rows\":" << target.m_rows
				   << ",\"consumed\":"
				   << (m_dsl_stats_consumed_targets.count(&target) > 0 ? "true" : "false")
				   << "}";
	}
	trace.Os() << "]}" << std::endl;
}

void
COptCtxt::TraceDSLExperimentCandidate(
	const CDSLRule *prule, const CHAR *placement, const CHAR *status,
	const CExpression *pexprState, const CExpression *pexprSource,
	const CExpression *pexprTarget, const CHAR *bindingPath, ULONG matchUs,
	ULONG constraintUs, ULONG instantiateUs, BOOL applied,
	const CDSLRewriteDecision *decision)
{
	if (!GPOS_FTRACE(EopttracePrintDSLRule))
	{
		return;
	}
	if (nullptr == m_pdslStatsExperimentSnapshot)
	{
		const CHAR *path =
			m_optimizer_config->GetHint()->SzDSLStatsExperimentPath();
		if (nullptr == path || '\0' == path[0])
		{
			return;
		}
	}
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(nullptr != placement);
	GPOS_ASSERT(nullptr != status);
	GPOS_ASSERT(nullptr != pexprState);
	GPOS_ASSERT(nullptr != pexprSource);

	const std::string state =
		CDSLStatsExperimentSnapshot::Fingerprint(m_mp, pexprState);
	const std::string source =
		CDSLStatsExperimentSnapshot::Fingerprint(m_mp, pexprSource);
	const CGroupExpression *pgexpr = pexprSource->Pgexpr();
	std::string binding(source);
	if (nullptr != bindingPath)
	{
		binding.append("@").append(bindingPath);
	}
	else if (nullptr != pgexpr)
	{
		binding.append("@g")
			.append(std::to_string(pgexpr->Pgroup()->Id()))
			.append(":e")
			.append(std::to_string(pgexpr->Id()));
	}

	++m_ulDSLExperimentCandidates;
	if (applied)
	{
		++m_ulDSLExperimentApplications;
	}
	std::ostringstream event;
	event << ",\"sequence\":" << ++m_ulDSLExperimentSequence
		  << ",\"state_fingerprint\":\"" << state
		  << "\",\"rule_id\":"
		  << CDSLRuleEngine::Instance()->UlRuleId(prule)
		  << ",\"rule_hash\":\"" << prule->SzIdentity()
		  << "\",\"binding_fingerprint\":\"" << binding
		  << "\",\"source_fingerprint\":\"" << source << "\"";
	if (nullptr != pexprTarget)
	{
		event << ",\"target_fingerprint\":\""
			  << CDSLStatsExperimentSnapshot::Fingerprint(m_mp, pexprTarget)
			  << "\"";
	}
	if (nullptr != pgexpr)
	{
		event << ",\"group\":" << pgexpr->Pgroup()->Id()
			  << ",\"group_expression\":" << pgexpr->Id()
			  << ",\"memo_version\":" << m_ulDSLMemoVersion;
	}
	if (nullptr != bindingPath)
	{
		event << ",\"binding_path\":\"" << bindingPath << "\"";
	}
	event << ",\"placement\":\"" << placement << "\",\"status\":\""
		  << status << "\",\"match_us\":" << matchUs
		  << ",\"constraint_us\":" << constraintUs
		  << ",\"instantiate_us\":" << instantiateUs
		  << ",\"evaluated\":" << (nullptr != decision ? "true" : "false");
	if (nullptr != decision)
	{
		const auto contextReference = [&](const CHAR *field, const std::string &value)
		{
			const std::string key = std::string(field) + ":" + value;
			auto found = m_dsl_experiment_context_ids.find(key);
			ULONG id;
			if (m_dsl_experiment_context_ids.end() == found)
			{
				id = m_dsl_experiment_context_ids.size() + 1;
				m_dsl_experiment_context_ids.emplace(key, id);
				CAutoTrace context(m_mp);
				context.Os() << "DSL_TRACE {\"kind\":\"candidate_context\",\"engine\":\"pgorca\","
					<< "\"context_id\":" << id << ",\"field\":\"" << field
					<< "\",\"value\":" << value.c_str() << "}" << std::endl;
			}
			else
				id = found->second;
			event << ",\"" << field << "_id\":" << id;
		};
		event << ",\"bound_symbols\":"
			<< (nullptr == decision->Pmodel() ? 0 : decision->Pmodel()->Size());
		if (!decision->InputContext().empty())
			contextReference("input_context", decision->InputContext());
		contextReference("binding_context",
			CDSLStatsExperimentSnapshot::BindingContext(prule, decision->Pmodel()));
		if (nullptr != decision->PconFailed())
			event << ",\"failed_constraint\":\""
				<< CDSLConstraintKindTable::SzName(decision->PconFailed()->Edslcon())
				<< "\",\"failed_constraint_index\":" << decision->UlFailedConstraint();
	}
	event << "}";
	if (nullptr == m_pdslStatsExperimentSnapshot)
	{
		m_dsl_pending_experiment_candidates.push_back(event.str());
		return;
	}
	EmitExperimentCandidate(m_mp, m_pdslStatsExperimentSnapshot->SzId(),
						 event.str());
}

void
COptCtxt::TraceDSLExperimentCandidateOutcome(
	const CDSLRule *prule, const CHAR *status, ULONG candidateSequence,
	ULONG memoVersionBefore, const CGroup *group,
	const CGroupExpression *gexpr, ULONG insertionVersionBefore)
{
	if (0 == candidateSequence || nullptr == m_pdslStatsExperimentSnapshot ||
		!GPOS_FTRACE(EopttracePrintDSLRule))
	{
		return;
	}
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(nullptr != status);
	GPOS_ASSERT(nullptr != group);
	if (0 == std::strcmp(status, "memo_inserted"))
	{
		++m_ulDSLExperimentApplications;
	}

	CAutoTrace trace(m_mp);
	trace.Os() << "DSL_TRACE {\"kind\":\"rule_candidate_outcome\","
				   << "\"engine\":\"pgorca\",\"experiment\":\""
				   << JsonEscape(m_pdslStatsExperimentSnapshot->SzId()).c_str()
				   << "\",\"candidate_sequence\":" << candidateSequence
				   << ",\"rule_id\":"
				   << CDSLRuleEngine::Instance()->UlRuleId(prule)
				   << ",\"rule_hash\":\"" << prule->SzIdentity()
				   << "\",\"status\":\"" << status
				   << "\",\"memo_version_before\":" << memoVersionBefore
				   << ",\"memo_version_at_insertion_start\":" << insertionVersionBefore
				   << ",\"memo_version_after\":" << m_ulDSLMemoVersion
				   << ",\"group\":" << group->Id();
	if (nullptr != gexpr)
	{
		trace.Os() << ",\"group_expression\":" << gexpr->Id();
	}
	trace.Os() << "}" << std::endl;
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
	m_dsl_pending_alternative_rules[pexpr] = {
		prule, inputOrigins, m_ulDSLExperimentSequence, m_ulDSLMemoVersion};
}

const CDSLRule *
COptCtxt::PdslruleTakePendingAlternative(
	const CExpression *pexpr, CDSLTargetInputOriginArray *inputOrigins,
	ULONG *candidateSequence, ULONG *memoVersion)
{
	GPOS_ASSERT(nullptr != inputOrigins);
	GPOS_ASSERT(nullptr != candidateSequence);
	GPOS_ASSERT(nullptr != memoVersion);
	*candidateSequence = 0;
	*memoVersion = 0;
	auto found = m_dsl_pending_alternative_rules.find(pexpr);
	if (m_dsl_pending_alternative_rules.end() == found)
		return nullptr;
	const CDSLRule *prule = found->second.m_prule;
	*inputOrigins = std::move(found->second.m_input_origins);
	*candidateSequence = found->second.m_ul_candidate_sequence;
	*memoVersion = found->second.m_ul_memo_version;
	m_dsl_pending_alternative_rules.erase(found);
	return prule;
}

void
COptCtxt::RegisterDSLGroupExpressionOrigin(const CGroupExpression *pgexpr,
										 const CDSLRule *prule,
										 const CHAR *szTargetPath,
										 const CHAR *szRelation, const CHAR *outcome)
{
	GPOS_ASSERT(nullptr != pgexpr);
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(nullptr != szTargetPath);
	GPOS_ASSERT(nullptr != szRelation);
	// Equivalent generators are diagnostic alternatives, not selected-plan credit.
	GPOS_ASSERT(nullptr != outcome);
	GPOS_ASSERT(0 == std::strcmp(outcome, "memo_inserted") ||
		0 == std::strcmp(outcome, "memo_duplicate") ||
		0 == std::strcmp(outcome, "memo_rehashed"));
	if (0 != std::strcmp(outcome, "memo_inserted") &&
		(!GPOS_FTRACE(EopttracePrintDSLRule) ||
		 (0 == std::strcmp(outcome, "memo_duplicate") && 0 == m_ulDSLExperimentSequence)))
		return;
	auto &origins = m_dsl_group_expression_origins[pgexpr];
	for (const auto &origin : origins)
	{
		if (0 == std::strcmp(origin.m_prule->SzIdentity(), prule->SzIdentity()) &&
			origin.m_target_path == szTargetPath &&
			origin.m_relation == szRelation && origin.m_outcome == outcome)
			return;
	}
	origins.push_back({prule, szTargetPath, szRelation, outcome});
}

const std::vector<SDSLGroupExpressionOrigin> *
COptCtxt::DSLGroupExpressionOrigins(const CGroupExpression *pgexpr) const
{
	const auto found = m_dsl_group_expression_origins.find(pgexpr);
	return m_dsl_group_expression_origins.end() == found ? nullptr : &found->second;
}

void
COptCtxt::MergeDSLGroupExpressionOrigins(const CGroupExpression *from,
	const CGroupExpression *to)
{
	if (!GPOS_FTRACE(EopttracePrintDSLRule) || from == to)
		return;
	const auto *origins = DSLGroupExpressionOrigins(from);
	if (nullptr == origins)
		return;
	// Hash-map rehash preserves references to its values. Keep the old entry
	// for extracted bindings still referring to the duplicate expression.
	for (const auto &origin : *origins)
		RegisterDSLGroupExpressionOrigin(to, origin.m_prule,
			origin.m_target_path.c_str(), origin.m_relation.c_str(), "memo_rehashed");
}

void
COptCtxt::RecordDSLSelectedPlanRule(const CGroupExpression *pgexpr)
{
	for (const CGroupExpression *origin = pgexpr; nullptr != origin;
		 origin = origin->PgexprOrigin())
	{
		const auto *origins = DSLGroupExpressionOrigins(origin);
		if (nullptr != origins)
		{
			for (const auto &producer : *origins)
			{
				if (producer.m_outcome == "memo_inserted")
				{
					++m_dsl_selected_plan_rules[producer.m_prule->SzIdentity()];
					return;
				}
			}
		}
	}
}

void
COptCtxt::TraceDSLCBOEdge(const CDSLRule *prule,
							 const CExpression *pexprSource, const CHAR *status,
							 const std::string &bindingPath)
{
	if (!GPOS_FTRACE(EopttracePrintDSLRule))
		return;
	// Full attempt tracing uses the existing statistics-experiment stream. Keep
	// ordinary diagnostics root/ready-only: their fixed task buffer must also
	// retain the final Memo provenance and rule summaries.
	const BOOL full = 0 != m_ulDSLExperimentSequence;
	if (!full && 0 != std::strcmp(status, "ready_cbo"))
		return;
	GPOS_CHECK_STACK_SIZE;
	GPOS_CHECK_ABORT;
	// Walk the extracted binding, never other alternatives of its Memo groups.
	// An original root may consume a DSL-produced child. Binding paths are not
	// positions in an adapted DSL template, so keep these coordinate spaces apart.
	for (ULONG i = 0; full && i < pexprSource->Arity(); ++i)
	{
		TraceDSLCBOEdge(prule, (*pexprSource)[i], status,
			bindingPath + "/" + std::to_string(i));
	}
	const auto *origins = DSLGroupExpressionOrigins(pexprSource->Pgexpr());
	if (nullptr == origins)
		return;

	const CGroupExpression *pgexpr = pexprSource->Pgexpr();
	for (const auto &producer : *origins)
	{
		if (!full && producer.m_outcome != "memo_inserted")
			continue;
		CAutoTrace trace(m_mp);
		trace.Os() << "DSL_TRACE {\"kind\":\"rule_edge\",\"engine\":\"pgorca\","
			<< "\"scheduler\":\"cbo\",\"src_rule\":\""
			<< producer.m_prule->SzIdentity()
			<< "\",\"dst_rule\":\"" << prule->SzIdentity()
			<< "\",\"target_path\":\"" << producer.m_target_path.c_str()
			<< "\",\"src_target_path\":\"" << producer.m_target_path.c_str()
			<< "\",\"dst_source_path\":"
			<< (bindingPath == "r" ? "\"r\"" : "null")
			<< ",\"dst_binding_path\":\"" << bindingPath.c_str()
			<< "\",\"path_kind\":\""
			<< (bindingPath == "r" ? "instantiated_expression" : "source_binding_expression")
			<< "\",\"candidate_status\":\"" << status
			<< "\",\"dst_candidate_sequence\":" << m_ulDSLExperimentSequence
			<< ",\"binding_edge_sequence\":" << ++m_ulDSLBindingOriginEdges
			<< ",\"binding_group\":" << pgexpr->Pgroup()->Id()
			<< ",\"binding_group_expression\":" << pgexpr->Id()
			<< ",\"evidence\":\"runtime_observed\",\"relation\":\""
			<< (0 == std::strcmp(status, "ready_cbo")
				? producer.m_relation.c_str() : "binding_observed")
			<< "\",\"producer_relation\":\"" << producer.m_relation.c_str()
			<< "\",\"producer_outcome\":\""
			<< producer.m_outcome.c_str()
			<< "\"}" << std::endl;
	}
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
