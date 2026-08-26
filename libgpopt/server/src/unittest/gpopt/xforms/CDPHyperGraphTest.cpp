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

#include "gpopt/xforms/CDPHyperGraph.h"
#include "gpopt/xforms/CDPHyperJoinRegion.h"
#include "gpopt/xforms/CDPHyperPlan.h"
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
