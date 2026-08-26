//---------------------------------------------------------------------------
//	@filename:
//		CDPHyperGraph.cpp
//---------------------------------------------------------------------------
#include "gpopt/xforms/CDPHyperGraph.h"

#include <algorithm>

#include "gpos/common/CAutoRef.h"
#include "gpos/common/CBitSetIter.h"

using namespace gpopt;

CDPHyperEnumerator::CNeighborhoodCache::CNeighborhoodCache(
	CMemoryPool *mp, const CBitSet *neighborhood)
	: m_mp(mp),
	  m_taboo(GPOS_NEW(mp) CBitSet(mp)),
	  m_last_grown(GPOS_NEW(mp) CBitSet(mp)),
	  m_last_neighborhood(GPOS_NEW(mp) CBitSet(mp)),
	  m_last_full_neighborhood(GPOS_NEW(mp) CBitSet(mp)),
	  m_initialized(false)
{
	if (0 < neighborhood->Size())
	{
		CBitSetIter iter(*neighborhood);
		GPOS_ASSERT(iter.Advance());
		(void) m_taboo->ExchangeSet(iter.Bit());
	}
}

CDPHyperEnumerator::CNeighborhoodCache::~CNeighborhoodCache()
{
	m_taboo->Release();
	m_last_grown->Release();
	m_last_neighborhood->Release();
	m_last_full_neighborhood->Release();
}

CBitSet *
CDPHyperEnumerator::CNeighborhoodCache::PbsToSearch(
	const CBitSet *just_grown, CBitSet *neighborhood,
	CBitSet *full_neighborhood)
{
	CBitSet *to_search = GPOS_NEW(m_mp) CBitSet(m_mp, *just_grown);
	if (m_initialized && just_grown->ContainsAll(m_last_grown))
	{
		neighborhood->Union(m_last_neighborhood);
		full_neighborhood->Union(m_last_full_neighborhood);
		to_search->Difference(m_last_grown);
	}
	return to_search;
}

void
CDPHyperEnumerator::CNeighborhoodCache::Store(
	const CBitSet *just_grown, const CBitSet *neighborhood,
	const CBitSet *full_neighborhood)
{
	if (!just_grown->IsDisjoint(m_taboo))
	{
		return;
	}
	m_last_grown->Release();
	m_last_neighborhood->Release();
	m_last_full_neighborhood->Release();
	m_last_grown = GPOS_NEW(m_mp) CBitSet(m_mp, *just_grown);
	m_last_neighborhood = GPOS_NEW(m_mp) CBitSet(m_mp, *neighborhood);
	m_last_full_neighborhood =
		GPOS_NEW(m_mp) CBitSet(m_mp, *full_neighborhood);
	m_initialized = true;
}

CDPHyperGraph::SEdge::SEdge(CMemoryPool *mp, const CBitSet *left,
								const CBitSet *right, ULONG edge_id)
	: m_left(GPOS_NEW(mp) CBitSet(mp, *left)),
	  m_right(GPOS_NEW(mp) CBitSet(mp, *right)),
	  m_edge_id(edge_id)
{
}

CDPHyperGraph::SEdge::~SEdge()
{
	m_left->Release();
	m_right->Release();
}

CDPHyperGraph::SNode::SNode(CMemoryPool *mp)
	: m_simple_neighborhood(GPOS_NEW(mp) CBitSet(mp))
{
}

CDPHyperGraph::SNode::~SNode()
{
	m_simple_neighborhood->Release();
}

CDPHyperGraph::CDPHyperGraph(CMemoryPool *mp, ULONG node_count) : m_mp(mp)
{
	m_nodes.reserve(node_count);
	for (ULONG node_id = 0; node_id < node_count; ++node_id)
	{
		m_nodes.push_back(GPOS_NEW(mp) SNode(mp));
	}
}

CDPHyperGraph::~CDPHyperGraph()
{
	for (SEdge *edge : m_edges)
	{
		GPOS_DELETE(edge);
	}
	for (SNode *node : m_nodes)
	{
		GPOS_DELETE(node);
	}
}

