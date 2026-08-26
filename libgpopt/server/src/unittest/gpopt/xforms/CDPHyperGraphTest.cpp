//---------------------------------------------------------------------------
//	@filename:
//		CDPHyperGraphTest.cpp
//---------------------------------------------------------------------------
#include "unittest/gpopt/xforms/CDPHyperGraphTest.h"

#include <initializer_list>
#include <cstdint>
#include <vector>

#include "gpos/common/CAutoRef.h"
#include "gpos/common/CBitSetIter.h"
#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogicalLeftAntiSemiJoin.h"
#include "gpopt/operators/CLogicalLeftAntiSemiJoinNotIn.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"
#include "gpopt/xforms/CDPHyperGraph.h"
#include "gpopt/xforms/CDPHyperJoinRegion.h"
#include "gpopt/xforms/CDPHyperPlan.h"
#include "gpopt/xforms/CJoinRegionSpec.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

namespace
{
class CRecordingReceiver : public IDPHyperReceiver
{
private:
	CMemoryPool *m_mp;
	std::vector<CBitSet *> m_seen;

public:
	explicit CRecordingReceiver(CMemoryPool *mp) : m_mp(mp)
	{
	}

	~CRecordingReceiver() override
	{
		for (CBitSet *set : m_seen)
		{
			set->Release();
		}
	}

	BOOL
	HasSeen(const CBitSet *nodes) const override
	{
		for (const CBitSet *seen : m_seen)
		{
			if (seen->Equals(nodes))
			{
				return true;
			}
		}
		return false;
	}

	BOOL
	FoundSingleNode(ULONG node_id) override
	{
		CAutoRef<CBitSet> node(GPOS_NEW(m_mp) CBitSet(m_mp));
		(void) node->ExchangeSet(node_id);
		if (!HasSeen(node.Value()))
		{
			m_seen.push_back(node.Reset());
		}
		return false;
	}

	BOOL
	FoundSubgraphPair(const CBitSet *left, const CBitSet *right,
						   ULONG) override
	{
		GPOS_UNITTEST_ASSERT(left->IsDisjoint(right));
		CAutoRef<CBitSet> joined(GPOS_NEW(m_mp) CBitSet(m_mp, *left));
		joined->Union(right);
		if (!HasSeen(joined.Value()))
		{
			m_seen.push_back(joined.Reset());
		}
		return false;
	}

	ULONG
	SeenCount() const
	{
		return m_seen.size();
	}
};

class CEligibilityReceiver : public IDPHyperReceiver
{
private:
	const CDPHyperJoinRegion *m_region;
	CDPHyperPlan m_plan;

public:
	CEligibilityReceiver(CMemoryPool *mp, const CDPHyperJoinRegion *region)
		: m_region(region), m_plan(mp, 100)
	{
	}

	BOOL
	FoundSingleNode(ULONG node_id) override
	{
		return m_plan.FoundSingleNode(node_id);
	}

	BOOL
	HasSeen(const CBitSet *nodes) const override
	{
		return m_plan.HasSeen(nodes);
	}

	BOOL
	FoundSubgraphPair(const CBitSet *left, const CBitSet *right,
					   ULONG edge_id) override
	{
		return m_region->FPairApplicable(left, right, edge_id) &&
			   m_plan.FoundSubgraphPair(left, right, edge_id);
	}

	BOOL
	Complete(ULONG node_count) const
	{
		return m_plan.Complete(node_count);
	}
};

CBitSet *
Pbs(CMemoryPool *mp, std::initializer_list<ULONG> nodes)
{
	CBitSet *set = GPOS_NEW(mp) CBitSet(mp);
	for (ULONG node : nodes)
	{
		(void) set->ExchangeSet(node);
	}
	return set;
}

BOOL
FSet(const CBitSet *set, std::initializer_list<ULONG> nodes)
{
	if (set->Size() != nodes.size())
	{
		return false;
	}
	for (ULONG node : nodes)
	{
		if (!set->Get(node))
		{
			return false;
		}
	}
	return true;
}

template <class TJoin>
CExpression *
PexprJoin(CMemoryPool *mp, CExpression *left, CExpression *right,
		  CExpression *predicate)
{
	left->AddRef();
	right->AddRef();
	predicate->AddRef();
	return GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) TJoin(mp), left, right,
									 predicate);
}

void
AddSimpleEdge(CMemoryPool *mp, CDPHyperGraph *graph, ULONG left, ULONG right,
			  ULONG edge_id)
{
	CAutoRef<CBitSet> left_set(Pbs(mp, {left}));
	CAutoRef<CBitSet> right_set(Pbs(mp, {right}));
	graph->AddEdge(left_set.Value(), right_set.Value(), edge_id);
}

struct SMaskEdge
{
	ULONG m_left;
	ULONG m_right;
};

CBitSet *
PbsFromMask(CMemoryPool *mp, ULONG mask)
{
	CBitSet *set = GPOS_NEW(mp) CBitSet(mp);
	for (ULONG node = 0; 0 != mask; ++node, mask >>= 1)
	{
		if (0 != (mask & 1))
		{
			(void) set->ExchangeSet(node);
		}
	}
	return set;
}

BOOL
FEdgeConnects(const SMaskEdge &edge, ULONG left, ULONG right)
{
	return ((edge.m_left & left) == edge.m_left &&
			(edge.m_right & right) == edge.m_right) ||
		   ((edge.m_right & left) == edge.m_right &&
			(edge.m_left & right) == edge.m_left);
}

// Independent dynamic-programming oracle for hypergraph connectivity. A set
// is constructible iff it is a singleton, or has a bipartition whose two
// sides are constructible and are joined by an eligible hyperedge.
BOOL
FHyperConnected(ULONG subset, const std::vector<SMaskEdge> &edges,
				 std::vector<INT> *memo)
{
	if (0 <= (*memo)[subset])
	{
		return 1 == (*memo)[subset];
	}
	if (0 == (subset & (subset - 1)))
	{
		(*memo)[subset] = 1;
		return true;
	}

	const ULONG anchor = subset & (~subset + 1);
	for (ULONG left = (subset - 1) & subset; 0 != left;
		 left = (left - 1) & subset)
	{
		if (0 == (left & anchor))
		{
			continue;
		}
		const ULONG right = subset ^ left;
		if (0 == right || !FHyperConnected(left, edges, memo) ||
			!FHyperConnected(right, edges, memo))
		{
			continue;
		}
		for (const SMaskEdge &edge : edges)
		{
			if (FEdgeConnects(edge, left, right))
			{
				(*memo)[subset] = 1;
				return true;
			}
		}
	}
	(*memo)[subset] = 0;
	return false;
}

