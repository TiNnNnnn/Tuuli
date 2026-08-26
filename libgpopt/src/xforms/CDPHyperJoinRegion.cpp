//---------------------------------------------------------------------------
//	@filename:
//		CDPHyperJoinRegion.cpp
//---------------------------------------------------------------------------
#include "gpopt/xforms/CDPHyperJoinRegion.h"

#include "gpos/common/CBitSetIter.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CPredicateUtils.h"

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
	  m_edge_budget_exhausted(false)
{
	GPOS_ASSERT(nullptr != mp && nullptr != components && nullptr != conjuncts);
	GPOS_ASSERT(1 < components->Size());
	GPOS_ASSERT(0 < edge_budget);
	components->AddRef();
	conjuncts->AddRef();
}

CDPHyperJoinRegion::~CDPHyperJoinRegion()
{
	GPOS_DELETE(m_graph);
	for (CBitSet *cover : m_predicate_covers)
	{
		cover->Release();
	}
	m_components->Release();
	m_conjuncts->Release();
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
	return true;
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