void
CDPHyperGraph::AttachEdgeToNodes(ULONG forward_edge, ULONG reverse_edge,
								 const CBitSet *left,
								 const CBitSet *right)
{
	const BOOL simple = 1 == left->Size() && 1 == right->Size();
	if (simple)
	{
		CBitSetIter left_iter(*left);
		CBitSetIter right_iter(*right);
		GPOS_ASSERT(left_iter.Advance() && right_iter.Advance());
		SNode *left_node = m_nodes[left_iter.Bit()];
		SNode *right_node = m_nodes[right_iter.Bit()];
		left_node->m_simple_neighborhood->Union(right);
		right_node->m_simple_neighborhood->Union(left);
		left_node->m_simple_edges.push_back(forward_edge);
		right_node->m_simple_edges.push_back(reverse_edge);
		return;
	}

	CBitSetIter left_iter(*left);
	while (left_iter.Advance())
	{
		GPOS_ASSERT(left_iter.Bit() < m_nodes.size());
		m_nodes[left_iter.Bit()]->m_complex_edges.push_back(forward_edge);
	}
	CBitSetIter right_iter(*right);
	while (right_iter.Advance())
	{
		GPOS_ASSERT(right_iter.Bit() < m_nodes.size());
		m_nodes[right_iter.Bit()]->m_complex_edges.push_back(reverse_edge);
	}
}

void
CDPHyperGraph::AddEdge(const CBitSet *left, const CBitSet *right,
					   ULONG edge_id)
{
	GPOS_ASSERT(nullptr != left && nullptr != right);
	GPOS_ASSERT(0 < left->Size() && 0 < right->Size());
	GPOS_ASSERT(left->IsDisjoint(right));

	const ULONG forward = m_edges.size();
	m_edges.push_back(GPOS_NEW(m_mp) SEdge(m_mp, left, right, edge_id));
	const ULONG reverse = m_edges.size();
	m_edges.push_back(GPOS_NEW(m_mp) SEdge(m_mp, right, left, edge_id));
	AttachEdgeToNodes(forward, reverse, left, right);
}

CDPHyperEnumerator::CDPHyperEnumerator(CMemoryPool *mp,
									   const CDPHyperGraph *graph,
									   IDPHyperReceiver *receiver)
	: m_mp(mp), m_graph(graph), m_receiver(receiver)
{
	GPOS_ASSERT(nullptr != mp && nullptr != graph && nullptr != receiver);
}

CBitSet *
CDPHyperEnumerator::PbsSingleton(ULONG node_id) const
{
	CBitSet *set = GPOS_NEW(m_mp) CBitSet(m_mp);
	(void) set->ExchangeSet(node_id);
	return set;
}

CBitSet *
CDPHyperEnumerator::PbsLowerNodes(ULONG upper_bound) const
{
	CBitSet *set = GPOS_NEW(m_mp) CBitSet(m_mp);
	for (ULONG node_id = 0; node_id < upper_bound; ++node_id)
	{
		(void) set->ExchangeSet(node_id);
	}
	return set;
}

ULONG
CDPHyperEnumerator::LowestBit(const CBitSet *set) const
{
	GPOS_ASSERT(nullptr != set && 0 < set->Size());
	CBitSetIter iter(*set);
	GPOS_ASSERT(iter.Advance());
	return iter.Bit();
}

CBitSet *
CDPHyperEnumerator::PbsLowestBit(const CBitSet *set) const
{
	return PbsSingleton(LowestBit(set));
}

BOOL
CDPHyperEnumerator::EnumerateSubsetsRecursive(
	const std::vector<ULONG> &bits, ULONG pos, CBitSet *current,
	const SubsetCallback &callback)
{
	GPOS_CHECK_ABORT;
	if (pos == bits.size())
	{
		return 0 < current->Size() && callback(current);
	}

	if (EnumerateSubsetsRecursive(bits, pos + 1, current, callback))
	{
		return true;
	}

	(void) current->ExchangeSet(bits[pos]);
	const BOOL abort =
		EnumerateSubsetsRecursive(bits, pos + 1, current, callback);
	(void) current->ExchangeClear(bits[pos]);
	return abort;
}

