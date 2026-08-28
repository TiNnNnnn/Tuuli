//---------------------------------------------------------------------------
//	@filename:
//		CJobJoinEnumeration.cpp
//---------------------------------------------------------------------------
#include "gpopt/search/CJobJoinEnumeration.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

#include "gpos/common/CAutoRef.h"
#include "gpos/common/CWallClock.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/engine/CEngine.h"
#include "gpopt/engine/CHint.h"
#include "gpopt/operators/CLogicalFullOuterJoin.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalLeftAntiSemiJoin.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CNormalizer.h"
#include "gpopt/operators/CPredicateUtils.h"
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
#include "gpopt/xforms/CDPHyperGraphSimplifier.h"
#include "gpopt/xforms/CDPHyperPlan.h"
#include "gpopt/xforms/CJoinOrderGreedy.h"
#include "gpopt/xforms/CJoinRegionSpec.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

BOOL
CJobJoinEnumeration::FReplacesNativeXform(CXform::EXformId exfid)
{
	switch (exfid)
	{
		case CXform::ExfInnerJoinCommutativity:
		case CXform::ExfJoinAssociativity:
		case CXform::ExfInnerJoinSemiJoinSwap:
		case CXform::ExfInnerJoinAntiSemiJoinSwap:
		case CXform::ExfInnerJoinAntiSemiJoinNotInSwap:
		case CXform::ExfLeftJoin2RightJoin:
		case CXform::ExfSemiJoinSemiJoinSwap:
		case CXform::ExfSemiJoinAntiSemiJoinSwap:
		case CXform::ExfSemiJoinAntiSemiJoinNotInSwap:
		case CXform::ExfSemiJoinInnerJoinSwap:
		case CXform::ExfAntiSemiJoinAntiSemiJoinSwap:
		case CXform::ExfAntiSemiJoinAntiSemiJoinNotInSwap:
		case CXform::ExfAntiSemiJoinSemiJoinSwap:
		case CXform::ExfAntiSemiJoinInnerJoinSwap:
		case CXform::ExfFullJoinCommutativity:
			return true;
		default:
			return false;
	}
}

