//---------------------------------------------------------------------------
//	@filename:
//		CDPHyperGraph.h
//
//	@doc:
//		Dynamic-bitset hypergraph and DPHyp connected-subgraph enumerator.
//		The enumerator is independent of Memo and costing; a receiver owns the
//		policy for recording subsets and materializing join alternatives.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDPHyperGraph_H
#define GPOPT_CDPHyperGraph_H

#include <functional>
#include <vector>

#include "gpos/base.h"
#include "gpos/common/CBitSet.h"

namespace gpopt
{
using namespace gpos;

// Receiver contract used by the DPHyp CSG-CMP enumeration. Returning true
// aborts enumeration, which is used for cancellation and pair budgets.
class IDPHyperReceiver
{
public:
	virtual ~IDPHyperReceiver() = default;

	virtual BOOL FoundSingleNode(ULONG node_id) = 0;
	virtual BOOL HasSeen(const CBitSet *nodes) const = 0;
	virtual BOOL FoundSubgraphPair(const CBitSet *left, const CBitSet *right,
									 ULONG edge_id) = 0;
};

class CDPHyperGraph
{
public:
	struct SEdge
	{
		CBitSet *m_left;
		CBitSet *m_right;
		ULONG m_edge_id;

		SEdge(CMemoryPool *mp, const CBitSet *left, const CBitSet *right,
			  ULONG edge_id);
		~SEdge();
	};

	struct SNode
	{
		CBitSet *m_simple_neighborhood;
		std::vector<ULONG> m_simple_edges;
		std::vector<ULONG> m_complex_edges;

		explicit SNode(CMemoryPool *mp);
		~SNode();
	};

private:
	CMemoryPool *m_mp;
	std::vector<SNode *> m_nodes;
	std::vector<SEdge *> m_edges;

	void AttachEdgeToNodes(ULONG forward_edge, ULONG reverse_edge,
						   const CBitSet *left, const CBitSet *right);

public:
	CDPHyperGraph(const CDPHyperGraph &) = delete;

	CDPHyperGraph(CMemoryPool *mp, ULONG node_count);
	~CDPHyperGraph();

	ULONG
	NodeCount() const
	{
		return m_nodes.size();
	}

	ULONG
	DirectedEdgeCount() const
	{
		return m_edges.size();
	}

	const SNode *
	Node(ULONG node_id) const
	{
		GPOS_ASSERT(node_id < m_nodes.size());
		return m_nodes[node_id];
	}

	const SEdge *
	Edge(ULONG edge_id) const
	{
		GPOS_ASSERT(edge_id < m_edges.size());
		return m_edges[edge_id];
	}

	// Add an undirected hyperedge. Internally both directions are retained so
	// every edge attached to a node has that node on its left side.
	void AddEdge(const CBitSet *left, const CBitSet *right, ULONG edge_id);
};

class CDPHyperEnumerator
{
private:
	using SubsetCallback = std::function<BOOL(const CBitSet *)>;

	class CNeighborhoodCache
	{
	private:
		CMemoryPool *m_mp;
		CBitSet *m_taboo;
		CBitSet *m_last_grown;
		CBitSet *m_last_neighborhood;
		CBitSet *m_last_full_neighborhood;
		BOOL m_initialized;

	public:
		CNeighborhoodCache(CMemoryPool *mp, const CBitSet *neighborhood);
		~CNeighborhoodCache();

		CBitSet *PbsToSearch(const CBitSet *just_grown,
						 CBitSet *neighborhood,
						 CBitSet *full_neighborhood);
		void Store(const CBitSet *just_grown, const CBitSet *neighborhood,
				   const CBitSet *full_neighborhood);
	};

	CMemoryPool *m_mp;
	const CDPHyperGraph *m_graph;
	IDPHyperReceiver *m_receiver;

	CBitSet *PbsSingleton(ULONG node_id) const;
	CBitSet *PbsLowerNodes(ULONG upper_bound) const;
	CBitSet *PbsLowestBit(const CBitSet *set) const;
	ULONG LowestBit(const CBitSet *set) const;

	BOOL EnumerateSubsets(const CBitSet *set, const SubsetCallback &callback);
	BOOL EnumerateSubsetsRecursive(const std::vector<ULONG> &bits, ULONG pos,
								 CBitSet *current,
								 const SubsetCallback &callback);

	CBitSet *PbsNeighborhood(const CBitSet *subgraph,
							 const CBitSet *forbidden,
							 const CBitSet *just_grown,
							 CNeighborhoodCache *cache,
							 CBitSet *full_neighborhood) const;

	BOOL EnumerateComplements(ULONG lowest_node, const CBitSet *subgraph,
							  const CBitSet *full_neighborhood,
							  const CBitSet *neighborhood);
	BOOL ExpandSubgraph(ULONG lowest_node, const CBitSet *subgraph,
						 const CBitSet *full_neighborhood,
						 const CBitSet *neighborhood,
						 const CBitSet *forbidden);
	BOOL ExpandComplement(ULONG lowest_node, const CBitSet *subgraph,
						   const CBitSet *subgraph_full_neighborhood,
						   const CBitSet *complement,
						   const CBitSet *neighborhood,
						   const CBitSet *forbidden);
	BOOL TryConnecting(const CBitSet *subgraph,
					   const CBitSet *subgraph_full_neighborhood,
					   const CBitSet *complement);

public:
	CDPHyperEnumerator(const CDPHyperEnumerator &) = delete;

	CDPHyperEnumerator(CMemoryPool *mp, const CDPHyperGraph *graph,
					 IDPHyperReceiver *receiver);

	// Returns true only when the receiver requested an early abort.
	BOOL Enumerate();
};

}  // namespace gpopt

#endif  // !GPOPT_CDPHyperGraph_H
