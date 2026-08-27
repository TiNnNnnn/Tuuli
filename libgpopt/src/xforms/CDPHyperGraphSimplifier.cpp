//---------------------------------------------------------------------------
//	@filename:
//		CDPHyperGraphSimplifier.cpp
//---------------------------------------------------------------------------
#include "gpopt/xforms/CDPHyperGraphSimplifier.h"

#include <algorithm>
#include <limits>

#include "gpos/common/CAutoRef.h"

using namespace gpopt;

CDPHyperGraphSimplifier::SEdgeChange::SEdgeChange(
	CMemoryPool *mp, ULONG edge, const CBitSet *old_left,
	const CBitSet *old_right, const CBitSet *new_left,
	const CBitSet *new_right)
	: m_edge(edge),
	  m_old_left(GPOS_NEW(mp) CBitSet(mp, *old_left)),
	  m_old_right(GPOS_NEW(mp) CBitSet(mp, *old_right)),
	  m_new_left(GPOS_NEW(mp) CBitSet(mp, *new_left)),
	  m_new_right(GPOS_NEW(mp) CBitSet(mp, *new_right))
{
}

CDPHyperGraphSimplifier::SEdgeChange::~SEdgeChange()
{
	m_old_left->Release();
	m_old_right->Release();
	m_new_left->Release();
	m_new_right->Release();
}

BOOL
CDPHyperGraphSimplifier::SBestCompare::operator()(
	const SBestSimplification *left,
	const SBestSimplification *right) const
{
	GPOS_ASSERT(left->m_step.has_value() && right->m_step.has_value());
	return left->m_step->m_benefit < right->m_step->m_benefit;
}

CDPHyperGraphSimplifier::CDPHyperGraphSimplifier(
	CMemoryPool *mp, CDPHyperGraph *graph, ULONG pair_budget,
	CDPHyperPlan::PairFilter pair_filter, EdgeFilter edge_filter,
	JoinCost join_cost)
	: m_mp(mp),
	  m_graph(graph),
	  m_pair_budget(pair_budget),
	  m_pair_filter(std::move(pair_filter)),
	  m_edge_filter(std::move(edge_filter)),
	  m_join_cost(std::move(join_cost)),
	  m_order(mp, graph->LogicalEdgeCount())
{
	GPOS_ASSERT(nullptr != mp && nullptr != graph && 0 < pair_budget);
	m_best.reserve(graph->LogicalEdgeCount());
	for (ULONG edge = 0; edge < graph->LogicalEdgeCount(); ++edge)
	{
		m_best.push_back(GPOS_NEW(mp) SBestSimplification());
	}
	InitFirstStep();
}

CDPHyperGraphSimplifier::~CDPHyperGraphSimplifier()
{
	for (SBestSimplification *best : m_best)
	{
		GPOS_DELETE(best);
	}
}

BOOL
CDPHyperGraphSimplifier::FSubset(const CBitSet *subset,
								 const CBitSet *superset)
{
	return superset->ContainsAll(subset);
}

CBitSet *
CDPHyperGraphSimplifier::PbsUnion(CMemoryPool *mp, const CBitSet *left,
								  const CBitSet *right)
{
	CBitSet *result = GPOS_NEW(mp) CBitSet(mp, *left);
	result->Union(right);
	return result;
}

const CDPHyperGraph::SEdge *
CDPHyperGraphSimplifier::Edge(ULONG edge) const
{
	GPOS_ASSERT(edge < m_graph->LogicalEdgeCount());
	return m_graph->Edge(edge * 2);
}

CBitSet *
CDPHyperGraphSimplifier::PbsUsedNodes(ULONG edge) const
{
	return PbsUnion(m_mp, Edge(edge)->m_left, Edge(edge)->m_right);
}

BOOL
CDPHyperGraphSimplifier::FTryGetSuperset(const CBitSet *left,
									 const CBitSet *right,
									 const CBitSet **superset) const
{
	GPOS_ASSERT(nullptr != superset);
	if (FSubset(left, right))
	{
		*superset = right;
		return true;
	}
	if (FSubset(right, left))
	{
		*superset = left;
		return true;
	}
	return false;
}

