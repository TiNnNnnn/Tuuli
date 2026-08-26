//---------------------------------------------------------------------------
//	@filename:
//		CJobJoinEnumeration.cpp
//---------------------------------------------------------------------------
#include "gpopt/search/CJobJoinEnumeration.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

#include "gpos/common/CAutoRef.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/engine/CEngine.h"
#include "gpopt/engine/CHint.h"
#include "gpopt/operators/CLogicalFullOuterJoin.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalLeftAntiSemiJoin.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"
#include "gpopt/operators/CLogicalNAryJoin.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarNAryJoinPredList.h"
#include "gpopt/optimizer/COptimizerConfig.h"
#include "gpopt/search/CGroup.h"
#include "gpopt/search/CGroupExpression.h"
#include "gpopt/search/CGroupProxy.h"
#include "gpopt/search/CJobFactory.h"
#include "gpopt/search/CJobGroupExploration.h"
#include "gpopt/search/CJobGroupExpressionExploration.h"
#include "gpopt/search/CScheduler.h"
#include "gpopt/search/CSchedulerContext.h"
#include "gpopt/xforms/CDPHyperJoinRegion.h"
#include "gpopt/xforms/CDPHyperPlan.h"
#include "gpopt/xforms/CJoinRegionSpec.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

namespace
{
class CSubsetGroups
{
	struct SEntry
	{
		CBitSet *m_nodes;
		CGroup *m_group;

		SEntry(CMemoryPool *mp, const CBitSet *nodes, CGroup *group)
			: m_nodes(GPOS_NEW(mp) CBitSet(mp, *nodes)), m_group(group)
		{
		}
		~SEntry()
		{
			m_nodes->Release();
		}
	};

	CMemoryPool *m_mp;
	std::unordered_map<ULONG, std::vector<SEntry *>> m_buckets;
	std::vector<SEntry *> m_entries;

public:
	explicit CSubsetGroups(CMemoryPool *mp) : m_mp(mp)
	{
	}

	~CSubsetGroups()
	{
		for (SEntry *entry : m_entries)
		{
			GPOS_DELETE(entry);
		}
	}

	CGroup *
	Lookup(const CBitSet *nodes) const
	{
		auto bucket = m_buckets.find(nodes->HashValue());
		if (m_buckets.end() == bucket)
		{
			return nullptr;
		}
		for (const SEntry *entry : bucket->second)
		{
			if (entry->m_nodes->Equals(nodes))
			{
				return entry->m_group;
			}
		}
		return nullptr;
	}

