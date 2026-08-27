//---------------------------------------------------------------------------
//	@filename:
//		CDPHyperOrderConstraints.cpp
//---------------------------------------------------------------------------
#include "gpopt/xforms/CDPHyperOrderConstraints.h"

using namespace gpopt;

CDPHyperOrderConstraints::CDPHyperOrderConstraints(CMemoryPool *mp,
											 ULONG edge_count)
	: m_mp(mp)
{
	GPOS_ASSERT(nullptr != mp);
	m_reachable.reserve(edge_count);
	for (ULONG edge = 0; edge < edge_count; ++edge)
	{
		CBitSet *reachable = GPOS_NEW(mp) CBitSet(mp);
		(void) reachable->ExchangeSet(edge);
		m_reachable.push_back(reachable);
	}
}

CDPHyperOrderConstraints::~CDPHyperOrderConstraints()
{
	for (CBitSet *reachable : m_reachable)
	{
		reachable->Release();
	}
}

BOOL
CDPHyperOrderConstraints::WouldCreateCycle(ULONG before, ULONG after) const
{
	GPOS_ASSERT(before < m_reachable.size() && after < m_reachable.size());
	return m_reachable[after]->Get(before);
}

BOOL
CDPHyperOrderConstraints::TryAdd(ULONG before, ULONG after)
{
	if (WouldCreateCycle(before, after))
	{
		return false;
	}
	if (m_reachable[before]->Get(after))
	{
		return true;
	}

	// All predecessors of 'before' can now reach every successor of 'after'.
	// Updating the complete closure makes cycle checks O(1) without imposing a
	// fixed edge-count limit.
	for (CBitSet *reachable : m_reachable)
	{
		if (reachable->Get(before))
		{
			reachable->Union(m_reachable[after]);
		}
	}
	return true;
}