BOOL
FSeen(CMemoryPool *mp, const CRecordingReceiver &receiver,
	  std::initializer_list<ULONG> nodes)
{
	CAutoRef<CBitSet> set(Pbs(mp, nodes));
	return receiver.HasSeen(set.Value());
}

BOOL
FConnected(ULONG node_count, ULONG graph_mask, ULONG subset)
{
	GPOS_ASSERT(0 != subset);
	ULONG reached = subset & (~subset + 1);
	for (;;)
	{
		ULONG next = reached;
		ULONG edge_bit = 0;
		for (ULONG left = 0; left < node_count; ++left)
		{
			for (ULONG right = left + 1; right < node_count;
				 ++right, ++edge_bit)
			{
				if (0 == (graph_mask & (ULONG(1) << edge_bit)))
				{
					continue;
				}
				const ULONG left_bit = ULONG(1) << left;
				const ULONG right_bit = ULONG(1) << right;
				if (0 != (reached & left_bit) && 0 != (subset & right_bit))
				{
					next |= right_bit;
				}
				if (0 != (reached & right_bit) && 0 != (subset & left_bit))
				{
					next |= left_bit;
				}
			}
		}
		if (next == reached)
		{
			return reached == subset;
		}
		reached = next;
	}
}

// A frozen Cartesian bridge may combine whole predicate-connected components,
// but it must never make a partial component cross-joinable. Within one
// component the ordinary graph-connectivity rule still applies.
BOOL
FFrozenCartesianConstructible(ULONG node_count, ULONG graph_mask,
								ULONG subset)
{
	if (FConnected(node_count, graph_mask, subset))
	{
		return true;
	}

	const ULONG all_nodes = (ULONG(1) << node_count) - 1;
	ULONG remaining = subset;
	while (0 != remaining)
	{
		const ULONG seed = remaining & (~remaining + 1);
		ULONG component = seed;
		for (;;)
		{
			ULONG next = component;
			ULONG edge_bit = 0;
			for (ULONG left = 0; left < node_count; ++left)
			{
				for (ULONG right = left + 1; right < node_count;
					 ++right, ++edge_bit)
				{
					if (0 == (graph_mask & (ULONG(1) << edge_bit)))
					{
						continue;
					}
					const ULONG left_bit = ULONG(1) << left;
					const ULONG right_bit = ULONG(1) << right;
					if (0 != (component & left_bit))
					{
						next |= right_bit;
					}
					if (0 != (component & right_bit))
					{
						next |= left_bit;
					}
				}
			}
			next &= all_nodes;
			if (next == component)
			{
				break;
			}
			component = next;
		}
		if ((subset & component) != component)
		{
			return false;
		}
		remaining &= ~component;
	}
	return true;
}
}  // namespace

GPOS_RESULT
CDPHyperGraphTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CDPHyperGraphTest::EresUnittest_Chain),
		GPOS_UNITTEST_FUNC(CDPHyperGraphTest::EresUnittest_Star),
		GPOS_UNITTEST_FUNC(CDPHyperGraphTest::EresUnittest_Hyperedge),
		GPOS_UNITTEST_FUNC(CDPHyperGraphTest::EresUnittest_Disconnected),
		GPOS_UNITTEST_FUNC(CDPHyperGraphTest::EresUnittest_DynamicBitset),
		GPOS_UNITTEST_FUNC(
			CDPHyperGraphTest::EresUnittest_ExhaustiveSimpleGraphs),
		GPOS_UNITTEST_FUNC(
			CDPHyperGraphTest::EresUnittest_DifferentialHypergraphs),
		GPOS_UNITTEST_FUNC(CDPHyperGraphTest::EresUnittest_AtomicBudget),
		GPOS_UNITTEST_FUNC(CDPHyperGraphTest::EresUnittest_JoinRegion),
		GPOS_UNITTEST_FUNC(
			CDPHyperGraphTest::EresUnittest_BinaryJoinRegionSpec),
		GPOS_UNITTEST_FUNC(
			CDPHyperGraphTest::EresUnittest_CartesianComponents),
		GPOS_UNITTEST_FUNC(
			CDPHyperGraphTest::EresUnittest_CartesianDifferential),
	};
	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDPHyperGraphTest::EresUnittest_Chain()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDPHyperGraph graph(mp, 4);
	AddSimpleEdge(mp, &graph, 0, 1, 0);
	AddSimpleEdge(mp, &graph, 1, 2, 1);
	AddSimpleEdge(mp, &graph, 2, 3, 2);
	CRecordingReceiver receiver(mp);
	CDPHyperEnumerator enumerator(mp, &graph, &receiver);
	GPOS_UNITTEST_ASSERT(!enumerator.Enumerate());
	GPOS_UNITTEST_ASSERT(10 == receiver.SeenCount());
	GPOS_UNITTEST_ASSERT(FSeen(mp, receiver, {0, 1, 2, 3}));
	GPOS_UNITTEST_ASSERT(!FSeen(mp, receiver, {0, 2}));
	return GPOS_OK;
}

GPOS_RESULT
CDPHyperGraphTest::EresUnittest_Star()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDPHyperGraph graph(mp, 4);
	AddSimpleEdge(mp, &graph, 0, 1, 0);
	AddSimpleEdge(mp, &graph, 0, 2, 1);
	AddSimpleEdge(mp, &graph, 0, 3, 2);
	CRecordingReceiver receiver(mp);
	CDPHyperEnumerator enumerator(mp, &graph, &receiver);
	GPOS_UNITTEST_ASSERT(!enumerator.Enumerate());
	GPOS_UNITTEST_ASSERT(11 == receiver.SeenCount());
	GPOS_UNITTEST_ASSERT(FSeen(mp, receiver, {0, 1, 2, 3}));
	GPOS_UNITTEST_ASSERT(!FSeen(mp, receiver, {1, 2}));
	return GPOS_OK;
}

GPOS_RESULT
CDPHyperGraphTest::EresUnittest_Hyperedge()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDPHyperGraph graph(mp, 3);
	AddSimpleEdge(mp, &graph, 0, 1, 0);
	CAutoRef<CBitSet> left(Pbs(mp, {0, 1}));
	CAutoRef<CBitSet> right(Pbs(mp, {2}));
	graph.AddEdge(left.Value(), right.Value(), 1);
	CRecordingReceiver receiver(mp);
	CDPHyperEnumerator enumerator(mp, &graph, &receiver);
	GPOS_UNITTEST_ASSERT(!enumerator.Enumerate());
	GPOS_UNITTEST_ASSERT(FSeen(mp, receiver, {0, 1, 2}));
	GPOS_UNITTEST_ASSERT(!FSeen(mp, receiver, {0, 2}));
	GPOS_UNITTEST_ASSERT(!FSeen(mp, receiver, {1, 2}));
	return GPOS_OK;
}