	void
	Record(const CBitSet *nodes, CGroup *group)
	{
		CGroup *existing = Lookup(nodes);
		GPOS_ASSERT_IMP(nullptr != existing, existing == group);
		if (nullptr != existing)
		{
			return;
		}
		SEntry *entry = GPOS_NEW(m_mp) SEntry(m_mp, nodes, group);
		m_entries.push_back(entry);
		m_buckets[nodes->HashValue()].push_back(entry);
	}
};

CExpression *
PexprGroupLeaf(CMemoryPool *mp, CGroup *group)
{
	CGroupProxy proxy(group);
	CGroupExpression *gexpr = proxy.PgexprNextLogical(nullptr);
	GPOS_ASSERT(nullptr != gexpr);
	gexpr->Pop()->AddRef();
	return GPOS_NEW(mp) CExpression(mp, gexpr->Pop(), gexpr);
}

CExpression *
PexprJoin(CMemoryPool *mp, COperator::EOperatorId join_type,
		  CGroup *left, CGroup *right, CExpression *predicate)
{
	switch (join_type)
	{
		case COperator::EopLogicalInnerJoin:
			return CUtils::PexprLogicalJoin<CLogicalInnerJoin>(
				mp, PexprGroupLeaf(mp, left), PexprGroupLeaf(mp, right),
				predicate, CXform::ExfExpandNAryJoinDPHyper);
		case COperator::EopLogicalLeftOuterJoin:
			return CUtils::PexprLogicalJoin<CLogicalLeftOuterJoin>(
				mp, PexprGroupLeaf(mp, left), PexprGroupLeaf(mp, right),
				predicate, CXform::ExfExpandNAryJoinDPHyper);
		case COperator::EopLogicalLeftSemiJoin:
			return CUtils::PexprLogicalJoin<CLogicalLeftSemiJoin>(
				mp, PexprGroupLeaf(mp, left), PexprGroupLeaf(mp, right),
				predicate, CXform::ExfExpandNAryJoinDPHyper);
		case COperator::EopLogicalLeftAntiSemiJoin:
			return CUtils::PexprLogicalJoin<CLogicalLeftAntiSemiJoin>(
				mp, PexprGroupLeaf(mp, left), PexprGroupLeaf(mp, right),
				predicate, CXform::ExfExpandNAryJoinDPHyper);
		case COperator::EopLogicalFullOuterJoin:
			return CUtils::PexprLogicalJoin<CLogicalFullOuterJoin>(
				mp, PexprGroupLeaf(mp, left), PexprGroupLeaf(mp, right),
				predicate, CXform::ExfExpandNAryJoinDPHyper);
		default:
			GPOS_ASSERT(!"Unsupported DPHyper join type");
			return nullptr;
	}
}

CGroupExpression *
PgexprLogicalRepresentative(CGroup *group)
{
	CGroupProxy proxy(group);
	return proxy.PgexprNextLogical(nullptr);
}

BOOL
FSupportedDPHyperJoin(COperator::EOperatorId op_id)
{
	switch (op_id)
	{
		case COperator::EopLogicalInnerJoin:
		case COperator::EopLogicalLeftOuterJoin:
		case COperator::EopLogicalLeftSemiJoin:
		case COperator::EopLogicalLeftAntiSemiJoin:
		case COperator::EopLogicalFullOuterJoin:
			return true;
		default:
			return false;
	}
}

CGroupExpression *
PgexprDPHyperRegionMember(CGroup *group)
{
	CGroupProxy proxy(group);
	CGroupExpression *gexpr = nullptr;
	while (nullptr != (gexpr = proxy.PgexprNextLogical(gexpr)))
	{
		if (FSupportedDPHyperJoin(gexpr->Pop()->Eopid()) &&
			CLogicalJoin::PopConvert(gexpr->Pop())->FDPHyperRegionMember())
		{
			return gexpr;
		}
	}
	return nullptr;
}

// Reconstruct only the immutable, preprocessing-marked binary skeleton from
// Memo. Alternatives produced by commutativity, associativity, or DSL xforms
// are deliberately ignored. This is the ORCA equivalent of Horn's recursive
// walk over GetFirstExpr() for one maximal join region.
CExpression *
PexprBinaryJoinRegion(CMemoryPool *mp, CGroupExpression *gexpr,
					  std::vector<CGroup *> *skeleton_groups,
					  std::vector<CGroupExpression *> *region_members)
{
	GPOS_ASSERT(nullptr != gexpr);
	GPOS_ASSERT(nullptr != skeleton_groups);
	GPOS_ASSERT(nullptr != region_members);
	GPOS_ASSERT(FSupportedDPHyperJoin(gexpr->Pop()->Eopid()));
	GPOS_ASSERT(
		CLogicalJoin::PopConvert(gexpr->Pop())->FDPHyperRegionMember());

	CExpression *children[2] = {nullptr, nullptr};
	for (ULONG child = 0; child < 2; ++child)
	{
		CGroupExpression *member =
			PgexprDPHyperRegionMember((*gexpr)[child]);
		children[child] = nullptr == member
						  ? PexprGroupLeaf(mp, (*gexpr)[child])
						  : PexprBinaryJoinRegion(mp, member, skeleton_groups,
										  region_members);
		if (nullptr == children[child])
		{
			if (0 < child)
			{
				children[0]->Release();
			}
			return nullptr;
		}
	}
	CExpression *predicate = (*gexpr)[2]->PexprScalarRep();
	if (nullptr == predicate)
	{
		children[0]->Release();
		children[1]->Release();
		return nullptr;
	}
	predicate->AddRef();
	// CJoinRegionSpec records edges in the same post-order. Retain the Memo
	// group for every original subtree, as Horn does while extracting its
	// hypergraph, so an existing node-set is never rematerialized elsewhere.
	skeleton_groups->push_back(gexpr->Pgroup());
	region_members->push_back(gexpr);
	gexpr->Pop()->AddRef();
	return GPOS_NEW(mp) CExpression(mp, gexpr->Pop(), children[0], children[1],
									 predicate);
}

void
TraceUnsupported(CGroupExpression *pgexpr, const CHAR *reason,
				 ULONG node_count)
{
	if (GPOS_FTRACE(EopttracePrintXformResults))
	{
		GPOS_TRACE_FORMAT(
			"DPHyper: status=fallback reason=%s group=%d nodes=%d",
			reason, pgexpr->Pgroup()->Id(), node_count);
	}
}

void
PublishRegionStatus(
	const std::vector<CGroupExpression *> &region_members,
	CGroupExpression::EDPHyperStatus status)
{
	GPOS_ASSERT(CGroupExpression::EdphSucceeded == status ||
				CGroupExpression::EdphFallback == status ||
				CGroupExpression::EdphNativeFallback == status);
	for (CGroupExpression *member : region_members)
	{
		if (CGroupExpression::EdphUnrequested == member->DPHyperStatus())
		{
			member->SetDPHyperStatus(CGroupExpression::EdphScheduled);
		}
		if (CGroupExpression::EdphScheduled == member->DPHyperStatus())
		{
			member->SetDPHyperStatus(status);
		}
		// A nested member may already own a completed smaller region. Preserve
		// that terminal decision; the new maximal root publishes its own status.
	}
}

BOOL
FBuildBinarySkeletonPlan(CMemoryPool *mp, CDPHyperJoinRegion *region,
						 const CJoinRegionSpec *spec, CDPHyperPlan *plan)
{
	GPOS_ASSERT(nullptr != mp && nullptr != region && nullptr != spec &&
				nullptr != plan);
	for (ULONG node = 0; node < spec->NodeCount(); ++node)
	{
		if (plan->FoundSingleNode(node))
		{
			return false;
		}
	}

	// CJoinRegionSpec records the immutable input tree in post-order. Every
	// child subset is therefore present before its parent cut. Replaying these
	// n-1 cuts is a complete semantic plan even when exhaustive CSG-CMP search
	// exceeded its alternative budget.
	for (ULONG edge = 0; edge < spec->EdgeCount(); ++edge)
	{
		const CJoinRegionSpec::CEdge *skeleton_edge = spec->Edge(edge);
		if (!plan->HasSeen(skeleton_edge->Left()) ||
			!plan->HasSeen(skeleton_edge->Right()))
		{
			return false;
		}
		CDPHyperJoinRegion::SJoinRequest request;
		if (!region->FBuildJoinRequest(skeleton_edge->Left(),
								   skeleton_edge->Right(), &request) ||
			plan->FoundSubgraphPair(skeleton_edge->Left(),
								 skeleton_edge->Right(), edge))
		{
			return false;
		}
	}
	return plan->Complete(spec->NodeCount());
}
}  // namespace

