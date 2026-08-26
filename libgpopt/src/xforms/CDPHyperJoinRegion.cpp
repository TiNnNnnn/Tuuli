//---------------------------------------------------------------------------
//	@filename:
//		CDPHyperJoinRegion.cpp
//---------------------------------------------------------------------------
#include "gpopt/xforms/CDPHyperJoinRegion.h"

#include <algorithm>

#include "gpos/common/CAutoRef.h"
#include "gpos/common/CBitSetIter.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/search/CGroup.h"
#include "gpopt/search/CGroupExpression.h"
#include "gpopt/xforms/CJoinRegionSpec.h"

using namespace gpopt;

namespace
{
BOOL
FAtomMatches(CExpression *left, CExpression *right)
{
	CGroupExpression *left_gexpr = left->Pgexpr();
	CGroupExpression *right_gexpr = right->Pgexpr();
	if (nullptr != left_gexpr || nullptr != right_gexpr)
	{
		return nullptr != left_gexpr && nullptr != right_gexpr &&
			   CGroup::FDuplicateGroups(left_gexpr->Pgroup(),
								   right_gexpr->Pgroup());
	}
	return left->Matches(right);
}

template <class Match>
BOOL
FUnorderedMatches(CExpressionArray *left, CExpressionArray *right,
				  Match match)
{
	if (left->Size() != right->Size())
	{
		return false;
	}
	std::vector<BOOL> used(right->Size(), false);
	for (ULONG left_idx = 0; left_idx < left->Size(); ++left_idx)
	{
		BOOL found = false;
		for (ULONG right_idx = 0; right_idx < right->Size(); ++right_idx)
		{
			if (!used[right_idx] &&
				match((*left)[left_idx], (*right)[right_idx]))
			{
				used[right_idx] = true;
				found = true;
				break;
			}
		}
		if (!found)
		{
			return false;
		}
	}
	return true;
}
}  // namespace

CDPHyperGraphFingerprint::CDPHyperGraphFingerprint(
	CMemoryPool *mp, CExpressionArray *atoms, CExpressionArray *predicates,
	const CDPHyperGraph *graph)
	: m_mp(mp),
	  m_atoms(GPOS_NEW(mp) CExpressionArray(mp)),
	  m_predicates(GPOS_NEW(mp) CExpressionArray(mp)),
	  m_hash(0)
{
	GPOS_ASSERT(nullptr != graph && 0 == graph->DirectedEdgeCount() % 2);
	std::vector<ULONG> atom_hashes;
	std::vector<ULONG> predicate_hashes;
	std::vector<ULONG> edge_hashes;
	for (ULONG atom = 0; atom < atoms->Size(); ++atom)
	{
		CExpression *expr = (*atoms)[atom];
		expr->AddRef();
		m_atoms->Append(expr);
		atom_hashes.push_back(nullptr == expr->Pgexpr()
								? CExpression::UlHashDedup(expr)
								: expr->Pgexpr()->Pgroup()->Id());
	}
	for (ULONG predicate = 0; predicate < predicates->Size(); ++predicate)
	{
		CExpression *expr = (*predicates)[predicate];
		expr->AddRef();
		m_predicates->Append(expr);
		predicate_hashes.push_back(CExpression::UlHashDedup(expr));
	}
	for (ULONG edge = 0; edge < graph->DirectedEdgeCount(); edge += 2)
	{
		const CDPHyperGraph::SEdge *graph_edge = graph->Edge(edge);
		m_edges.emplace_back(GPOS_NEW(mp)
							 CBitSet(mp, *graph_edge->m_left),
						 GPOS_NEW(mp) CBitSet(mp, *graph_edge->m_right));
		const ULONG smaller = std::min(graph_edge->m_left->Size(),
									 graph_edge->m_right->Size());
		const ULONG larger = std::max(graph_edge->m_left->Size(),
								  graph_edge->m_right->Size());
		edge_hashes.push_back(CombineHashes(
			gpos::HashValue<ULONG>(&smaller),
			gpos::HashValue<ULONG>(&larger)));
	}
	std::sort(atom_hashes.begin(), atom_hashes.end());
	std::sort(predicate_hashes.begin(), predicate_hashes.end());
	std::sort(edge_hashes.begin(), edge_hashes.end());
	const ULONG atom_count = atom_hashes.size();
	const ULONG predicate_count = predicate_hashes.size();
	const ULONG edge_count = edge_hashes.size();
	m_hash = gpos::HashValue<ULONG>(&atom_count);
	m_hash = CombineHashes(m_hash,
						   gpos::HashValue<ULONG>(&predicate_count));
	m_hash = CombineHashes(m_hash,
						   gpos::HashValue<ULONG>(&edge_count));
	for (ULONG hash : atom_hashes)
	{
		m_hash = CombineHashes(m_hash, hash);
	}
	for (ULONG hash : predicate_hashes)
	{
		m_hash = CombineHashes(m_hash, hash);
	}
	for (ULONG hash : edge_hashes)
	{
		m_hash = CombineHashes(m_hash, hash);
	}
}