BOOL
CJobJoinEnumeration::FNativeJoinEnumerationXform(CXform::EXformId exfid)
{
	if (FReplacesNativeXform(exfid))
	{
		return true;
	}
	switch (exfid)
	{
		case CXform::ExfAntiSemiJoinNotInAntiSemiJoinSwap:
		case CXform::ExfAntiSemiJoinNotInAntiSemiJoinNotInSwap:
		case CXform::ExfAntiSemiJoinNotInSemiJoinSwap:
		case CXform::ExfAntiSemiJoinNotInInnerJoinSwap:
			return true;
		default:
			return false;
	}
}

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

	static CGroup *
	CanonicalGroup(CGroup *group)
	{
		GPOS_ASSERT(nullptr != group);
		while (group->FDuplicateGroup())
		{
			group = group->PgroupDuplicate();
		}
		return group;
	}

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
				return CanonicalGroup(entry->m_group);
			}
		}
		return nullptr;
	}

	CGroup *
	Record(const CBitSet *nodes, CGroup *group)
	{
		group = CanonicalGroup(group);
		CGroup *existing = Lookup(nodes);
		if (nullptr != existing)
		{
			if (existing != group)
			{
				// PgroupInsert may find an identical expression in another Memo
				// group and return that container instead of the requested subset
				// group.  This is the same deferred-equivalence state handled by
				// CEngine::InsertXformResult: the groups denote the same DPHyper
				// node set and must be joined in the Memo duplicate chain before
				// later pairs use the subset as an input.
				GPOS_ASSERT(!CGroup::FReachable(m_mp, existing, group));
				GPOS_ASSERT(!CGroup::FReachable(m_mp, group, existing));
				CMemo::MarkDuplicates(existing, group);
				existing = CanonicalGroup(existing);
			}
			return existing;
		}
		SEntry *entry = GPOS_NEW(m_mp) SEntry(m_mp, nodes, group);
		m_entries.push_back(entry);
		m_buckets[nodes->HashValue()].push_back(entry);
		return group;
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

void
DeriveStats(CMemoryPool *mp, CExpression *expr)
{
	if (nullptr == expr->Pstats())
	{
		CExpressionHandle exprhdl(mp);
		exprhdl.Attach(expr);
		exprhdl.DeriveStats(mp, mp, nullptr /*required properties*/,
							nullptr /*stats context*/);
	}
	GPOS_ASSERT(nullptr != expr->Pstats());
}

// Insert a binary logical tree while retaining every newly-created relational
// child group for the same exploration closure used by successful DPHyper
// enumeration. Scalar subtrees remain immutable expression children.
CGroup *
PgroupMaterializeBinaryTree(CMemoryPool *mp, CEngine *engine,
							CExpression *expr, CGroup *target,
							CGroupExpression *origin,
							std::vector<CGroup *> *intermediate_groups)
{
	GPOS_ASSERT(nullptr != mp && nullptr != engine && nullptr != expr &&
				nullptr != origin && nullptr != intermediate_groups);
	if (nullptr != expr->Pgexpr())
	{
		GPOS_ASSERT(nullptr == target);
		return expr->Pgexpr()->Pgroup();
	}

	CExpressionArray *children = GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG child = 0; child < expr->Arity(); ++child)
	{
		CExpression *child_expr = (*expr)[child];
		if (child_expr->Pop()->FScalar())
		{
			child_expr->AddRef();
			children->Append(child_expr);
			continue;
		}
		CGroup *child_group = PgroupMaterializeBinaryTree(
			mp, engine, child_expr, nullptr, origin, intermediate_groups);
		children->Append(PexprGroupLeaf(mp, child_group));
	}
	expr->Pop()->AddRef();
	CExpression *materialized =
		GPOS_NEW(mp) CExpression(mp, expr->Pop(), children);
	CGroup *group = engine->PgroupInsert(
		target, materialized, CXform::ExfDPHyperJoinRegion, origin,
		nullptr == target /*intermediate*/);
	materialized->Release();
	GPOS_ASSERT(nullptr != group);
	if (nullptr == target &&
		intermediate_groups->end() ==
			std::find(intermediate_groups->begin(), intermediate_groups->end(),
					  group))
	{
		intermediate_groups->push_back(group);
	}
	return group;
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
				predicate, CXform::ExfDPHyperJoinRegion);
		case COperator::EopLogicalLeftOuterJoin:
			return CUtils::PexprLogicalJoin<CLogicalLeftOuterJoin>(
				mp, PexprGroupLeaf(mp, left), PexprGroupLeaf(mp, right),
				predicate, CXform::ExfDPHyperJoinRegion);
		case COperator::EopLogicalLeftSemiJoin:
			return CUtils::PexprLogicalJoin<CLogicalLeftSemiJoin>(
				mp, PexprGroupLeaf(mp, left), PexprGroupLeaf(mp, right),
				predicate, CXform::ExfDPHyperJoinRegion);
		case COperator::EopLogicalLeftAntiSemiJoin:
			return CUtils::PexprLogicalJoin<CLogicalLeftAntiSemiJoin>(
				mp, PexprGroupLeaf(mp, left), PexprGroupLeaf(mp, right),
				predicate, CXform::ExfDPHyperJoinRegion);
		case COperator::EopLogicalFullOuterJoin:
			return CUtils::PexprLogicalJoin<CLogicalFullOuterJoin>(
				mp, PexprGroupLeaf(mp, left), PexprGroupLeaf(mp, right),
				predicate, CXform::ExfDPHyperJoinRegion);
		default:
			GPOS_ASSERT(!"Unsupported DPHyper join type");
			return nullptr;
	}
}

CExpression *
PexprJoinForCost(CMemoryPool *mp, COperator::EOperatorId join_type,
				 CExpression *left, CExpression *right,
				 CExpression *predicate)
{
	left->AddRef();
	right->AddRef();
	switch (join_type)
	{
		case COperator::EopLogicalInnerJoin:
			return CUtils::PexprLogicalJoin<CLogicalInnerJoin>(
				mp, left, right, predicate,
				CXform::ExfDPHyperJoinRegion);
		case COperator::EopLogicalLeftOuterJoin:
			return CUtils::PexprLogicalJoin<CLogicalLeftOuterJoin>(
				mp, left, right, predicate,
				CXform::ExfDPHyperJoinRegion);
		case COperator::EopLogicalLeftSemiJoin:
			return CUtils::PexprLogicalJoin<CLogicalLeftSemiJoin>(
				mp, left, right, predicate,
				CXform::ExfDPHyperJoinRegion);
		case COperator::EopLogicalLeftAntiSemiJoin:
			return CUtils::PexprLogicalJoin<CLogicalLeftAntiSemiJoin>(
				mp, left, right, predicate,
				CXform::ExfDPHyperJoinRegion);
		case COperator::EopLogicalFullOuterJoin:
			return CUtils::PexprLogicalJoin<CLogicalFullOuterJoin>(
				mp, left, right, predicate,
				CXform::ExfDPHyperJoinRegion);
		default:
			left->Release();
			right->Release();
			predicate->Release();
			return nullptr;
	}
}