CJobJoinEnumeration::CJobJoinEnumeration()
	: m_pgexpr(nullptr),
	  m_materialized(false),
	  m_native_fallback_materialized(false)
{
}

CJobJoinEnumeration::~CJobJoinEnumeration() = default;

void
CJobJoinEnumeration::Init(CGroupExpression *pgexpr)
{
	GPOS_ASSERT(!FInit());
	GPOS_ASSERT(nullptr != pgexpr);
	m_pgexpr = pgexpr;
	m_materialized = false;
	m_native_fallback_materialized = false;
	m_intermediate_groups.clear();
	SetInit();
}

void
CJobJoinEnumeration::ScheduleJob(CSchedulerContext *psc,
								 CGroupExpression *pgexpr, CJob *parent)
{
	CJob *job = psc->Pjf()->PjCreate(CJob::EjtJoinEnumeration);
	CJobJoinEnumeration *enumeration = PjConvert(job);
	enumeration->Init(pgexpr);
	psc->Psched()->Add(enumeration, parent);
}

BOOL
CJobJoinEnumeration::FExecute(CSchedulerContext *psc)
{
	GPOS_ASSERT(FInit());
	if (!m_materialized)
	{
		if (!FEnumerate(psc))
		{
			const CGroupExpression::EDPHyperStatus status =
				m_native_fallback_materialized
					? CGroupExpression::EdphNativeFallback
					: CGroupExpression::EdphFallback;
			if (CGroupExpression::EdphScheduled ==
				m_pgexpr->DPHyperStatus())
			{
				m_pgexpr->SetDPHyperStatus(status);
			}
			else
			{
				GPOS_ASSERT(status == m_pgexpr->DPHyperStatus());
			}
			return true;
		}
		m_materialized = true;
	}

	// DPHyper can reuse a subset group which was explored before this maximal
	// region was enumerated. In that case the group job queue is already closed,
	// but the newly inserted DPHyper expressions still need their exploration
	// xforms. Schedule them directly and repeat until transformation results no
	// longer add unexplored expressions. Fresh subset groups use the normal group
	// exploration job, which performs the same closure and advances group state.
	BOOL scheduled = false;
	for (CGroup *group : m_intermediate_groups)
	{
		if (!group->FExplored())
		{
			CJobGroupExploration::ScheduleJob(psc, group, this);
			scheduled = true;
			continue;
		}

		CGroupProxy gp(group);
		CGroupExpression *pgexpr = gp.PgexprFirst();
		while (nullptr != pgexpr)
		{
			CGroupExpression *next = gp.PgexprNext(pgexpr);
			if (pgexpr->Pop()->FLogical() && !pgexpr->FExplored())
			{
				CJobGroupExpressionExploration::ScheduleJob(psc, pgexpr,
													this);
				scheduled = true;
			}
			pgexpr = next;
		}
	}
	if (scheduled)
	{
		return false;
	}

	if (CGroupExpression::EdphScheduled == m_pgexpr->DPHyperStatus())
	{
		m_pgexpr->SetDPHyperStatus(CGroupExpression::EdphSucceeded);
	}
	else
	{
		GPOS_ASSERT(CGroupExpression::EdphSucceeded ==
						m_pgexpr->DPHyperStatus());
	}
	return true;
}