BOOL
CDPHyperEnumerator::EnumerateSubsets(const CBitSet *set,
									 const SubsetCallback &callback)
{
	std::vector<ULONG> bits;
	bits.reserve(set->Size());
	CBitSetIter iter(*set);
	while (iter.Advance())
	{
		bits.push_back(iter.Bit());
	}
	// Match the numeric-mask order used by DPHyp: the lowest node toggles
	// fastest. The two-pass recursion in Expand* still guarantees that all
	// smaller connected subplans are recorded before they are consumed.
	std::reverse(bits.begin(), bits.end());
	CAutoRef<CBitSet> current(GPOS_NEW(m_mp) CBitSet(m_mp));
	return EnumerateSubsetsRecursive(bits, 0, current.Value(), callback);
}

CBitSet *
CDPHyperEnumerator::PbsNeighborhood(const CBitSet *subgraph,
									const CBitSet *forbidden,
									const CBitSet *just_grown,
									CNeighborhoodCache *cache,
									CBitSet *full_neighborhood) const
{
	GPOS_ASSERT(subgraph->ContainsAll(just_grown));
	CBitSet *neighborhood = GPOS_NEW(m_mp) CBitSet(m_mp);
	CAutoRef<CBitSet> to_search(cache->PbsToSearch(
		just_grown, neighborhood, full_neighborhood));

	CBitSetIter grown_iter(*to_search.Value());
	while (grown_iter.Advance())
	{
		const CDPHyperGraph::SNode *node = m_graph->Node(grown_iter.Bit());
		neighborhood->Union(node->m_simple_neighborhood);

		for (ULONG edge_idx : node->m_complex_edges)
		{
			const CDPHyperGraph::SEdge *edge = m_graph->Edge(edge_idx);
			if (!subgraph->ContainsAll(edge->m_left) ||
				!edge->m_right->IsDisjoint(subgraph) ||
				!edge->m_right->IsDisjoint(forbidden))
			{
				continue;
			}

			full_neighborhood->Union(edge->m_right);
			if (edge->m_right->IsDisjoint(neighborhood))
			{
				CAutoRef<CBitSet> representative(PbsLowestBit(edge->m_right));
				neighborhood->Union(representative.Value());
			}
		}
	}

	neighborhood->Difference(subgraph);
	neighborhood->Difference(forbidden);
	full_neighborhood->Union(neighborhood);
	cache->Store(just_grown, neighborhood, full_neighborhood);
	return neighborhood;
}

BOOL
CDPHyperEnumerator::TryConnecting(
	const CBitSet *subgraph, const CBitSet *subgraph_full_neighborhood,
	const CBitSet *complement)
{
	CAutoRef<CBitSet> candidates(
		GPOS_NEW(m_mp) CBitSet(m_mp, *complement));
	candidates->Intersection(subgraph_full_neighborhood);

	CBitSetIter node_iter(*candidates);
	while (node_iter.Advance())
	{
		const ULONG node_id = node_iter.Bit();
		const CDPHyperGraph::SNode *node = m_graph->Node(node_id);
		if (!node->m_simple_neighborhood->IsDisjoint(subgraph))
		{
			for (ULONG edge_idx : node->m_simple_edges)
			{
				const CDPHyperGraph::SEdge *edge = m_graph->Edge(edge_idx);
				if (!edge->m_right->IsDisjoint(subgraph) &&
					!edge->m_left->IsDisjoint(complement) &&
					m_receiver->FoundSubgraphPair(
						subgraph, complement, edge->m_edge_id))
				{
					return true;
				}
			}
		}

		for (ULONG edge_idx : node->m_complex_edges)
		{
			const CDPHyperGraph::SEdge *edge = m_graph->Edge(edge_idx);
			if (node_id == LowestBit(edge->m_left) &&
				complement->ContainsAll(edge->m_left) &&
				subgraph->ContainsAll(edge->m_right) &&
				m_receiver->FoundSubgraphPair(subgraph, complement,
										  edge->m_edge_id))
			{
				return true;
			}
		}
	}
	return false;
}

