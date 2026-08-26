//---------------------------------------------------------------------------
//	@filename:
//		CDPHyperJoinRegion.cpp
//---------------------------------------------------------------------------
#include "gpopt/xforms/CDPHyperJoinRegion.h"

#include "gpos/common/CAutoRef.h"
#include "gpos/common/CBitSetIter.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/xforms/CJoinRegionSpec.h"

using namespace gpopt;

CDPHyperJoinRegion::CDPHyperJoinRegion(CMemoryPool *mp,
									 CExpressionArray *components,
									 CExpressionArray *conjuncts,
									 ULONG edge_budget)
	: m_mp(mp),
	  m_components(components),
	  m_conjuncts(conjuncts),
	  m_graph(nullptr),
	  m_edge_budget(edge_budget),
	  m_generated_edges(0),
	  m_cartesian_edges(0),
	  m_edge_budget_exhausted(false)
{
	GPOS_ASSERT(nullptr != mp && nullptr != components && nullptr != conjuncts);
	GPOS_ASSERT(1 < components->Size());
	GPOS_ASSERT(0 < edge_budget);
	components->AddRef();
	conjuncts->AddRef();
}

CDPHyperJoinRegion::CDPHyperJoinRegion(CMemoryPool *mp,
									   const CJoinRegionSpec *spec,
									   ULONG edge_budget)
	: m_mp(mp),
	  m_components(GPOS_NEW(mp) CExpressionArray(mp)),
	  m_conjuncts(GPOS_NEW(mp) CExpressionArray(mp)),
	  m_graph(nullptr),
	  m_edge_budget(edge_budget),
	  m_generated_edges(0),
	  m_cartesian_edges(0),
	  m_edge_budget_exhausted(false)
{
	GPOS_ASSERT(nullptr != mp && nullptr != spec);
	GPOS_ASSERT(spec->PureInner() && 1 < spec->NodeCount());
	GPOS_ASSERT(0 < edge_budget);
	for (ULONG node = 0; node < spec->NodeCount(); ++node)
	{
		spec->Atom(node)->AddRef();
		m_components->Append(spec->Atom(node));
	}
	for (ULONG edge = 0; edge < spec->EdgeCount(); ++edge)
	{
		m_skeleton_edges.emplace_back(
			GPOS_NEW(mp) CBitSet(mp, *spec->Edge(edge)->Left()),
			GPOS_NEW(mp) CBitSet(mp, *spec->Edge(edge)->Right()));
		CExpressionArray *edge_conjuncts = CPredicateUtils::PdrgpexprConjuncts(
			mp, spec->Edge(edge)->Predicate());
		for (ULONG conjunct = 0; conjunct < edge_conjuncts->Size(); ++conjunct)
		{
			(*edge_conjuncts)[conjunct]->AddRef();
			m_conjuncts->Append((*edge_conjuncts)[conjunct]);
		}
		edge_conjuncts->Release();
	}
}

CDPHyperJoinRegion::~CDPHyperJoinRegion()
{
	GPOS_DELETE(m_graph);
	for (CBitSet *cover : m_predicate_covers)
	{
		cover->Release();
	}
	for (const auto &edge : m_skeleton_edges)
	{
		edge.first->Release();
		edge.second->Release();
	}
	m_components->Release();
	m_conjuncts->Release();
}

BOOL
CDPHyperJoinRegion::FPredicateCrosses(const CBitSet *left,
									  const CBitSet *right) const
{
	CAutoRef<CBitSet> joined(GPOS_NEW(m_mp) CBitSet(m_mp, *left));
	joined->Union(right);
	for (const CBitSet *cover : m_predicate_covers)
	{
		if (1 < cover->Size() && joined->ContainsAll(cover) &&
			!left->IsDisjoint(cover) && !right->IsDisjoint(cover))
		{
			return true;
		}
	}
	return false;
}