GPOS_RESULT
CDPHyperGraphTest::EresUnittest_Disconnected()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDPHyperGraph graph(mp, 3);
	AddSimpleEdge(mp, &graph, 0, 1, 0);
	CRecordingReceiver receiver(mp);
	CDPHyperEnumerator enumerator(mp, &graph, &receiver);
	GPOS_UNITTEST_ASSERT(!enumerator.Enumerate());
	GPOS_UNITTEST_ASSERT(!FSeen(mp, receiver, {0, 1, 2}));
	return GPOS_OK;
}

GPOS_RESULT
CDPHyperGraphTest::EresUnittest_DynamicBitset()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDPHyperGraph graph(mp, 65);
	for (ULONG node = 0; node + 1 < 65; ++node)
	{
		AddSimpleEdge(mp, &graph, node, node + 1, node);
	}
	CRecordingReceiver receiver(mp);
	CDPHyperEnumerator enumerator(mp, &graph, &receiver);
	GPOS_UNITTEST_ASSERT(!enumerator.Enumerate());
	GPOS_UNITTEST_ASSERT(65 * 66 / 2 == receiver.SeenCount());
	CAutoRef<CBitSet> all(GPOS_NEW(mp) CBitSet(mp));
	for (ULONG node = 0; node < 65; ++node)
	{
		(void) all->ExchangeSet(node);
	}
	GPOS_UNITTEST_ASSERT(receiver.HasSeen(all.Value()));
	return GPOS_OK;
}

GPOS_RESULT
CDPHyperGraphTest::EresUnittest_ExhaustiveSimpleGraphs()
{
	// Differentially validate every undirected graph on five nodes. The
	// reference is a deliberately simple reachability test, independent of
	// the CSG-CMP enumeration.
	constexpr ULONG node_count = 5;
	constexpr ULONG edge_count = node_count * (node_count - 1) / 2;
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	for (ULONG graph_mask = 0; graph_mask < (ULONG(1) << edge_count);
		 ++graph_mask)
	{
		CDPHyperGraph graph(mp, node_count);
		ULONG edge_bit = 0;
		for (ULONG left = 0; left < node_count; ++left)
		{
			for (ULONG right = left + 1; right < node_count;
				 ++right, ++edge_bit)
			{
				if (0 != (graph_mask & (ULONG(1) << edge_bit)))
				{
					AddSimpleEdge(mp, &graph, left, right, edge_bit);
				}
			}
		}

		CRecordingReceiver receiver(mp);
		CDPHyperEnumerator enumerator(mp, &graph, &receiver);
		GPOS_UNITTEST_ASSERT(!enumerator.Enumerate());
		for (ULONG subset = 1; subset < (ULONG(1) << node_count); ++subset)
		{
			CAutoRef<CBitSet> nodes(GPOS_NEW(mp) CBitSet(mp));
			for (ULONG node = 0; node < node_count; ++node)
			{
				if (0 != (subset & (ULONG(1) << node)))
				{
					(void) nodes->ExchangeSet(node);
				}
			}
			GPOS_UNITTEST_ASSERT(FConnected(node_count, graph_mask, subset) ==
							 receiver.HasSeen(nodes.Value()));
		}
	}
	return GPOS_OK;
}

GPOS_RESULT
CDPHyperGraphTest::EresUnittest_DifferentialHypergraphs()
{
	// Validate complex-edge enumeration against an implementation-independent
	// oracle. The deterministic corpus combines ordinary and generalized
	// endpoints over five nodes, including disconnected graphs.
	constexpr ULONG node_count = 5;
	constexpr ULONG all_nodes = (ULONG(1) << node_count) - 1;
	std::vector<SMaskEdge> catalog;
	for (ULONG left = 1; left <= all_nodes; ++left)
	{
		for (ULONG right = left + 1; right <= all_nodes; ++right)
		{
			if (0 == (left & right))
			{
				catalog.push_back({left, right});
			}
		}
	}

	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	uint32_t random = 0x9e3779b9U;
	for (ULONG graph_id = 0; graph_id < 512; ++graph_id)
	{
		CDPHyperGraph graph(mp, node_count);
		std::vector<SMaskEdge> edges;
		std::vector<BOOL> selected(catalog.size(), false);
		const ULONG edge_count = graph_id % 9;
		for (ULONG edge_id = 0; edge_id < edge_count; ++edge_id)
		{
			random ^= random << 13;
			random ^= random >> 17;
			random ^= random << 5;
			ULONG index = random % catalog.size();
			while (selected[index])
			{
				index = (index + 1) % catalog.size();
			}
			selected[index] = true;
			const SMaskEdge edge = catalog[index];
			edges.push_back(edge);
			CAutoRef<CBitSet> left(PbsFromMask(mp, edge.m_left));
			CAutoRef<CBitSet> right(PbsFromMask(mp, edge.m_right));
			graph.AddEdge(left.Value(), right.Value(), edge_id);
		}

		CRecordingReceiver receiver(mp);
		CDPHyperEnumerator enumerator(mp, &graph, &receiver);
		GPOS_UNITTEST_ASSERT(!enumerator.Enumerate());
		std::vector<INT> memo(ULONG(1) << node_count, -1);
		memo[0] = 0;
		for (ULONG subset = 1; subset <= all_nodes; ++subset)
		{
			CAutoRef<CBitSet> nodes(PbsFromMask(mp, subset));
			GPOS_UNITTEST_ASSERT(
				FHyperConnected(subset, edges, &memo) ==
				receiver.HasSeen(nodes.Value()));
		}
	}
	return GPOS_OK;
}