BOOL
CJobJoinEnumeration::FEnumerate(CSchedulerContext *psc)
{
	CMemoryPool *mp = psc->GetGlobalMemoryPool();
	CDPHyperJoinRegion *region = nullptr;
	CJoinRegionSpec *binary_spec = nullptr;
	std::vector<CGroup *> component_groups;
	std::vector<CGroup *> skeleton_groups;
	std::vector<CGroupExpression *> region_members;

	if (COperator::EopLogicalNAryJoin == m_pgexpr->Pop()->Eopid() &&
		3 <= m_pgexpr->Arity())
	{
		CLogicalNAryJoin *nary =
			CLogicalNAryJoin::PopConvert(m_pgexpr->Pop());
		const ULONG node_count = m_pgexpr->Arity() - 1;
		if (nary->HasOuterJoinChildren())
		{
			TraceUnsupported(m_pgexpr, "non_inner_join", node_count);
			return false;
		}

		CExpressionArray *components = GPOS_NEW(mp) CExpressionArray(mp);
		for (ULONG node = 0; node < node_count; ++node)
		{
			CGroupExpression *child_gexpr =
				PgexprLogicalRepresentative((*m_pgexpr)[node]);
			if (nullptr == child_gexpr)
			{
				components->Release();
				TraceUnsupported(m_pgexpr, "missing_component", node_count);
				return false;
			}
			child_gexpr->Pop()->AddRef();
			CExpression *component =
				GPOS_NEW(mp) CExpression(mp, child_gexpr->Pop(), child_gexpr);
			// Sibling-correlated/LATERAL regions require directed dependency
			// edges. Until represented, retain native enumeration.
			if (0 < component->DeriveOuterReferences()->Size())
			{
				component->Release();
				components->Release();
				TraceUnsupported(m_pgexpr, "lateral_dependency", node_count);
				return false;
			}
			components->Append(component);
			component_groups.push_back((*m_pgexpr)[node]);
		}

		CExpression *scalar = (*m_pgexpr)[node_count]->PexprScalarRep();
		if (nullptr == scalar ||
			COperator::EopScalarNAryJoinPredList == scalar->Pop()->Eopid())
		{
			components->Release();
			TraceUnsupported(m_pgexpr,
						 nullptr == scalar ? "missing_predicate"
									   : "non_inner_predicate_list",
						 node_count);
			return false;
		}
		CExpressionArray *conjuncts =
			CPredicateUtils::PdrgpexprConjuncts(mp, scalar);
		CHint *hint =
			COptCtxt::PoctxtFromTLS()->GetOptimizerConfig()->GetHint();
		region = GPOS_NEW(mp) CDPHyperJoinRegion(
			mp, components, conjuncts, hint->UlDPHyperEdgeBudget());
		components->Release();
		conjuncts->Release();
	}
	else if (FSupportedDPHyperJoin(m_pgexpr->Pop()->Eopid()) &&
			 CLogicalJoin::PopConvert(m_pgexpr->Pop())
				 ->FDPHyperRegionRoot())
	{
		CExpression *tree =
			PexprBinaryJoinRegion(mp, m_pgexpr, &skeleton_groups,
							  &region_members);
		if (nullptr == tree)
		{
			TraceUnsupported(m_pgexpr, "missing_predicate", 0);
			return false;
		}
		binary_spec = GPOS_NEW(mp) CJoinRegionSpec(mp);
		if (!binary_spec->Build(tree) || !binary_spec->CDCSupported())
		{
			tree->Release();
			GPOS_DELETE(binary_spec);
			TraceUnsupported(m_pgexpr, "invalid_binary_region", 0);
			return false;
		}
		if (binary_spec->HasExternalDependencies())
		{
			tree->Release();
			const ULONG node_count = binary_spec->NodeCount();
			GPOS_DELETE(binary_spec);
			TraceUnsupported(m_pgexpr, "lateral_dependency", node_count);
			return false;
		}
		for (ULONG node = 0; node < binary_spec->NodeCount(); ++node)
		{
			CExpression *component = binary_spec->Atom(node);
			if (nullptr == component->Pgexpr())
			{
				tree->Release();
				const ULONG node_count = binary_spec->NodeCount();
				GPOS_DELETE(binary_spec);
				TraceUnsupported(m_pgexpr, "missing_component", node_count);
				return false;
			}
			component_groups.push_back(component->Pgexpr()->Pgroup());
		}
		CHint *hint =
			COptCtxt::PoctxtFromTLS()->GetOptimizerConfig()->GetHint();
		region = GPOS_NEW(mp)
			CDPHyperJoinRegion(mp, binary_spec, hint->UlDPHyperEdgeBudget());
		tree->Release();
	}
	else
	{
		TraceUnsupported(m_pgexpr, "not_join_region_owner", 0);
		return false;
	}

	const BOOL success = FEnumerateRegion(psc, region, component_groups,
									 binary_spec, skeleton_groups,
									 region_members);
	GPOS_DELETE(region);
	GPOS_DELETE(binary_spec);
	return success;
}