void
CDPHyperGraphSimplifier::ExtractJoinDependencies()
{
	for (ULONG edge1 = 0; edge1 < m_graph->LogicalEdgeCount(); ++edge1)
	{
		CAutoRef<CBitSet> used1(PbsUsedNodes(edge1));
		for (ULONG edge2 = edge1 + 1;
			 edge2 < m_graph->LogicalEdgeCount(); ++edge2)
		{
			CAutoRef<CBitSet> used2(PbsUsedNodes(edge2));
			if (FSubset(used1.Value(), used2.Value()))
			{
				const BOOL added GPOS_ASSERTS_ONLY =
					m_order.TryAdd(edge1, edge2);
				GPOS_ASSERT(added);
			}
			else if (FSubset(used2.Value(), used1.Value()))
			{
				const BOOL added GPOS_ASSERTS_ONLY =
					m_order.TryAdd(edge2, edge1);
				GPOS_ASSERT(added);
			}
		}
	}
}

void
CDPHyperGraphSimplifier::InitFirstStep()
{
	ExtractJoinDependencies();
	for (ULONG edge = 0; edge < m_graph->LogicalEdgeCount(); ++edge)
	{
		ProcessNeighbors(edge, edge + 1, m_graph->LogicalEdgeCount());
	}
}

std::shared_ptr<CDPHyperGraphSimplifier::SEdgeChange>
CDPHyperGraphSimplifier::Pchange(ULONG edge, const CBitSet *new_left,
								 const CBitSet *new_right)
{
	const CDPHyperGraph::SEdge *old = Edge(edge);
	return std::shared_ptr<SEdgeChange>(
		GPOS_NEW(m_mp) SEdgeChange(m_mp, edge, old->m_left, old->m_right,
										 new_left, new_right),
		[](SEdgeChange *change) { GPOS_DELETE(change); });
}

std::optional<CDPHyperGraphSimplifier::SSimplificationStep>
CDPHyperGraphSimplifier::ThreeLeftJoin(
	const CBitSet *set1, ULONG edge1, const CBitSet *set2, ULONG edge2,
	const CBitSet *set3)
{
	CAutoRef<CBitSet> new_left(PbsUnion(m_mp, set1, set2));
	if (!new_left->IsDisjoint(set3))
	{
		return std::nullopt;
	}
	DOUBLE first_cost = 0.0;
	DOUBLE total_cost = 0.0;
	if (!m_join_cost(set1, set2, &first_cost) ||
		!m_join_cost(new_left.Value(), set3, &total_cost))
	{
		return std::nullopt;
	}
	SSimplificationStep step;
	step.m_benefit = total_cost;
	step.m_before = edge1;
	step.m_after = edge2;
	step.m_change = Pchange(edge2, new_left.Value(), set3);
	return step;
}

std::optional<CDPHyperGraphSimplifier::SSimplificationStep>
CDPHyperGraphSimplifier::ThreeRightJoin(
	const CBitSet *set1, ULONG edge1, const CBitSet *set2, ULONG edge2,
	const CBitSet *set3)
{
	CAutoRef<CBitSet> new_right(PbsUnion(m_mp, set2, set3));
	if (!set1->IsDisjoint(new_right.Value()))
	{
		return std::nullopt;
	}
	DOUBLE first_cost = 0.0;
	DOUBLE total_cost = 0.0;
	if (!m_join_cost(set2, set3, &first_cost) ||
		!m_join_cost(set1, new_right.Value(), &total_cost))
	{
		return std::nullopt;
	}
	SSimplificationStep step;
	step.m_benefit = total_cost;
	step.m_before = edge2;
	step.m_after = edge1;
	step.m_change = Pchange(edge1, set1, new_right.Value());
	return step;
}

CDPHyperGraphSimplifier::SSimplificationStep
CDPHyperGraphSimplifier::OrderJoin(
	const SSimplificationStep &edge1_before2,
	const SSimplificationStep &edge2_before1, ULONG, ULONG)
{
	const DOUBLE cost1 = edge1_before2.m_benefit;
	const DOUBLE cost2 = edge2_before1.m_benefit;
	SSimplificationStep selected =
		cost1 < cost2 ? edge1_before2 : edge2_before1;
	const DOUBLE lower = std::min(cost1, cost2);
	const DOUBLE higher = std::max(cost1, cost2);
	selected.m_benefit =
		0.0 < lower ? higher / lower : std::numeric_limits<DOUBLE>::max();
	return selected;
}