CDPHyperGraphFingerprint::~CDPHyperGraphFingerprint()
{
	m_atoms->Release();
	m_predicates->Release();
	for (const auto &edge : m_edges)
	{
		edge.first->Release();
		edge.second->Release();
	}
}

BOOL
CDPHyperGraphFingerprint::Matches(
	const CDPHyperGraphFingerprint *other) const
{
	if (nullptr == other || m_atoms->Size() != other->m_atoms->Size() ||
		m_edges.size() != other->m_edges.size() ||
		!FUnorderedMatches(
			m_predicates, other->m_predicates,
			[](CExpression *left, CExpression *right) {
				return left->Matches(right);
			}))
	{
		return false;
	}

	std::vector<ULONG> mapping(m_atoms->Size(), gpos::ulong_max);
	std::vector<BOOL> used_atoms(other->m_atoms->Size(), false);
	std::function<BOOL(ULONG)> find_mapping = [&](ULONG atom) {
		if (atom == m_atoms->Size())
		{
			std::vector<BOOL> used_edges(other->m_edges.size(), false);
			for (const auto &edge : m_edges)
			{
				CAutoRef<CBitSet> mapped_left(
					GPOS_NEW(m_mp) CBitSet(m_mp));
				CAutoRef<CBitSet> mapped_right(
					GPOS_NEW(m_mp) CBitSet(m_mp));
				CBitSetIter left_iter(*edge.first);
				while (left_iter.Advance())
				{
					(void) mapped_left->ExchangeSet(mapping[left_iter.Bit()]);
				}
				CBitSetIter right_iter(*edge.second);
				while (right_iter.Advance())
				{
					(void) mapped_right->ExchangeSet(mapping[right_iter.Bit()]);
				}

				BOOL found = false;
				for (ULONG other_edge = 0; other_edge < other->m_edges.size();
					 ++other_edge)
				{
					if (used_edges[other_edge])
					{
						continue;
					}
					const auto &candidate = other->m_edges[other_edge];
					if ((mapped_left->Equals(candidate.first) &&
						 mapped_right->Equals(candidate.second)) ||
						(mapped_left->Equals(candidate.second) &&
						 mapped_right->Equals(candidate.first)))
					{
						used_edges[other_edge] = true;
						found = true;
						break;
					}
				}
				if (!found)
				{
					return false;
				}
			}
			return true;
		}

		for (ULONG candidate = 0; candidate < other->m_atoms->Size();
			 ++candidate)
		{
			if (!used_atoms[candidate] &&
				FAtomMatches((*m_atoms)[atom], (*other->m_atoms)[candidate]))
			{
				mapping[atom] = candidate;
				used_atoms[candidate] = true;
				if (find_mapping(atom + 1))
				{
					return true;
				}
				used_atoms[candidate] = false;
			}
		}
		mapping[atom] = gpos::ulong_max;
		return false;
	};
	return find_mapping(0);
}