GPOS_RESULT
CDPHyperGraphTest::EresUnittest_AtomicBudget()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDPHyperGraph graph(mp, 4);
	AddSimpleEdge(mp, &graph, 0, 1, 0);
	AddSimpleEdge(mp, &graph, 1, 2, 1);
	AddSimpleEdge(mp, &graph, 2, 3, 2);

	CDPHyperPlan limited(mp, 2);
	CDPHyperEnumerator limited_enumerator(mp, &graph, &limited);
	GPOS_UNITTEST_ASSERT(limited_enumerator.Enumerate());
	GPOS_UNITTEST_ASSERT(limited.BudgetExhausted());
	GPOS_UNITTEST_ASSERT(2 == limited.PairCount());
	GPOS_UNITTEST_ASSERT(!limited.Complete(graph.NodeCount()));

	CDPHyperPlan complete(mp, 100);
	CDPHyperEnumerator complete_enumerator(mp, &graph, &complete);
	GPOS_UNITTEST_ASSERT(!complete_enumerator.Enumerate());
	GPOS_UNITTEST_ASSERT(!complete.BudgetExhausted());
	GPOS_UNITTEST_ASSERT(complete.Complete(graph.NodeCount()));

	// Pair identity and connecting-edge provenance are distinct. Multiple
	// hyperedges may expose the same cut and must survive pair deduplication.
	CDPHyperPlan provenance(mp, 10);
	GPOS_UNITTEST_ASSERT(!provenance.FoundSingleNode(0));
	GPOS_UNITTEST_ASSERT(!provenance.FoundSingleNode(1));
	CAutoRef<CBitSet> node0(Pbs(mp, {0}));
	CAutoRef<CBitSet> node1(Pbs(mp, {1}));
	GPOS_UNITTEST_ASSERT(
		!provenance.FoundSubgraphPair(node0.Value(), node1.Value(), 7));
	GPOS_UNITTEST_ASSERT(
		!provenance.FoundSubgraphPair(node1.Value(), node0.Value(), 9));
	GPOS_UNITTEST_ASSERT(1 == provenance.PairCount());
	GPOS_UNITTEST_ASSERT(
		2 == provenance.Pairs()[0]->m_connecting_edges.size());
	GPOS_UNITTEST_ASSERT(
		7 == provenance.Pairs()[0]->m_connecting_edges[0]);
	GPOS_UNITTEST_ASSERT(
		9 == provenance.Pairs()[0]->m_connecting_edges[1]);

	CDPHyperPlan filtered(
		mp, 10,
		[](const CBitSet *, const CBitSet *, ULONG edge_id) {
			return 7 == edge_id;
		});
	GPOS_UNITTEST_ASSERT(!filtered.FoundSingleNode(0));
	GPOS_UNITTEST_ASSERT(!filtered.FoundSingleNode(1));
	GPOS_UNITTEST_ASSERT(
		!filtered.FoundSubgraphPair(node0.Value(), node1.Value(), 9));
	GPOS_UNITTEST_ASSERT(0 == filtered.PairCount());
	GPOS_UNITTEST_ASSERT(
		!filtered.FoundSubgraphPair(node0.Value(), node1.Value(), 7));
	GPOS_UNITTEST_ASSERT(1 == filtered.PairCount());
	return GPOS_OK;
}