std::optional<CDPHyperGraphSimplifier::SSimplificationStep>
CDPHyperGraphSimplifier::MakeStep(ULONG edge1, ULONG edge2)
{
	CAutoRef<CBitSet> used1(PbsUsedNodes(edge1));
	CAutoRef<CBitSet> used2(PbsUsedNodes(edge2));
	if (FSubset(used1.Value(), used2.Value()) ||
		FSubset(used2.Value(), used1.Value()) ||
		m_order.WouldCreateCycle(edge1, edge2) ||
		m_order.WouldCreateCycle(edge2, edge1) ||
		!m_edge_filter(Edge(edge1)->m_edge_id) ||
		!m_edge_filter(Edge(edge2)->m_edge_id))
	{
		return std::nullopt;
	}

	const CBitSet *left1 = Edge(edge1)->m_left;
	const CBitSet *right1 = Edge(edge1)->m_right;
	const CBitSet *left2 = Edge(edge2)->m_left;
	const CBitSet *right2 = Edge(edge2)->m_right;
	const CBitSet *common = nullptr;
	std::optional<SSimplificationStep> first;
	std::optional<SSimplificationStep> second;
	if (FTryGetSuperset(left1, left2, &common) &&
		right1->IsDisjoint(right2))
	{
		first = ThreeLeftJoin(common, edge1, right1, edge2, right2);
		second = ThreeLeftJoin(common, edge2, right2, edge1, right1);
	}
	else if (FTryGetSuperset(left1, right2, &common) &&
			 left2->IsDisjoint(right1))
	{
		first = ThreeRightJoin(left2, edge2, common, edge1, right1);
		second = ThreeLeftJoin(left2, edge2, common, edge1, right1);
	}
	else if (FTryGetSuperset(right1, left2, &common) &&
			 left1->IsDisjoint(right2))
	{
		first = ThreeLeftJoin(left1, edge1, common, edge2, right2);
		second = ThreeRightJoin(left1, edge1, common, edge2, right2);
	}
	else if (FTryGetSuperset(right1, right2, &common) &&
			 left1->IsDisjoint(left2))
	{
		first = ThreeRightJoin(left2, edge2, left1, edge1, common);
		second = ThreeRightJoin(left1, edge1, left2, edge2, common);
	}
	if (!first.has_value() || !second.has_value())
	{
		return std::nullopt;
	}
	return OrderJoin(*first, *second, edge1, edge2);
}

BOOL
CDPHyperGraphSimplifier::TrySetStep(const SSimplificationStep &step,
									SBestSimplification *best,
									ULONG neighbor)
{
	if (-1 == best->m_best_neighbor || !best->m_in_queue ||
		best->m_step->m_benefit <= step.m_benefit)
	{
		best->m_best_neighbor = neighbor;
		best->m_step = step;
		UpdateQueue(best);
		return true;
	}
	return false;
}

void
CDPHyperGraphSimplifier::UpdateQueue(SBestSimplification *best)
{
	if (!best->m_in_queue)
	{
		if (-1 != best->m_best_neighbor)
		{
			m_queue.push(best);
			best->m_in_queue = true;
		}
		return;
	}

	decltype(m_queue) rebuilt;
	while (!m_queue.empty())
	{
		SBestSimplification *candidate = m_queue.top();
		m_queue.pop();
		if (candidate != best)
		{
			rebuilt.push(candidate);
		}
	}
	if (-1 == best->m_best_neighbor)
	{
		best->m_in_queue = false;
	}
	else
	{
		rebuilt.push(best);
	}
	m_queue = std::move(rebuilt);
}

void
CDPHyperGraphSimplifier::ProcessNeighbors(ULONG edge1, ULONG begin,
									  ULONG end)
{
	for (ULONG edge2 = begin; edge2 < std::min(end, edge1); ++edge2)
	{
		SBestSimplification *best = m_best[edge2];
		std::optional<SSimplificationStep> step = MakeStep(edge1, edge2);
		if (step.has_value() && TrySetStep(*step, best, edge1))
		{
			continue;
		}
		if (best->m_best_neighbor == static_cast<INT>(edge1))
		{
			ProcessNeighbors(edge2, edge2 + 1,
							 m_graph->LogicalEdgeCount());
		}
	}

	SBestSimplification *best = m_best[edge1];
	best->m_best_neighbor = -1;
	for (ULONG edge2 = std::max(begin, edge1 + 1); edge2 < end; ++edge2)
	{
		std::optional<SSimplificationStep> step = MakeStep(edge1, edge2);
		if (step.has_value())
		{
			(void) TrySetStep(*step, best, edge2);
		}
	}
	UpdateQueue(best);
}