BOOL
CDPHyperJoinRegion::AddMissingSkeletonEdges()
{
	for (const auto &edge : m_skeleton_edges)
	{
		if (FPredicateCrosses(edge.first, edge.second))
		{
			continue;
		}
		if (m_generated_edges >= m_edge_budget)
		{
			m_edge_budget_exhausted = true;
			return false;
		}
		m_graph->AddEdge(edge.first, edge.second,
						 m_conjuncts->Size() + m_cartesian_edges);
		++m_generated_edges;
		++m_cartesian_edges;
	}
	return true;
}

CBitSet *
CDPHyperJoinRegion::PbsPredicateCover(CExpression *predicate) const
{
	CBitSet *cover = GPOS_NEW(m_mp) CBitSet(m_mp);
	CColRefSet *used = predicate->DeriveUsedColumns();
	for (ULONG node = 0; node < m_components->Size(); ++node)
	{
		if (!used->IsDisjoint((*m_components)[node]->DeriveOutputColumns()))
		{
			(void) cover->ExchangeSet(node);
		}
	}
	return cover;
}

BOOL
CDPHyperJoinRegion::AddPredicatePartitionsRecursive(
	ULONG predicate_id, const std::vector<ULONG> &nodes, ULONG pos,
	CBitSet *left, CBitSet *right)
{
	if (pos == nodes.size())
	{
		if (0 == right->Size())
		{
			return true;
		}
		if (m_generated_edges >= m_edge_budget)
		{
			m_edge_budget_exhausted = true;
			return false;
		}
		m_graph->AddEdge(left, right, predicate_id);
		++m_generated_edges;
		return true;
	}

	const ULONG node = nodes[pos];
	(void) left->ExchangeSet(node);
	if (!AddPredicatePartitionsRecursive(predicate_id, nodes, pos + 1, left,
										 right))
	{
		return false;
	}
	(void) left->ExchangeClear(node);

	(void) right->ExchangeSet(node);
	const BOOL success = AddPredicatePartitionsRecursive(
		predicate_id, nodes, pos + 1, left, right);
	(void) right->ExchangeClear(node);
	return success;
}

BOOL
CDPHyperJoinRegion::AddPredicatePartitions(ULONG predicate_id,
										const CBitSet *cover)
{
	GPOS_ASSERT(1 < cover->Size());
	std::vector<ULONG> nodes;
	nodes.reserve(cover->Size());
	CBitSetIter iter(*cover);
	while (iter.Advance())
	{
		nodes.push_back(iter.Bit());
	}

	// Pin the lowest node to the left endpoint. This emits each unordered
	// bipartition once while retaining every exact eligibility split.
	CBitSet *left = GPOS_NEW(m_mp) CBitSet(m_mp);
	CBitSet *right = GPOS_NEW(m_mp) CBitSet(m_mp);
	(void) left->ExchangeSet(nodes[0]);
	const BOOL success = AddPredicatePartitionsRecursive(
		predicate_id, nodes, 1, left, right);
	left->Release();
	right->Release();
	return success;
}