GPOS_RESULT
CDPHyperGraphTest::EresUnittest_JoinRegion()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *cols0 = nullptr;
	CColRefArray *cols1 = nullptr;
	CColRefArray *cols2 = nullptr;
	CExpression *get0 = fix.PexprLogicalGet("dph_r0", 1, &cols0);
	CExpression *get1 = fix.PexprLogicalGet("dph_r1", 1, &cols1);
	CExpression *get2 = fix.PexprLogicalGet("dph_r2", 1, &cols2);
	CExpression *pred01 = fix.PexprEqPred((*cols0)[0], (*cols1)[0]);
	CExpression *pred12 = fix.PexprEqPred((*cols1)[0], (*cols2)[0]);

	CExpressionArray *components = GPOS_NEW(mp) CExpressionArray(mp);
	get0->AddRef();
	get1->AddRef();
	get2->AddRef();
	components->Append(get0);
	components->Append(get1);
	components->Append(get2);
	CExpressionArray *conjuncts = GPOS_NEW(mp) CExpressionArray(mp);
	pred01->AddRef();
	pred12->AddRef();
	conjuncts->Append(pred01);
	conjuncts->Append(pred12);

	CDPHyperJoinRegion region(mp, components, conjuncts, 100);
	GPOS_UNITTEST_ASSERT(region.Build());
	GPOS_UNITTEST_ASSERT(2 == region.GeneratedEdgeCount());
	GPOS_UNITTEST_ASSERT(4 == region.Graph()->DirectedEdgeCount());
	GPOS_UNITTEST_ASSERT(2 == region.PredicateCover(0)->Size());
	GPOS_UNITTEST_ASSERT(region.PredicateCover(0)->Get(0));
	GPOS_UNITTEST_ASSERT(region.PredicateCover(0)->Get(1));

	CDPHyperPlan plan(mp, 100);
	CDPHyperEnumerator enumerator(mp, region.Graph(), &plan);
	GPOS_UNITTEST_ASSERT(!enumerator.Enumerate());
	GPOS_UNITTEST_ASSERT(plan.Complete(region.NodeCount()));
	GPOS_UNITTEST_ASSERT(6 == plan.SeenCount());

	components->Release();
	conjuncts->Release();
	pred01->Release();
	pred12->Release();
	get0->Release();
	get1->Release();
	get2->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDPHyperGraphTest::EresUnittest_BinaryJoinRegionSpec()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *cols0 = nullptr;
	CColRefArray *cols1 = nullptr;
	CColRefArray *cols2 = nullptr;
	CExpression *get0 = fix.PexprLogicalGet("dph_s0", 1, &cols0);
	CExpression *get1 = fix.PexprLogicalGet("dph_s1", 1, &cols1);
	CExpression *get2 = fix.PexprLogicalGet("dph_s2", 1, &cols2);
	CExpression *pred01 = fix.PexprEqPred((*cols0)[0], (*cols1)[0]);
	CExpression *pred12 = fix.PexprEqPred((*cols1)[0], (*cols2)[0]);
	CExpression *pred02 = fix.PexprEqPred((*cols0)[0], (*cols2)[0]);

	CExpression *join01 = fix.PexprLogicalInnerJoin(get0, get1, pred01);
	CExpression *root = fix.PexprLogicalInnerJoin(join01, get2, pred12);
	CJoinRegionSpec inner_spec(mp);
	GPOS_UNITTEST_ASSERT(inner_spec.Build(root));
	GPOS_UNITTEST_ASSERT(3 == inner_spec.NodeCount());
	GPOS_UNITTEST_ASSERT(2 == inner_spec.EdgeCount());
	GPOS_UNITTEST_ASSERT(inner_spec.PureInner());
	GPOS_UNITTEST_ASSERT(COperator::EopLogicalInnerJoin ==
						 inner_spec.Edge(0)->JoinType());
	GPOS_UNITTEST_ASSERT(FSet(inner_spec.Edge(0)->Left(), {0}));
	GPOS_UNITTEST_ASSERT(FSet(inner_spec.Edge(0)->Right(), {1}));
	GPOS_UNITTEST_ASSERT(FSet(inner_spec.Edge(1)->Left(), {0, 1}));
	GPOS_UNITTEST_ASSERT(FSet(inner_spec.Edge(1)->Right(), {2}));
	CDPHyperJoinRegion binary_region(mp, &inner_spec, 100);
	GPOS_UNITTEST_ASSERT(binary_region.Build());
	GPOS_UNITTEST_ASSERT(3 == binary_region.NodeCount());
	GPOS_UNITTEST_ASSERT(2 == binary_region.PredicateCount());
	GPOS_UNITTEST_ASSERT(2 == binary_region.GeneratedEdgeCount());
	CDPHyperPlan binary_plan(mp, 100);
	CDPHyperEnumerator binary_enumerator(mp, binary_region.Graph(),
									  &binary_plan);
	GPOS_UNITTEST_ASSERT(!binary_enumerator.Enumerate());
	GPOS_UNITTEST_ASSERT(binary_plan.Complete(binary_region.NodeCount()));
	GPOS_UNITTEST_ASSERT(6 == binary_plan.SeenCount());

	CDPHyperGraphFingerprint *fingerprint = binary_region.Pfp();
	CExpressionArray *reversed_components =
		GPOS_NEW(mp) CExpressionArray(mp);
	get2->AddRef();
	get1->AddRef();
	get0->AddRef();
	reversed_components->Append(get2);
	reversed_components->Append(get1);
	reversed_components->Append(get0);
	CExpressionArray *reversed_predicates =
		GPOS_NEW(mp) CExpressionArray(mp);
	pred12->AddRef();
	pred01->AddRef();
	reversed_predicates->Append(pred12);
	reversed_predicates->Append(pred01);
	CDPHyperJoinRegion reordered_region(mp, reversed_components,
									 reversed_predicates, 100);
	GPOS_UNITTEST_ASSERT(reordered_region.Build());
	CDPHyperGraphFingerprint *reordered_fingerprint = reordered_region.Pfp();
	GPOS_UNITTEST_ASSERT(fingerprint->Matches(reordered_fingerprint));
	GPOS_UNITTEST_ASSERT(fingerprint->HashValue() ==
						 reordered_fingerprint->HashValue());

	CExpressionArray *fewer_predicates = GPOS_NEW(mp) CExpressionArray(mp);
	pred01->AddRef();
	fewer_predicates->Append(pred01);
	CDPHyperJoinRegion changed_region(mp, reversed_components,
								   fewer_predicates, 100);
	GPOS_UNITTEST_ASSERT(changed_region.Build());
	CDPHyperGraphFingerprint *changed_fingerprint = changed_region.Pfp();
	GPOS_UNITTEST_ASSERT(!fingerprint->Matches(changed_fingerprint));
	GPOS_DELETE(changed_fingerprint);
	GPOS_DELETE(reordered_fingerprint);
	GPOS_DELETE(fingerprint);
	reversed_components->Release();
	reversed_predicates->Release();
	fewer_predicates->Release();

	// Preserve an explicit predicate-free cut. Predicate-only component
	// freezing would keep {1,2}|{0} and lose the original {0}|{1} CROSS edge.
	CExpression *true_pred = CUtils::PexprScalarConstBool(mp, true);
	CExpression *cross01 = fix.PexprLogicalInnerJoin(get0, get1, true_pred);
	CExpression *cross_root =
		fix.PexprLogicalInnerJoin(cross01, get2, pred12);
	CJoinRegionSpec cross_spec(mp);
	GPOS_UNITTEST_ASSERT(cross_spec.Build(cross_root));
	CDPHyperJoinRegion cross_region(mp, &cross_spec, 100);
	GPOS_UNITTEST_ASSERT(cross_region.Build());
	GPOS_UNITTEST_ASSERT(2 == cross_region.GeneratedEdgeCount());
	GPOS_UNITTEST_ASSERT(1 == cross_region.CartesianEdgeCount());
	CDPHyperPlan cross_plan(mp, 100);
	CDPHyperEnumerator cross_enumerator(mp, cross_region.Graph(), &cross_plan);
	GPOS_UNITTEST_ASSERT(!cross_enumerator.Enumerate());
	GPOS_UNITTEST_ASSERT(cross_plan.Complete(cross_region.NodeCount()));
	CAutoRef<CBitSet> cross01_nodes(Pbs(mp, {0, 1}));
	GPOS_UNITTEST_ASSERT(cross_plan.HasSeen(cross01_nodes.Value()));

	// Atom and predicate multisets alone are not a graph fingerprint. Keep the
	// explicit CROSS skeleton cut distinct from the predicate-component cut.
	CExpressionArray *cross_components = GPOS_NEW(mp) CExpressionArray(mp);
	get0->AddRef();
	get1->AddRef();
	get2->AddRef();
	cross_components->Append(get0);
	cross_components->Append(get1);
	cross_components->Append(get2);
	CExpressionArray *cross_predicates = GPOS_NEW(mp) CExpressionArray(mp);
	true_pred->AddRef();
	pred12->AddRef();
	cross_predicates->Append(true_pred);
	cross_predicates->Append(pred12);
	CDPHyperJoinRegion component_cut_region(mp, cross_components,
										cross_predicates, 100);
	GPOS_UNITTEST_ASSERT(component_cut_region.Build());
	CDPHyperGraphFingerprint *cross_fingerprint = cross_region.Pfp();
	CDPHyperGraphFingerprint *component_cut_fingerprint =
		component_cut_region.Pfp();
	GPOS_UNITTEST_ASSERT(
		!cross_fingerprint->Matches(component_cut_fingerprint));
	GPOS_DELETE(component_cut_fingerprint);
	GPOS_DELETE(cross_fingerprint);
	cross_predicates->Release();
	cross_components->Release();

	// An outer edge is retained with its directional boundary.  DPHyper may
	// reject it initially, but the transient descriptor must not erase it.
	CExpression *loj01 = fix.PexprLogicalLeftOuterJoin(get0, get1, pred01);
	CExpression *outer_root =
		fix.PexprLogicalInnerJoin(loj01, get2, pred12);
	CJoinRegionSpec outer_spec(mp);
	GPOS_UNITTEST_ASSERT(outer_spec.Build(outer_root));
	GPOS_UNITTEST_ASSERT(!outer_spec.PureInner());
	GPOS_UNITTEST_ASSERT(outer_spec.CDCSupported());
	GPOS_UNITTEST_ASSERT(COperator::EopLogicalLeftOuterJoin ==
						 outer_spec.Edge(0)->JoinType());
	GPOS_UNITTEST_ASSERT(FSet(outer_spec.Edge(0)->Left(), {0}));
	GPOS_UNITTEST_ASSERT(FSet(outer_spec.Edge(0)->Right(), {1}));
	GPOS_UNITTEST_ASSERT(FSet(outer_spec.Edge(1)->Left(), {0, 1}));
	GPOS_UNITTEST_ASSERT(FSet(outer_spec.Edge(1)->Right(), {2}));
	GPOS_UNITTEST_ASSERT(FSet(outer_spec.Edge(0)->SES(), {0, 1}));
	GPOS_UNITTEST_ASSERT(FSet(outer_spec.Edge(0)->TES(), {0, 1}));
	// (A LOJ B) JOIN C cannot expose B before A. CD-C absorbs B -> A
	// into the root TES, freezing the original outer subtree.
	GPOS_UNITTEST_ASSERT(FSet(outer_spec.Edge(1)->SES(), {1, 2}));
	GPOS_UNITTEST_ASSERT(FSet(outer_spec.Edge(1)->TES(), {0, 1, 2}));
	GPOS_UNITTEST_ASSERT(outer_spec.Edge(1)->ConflictRules().empty());
	CExpression *marked_outer = CJoinRegionSpec::PexprMarkDPHyperRegions(
		mp, outer_root, true /*include complex*/);
	CLogicalJoin *marked_root =
		CLogicalJoin::PopConvert(marked_outer->Pop());
	CLogicalJoin *marked_child =
		CLogicalJoin::PopConvert((*marked_outer)[0]->Pop());
	GPOS_UNITTEST_ASSERT(marked_root->FDPHyperRegionMember());
	GPOS_UNITTEST_ASSERT(marked_root->FDPHyperRegionRoot());
	GPOS_UNITTEST_ASSERT(marked_child->FDPHyperRegionMember());
	GPOS_UNITTEST_ASSERT(!marked_child->FDPHyperRegionRoot());
	marked_outer->Release();
	CAutoRef<CBitSet> node0(Pbs(mp, {0}));
	CAutoRef<CBitSet> node1(Pbs(mp, {1}));
	CAutoRef<CBitSet> node2(Pbs(mp, {2}));
	CAutoRef<CBitSet> nodes01(Pbs(mp, {0, 1}));
	CAutoRef<CBitSet> nodes12(Pbs(mp, {1, 2}));
	GPOS_UNITTEST_ASSERT(
		outer_spec.Edge(0)->FApplicable(node0.Value(), node1.Value()));
	GPOS_UNITTEST_ASSERT(
		!outer_spec.Edge(0)->FApplicable(node1.Value(), node0.Value()));
	CDPHyperJoinRegion outer_eligibility(mp, &outer_spec, 100);
	GPOS_UNITTEST_ASSERT(outer_eligibility.Build());

	// Graph topology alone is insufficient for mixed regions.  Preserve the
	// logical join kind and the direction of non-commutative edges in Memo
	// ownership fingerprints.
	CJoinRegionSpec binary_inner_spec(mp);
	GPOS_UNITTEST_ASSERT(binary_inner_spec.Build(join01));
	CDPHyperJoinRegion binary_inner_region(mp, &binary_inner_spec, 100);
	GPOS_UNITTEST_ASSERT(binary_inner_region.Build());
	CJoinRegionSpec binary_outer_spec(mp);
	GPOS_UNITTEST_ASSERT(binary_outer_spec.Build(loj01));
	CDPHyperJoinRegion binary_outer_region(mp, &binary_outer_spec, 100);
	GPOS_UNITTEST_ASSERT(binary_outer_region.Build());
	CDPHyperGraphFingerprint *binary_inner_fingerprint =
		binary_inner_region.Pfp();
	CDPHyperGraphFingerprint *binary_outer_fingerprint =
		binary_outer_region.Pfp();
	GPOS_UNITTEST_ASSERT(
		!binary_inner_fingerprint->Matches(binary_outer_fingerprint));
	GPOS_UNITTEST_ASSERT(binary_inner_fingerprint->HashValue() !=
						 binary_outer_fingerprint->HashValue());

	CExpression *reversed_loj =
		fix.PexprLogicalLeftOuterJoin(get1, get0, pred01);
	CJoinRegionSpec reversed_outer_spec(mp);
	GPOS_UNITTEST_ASSERT(reversed_outer_spec.Build(reversed_loj));
	CDPHyperJoinRegion reversed_outer_region(mp, &reversed_outer_spec, 100);
	GPOS_UNITTEST_ASSERT(reversed_outer_region.Build());
	CDPHyperGraphFingerprint *reversed_outer_fingerprint =
		reversed_outer_region.Pfp();
	GPOS_UNITTEST_ASSERT(
		!binary_outer_fingerprint->Matches(reversed_outer_fingerprint));
	GPOS_DELETE(reversed_outer_fingerprint);
	GPOS_DELETE(binary_outer_fingerprint);
	GPOS_DELETE(binary_inner_fingerprint);
	reversed_loj->Release();

	std::vector<CDPHyperJoinRegion::SApplicableEdge> outer_edges =
		outer_eligibility.ApplicableEdges(node0.Value(), node1.Value());
	GPOS_UNITTEST_ASSERT(1 == outer_edges.size());
	GPOS_UNITTEST_ASSERT(0 == outer_edges[0].m_edge_id);
	GPOS_UNITTEST_ASSERT(!outer_edges[0].m_swapped);
	outer_edges =
		outer_eligibility.ApplicableEdges(node1.Value(), node0.Value());
	GPOS_UNITTEST_ASSERT(1 == outer_edges.size());
	GPOS_UNITTEST_ASSERT(outer_edges[0].m_swapped);
	CEligibilityReceiver outer_receiver(mp, &outer_eligibility);
	CDPHyperEnumerator outer_enumerator(
		mp, outer_eligibility.Graph(), &outer_receiver);
	GPOS_UNITTEST_ASSERT(!outer_enumerator.Enumerate());
	GPOS_UNITTEST_ASSERT(outer_receiver.Complete(3));
	GPOS_UNITTEST_ASSERT(outer_receiver.HasSeen(nodes01.Value()));
	GPOS_UNITTEST_ASSERT(!outer_receiver.HasSeen(nodes12.Value()));

	// (A JOIN B) LOJ C can associate to A JOIN (B LOJ C) because the
	// outer predicate only needs B and C. No conflict widens its TES to A.
	CExpression *safe_outer_root =
		fix.PexprLogicalLeftOuterJoin(join01, get2, pred12);
	CJoinRegionSpec safe_outer_spec(mp);
	GPOS_UNITTEST_ASSERT(safe_outer_spec.Build(safe_outer_root));
	GPOS_UNITTEST_ASSERT(FSet(safe_outer_spec.Edge(1)->SES(), {1, 2}));
	GPOS_UNITTEST_ASSERT(FSet(safe_outer_spec.Edge(1)->TES(), {1, 2}));
	GPOS_UNITTEST_ASSERT(
		safe_outer_spec.Edge(1)->FApplicable(node1.Value(), node2.Value()));
	GPOS_UNITTEST_ASSERT(
		!safe_outer_spec.Edge(1)->FApplicable(node2.Value(), node1.Value()));
	CDPHyperJoinRegion safe_outer_eligibility(mp, &safe_outer_spec, 100);
	GPOS_UNITTEST_ASSERT(safe_outer_eligibility.Build());
	std::vector<CDPHyperJoinRegion::SApplicableEdge> safe_edges =
		safe_outer_eligibility.ApplicableEdges(node1.Value(), node2.Value());
	GPOS_UNITTEST_ASSERT(1 == safe_edges.size());
	GPOS_UNITTEST_ASSERT(1 == safe_edges[0].m_edge_id);
	GPOS_UNITTEST_ASSERT(!safe_edges[0].m_swapped);
	CDPHyperJoinRegion::SJoinRequest safe_request;
	GPOS_UNITTEST_ASSERT(safe_outer_eligibility.FBuildJoinRequest(
		node1.Value(), node2.Value(), &safe_request));
	GPOS_UNITTEST_ASSERT(COperator::EopLogicalLeftOuterJoin ==
						 safe_request.m_join_type);
	GPOS_UNITTEST_ASSERT(!safe_request.m_swapped);
	GPOS_UNITTEST_ASSERT(1 == safe_request.m_edge_ids.size());
	CExpression *safe_predicate =
		safe_outer_eligibility.PexprPredicate(safe_request);
	GPOS_UNITTEST_ASSERT(safe_predicate->Matches(pred12));
	safe_predicate->Release();
	GPOS_UNITTEST_ASSERT(safe_outer_eligibility.FBuildJoinRequest(
		node2.Value(), node1.Value(), &safe_request));
	GPOS_UNITTEST_ASSERT(safe_request.m_swapped);
	CEligibilityReceiver safe_outer_receiver(mp, &safe_outer_eligibility);
	CDPHyperEnumerator safe_outer_enumerator(
		mp, safe_outer_eligibility.Graph(), &safe_outer_receiver);
	GPOS_UNITTEST_ASSERT(!safe_outer_enumerator.Enumerate());
	GPOS_UNITTEST_ASSERT(safe_outer_receiver.Complete(3));
	GPOS_UNITTEST_ASSERT(safe_outer_receiver.HasSeen(nodes01.Value()));
	GPOS_UNITTEST_ASSERT(safe_outer_receiver.HasSeen(nodes12.Value()));

	// A LOJ (B JOIN C) cannot become (A LOJ B) JOIN C. The mirrored
	// associative conflict is absorbed and expands the LOJ TES to all nodes.
	CExpression *join12 = fix.PexprLogicalInnerJoin(get1, get2, pred12);
	CExpression *right_outer_root =
		fix.PexprLogicalLeftOuterJoin(get0, join12, pred01);
	CJoinRegionSpec right_outer_spec(mp);
	GPOS_UNITTEST_ASSERT(right_outer_spec.Build(right_outer_root));
	GPOS_UNITTEST_ASSERT(FSet(right_outer_spec.Edge(1)->SES(), {0, 1}));
	GPOS_UNITTEST_ASSERT(FSet(right_outer_spec.Edge(1)->TES(), {0, 1, 2}));
	CDPHyperJoinRegion right_outer_eligibility(mp, &right_outer_spec, 100);
	GPOS_UNITTEST_ASSERT(right_outer_eligibility.Build());
	CEligibilityReceiver right_outer_receiver(mp,
										&right_outer_eligibility);
	CDPHyperEnumerator right_outer_enumerator(
		mp, right_outer_eligibility.Graph(), &right_outer_receiver);
	GPOS_UNITTEST_ASSERT(!right_outer_enumerator.Enumerate());
	GPOS_UNITTEST_ASSERT(right_outer_receiver.Complete(3));
	GPOS_UNITTEST_ASSERT(!right_outer_receiver.HasSeen(nodes01.Value()));
	GPOS_UNITTEST_ASSERT(right_outer_receiver.HasSeen(nodes12.Value()));

	// Semi/anti joins participate in CD-C but keep their right input
	// directional. Moving an independent inner join into the preserved input
	// is legal, so the root TES remains {A,C}; B is not pulled into it.
	CExpression *semi01 = PexprJoin<CLogicalLeftSemiJoin>(
		mp, get0, get1, pred01);
	CExpression *semi_root =
		fix.PexprLogicalInnerJoin(semi01, get2, pred02);
	CJoinRegionSpec semi_spec(mp);
	GPOS_UNITTEST_ASSERT(semi_spec.Build(semi_root));
	GPOS_UNITTEST_ASSERT(semi_spec.CDCSupported());
	GPOS_UNITTEST_ASSERT(FSet(semi_spec.Edge(1)->TES(), {0, 2}));
	GPOS_UNITTEST_ASSERT(
		semi_spec.Edge(0)->FApplicable(node0.Value(), node1.Value()));
	GPOS_UNITTEST_ASSERT(
		!semi_spec.Edge(0)->FApplicable(node1.Value(), node0.Value()));

	CExpression *anti01 = PexprJoin<CLogicalLeftAntiSemiJoin>(
		mp, get0, get1, pred01);
	CExpression *anti_root =
		fix.PexprLogicalInnerJoin(anti01, get2, pred02);
	CJoinRegionSpec anti_spec(mp);
	GPOS_UNITTEST_ASSERT(anti_spec.Build(anti_root));
	GPOS_UNITTEST_ASSERT(anti_spec.CDCSupported());
	GPOS_UNITTEST_ASSERT(FSet(anti_spec.Edge(1)->TES(), {0, 2}));
	GPOS_UNITTEST_ASSERT(
		anti_spec.Edge(0)->FApplicable(node0.Value(), node1.Value()));
	GPOS_UNITTEST_ASSERT(
		!anti_spec.Edge(0)->FApplicable(node1.Value(), node0.Value()));

	// NOT IN is null-aware and cannot share ordinary anti-join algebra. Keep
	// the descriptor intact but force the future enumerator to native fallback.
	CExpression *notin01 = PexprJoin<CLogicalLeftAntiSemiJoinNotIn>(
		mp, get0, get1, pred01);
	CJoinRegionSpec notin_spec(mp);
	GPOS_UNITTEST_ASSERT(notin_spec.Build(notin01));
	GPOS_UNITTEST_ASSERT(!notin_spec.CDCSupported());

	notin01->Release();
	anti_root->Release();
	anti01->Release();
	semi_root->Release();
	semi01->Release();

	right_outer_root->Release();
	join12->Release();
	safe_outer_root->Release();

	outer_root->Release();
	loj01->Release();
	cross_root->Release();
	cross01->Release();
	true_pred->Release();
	root->Release();
	join01->Release();
	pred01->Release();
	pred12->Release();
	pred02->Release();
	get0->Release();
	get1->Release();
	get2->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDPHyperGraphTest::EresUnittest_CartesianComponents()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *cols0 = nullptr;
	CColRefArray *cols1 = nullptr;
	CColRefArray *cols2 = nullptr;
	CExpression *get0 = fix.PexprLogicalGet("dph_x0", 1, &cols0);
	CExpression *get1 = fix.PexprLogicalGet("dph_x1", 1, &cols1);
	CExpression *get2 = fix.PexprLogicalGet("dph_x2", 1, &cols2);
	CExpression *pred01 = fix.PexprEqPred((*cols0)[0], (*cols1)[0]);

	CExpressionArray *components = GPOS_NEW(mp) CExpressionArray(mp);
	get0->AddRef();
	get1->AddRef();
	get2->AddRef();
	components->Append(get0);
	components->Append(get1);
	components->Append(get2);
	CExpressionArray *conjuncts = GPOS_NEW(mp) CExpressionArray(mp);
	pred01->AddRef();
	conjuncts->Append(pred01);

	CDPHyperJoinRegion mixed(mp, components, conjuncts, 100);
	GPOS_UNITTEST_ASSERT(mixed.Build());
	GPOS_UNITTEST_ASSERT(2 == mixed.GeneratedEdgeCount());
	GPOS_UNITTEST_ASSERT(1 == mixed.CartesianEdgeCount());
	CDPHyperPlan mixed_plan(mp, 100);
	CDPHyperEnumerator mixed_enumerator(mp, mixed.Graph(), &mixed_plan);
	GPOS_UNITTEST_ASSERT(!mixed_enumerator.Enumerate());
	GPOS_UNITTEST_ASSERT(mixed_plan.Complete(mixed.NodeCount()));
	GPOS_UNITTEST_ASSERT(5 == mixed_plan.SeenCount());

	CExpressionArray *no_predicates = GPOS_NEW(mp) CExpressionArray(mp);
	CDPHyperJoinRegion cartesian(mp, components, no_predicates, 100);
	GPOS_UNITTEST_ASSERT(cartesian.Build());
	GPOS_UNITTEST_ASSERT(3 == cartesian.GeneratedEdgeCount());
	GPOS_UNITTEST_ASSERT(3 == cartesian.CartesianEdgeCount());
	CDPHyperPlan cartesian_plan(mp, 100);
	CDPHyperEnumerator cartesian_enumerator(mp, cartesian.Graph(),
										  &cartesian_plan);
	GPOS_UNITTEST_ASSERT(!cartesian_enumerator.Enumerate());
	GPOS_UNITTEST_ASSERT(cartesian_plan.Complete(cartesian.NodeCount()));
	GPOS_UNITTEST_ASSERT(7 == cartesian_plan.SeenCount());

	CDPHyperJoinRegion budget_limited(mp, components, no_predicates, 2);
	GPOS_UNITTEST_ASSERT(!budget_limited.Build());
	GPOS_UNITTEST_ASSERT(budget_limited.EdgeBudgetExhausted());
	GPOS_UNITTEST_ASSERT(2 == budget_limited.GeneratedEdgeCount());
	GPOS_UNITTEST_ASSERT(2 == budget_limited.CartesianEdgeCount());

	no_predicates->Release();
	components->Release();
	conjuncts->Release();
	pred01->Release();
	get0->Release();
	get1->Release();
	get2->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDPHyperGraphTest::EresUnittest_CartesianDifferential()
{
	// Validate region construction, rather than only the graph enumerator, for
	// every undirected predicate graph on five relations. The independent oracle
	// permits connected subsets and unions of whole connected components only.
	constexpr ULONG node_count = 5;
	constexpr ULONG edge_count = node_count * (node_count - 1) / 2;
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	std::vector<CExpression *> gets;
	std::vector<CColRefArray *> columns;
	for (ULONG node = 0; node < node_count; ++node)
	{
		CColRefArray *node_columns = nullptr;
		gets.push_back(fix.PexprLogicalGet("dph_diff", 1, &node_columns));
		columns.push_back(node_columns);
	}
	std::vector<CExpression *> predicates;
	for (ULONG left = 0; left < node_count; ++left)
	{
		for (ULONG right = left + 1; right < node_count; ++right)
		{
			predicates.push_back(
				fix.PexprEqPred((*columns[left])[0], (*columns[right])[0]));
		}
	}

	for (ULONG graph_mask = 0; graph_mask < (ULONG(1) << edge_count);
		 ++graph_mask)
	{
		CExpressionArray *components = GPOS_NEW(mp) CExpressionArray(mp);
		for (CExpression *get : gets)
		{
			get->AddRef();
			components->Append(get);
		}
		CExpressionArray *conjuncts = GPOS_NEW(mp) CExpressionArray(mp);
		for (ULONG edge = 0; edge < edge_count; ++edge)
		{
			if (0 != (graph_mask & (ULONG(1) << edge)))
			{
				predicates[edge]->AddRef();
				conjuncts->Append(predicates[edge]);
			}
		}

		CDPHyperJoinRegion region(mp, components, conjuncts, 100);
		components->Release();
		conjuncts->Release();
		GPOS_UNITTEST_ASSERT(region.Build());
		CDPHyperPlan plan(mp, 100000);
		CDPHyperEnumerator enumerator(mp, region.Graph(), &plan);
		GPOS_UNITTEST_ASSERT(!enumerator.Enumerate());
		for (ULONG subset = 1; subset < (ULONG(1) << node_count); ++subset)
		{
			CAutoRef<CBitSet> nodes(PbsFromMask(mp, subset));
			GPOS_UNITTEST_ASSERT(
				FFrozenCartesianConstructible(node_count, graph_mask, subset) ==
				plan.HasSeen(nodes.Value()));
		}
	}

	for (CExpression *predicate : predicates)
	{
		predicate->Release();
	}
	for (CExpression *get : gets)
	{
		get->Release();
	}
	return GPOS_OK;
}