BOOL
CJobJoinEnumeration::FEnumerateRegion(
	CSchedulerContext *psc, CDPHyperJoinRegion *region,
	const std::vector<CGroup *> &component_groups, const CJoinRegionSpec *spec,
	const std::vector<CGroup *> &skeleton_groups,
	const std::vector<CGroupExpression *> &region_members)
{
	GPOS_ASSERT(nullptr != region);
	const ULONG node_count = region->NodeCount();
	GPOS_ASSERT(node_count == component_groups.size());
	CMemoryPool *mp = psc->GetGlobalMemoryPool();
	CHint *hint =
		COptCtxt::PoctxtFromTLS()->GetOptimizerConfig()->GetHint();
	if (!region->Build())
	{
		MaterializeNativeFallback(psc, region, component_groups, spec);
		PublishRegionStatus(
			region_members,
			m_native_fallback_materialized
				? CGroupExpression::EdphNativeFallback
				: CGroupExpression::EdphFallback);
		if (GPOS_FTRACE(EopttracePrintXformResults))
		{
			GPOS_TRACE_FORMAT(
				"DPHyper: status=fallback reason=edge_budget group=%d "
				"nodes=%d edges=%d budget=%d owner=%s",
				m_pgexpr->Pgroup()->Id(), node_count,
				region->GeneratedEdgeCount(), hint->UlDPHyperEdgeBudget(),
				m_native_fallback_materialized ? "native_nary"
										   : "native_binary");
		}
		return false;
	}

	CDPHyperPlan exhaustive_plan(
		mp, hint->UlDPHyperPairBudget(),
		[region](const CBitSet *left, const CBitSet *right, ULONG edge_id) {
			if (!region->FPairApplicable(left, right, edge_id))
			{
				return false;
			}
			CDPHyperJoinRegion::SJoinRequest request;
			return region->FBuildJoinRequest(left, right, &request);
		});
	CDPHyperEnumerator enumerator(mp, region->Graph(), &exhaustive_plan);
	const BOOL enumeration_aborted = enumerator.Enumerate();
	const BOOL exhaustive_complete = exhaustive_plan.Complete(node_count);
	const BOOL budget_exhausted = exhaustive_plan.BudgetExhausted();
	const ULONG attempted_pairs = exhaustive_plan.PairCount();
	CDPHyperPlan skeleton_plan(mp, node_count - 1);
	const BOOL simplified =
		(!exhaustive_complete && budget_exhausted && nullptr != spec &&
		 FBuildBinarySkeletonPlan(mp, region, spec, &skeleton_plan));
	const CDPHyperPlan *plan =
		simplified ? &skeleton_plan : &exhaustive_plan;
	if ((!exhaustive_complete && !simplified) ||
		(enumeration_aborted && !budget_exhausted))
	{
		MaterializeNativeFallback(psc, region, component_groups, spec);
		PublishRegionStatus(
			region_members,
			m_native_fallback_materialized
				? CGroupExpression::EdphNativeFallback
				: CGroupExpression::EdphFallback);
		if (GPOS_FTRACE(EopttracePrintXformResults))
		{
			GPOS_TRACE_FORMAT(
				"DPHyper: status=fallback reason=%s group=%d nodes=%d "
				"pairs=%d budget=%d owner=%s",
				budget_exhausted ? "pair_budget" : "disconnected",
				m_pgexpr->Pgroup()->Id(), node_count, attempted_pairs,
				hint->UlDPHyperPairBudget(),
				m_native_fallback_materialized ? "native_nary"
										   : "native_binary");
		}
		return false;
	}

	CEngine *engine = psc->Peng();
	const ULONG dependency_count =
		nullptr == spec ? 0 : spec->DependencyCount();
	CDPHyperGraphFingerprint *fingerprint = region->Pfp();
	const ULONG fingerprint_hash = fingerprint->HashValue();
	if (!engine->FRegisterDPHyperFingerprint(m_pgexpr->Pgroup(), fingerprint))
	{
		PublishRegionStatus(region_members, CGroupExpression::EdphSucceeded);
		if (GPOS_FTRACE(EopttracePrintXformResults))
		{
			GPOS_TRACE_FORMAT(
				"DPHyper: status=reused group=%d root=%s nodes=%d "
				"dependencies=%d fingerprint=%u mode=%s",
				m_pgexpr->Pgroup()->Id(), m_pgexpr->Pop()->SzId(), node_count,
				dependency_count, fingerprint_hash,
				GPOS_FTRACE(EopttraceDPHyperShadow) ? "shadow"
											 : "replacement");
		}
		return true;
	}

	// Enumeration above is side-effect free. From this point onward all child
	// subsets are known to exist and pairs are topologically ordered, so Memo
	// insertion cannot expose a partial DPHyper result due to a graph/budget
	// failure.
	CSubsetGroups subset_groups(mp);
	for (ULONG node = 0; node < node_count; ++node)
	{
		CAutoRef<CBitSet> singleton(GPOS_NEW(mp) CBitSet(mp));
		(void) singleton->ExchangeSet(node);
		subset_groups.Record(singleton.Value(), component_groups[node]);
	}
	if (nullptr != spec)
	{
		GPOS_ASSERT(spec->EdgeCount() == skeleton_groups.size());
		for (ULONG edge = 0; edge < spec->EdgeCount(); ++edge)
		{
			CAutoRef<CBitSet> joined(
				GPOS_NEW(mp) CBitSet(mp, *spec->Edge(edge)->Left()));
			joined->Union(spec->Edge(edge)->Right());
			subset_groups.Record(joined.Value(), skeleton_groups[edge]);
		}
	}

	for (const CDPHyperPlan::SPair *pair : plan->Pairs())
	{
		CGroup *left_group = subset_groups.Lookup(pair->m_left);
		CGroup *right_group = subset_groups.Lookup(pair->m_right);
		GPOS_ASSERT(nullptr != left_group && nullptr != right_group);

		CAutoRef<CBitSet> joined(
			GPOS_NEW(mp) CBitSet(mp, *pair->m_left));
		joined->Union(pair->m_right);
		const BOOL full = joined->Size() == node_count;
		CGroup *target = full ? m_pgexpr->Pgroup()
							  : subset_groups.Lookup(joined.Value());

		CDPHyperJoinRegion::SJoinRequest request{};
		const BOOL complex_region = nullptr != spec && !spec->PureInner();
		const BOOL request_built =
			nullptr == spec || region->FBuildJoinRequest(
							   pair->m_left, pair->m_right, &request);
		GPOS_ASSERT(request_built);
		(void) request_built;
		COperator::EOperatorId join_type = COperator::EopLogicalInnerJoin;
		CExpression *predicate = nullptr;
		if (complex_region)
		{
			join_type = request.m_join_type;
			predicate = region->PexprPredicate(request);
		}
		else
		{
			predicate = region->PexprPredicate(
				pair->m_left, pair->m_right, full /*include residual*/);
		}
		if (nullptr != spec && request.m_swapped)
		{
			std::swap(left_group, right_group);
		}
		CExpression *join =
			PexprJoin(mp, join_type, left_group, right_group, predicate);
		target = engine->PgroupInsert(
			target, join, CXform::ExfExpandNAryJoinDPHyper, m_pgexpr,
			!full /*intermediate*/);
		join->Release();
		GPOS_ASSERT(nullptr != target);
		subset_groups.Record(joined.Value(), target);
		if (!full &&
			m_intermediate_groups.end() ==
				std::find(m_intermediate_groups.begin(),
						  m_intermediate_groups.end(), target))
		{
			m_intermediate_groups.push_back(target);
		}

		// DPHyp reports an unordered CSG-CMP pair once. Inner and FullOuter
		// joins are commutative logical operators, so retain both physical input
		// orientations. Directional joins keep only the CD-C-approved ordering.
		if ((COperator::EopLogicalInnerJoin == join_type ||
			 COperator::EopLogicalFullOuterJoin == join_type) &&
			(nullptr == spec || !request.m_dependency_directional))
		{
			predicate = complex_region
						? region->PexprPredicate(request)
						: region->PexprPredicate(
							  pair->m_right, pair->m_left,
							  full /*include residual*/);
			join = PexprJoin(mp, join_type, right_group, left_group, predicate);
			CGroup *reverse_target = engine->PgroupInsert(
				target, join, CXform::ExfExpandNAryJoinDPHyper, m_pgexpr,
				!full /*intermediate*/);
			join->Release();
			GPOS_ASSERT(reverse_target == target);
		}
	}
	if (GPOS_FTRACE(EopttracePrintXformResults))
	{
		GPOS_TRACE_FORMAT(
			"DPHyper: status=applied group=%d root=%s nodes=%d edges=%d "
			"cartesian_edges=%d dependencies=%d pairs=%d subsets=%d "
			"fingerprint=%u enumeration=%s strategy=%s reason=%s "
			"attempted_pairs=%d mode=%s",
			m_pgexpr->Pgroup()->Id(), m_pgexpr->Pop()->SzId(), node_count,
			region->GeneratedEdgeCount(), region->CartesianEdgeCount(),
			dependency_count, plan->PairCount(), plan->SeenCount(), fingerprint_hash,
			simplified ? "simplified" : "complete",
			simplified ? "binary_skeleton" : "none",
			simplified ? "pair_budget" : "none", attempted_pairs,
			GPOS_FTRACE(EopttraceDPHyperShadow) ? "shadow" : "replacement");
	}
	PublishRegionStatus(region_members, CGroupExpression::EdphSucceeded);
	return true;
}