CDPHyperJoinRegion::CDPHyperJoinRegion(CMemoryPool *mp,
									 CExpressionArray *components,
									 CExpressionArray *conjuncts,
									 ULONG edge_budget)
	: m_mp(mp),
	  m_components(components),
	  m_conjuncts(conjuncts),
	  m_spec(nullptr),
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
	  m_spec(spec),
	  m_graph(nullptr),
	  m_edge_budget(edge_budget),
	  m_generated_edges(0),
	  m_cartesian_edges(0),
	  m_edge_budget_exhausted(false)
{
	GPOS_ASSERT(nullptr != mp && nullptr != spec);
	GPOS_ASSERT(1 < spec->NodeCount());
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
	if (nullptr != m_spec && !m_spec->PureInner())
	{
		GPOS_ASSERT(m_spec->CDCSupported());
		for (ULONG edge_id = 0; edge_id < m_spec->EdgeCount(); ++edge_id)
		{
			if (m_generated_edges >= m_edge_budget)
			{
				m_edge_budget_exhausted = true;
				return false;
			}
			const CJoinRegionSpec::CEdge *edge = m_spec->Edge(edge_id);
			CAutoRef<CBitSet> left(
				GPOS_NEW(m_mp) CBitSet(m_mp, *edge->TES()));
			CAutoRef<CBitSet> right(
				GPOS_NEW(m_mp) CBitSet(m_mp, *edge->TES()));
			left->Intersection(edge->Left());
			right->Intersection(edge->Right());
			GPOS_ASSERT(0 < left->Size() && 0 < right->Size());
			m_graph->AddEdge(left.Value(), right.Value(), edge_id);
			++m_generated_edges;
		}
		return true;
	}
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

BOOL
CDPHyperJoinRegion::FPairApplicable(const CBitSet *left,
									const CBitSet *right, ULONG edge_id,
									BOOL *swapped) const
{
	if (nullptr != swapped)
	{
		*swapped = false;
	}
	if (nullptr == m_spec || m_spec->PureInner())
	{
		return true;
	}
	GPOS_ASSERT(edge_id < m_spec->EdgeCount());
	const CJoinRegionSpec::CEdge *edge = m_spec->Edge(edge_id);
	if (edge->FApplicable(left, right))
	{
		return true;
	}
	if (edge->FApplicable(right, left))
	{
		if (nullptr != swapped)
		{
			*swapped = true;
		}
		return true;
	}
	return false;
}

std::vector<CDPHyperJoinRegion::SApplicableEdge>
CDPHyperJoinRegion::ApplicableEdges(const CBitSet *left,
									const CBitSet *right) const
{
	std::vector<SApplicableEdge> result;
	if (nullptr == m_spec || m_spec->PureInner())
	{
		return result;
	}
	for (ULONG edge_id = 0; edge_id < m_spec->EdgeCount(); ++edge_id)
	{
		BOOL swapped = false;
		if (FPairApplicable(left, right, edge_id, &swapped))
		{
			result.push_back({edge_id, swapped});
		}
	}
	return result;
}

BOOL
CDPHyperJoinRegion::FBuildJoinRequest(const CBitSet *left,
									  const CBitSet *right,
									  SJoinRequest *request) const
{
	GPOS_ASSERT(nullptr != request);
	request->m_join_type = COperator::EopLogicalInnerJoin;
	request->m_swapped = false;
	request->m_edge_ids.clear();
	if (nullptr == m_spec || m_spec->PureInner())
	{
		return true;
	}

	std::vector<SApplicableEdge> applicable = ApplicableEdges(left, right);
	if (applicable.empty())
	{
		return false;
	}
	ULONG non_inner_count = 0;
	for (const SApplicableEdge &candidate : applicable)
	{
		const COperator::EOperatorId join_type =
			m_spec->Edge(candidate.m_edge_id)->JoinType();
		request->m_edge_ids.push_back(candidate.m_edge_id);
		if (COperator::EopLogicalInnerJoin != join_type)
		{
			++non_inner_count;
			request->m_join_type = join_type;
			request->m_swapped = candidate.m_swapped;
		}
	}
	// Combining an Inner predicate into the ON clause of a non-inner join can
	// change null-extension semantics. Until edge-state sequencing represents
	// both operators explicitly, only a sole non-inner edge is materializable.
	return 0 == non_inner_count ||
		   (1 == non_inner_count && 1 == applicable.size());
}

CExpression *
CDPHyperJoinRegion::PexprPredicate(const SJoinRequest &request) const
{
	GPOS_ASSERT(nullptr != m_spec && !m_spec->PureInner());
	CExpressionArray *predicates = GPOS_NEW(m_mp) CExpressionArray(m_mp);
	for (ULONG edge_id : request.m_edge_ids)
	{
		CExpression *predicate = m_spec->Edge(edge_id)->Predicate();
		predicate->AddRef();
		predicates->Append(predicate);
	}
	return CPredicateUtils::PexprConjunction(m_mp, predicates);
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

CDPHyperGraphFingerprint *
CDPHyperJoinRegion::Pfp() const
{
	GPOS_ASSERT(nullptr != m_graph);
	return GPOS_NEW(m_mp)
		CDPHyperGraphFingerprint(m_mp, m_components, m_conjuncts, m_graph);
}
