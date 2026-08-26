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
#include "gpopt/operators/CLogicalInnerJoin.h"
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

CGroupExpression *
PgexprLogicalRepresentative(CGroup *group)
{
	CGroupProxy proxy(group);
	return proxy.PgexprNextLogical(nullptr);
}

CGroupExpression *
PgexprDPHyperRegionMember(CGroup *group)
{
	CGroupProxy proxy(group);
	CGroupExpression *gexpr = nullptr;
	while (nullptr != (gexpr = proxy.PgexprNextLogical(gexpr)))
	{
		if (COperator::EopLogicalInnerJoin == gexpr->Pop()->Eopid() &&
			CLogicalInnerJoin::PopConvert(gexpr->Pop())
				->FDPHyperRegionMember())
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
					  std::vector<CGroup *> *skeleton_groups)
{
	GPOS_ASSERT(nullptr != gexpr);
	GPOS_ASSERT(nullptr != skeleton_groups);
	GPOS_ASSERT(COperator::EopLogicalInnerJoin == gexpr->Pop()->Eopid());
	GPOS_ASSERT(CLogicalInnerJoin::PopConvert(gexpr->Pop())
					->FDPHyperRegionMember());

	CExpression *children[2] = {nullptr, nullptr};
	for (ULONG child = 0; child < 2; ++child)
	{
		CGroupExpression *member =
			PgexprDPHyperRegionMember((*gexpr)[child]);
		children[child] = nullptr == member
						  ? PexprGroupLeaf(mp, (*gexpr)[child])
						  : PexprBinaryJoinRegion(mp, member, skeleton_groups);
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
			m_pgexpr->SetDPHyperStatus(
				m_native_fallback_materialized
					? CGroupExpression::EdphNativeFallback
					: CGroupExpression::EdphFallback);
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

	m_pgexpr->SetDPHyperStatus(CGroupExpression::EdphSucceeded);
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
	else if (COperator::EopLogicalInnerJoin == m_pgexpr->Pop()->Eopid() &&
			 CLogicalInnerJoin::PopConvert(m_pgexpr->Pop())
				 ->FDPHyperRegionRoot())
	{
		CExpression *tree =
			PexprBinaryJoinRegion(mp, m_pgexpr, &skeleton_groups);
		if (nullptr == tree)
		{
			TraceUnsupported(m_pgexpr, "missing_predicate", 0);
			return false;
		}
		binary_spec = GPOS_NEW(mp) CJoinRegionSpec(mp);
		if (!binary_spec->Build(tree) || !binary_spec->PureInner())
		{
			tree->Release();
			GPOS_DELETE(binary_spec);
			TraceUnsupported(m_pgexpr, "invalid_binary_region", 0);
			return false;
		}
		for (ULONG node = 0; node < binary_spec->NodeCount(); ++node)
		{
			CExpression *component = binary_spec->Atom(node);
			if (nullptr == component->Pgexpr() ||
				0 < component->DeriveOuterReferences()->Size())
			{
				tree->Release();
				const ULONG node_count = binary_spec->NodeCount();
				GPOS_DELETE(binary_spec);
				TraceUnsupported(m_pgexpr, "lateral_dependency",
								 node_count);
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
									 binary_spec, skeleton_groups);
	GPOS_DELETE(region);
	GPOS_DELETE(binary_spec);
	return success;
}

BOOL
CJobJoinEnumeration::FEnumerateRegion(
	CSchedulerContext *psc, CDPHyperJoinRegion *region,
	const std::vector<CGroup *> &component_groups, const CJoinRegionSpec *spec,
	const std::vector<CGroup *> &skeleton_groups)
{
	GPOS_ASSERT(nullptr != region);
	const ULONG node_count = region->NodeCount();
	GPOS_ASSERT(node_count == component_groups.size());
	CMemoryPool *mp = psc->GetGlobalMemoryPool();
	CHint *hint =
		COptCtxt::PoctxtFromTLS()->GetOptimizerConfig()->GetHint();
	if (!region->Build())
	{
		MaterializeNativeFallback(psc, region, component_groups);
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

	CDPHyperPlan plan(mp, hint->UlDPHyperPairBudget());
	CDPHyperEnumerator enumerator(mp, region->Graph(), &plan);
	if (enumerator.Enumerate() || !plan.Complete(node_count))
	{
		MaterializeNativeFallback(psc, region, component_groups);
		if (GPOS_FTRACE(EopttracePrintXformResults))
		{
			GPOS_TRACE_FORMAT(
				"DPHyper: status=fallback reason=%s group=%d nodes=%d "
				"pairs=%d budget=%d owner=%s",
				plan.BudgetExhausted() ? "pair_budget" : "disconnected",
				m_pgexpr->Pgroup()->Id(), node_count, plan.PairCount(),
				hint->UlDPHyperPairBudget(),
				m_native_fallback_materialized ? "native_nary"
										   : "native_binary");
		}
		return false;
	}

	CEngine *engine = psc->Peng();
	CDPHyperGraphFingerprint *fingerprint = region->Pfp();
	const ULONG fingerprint_hash = fingerprint->HashValue();
	if (!engine->FRegisterDPHyperFingerprint(m_pgexpr->Pgroup(), fingerprint))
	{
		if (GPOS_FTRACE(EopttracePrintXformResults))
		{
			GPOS_TRACE_FORMAT(
				"DPHyper: status=reused group=%d nodes=%d fingerprint=%u "
				"mode=%s",
				m_pgexpr->Pgroup()->Id(), node_count, fingerprint_hash,
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

	for (const CDPHyperPlan::SPair *pair : plan.Pairs())
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

		CExpression *predicate = region->PexprPredicate(
			pair->m_left, pair->m_right, full /*include residual*/);
		CExpression *join = CUtils::PexprLogicalJoin<CLogicalInnerJoin>(
			mp, PexprGroupLeaf(mp, left_group),
			PexprGroupLeaf(mp, right_group), predicate,
			CXform::ExfExpandNAryJoinDPHyper);
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

		// DPHyp reports an unordered CSG-CMP pair once. InnerJoin is physically
		// asymmetric, so retain both orientations in the same equivalence group.
		predicate = region->PexprPredicate(pair->m_right, pair->m_left,
										 full /*include residual*/);
		join = CUtils::PexprLogicalJoin<CLogicalInnerJoin>(
			mp, PexprGroupLeaf(mp, right_group),
			PexprGroupLeaf(mp, left_group), predicate,
			CXform::ExfExpandNAryJoinDPHyper);
		CGroup *reverse_target = engine->PgroupInsert(
			target, join, CXform::ExfExpandNAryJoinDPHyper, m_pgexpr,
			!full /*intermediate*/);
		join->Release();
		GPOS_ASSERT(reverse_target == target);
	}
	if (GPOS_FTRACE(EopttracePrintXformResults))
	{
		GPOS_TRACE_FORMAT(
			"DPHyper: status=applied group=%d nodes=%d edges=%d "
			"cartesian_edges=%d pairs=%d subsets=%d fingerprint=%u mode=%s",
			m_pgexpr->Pgroup()->Id(), node_count,
			region->GeneratedEdgeCount(), region->CartesianEdgeCount(),
			plan.PairCount(), plan.SeenCount(), fingerprint_hash,
			GPOS_FTRACE(EopttraceDPHyperShadow) ? "shadow" : "replacement");
	}
	return true;
}

void
CJobJoinEnumeration::MaterializeNativeFallback(
	CSchedulerContext *psc, CDPHyperJoinRegion *region,
	const std::vector<CGroup *> &component_groups)
{
	GPOS_ASSERT(nullptr != psc && nullptr != region);
	GPOS_ASSERT(region->NodeCount() == component_groups.size());
	if (GPOS_FTRACE(EopttraceDPHyperShadow))
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