class CDPHyperJoinCostCache
{
	struct SEntry
	{
		CBitSet *m_nodes;
		CExpression *m_expr;
		DOUBLE m_rows;
		DOUBLE m_cost;

		SEntry(CMemoryPool *mp, const CBitSet *nodes, CExpression *expr,
			   DOUBLE rows, DOUBLE cost)
			: m_nodes(GPOS_NEW(mp) CBitSet(mp, *nodes)),
			  m_expr(expr),
			  m_rows(rows),
			  m_cost(cost)
		{
			GPOS_ASSERT(nullptr != expr);
		}
		~SEntry()
		{
			m_nodes->Release();
			m_expr->Release();
		}
	};

	CMemoryPool *m_mp;
	CDPHyperJoinRegion *m_region;
	CEngine *m_engine;
	std::unordered_map<ULONG, std::vector<SEntry *>> m_buckets;
	std::vector<SEntry *> m_entries;
	ULONG m_cost_calls = 0;
	ULONG m_cost_failures = 0;
	ULONG m_output_hits = 0;
	ULONG m_output_misses = 0;
	ULONG m_stats_derivations = 0;
	ULONG m_stats_derivation_us = 0;
	ULONG m_predicate_build_us = 0;
	ULONG m_join_build_us = 0;
	ULONG m_exact_stats_hits = 0;
	ULONG m_exact_stats_misses = 0;

	SEntry *
	Lookup(const CBitSet *nodes) const
	{
		auto bucket = m_buckets.find(nodes->HashValue());
		if (m_buckets.end() == bucket)
		{
			return nullptr;
		}
		for (SEntry *entry : bucket->second)
		{
			if (entry->m_nodes->Equals(nodes))
			{
				return entry;
			}
		}
		return nullptr;
	}

	void
	DeriveStats(CExpression *expr)
	{
		if (nullptr == expr->Pstats())
		{
			CWallClock clock(true);
			CExpressionHandle exprhdl(m_mp);
			exprhdl.Attach(expr);
			exprhdl.DeriveStats(m_mp, m_mp, nullptr /*prprel*/,
								nullptr /*stats context*/);
			m_stats_derivations++;
			m_stats_derivation_us += clock.ElapsedUS();
		}
		GPOS_ASSERT(nullptr != expr->Pstats());
	}

	SEntry *
	Record(const CBitSet *nodes, CExpression *expr, DOUBLE rows, DOUBLE cost)
	{
		SEntry *entry = GPOS_NEW(m_mp) SEntry(m_mp, nodes, expr, rows, cost);
		m_entries.push_back(entry);
		m_buckets[nodes->HashValue()].push_back(entry);
		return entry;
	}

public:
	CDPHyperJoinCostCache(CMemoryPool *mp, CDPHyperJoinRegion *region,
						   CEngine *engine)
		: m_mp(mp), m_region(region), m_engine(engine)
	{
		for (ULONG node = 0; node < region->NodeCount(); ++node)
		{
			CAutoRef<CBitSet> singleton(GPOS_NEW(mp) CBitSet(mp));
			(void) singleton->ExchangeSet(node);
			CExpression *component = region->Component(node);
			DeriveStats(component);
			component->AddRef();
			const DOUBLE rows = component->Pstats()->Rows().Get();
			(void) Record(singleton.Value(), component, rows, rows);
		}
	}

	~CDPHyperJoinCostCache()
	{
		for (SEntry *entry : m_entries)
		{
			GPOS_DELETE(entry);
		}
	}