void
CJobJoinEnumeration::MaterializeNativeFallback(
	CSchedulerContext *psc, CDPHyperJoinRegion *region,
	const std::vector<CGroup *> &component_groups,
	const CJoinRegionSpec *spec)
{
	GPOS_ASSERT(nullptr != psc && nullptr != region);
	GPOS_ASSERT(region->NodeCount() == component_groups.size());
	// A mixed join skeleton cannot be represented by LogicalNAryJoin without
	// losing join type and direction. Because DPHyper runs before child-group
	// exploration, publishing ordinary fallback is sufficient: the preserved
	// binary skeleton and all native swap xforms remain untouched.
	if (GPOS_FTRACE(EopttraceDPHyperShadow) ||
		(nullptr != spec && !spec->PureInner()))
	{
		return;
	}
	CMemoryPool *mp = psc->GetGlobalMemoryPool();
	CExpressionArray *children = GPOS_NEW(mp) CExpressionArray(mp);
	for (CGroup *group : component_groups)
	{
		children->Append(PexprGroupLeaf(mp, group));
	}
	children->Append(region->PexprAllPredicates());
	CExpression *fallback = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalNAryJoin(mp), children);
	CGroup *target = psc->Peng()->PgroupInsert(
		m_pgexpr->Pgroup(), fallback, CXform::ExfExpandNAryJoinDPHyper,
		m_pgexpr, false /*intermediate*/);
	fallback->Release();
	GPOS_ASSERT(nullptr != target);
	GPOS_ASSERT(CGroup::FDuplicateGroups(target, m_pgexpr->Pgroup()));
	m_native_fallback_materialized = true;
}

#ifdef GPOS_DEBUG
IOstream &
CJobJoinEnumeration::OsPrint(IOstream &os) const
{
	return os << "DPHyper join enumeration for group expression "
			  << (nullptr == m_pgexpr ? GPOPT_INVALID_GEXPR_ID : m_pgexpr->Id());
}
#endif