BOOL
CDPHyperEnumerator::ExpandComplement(
	ULONG lowest_node, const CBitSet *subgraph,
	const CBitSet *subgraph_full_neighborhood, const CBitSet *complement,
	const CBitSet *neighborhood, const CBitSet *forbidden)
{
	GPOS_ASSERT(forbidden->ContainsAll(subgraph));
	GPOS_ASSERT(!forbidden->ContainsAll(complement));

	if (EnumerateSubsets(neighborhood, [&](const CBitSet *grow_by) {
			CAutoRef<CBitSet> grown(
				GPOS_NEW(m_mp) CBitSet(m_mp, *complement));
			grown->Union(grow_by);
			return m_receiver->HasSeen(grown.Value()) &&
				   TryConnecting(subgraph, subgraph_full_neighborhood,
								 grown.Value());
		}))
	{
		return true;
	}

	CNeighborhoodCache cache(m_mp, neighborhood);
	return EnumerateSubsets(neighborhood, [&](const CBitSet *grow_by) {
		CAutoRef<CBitSet> grown(
			GPOS_NEW(m_mp) CBitSet(m_mp, *complement));
		grown->Union(grow_by);

		CAutoRef<CBitSet> new_forbidden(
			GPOS_NEW(m_mp) CBitSet(m_mp, *forbidden));
		new_forbidden->Union(neighborhood);
		new_forbidden->Difference(grown.Value());

		CAutoRef<CBitSet> new_full(GPOS_NEW(m_mp) CBitSet(m_mp));
		CAutoRef<CBitSet> new_neighborhood(PbsNeighborhood(
			grown.Value(), new_forbidden.Value(), grow_by, &cache,
			new_full.Value()));
		return ExpandComplement(lowest_node, subgraph,
								subgraph_full_neighborhood, grown.Value(),
								new_neighborhood.Value(), new_forbidden.Value());
	});
}

BOOL
CDPHyperEnumerator::EnumerateComplements(
	ULONG lowest_node, const CBitSet *subgraph,
	const CBitSet *full_neighborhood, const CBitSet *input_neighborhood)
{
	CAutoRef<CBitSet> neighborhood(
		GPOS_NEW(m_mp) CBitSet(m_mp, *input_neighborhood));
	neighborhood->Difference(subgraph);
	CAutoRef<CBitSet> forbidden(PbsLowerNodes(lowest_node));
	CNeighborhoodCache cache(m_mp, neighborhood.Value());

	std::vector<ULONG> seeds;
	CBitSetIter seed_iter(*neighborhood);
	while (seed_iter.Advance())
	{
		seeds.push_back(seed_iter.Bit());
	}
	std::reverse(seeds.begin(), seeds.end());

	for (ULONG seed_id : seeds)
	{
		CAutoRef<CBitSet> seed(PbsSingleton(seed_id));
		const CDPHyperGraph::SNode *node = m_graph->Node(seed_id);
		if (!node->m_simple_neighborhood->IsDisjoint(subgraph))
		{
			for (ULONG edge_idx : node->m_simple_edges)
			{
				const CDPHyperGraph::SEdge *edge = m_graph->Edge(edge_idx);
				if (!edge->m_right->IsDisjoint(subgraph) &&
					m_receiver->FoundSubgraphPair(
						subgraph, seed.Value(), edge->m_edge_id))
				{
					return true;
				}
			}
		}
		for (ULONG edge_idx : node->m_complex_edges)
		{
			const CDPHyperGraph::SEdge *edge = m_graph->Edge(edge_idx);
			if (edge->m_left->Equals(seed.Value()) &&
				subgraph->ContainsAll(edge->m_right) &&
				m_receiver->FoundSubgraphPair(subgraph, seed.Value(),
										  edge->m_edge_id))
			{
				return true;
			}
		}

		CAutoRef<CBitSet> lower_seed(PbsLowerNodes(seed_id));
		CAutoRef<CBitSet> new_forbidden(
			GPOS_NEW(m_mp) CBitSet(m_mp, *forbidden));
		new_forbidden->Union(subgraph);
		lower_seed->Intersection(neighborhood.Value());
		new_forbidden->Union(lower_seed.Value());

		CAutoRef<CBitSet> new_full(GPOS_NEW(m_mp) CBitSet(m_mp));
		CAutoRef<CBitSet> new_neighborhood(PbsNeighborhood(
			seed.Value(), new_forbidden.Value(), seed.Value(), &cache,
			new_full.Value()));
		if (ExpandComplement(lowest_node, subgraph, full_neighborhood,
							 seed.Value(), new_neighborhood.Value(),
							 new_forbidden.Value()))
		{
			return true;
		}
	}
	return false;
}