std::optional<CDPHyperGraphSimplifier::SSimplificationStep>
CDPHyperGraphSimplifier::FetchStep()
{
	if (!m_unapplied.empty())
	{
		SSimplificationStep step = m_unapplied.top();
		m_unapplied.pop();
		return step;
	}
	while (!m_queue.empty())
	{
		SBestSimplification *best = m_queue.top();
		m_queue.pop();
		best->m_in_queue = false;
		if (-1 == best->m_best_neighbor || !best->m_step.has_value())
		{
			continue;
		}
		SSimplificationStep step = *best->m_step;
		if (m_order.TryAdd(step.m_before, step.m_after))
		{
			return step;
		}
		ProcessNeighbors(step.m_after, 0, m_graph->LogicalEdgeCount());
	}
	return std::nullopt;
}

void
CDPHyperGraphSimplifier::ModifyEdge(const SSimplificationStep &step,
									BOOL apply)
{
	const SEdgeChange *change = step.m_change.get();
	m_graph->ReplaceEdge(change->m_edge,
					 apply ? change->m_new_left : change->m_old_left,
					 apply ? change->m_new_right : change->m_old_right);
}

BOOL
CDPHyperGraphSimplifier::ApplyStep()
{
	const BOOL fresh = m_unapplied.empty();
	std::optional<SSimplificationStep> step = FetchStep();
	if (!step.has_value())
	{
		return false;
	}
	ModifyEdge(*step, true);
	m_applied.push(*step);
	if (fresh)
	{
		ProcessNeighbors(step->m_after, 0, m_graph->LogicalEdgeCount());
	}
	return true;
}

BOOL
CDPHyperGraphSimplifier::UnapplyStep()
{
	if (m_applied.empty())
	{
		return false;
	}
	SSimplificationStep step = m_applied.top();
	m_applied.pop();
	ModifyEdge(step, false);
	m_unapplied.push(step);
	return true;
}

BOOL
CDPHyperGraphSimplifier::ApplySteps(ULONG count)
{
	while (m_applied.size() < count)
	{
		if (!ApplyStep())
		{
			return false;
		}
	}
	while (m_applied.size() > count)
	{
		const BOOL unapplied GPOS_ASSERTS_ONLY = UnapplyStep();
		GPOS_ASSERT(unapplied);
	}
	return true;
}

BOOL
CDPHyperGraphSimplifier::EnumerationExceedsBudget() const
{
	CDPHyperPlan probe(m_mp, m_pair_budget, m_pair_filter);
	CDPHyperEnumerator enumerator(m_mp, m_graph, &probe);
	const BOOL aborted = enumerator.Enumerate();
	return aborted || !probe.Complete(m_graph->NodeCount());
}

BOOL
CDPHyperGraphSimplifier::Simplify()
{
	GPOS_ASSERT(0 < m_pair_budget);
	ULONG lower = 0;
	ULONG upper = 1;
	ULONG applied = 0;
	while (true)
	{
		while (applied < upper)
		{
			if (!ApplyStep())
			{
				if (EnumerationExceedsBudget())
				{
					(void) ApplySteps(0);
					return false;
				}
				break;
			}
			++applied;
		}
		if (applied < upper || !EnumerationExceedsBudget())
		{
			break;
		}
		upper *= 2;
	}

	upper = applied + 1;
	while (lower < upper)
	{
		const ULONG middle = lower + (upper - lower) / 2;
		if (!ApplySteps(middle))
		{
			lower = middle + 1;
			continue;
		}
		if (!EnumerationExceedsBudget())
		{
			upper = middle;
		}
		else
		{
			lower = middle + 1;
		}
	}
	// Binary search only lowers upper after probing a complete enumeration at
	// that step count, so the converged upper bound is already known to fit the
	// budget. Reapplying it restores the selected graph; probing it again would
	// repeat the same DPHyper enumeration immediately before the caller performs
	// the final materializing enumeration.
	if (!ApplySteps(upper))
	{
		(void) ApplySteps(0);
		return false;
	}
	return 0 < m_applied.size();
}

void
CDPHyperGraphSimplifier::Restore()
{
	const BOOL restored GPOS_ASSERTS_ONLY = ApplySteps(0);
	GPOS_ASSERT(restored);
}