	BOOL
	Cost(const CBitSet *left, const CBitSet *right, DOUBLE *cost)
	{
		GPOS_ASSERT(nullptr != cost && left->IsDisjoint(right));
		m_cost_calls++;
		SEntry *left_entry = Lookup(left);
		SEntry *right_entry = Lookup(right);
		if (nullptr == left_entry || nullptr == right_entry)
		{
			m_cost_failures++;
			return false;
		}

		CDPHyperJoinRegion::SJoinRequest request{};
		if (!m_region->FBuildJoinRequest(left, right, &request))
		{
			m_cost_failures++;
			return false;
		}
		CAutoRef<CBitSet> joined(GPOS_NEW(m_mp) CBitSet(m_mp, *left));
		joined->Union(right);
		SEntry *output = Lookup(joined.Value());
		if (nullptr == output)
		{
			m_output_misses++;
			CWallClock build_clock(true);
			CExpression *predicate = request.m_edge_ids.empty()
								 ? m_region->PexprPredicate(left, right, false)
								 : m_region->PexprPredicate(request);
			m_predicate_build_us += build_clock.ElapsedUS();
			CExpression *left_expr = left_entry->m_expr;
			CExpression *right_expr = right_entry->m_expr;
			if (request.m_swapped)
			{
				std::swap(left_expr, right_expr);
			}
			build_clock.Restart();
			CExpression *join = PexprJoinForCost(
				m_mp, request.m_join_type, left_expr, right_expr, predicate);
			m_join_build_us += build_clock.ElapsedUS();
			if (nullptr == join)
			{
				m_cost_failures++;
				return false;
			}
			CExpression *cached = m_engine->PexprLookupDPHyperCostStats(join);
			if (nullptr != cached)
			{
				join->Release();
				join = cached;
				m_exact_stats_hits++;
			}
			else
			{
				DeriveStats(join);
				m_engine->RegisterDPHyperCostStats(join);
				m_exact_stats_misses++;
			}
			output = Record(joined.Value(), join,
							join->Pstats()->Rows().Get(), 0.0);
		}
		else
		{
			m_output_hits++;
		}

		const DOUBLE input_rows = left_entry->m_rows + right_entry->m_rows;
		DOUBLE local_join = input_rows;
		if (!m_region->FHasEqualityPredicate(left, right))
		{
			local_join = left_entry->m_rows * right_entry->m_rows;
		}
		const DOUBLE total = input_rows + local_join + output->m_rows +
							 left_entry->m_cost + right_entry->m_cost;
		if (0.0 == output->m_cost || total < output->m_cost)
		{
			output->m_cost = total;
		}
		*cost = total;
		return true;
	}