BOOL
CDPHyperJoinRegion::AddCartesianComponentEdges()
{
	const ULONG node_count = m_components->Size();
	std::vector<ULONG> parent(node_count);
	for (ULONG node = 0; node < node_count; ++node)
	{
		parent[node] = node;
	}
	auto find_root = [&parent](ULONG node) {
		while (parent[node] != node)
		{
			parent[node] = parent[parent[node]];
			node = parent[node];
		}
		return node;
	};
	for (const CBitSet *cover : m_predicate_covers)
	{
		if (cover->Size() < 2)
		{
			continue;
		}
		CBitSetIter iter(*cover);
		GPOS_ASSERT(iter.Advance());
		const ULONG first = iter.Bit();
		while (iter.Advance())
		{
			const ULONG first_root = find_root(first);
			const ULONG other_root = find_root(iter.Bit());
			if (first_root != other_root)
			{
				parent[other_root] = first_root;
			}
		}
	}
	// A binary-tree descriptor contributes exact, already-legal skeleton cuts.
	// Include them in connectivity even when a predicate edge made the explicit
	// skeleton edge redundant. This prevents inferred Cartesian edges from
	// replacing the original CROSS JOIN boundary.
	for (const auto &edge : m_skeleton_edges)
	{
		CAutoRef<CBitSet> joined(GPOS_NEW(m_mp) CBitSet(m_mp, *edge.first));
		joined->Union(edge.second);
		CBitSetIter iter(*joined.Value());
		GPOS_ASSERT(iter.Advance());
		const ULONG first = iter.Bit();
		while (iter.Advance())
		{
			const ULONG first_root = find_root(first);
			const ULONG other_root = find_root(iter.Bit());
			if (first_root != other_root)
			{
				parent[other_root] = first_root;
			}
		}
	}

	std::vector<CBitSet *> by_root(node_count, nullptr);
	std::vector<CBitSet *> components;
	for (ULONG node = 0; node < node_count; ++node)
	{
		const ULONG root = find_root(node);
		if (nullptr == by_root[root])
		{
			CBitSet *component = GPOS_NEW(m_mp) CBitSet(m_mp);
			by_root[root] = component;
			components.push_back(component);
		}
		(void) by_root[root]->ExchangeSet(node);
	}

	BOOL success = true;
	for (ULONG left = 0; success && left < components.size(); ++left)
	{
		for (ULONG right = left + 1; right < components.size(); ++right)
		{
			if (m_generated_edges >= m_edge_budget)
			{
				m_edge_budget_exhausted = true;
				success = false;
				break;
			}
			// Freeze Cartesian products at predicate-connected component
			// boundaries. This permits every component order without introducing
			// cross products into the interior of a connected component.
			m_graph->AddEdge(components[left], components[right],
							 m_conjuncts->Size() + m_cartesian_edges);
			++m_generated_edges;
			++m_cartesian_edges;
		}
	}
	for (CBitSet *component : components)
	{
		component->Release();
	}
	return success;
}

BOOL
CDPHyperJoinRegion::Build()
{
	GPOS_ASSERT(nullptr == m_graph);
	m_graph = GPOS_NEW(m_mp) CDPHyperGraph(m_mp, m_components->Size());
	m_predicate_covers.reserve(m_conjuncts->Size());
	for (ULONG predicate = 0; predicate < m_conjuncts->Size(); ++predicate)
	{
		CBitSet *cover = PbsPredicateCover((*m_conjuncts)[predicate]);
		m_predicate_covers.push_back(cover);
		if (1 < cover->Size() && !AddPredicatePartitions(predicate, cover))
		{
			return false;
		}
	}
	return AddMissingSkeletonEdges() && AddCartesianComponentEdges();
}

CExpression *
CDPHyperJoinRegion::PexprPredicate(const CBitSet *left,
								   const CBitSet *right,
								   BOOL include_residual) const
{
	GPOS_ASSERT(nullptr != left && nullptr != right && left->IsDisjoint(right));
	CBitSet *joined = GPOS_NEW(m_mp) CBitSet(m_mp, *left);
	joined->Union(right);
	CExpressionArray *predicates = GPOS_NEW(m_mp) CExpressionArray(m_mp);
	for (ULONG predicate = 0; predicate < m_conjuncts->Size(); ++predicate)
	{
		const CBitSet *cover = m_predicate_covers[predicate];
		const BOOL crossing = joined->ContainsAll(cover) &&
							  !left->IsDisjoint(cover) &&
							  !right->IsDisjoint(cover);
		const BOOL residual = include_residual && cover->Size() < 2;
		if (crossing || residual)
		{
			(*m_conjuncts)[predicate]->AddRef();
			predicates->Append((*m_conjuncts)[predicate]);
		}
	}
	joined->Release();
	return CPredicateUtils::PexprConjunction(m_mp, predicates);
}

CExpression *
CDPHyperJoinRegion::PexprAllPredicates() const
{
	CExpressionArray *predicates = GPOS_NEW(m_mp) CExpressionArray(m_mp);
	for (ULONG predicate = 0; predicate < m_conjuncts->Size(); ++predicate)
	{
		(*m_conjuncts)[predicate]->AddRef();
		predicates->Append((*m_conjuncts)[predicate]);
	}
	return CPredicateUtils::PexprConjunction(m_mp, predicates);
}
