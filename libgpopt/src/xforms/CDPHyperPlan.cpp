//---------------------------------------------------------------------------
//	@filename:
//		CDPHyperPlan.cpp
//---------------------------------------------------------------------------
#include "gpopt/xforms/CDPHyperPlan.h"

#include "gpos/common/CAutoRef.h"

using namespace gpopt;

CDPHyperPlan::SPair::SPair(CMemoryPool *mp, const CBitSet *left,
							 const CBitSet *right, ULONG connecting_edge)
	: m_left(GPOS_NEW(mp) CBitSet(mp, *left)),
	  m_right(GPOS_NEW(mp) CBitSet(mp, *right)),
	  m_connecting_edge(connecting_edge)
{
}

CDPHyperPlan::SPair::~SPair()
{
	m_left->Release();
	m_right->Release();
}

CDPHyperPlan::CDPHyperPlan(CMemoryPool *mp, ULONG pair_budget)
	: m_mp(mp),
	  m_pair_budget(pair_budget),
	  m_budget_exhausted(false),
	  m_seen_count(0)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(0 < pair_budget);
}

CDPHyperPlan::~CDPHyperPlan()
{
	for (auto &bucket : m_seen)
	{
		for (CBitSet *nodes : bucket.second)
		{
			nodes->Release();
		}
	}
	for (SPair *pair : m_pairs)
	{
		GPOS_DELETE(pair);
	}
}

BOOL
CDPHyperPlan::HasSeen(const CBitSet *nodes) const
{
	auto bucket = m_seen.find(nodes->HashValue());
	if (m_seen.end() == bucket)
	{
		return false;
	}
	for (const CBitSet *seen : bucket->second)
	{
		if (seen->Equals(nodes))
		{
			return true;
		}
	}
	return false;
}

BOOL
CDPHyperPlan::RecordSeen(const CBitSet *nodes)
{
	if (HasSeen(nodes))
	{
		return false;
	}
	m_seen[nodes->HashValue()].push_back(
		GPOS_NEW(m_mp) CBitSet(m_mp, *nodes));
	++m_seen_count;
	return true;
}

BOOL
CDPHyperPlan::FoundSingleNode(ULONG node_id)
{
	CAutoRef<CBitSet> node(GPOS_NEW(m_mp) CBitSet(m_mp));
	(void) node->ExchangeSet(node_id);
	(void) RecordSeen(node.Value());
	return false;
}

BOOL
CDPHyperPlan::HasPair(const CBitSet *left, const CBitSet *right,
					  ULONG union_hash) const
{
	auto bucket = m_pairs_by_union.find(union_hash);
	if (m_pairs_by_union.end() == bucket)
	{
		return false;
	}
	for (const SPair *pair : bucket->second)
	{
		if ((pair->m_left->Equals(left) && pair->m_right->Equals(right)) ||
			(pair->m_left->Equals(right) && pair->m_right->Equals(left)))
		{
			return true;
		}
	}
	return false;
}

BOOL
CDPHyperPlan::FoundSubgraphPair(const CBitSet *left, const CBitSet *right,
							 ULONG edge_id)
{
	GPOS_ASSERT(left->IsDisjoint(right));
	GPOS_ASSERT(HasSeen(left));
	GPOS_ASSERT(HasSeen(right));

	CAutoRef<CBitSet> joined(GPOS_NEW(m_mp) CBitSet(m_mp, *left));
	joined->Union(right);
	const ULONG union_hash = joined->HashValue();
	if (HasPair(left, right, union_hash))
	{
		return false;
	}
	if (m_pairs.size() >= m_pair_budget)
	{
		m_budget_exhausted = true;
		return true;
	}

	SPair *pair = GPOS_NEW(m_mp) SPair(m_mp, left, right, edge_id);
	m_pairs.push_back(pair);
	m_pairs_by_union[union_hash].push_back(pair);
	(void) RecordSeen(joined.Value());
	return false;
}

BOOL
CDPHyperPlan::Complete(ULONG node_count) const
{
	if (m_budget_exhausted || 0 == node_count)
	{
		return false;
	}
	CAutoRef<CBitSet> all(GPOS_NEW(m_mp) CBitSet(m_mp));
	for (ULONG node = 0; node < node_count; ++node)
	{
		(void) all->ExchangeSet(node);
	}
	return HasSeen(all.Value());
}