	ULONG CostCalls() const { return m_cost_calls; }
	ULONG CostFailures() const { return m_cost_failures; }
	ULONG OutputHits() const { return m_output_hits; }
	ULONG OutputMisses() const { return m_output_misses; }
	ULONG StatsDerivations() const { return m_stats_derivations; }
	ULONG StatsDerivationUS() const { return m_stats_derivation_us; }
	ULONG PredicateBuildUS() const { return m_predicate_build_us; }
	ULONG JoinBuildUS() const { return m_join_build_us; }
	ULONG ExactStatsHits() const { return m_exact_stats_hits; }
	ULONG ExactStatsMisses() const { return m_exact_stats_misses; }
	ULONG EntryCount() const { return static_cast<ULONG>(m_entries.size()); }
};

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
// are deliberately ignored so one maximal region has a stable graph source.
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
	// group for every original subtree so an existing node-set is never
	// rematerialized elsewhere.
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
	if (GPOS_FTRACE(EopttracePrintXformResults) ||
		GPOS_FTRACE(EopttracePrintDSLRule))
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
	  m_native_fallback_materialized(false),
	  m_enumeration_us(0),
	  m_exploration_us(0),
	  m_waiting_for_exploration(false),
	  m_exploration_clock(false)
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
	m_enumeration_us = 0;
	m_exploration_us = 0;
	m_waiting_for_exploration = false;
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
	if (m_waiting_for_exploration)
	{
		m_exploration_us += m_exploration_clock.ElapsedUS();
		m_waiting_for_exploration = false;
	}
	if (!m_materialized)
	{
		CWallClock enumeration_clock(true);
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
		m_enumeration_us = enumeration_clock.ElapsedUS();
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
		m_exploration_clock.Restart();
		m_waiting_for_exploration = true;
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
	if (GPOS_FTRACE(EopttracePrintXformResults) ||
		GPOS_FTRACE(EopttracePrintDSLRule))
	{
		GPOS_TRACE_FORMAT(
			"DPHyper: status=explored group=%d intermediate_groups=%d "
			"enumeration_us=%d exploration_us=%d mode=%s",
			m_pgexpr->Pgroup()->Id(),
			static_cast<ULONG>(m_intermediate_groups.size()),
			m_enumeration_us, m_exploration_us,
			GPOS_FTRACE(EopttraceDPHyperShadow) ? "shadow" : "replacement");
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

	if (FSupportedDPHyperJoin(m_pgexpr->Pop()->Eopid()) &&
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
	CWallClock phase_clock(true);
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
				m_native_fallback_materialized ? "greedy_binary"
										   : "native_binary");
		}
		return false;
	}
	const ULONG build_us = phase_clock.ElapsedUS();

	phase_clock.Restart();
	CEngine *engine = psc->Peng();
	CDPHyperGraphFingerprint *fingerprint = region->Pfp();
	const ULONG fingerprint_hash = fingerprint->HashValue();
	const ULONG dependency_count =
		nullptr == spec ? 0 : spec->DependencyCount();
	const BOOL fingerprint_exists = engine->FHasDPHyperFingerprint(
		m_pgexpr->Pgroup(), fingerprint);
	const ULONG fingerprint_us = phase_clock.ElapsedUS();
	if (fingerprint_exists)
	{
		GPOS_DELETE(fingerprint);
		PublishRegionStatus(region_members, CGroupExpression::EdphSucceeded);
		if (GPOS_FTRACE(EopttracePrintXformResults))
		{
			GPOS_TRACE_FORMAT(
				"DPHyper: status=reused group=%d root=%s nodes=%d "
				"dependencies=%d fingerprint=%u build_us=%d "
				"fingerprint_us=%d mode=%s",
				m_pgexpr->Pgroup()->Id(), m_pgexpr->Pop()->SzId(), node_count,
				dependency_count, fingerprint_hash, build_us, fingerprint_us,
				GPOS_FTRACE(EopttraceDPHyperShadow) ? "shadow"
													 : "replacement");
		}
		return true;
	}

	phase_clock.Restart();
	CDPHyperPlan::PairFilter pair_filter =
		[region](const CBitSet *left, const CBitSet *right, ULONG edge_id) {
			if (!region->FPairApplicable(left, right, edge_id))
			{
				return false;
			}
			CDPHyperJoinRegion::SJoinRequest request;
			return region->FBuildJoinRequest(left, right, &request);
		};
	CDPHyperPlan exhaustive_plan(mp, hint->UlDPHyperPairBudget(), pair_filter);
	CWallClock enumeration_detail_clock(true);
	CDPHyperEnumerator enumerator(mp, region->Graph(), &exhaustive_plan);
	const BOOL enumeration_aborted = enumerator.Enumerate();
	const ULONG exhaustive_us = enumeration_detail_clock.ElapsedUS();
	const BOOL exhaustive_complete = exhaustive_plan.Complete(node_count);
	const BOOL budget_exhausted = exhaustive_plan.BudgetExhausted();
	const ULONG attempted_pairs = exhaustive_plan.PairCount();

	CDPHyperPlan graph_plan(mp, hint->UlDPHyperPairBudget(), pair_filter);
	BOOL graph_simplified = false;
	ULONG simplification_steps = 0;
	ULONG simplifier_init_us = 0;
	ULONG simplifier_search_us = 0;
	ULONG simplified_enumeration_us = 0;
	ULONG cost_cache_calls = 0;
	ULONG cost_cache_failures = 0;
	ULONG cost_cache_hits = 0;
	ULONG cost_cache_misses = 0;
	ULONG cost_cache_entries = 0;
	ULONG cost_cache_stats_derivations = 0;
	ULONG cost_cache_stats_us = 0;
	ULONG cost_cache_predicate_us = 0;
	ULONG cost_cache_join_us = 0;
	ULONG cost_cache_exact_stats_hits = 0;
	ULONG cost_cache_exact_stats_misses = 0;
	if (!exhaustive_complete && budget_exhausted)
	{
		enumeration_detail_clock.Restart();
		CDPHyperJoinCostCache cost_cache(mp, region, engine);
		CDPHyperGraphSimplifier simplifier(
			mp, region->MutableGraph(), hint->UlDPHyperPairBudget(), pair_filter,
			[region](ULONG edge_id) {
				return region->FEqualityEdge(edge_id);
			},
			[&cost_cache](const CBitSet *left, const CBitSet *right,
						  DOUBLE *cost) {
				return cost_cache.Cost(left, right, cost);
			});
		simplifier_init_us = enumeration_detail_clock.ElapsedUS();
		enumeration_detail_clock.Restart();
		if (simplifier.Simplify())
		{
			simplifier_search_us = enumeration_detail_clock.ElapsedUS();
			enumeration_detail_clock.Restart();
			CDPHyperEnumerator simplified_enumerator(mp, region->Graph(),
											   &graph_plan);
			graph_simplified = !simplified_enumerator.Enumerate() &&
							   graph_plan.Complete(node_count);
			simplified_enumeration_us =
				enumeration_detail_clock.ElapsedUS();
			simplification_steps = simplifier.AppliedStepCount();
			if (!graph_simplified)
			{
				simplifier.Restore();
				 simplification_steps = 0;
			}
		}
		cost_cache_calls = cost_cache.CostCalls();
		cost_cache_failures = cost_cache.CostFailures();
		cost_cache_hits = cost_cache.OutputHits();
		cost_cache_misses = cost_cache.OutputMisses();
		cost_cache_entries = cost_cache.EntryCount();
		cost_cache_stats_derivations = cost_cache.StatsDerivations();
		cost_cache_stats_us = cost_cache.StatsDerivationUS();
		cost_cache_predicate_us = cost_cache.PredicateBuildUS();
		cost_cache_join_us = cost_cache.JoinBuildUS();
		cost_cache_exact_stats_hits = cost_cache.ExactStatsHits();
		cost_cache_exact_stats_misses = cost_cache.ExactStatsMisses();
	}
	CDPHyperPlan skeleton_plan(mp, node_count - 1);
	const BOOL skeleton_simplified =
		(!exhaustive_complete && !graph_simplified && budget_exhausted &&
		 nullptr != spec &&
		 FBuildBinarySkeletonPlan(mp, region, spec, &skeleton_plan));
	const BOOL simplified = graph_simplified || skeleton_simplified;
	const CDPHyperPlan *plan =
		graph_simplified
			? &graph_plan
			: (skeleton_simplified ? &skeleton_plan : &exhaustive_plan);
	const ULONG enumeration_us = phase_clock.ElapsedUS();
	if ((!exhaustive_complete && !simplified) ||
		(enumeration_aborted && !budget_exhausted))
	{
		GPOS_DELETE(fingerprint);
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
				m_native_fallback_materialized ? "greedy_binary"
										   : "native_binary");
		}
		return false;
	}

	phase_clock.Restart();
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
	const ULONG registration_us = phase_clock.ElapsedUS();

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

	phase_clock.Restart();
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
		// DPHyp reports an unordered CSG-CMP pair once. Inner and FullOuter
		// joins are commutative logical operators, so retain both physical input
		// orientations. Their predicate is identical: pure-inner predicate
		// selection is symmetric in the two subsets, while a complex request
		// already records its CD-C-approved edge set. Share that immutable tree
		// instead of rebuilding the same conjunction for the reverse expression.
		const BOOL add_reverse =
			(COperator::EopLogicalInnerJoin == join_type ||
			 COperator::EopLogicalFullOuterJoin == join_type) &&
			(nullptr == spec || !request.m_dependency_directional);
		CExpression *reverse_predicate = nullptr;
		if (add_reverse)
		{
			predicate->AddRef();
			reverse_predicate = predicate;
		}
		if (nullptr != spec && request.m_swapped)
		{
			std::swap(left_group, right_group);
		}
		CExpression *join =
			PexprJoin(mp, join_type, left_group, right_group, predicate);
		target = engine->PgroupInsert(
			target, join, CXform::ExfDPHyperJoinRegion, m_pgexpr,
			!full /*intermediate*/);
		join->Release();
		GPOS_ASSERT(nullptr != target);
		target = subset_groups.Record(joined.Value(), target);
		if (!full &&
			m_intermediate_groups.end() ==
				std::find(m_intermediate_groups.begin(),
						  m_intermediate_groups.end(), target))
		{
			m_intermediate_groups.push_back(target);
		}

		if (add_reverse)
		{
			join = PexprJoin(mp, join_type, right_group, left_group,
							 reverse_predicate);
			CGroup *reverse_target = engine->PgroupInsert(
				target, join, CXform::ExfDPHyperJoinRegion, m_pgexpr,
				!full /*intermediate*/);
			join->Release();
			reverse_target =
				subset_groups.Record(joined.Value(), reverse_target);
			GPOS_ASSERT(reverse_target == target);
			(void) reverse_target;
		}
	}
	const ULONG materialization_us = phase_clock.ElapsedUS();
	if (GPOS_FTRACE(EopttracePrintXformResults) ||
		GPOS_FTRACE(EopttracePrintDSLRule))
	{
		GPOS_TRACE_FORMAT(
			"DPHyper: status=applied group=%d root=%s nodes=%d edges=%d "
			"cartesian_edges=%d dependencies=%d pairs=%d subsets=%d "
			"fingerprint=%u enumeration=%s strategy=%s reason=%s "
			"attempted_pairs=%d simplification_steps=%d build_us=%d "
			"fingerprint_us=%d enumeration_us=%d registration_us=%d "
			"materialization_us=%d exhaustive_us=%d simplifier_init_us=%d "
			"simplifier_search_us=%d simplified_enumeration_us=%d "
			"cost_cache_calls=%d cost_cache_failures=%d cost_cache_hits=%d "
			"cost_cache_misses=%d cost_cache_entries=%d "
			"cost_cache_stats_derivations=%d cost_cache_stats_us=%d "
			"cost_cache_predicate_us=%d cost_cache_join_us=%d "
			"cost_cache_exact_stats_hits=%d "
			"cost_cache_exact_stats_misses=%d mode=%s",
			m_pgexpr->Pgroup()->Id(), m_pgexpr->Pop()->SzId(), node_count,
			region->GeneratedEdgeCount(), region->CartesianEdgeCount(),
			dependency_count, plan->PairCount(), plan->SeenCount(), fingerprint_hash,
			simplified ? "simplified" : "complete",
			graph_simplified
				? "graph_simplifier"
				: (skeleton_simplified ? "binary_skeleton" : "none"),
			simplified ? "pair_budget" : "none", attempted_pairs,
			simplification_steps, build_us, fingerprint_us, enumeration_us,
			registration_us, materialization_us, exhaustive_us,
			simplifier_init_us, simplifier_search_us,
			simplified_enumeration_us, cost_cache_calls,
			cost_cache_failures, cost_cache_hits, cost_cache_misses,
			cost_cache_entries, cost_cache_stats_derivations,
			cost_cache_stats_us, cost_cache_predicate_us, cost_cache_join_us,
			cost_cache_exact_stats_hits, cost_cache_exact_stats_misses,
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
	// Greedy only models pure inner regions. Mixed join skeletons retain their
	// original binary query order and native semantic transformations.
	if (GPOS_FTRACE(EopttraceDPHyperShadow) ||
		(nullptr != spec && !spec->PureInner()))
	{
		return;
	}
	CMemoryPool *mp = psc->GetGlobalMemoryPool();
	CExpressionArray *components = GPOS_NEW(mp) CExpressionArray(mp);
	for (CGroup *group : component_groups)
	{
		CExpression *component = PexprGroupLeaf(mp, group);
		DeriveStats(mp, component);
		components->Append(component);
	}
	CExpression *all_predicates = region->PexprAllPredicates();
	CExpressionArray *conjuncts =
		CPredicateUtils::PdrgpexprConjuncts(mp, all_predicates);
	all_predicates->Release();
	CJoinOrderGreedy greedy(mp, components, conjuncts);
	CExpression *result = greedy.PexprExpand();
	if (nullptr == result)
	{
		return;
	}
	CExpression *normalized = CNormalizer::PexprNormalize(mp, result);
	result->Release();
	CGroup *target = PgroupMaterializeBinaryTree(
		mp, psc->Peng(), normalized, m_pgexpr->Pgroup(), m_pgexpr,
		&m_intermediate_groups);
	normalized->Release();
	GPOS_ASSERT(CGroup::FDuplicateGroups(target, m_pgexpr->Pgroup()));
	m_native_fallback_materialized =
		CGroup::FDuplicateGroups(target, m_pgexpr->Pgroup());
}

#ifdef GPOS_DEBUG
IOstream &
CJobJoinEnumeration::OsPrint(IOstream &os) const
{
	return os << "DPHyper join enumeration for group expression "
			  << (nullptr == m_pgexpr ? GPOPT_INVALID_GEXPR_ID : m_pgexpr->Id());
}
#endif