BOOL
CDPHyperEnumerator::ExpandSubgraph(
	ULONG lowest_node, const CBitSet *subgraph,
	const CBitSet *full_neighborhood, const CBitSet *neighborhood,
	const CBitSet *forbidden)
{
	CNeighborhoodCache cache(m_mp, neighborhood);
	if (EnumerateSubsets(neighborhood, [&](const CBitSet *grow_by) {
			CAutoRef<CBitSet> grown(
				GPOS_NEW(m_mp) CBitSet(m_mp, *subgraph));
			grown->Union(grow_by);
			if (!m_receiver->HasSeen(grown.Value()))
			{
				return false;
			}

			CAutoRef<CBitSet> new_full(
				GPOS_NEW(m_mp) CBitSet(m_mp, *full_neighborhood));
			CAutoRef<CBitSet> new_neighborhood(PbsNeighborhood(
				grown.Value(), forbidden, grow_by, &cache, new_full.Value()));

			CAutoRef<CBitSet> high_nodes(PbsLowerNodes(m_graph->NodeCount()));
			CAutoRef<CBitSet> low_nodes(PbsLowerNodes(lowest_node));
			high_nodes->Difference(low_nodes.Value());
			CAutoRef<CBitSet> previously_forbidden(
				GPOS_NEW(m_mp) CBitSet(m_mp, *forbidden));
			previously_forbidden->Intersection(high_nodes.Value());
			new_neighborhood->Union(previously_forbidden.Value());
			new_neighborhood->Union(neighborhood);

			return EnumerateComplements(lowest_node, grown.Value(),
									new_full.Value(), new_neighborhood.Value());
		}))
	{
		return true;
	}

	return EnumerateSubsets(neighborhood, [&](const CBitSet *grow_by) {
		CAutoRef<CBitSet> grown(GPOS_NEW(m_mp) CBitSet(m_mp, *subgraph));
		grown->Union(grow_by);
		CAutoRef<CBitSet> new_forbidden(
			GPOS_NEW(m_mp) CBitSet(m_mp, *forbidden));
		new_forbidden->Union(neighborhood);
		new_forbidden->Difference(grown.Value());

		CAutoRef<CBitSet> new_full(
			GPOS_NEW(m_mp) CBitSet(m_mp, *full_neighborhood));
		CAutoRef<CBitSet> new_neighborhood(PbsNeighborhood(
			grown.Value(), new_forbidden.Value(), grow_by, &cache,
			new_full.Value()));
		return ExpandSubgraph(lowest_node, grown.Value(), new_full.Value(),
							  new_neighborhood.Value(), new_forbidden.Value());
	});
}

BOOL
CDPHyperEnumerator::Enumerate()
{
	for (ULONG seed_id = m_graph->NodeCount(); 0 < seed_id; --seed_id)
	{
		GPOS_CHECK_ABORT;
		const ULONG node_id = seed_id - 1;
		if (m_receiver->FoundSingleNode(node_id))
		{
			return true;
		}

		CAutoRef<CBitSet> seed(PbsSingleton(node_id));
		CAutoRef<CBitSet> forbidden(PbsLowerNodes(node_id));
		CAutoRef<CBitSet> full_neighborhood(
			GPOS_NEW(m_mp) CBitSet(m_mp));
		CAutoRef<CBitSet> empty(GPOS_NEW(m_mp) CBitSet(m_mp));
		CNeighborhoodCache cache(m_mp, empty.Value());
		CAutoRef<CBitSet> neighborhood(PbsNeighborhood(
			seed.Value(), forbidden.Value(), seed.Value(),
			&cache, full_neighborhood.Value()));
		if (EnumerateComplements(node_id, seed.Value(),
							 full_neighborhood.Value(), neighborhood.Value()))
		{
			return true;
		}
		forbidden->Union(seed.Value());
		if (ExpandSubgraph(node_id, seed.Value(), full_neighborhood.Value(),
						   neighborhood.Value(), forbidden.Value()))
		{
			return true;
		}
	}
	return false;
}
