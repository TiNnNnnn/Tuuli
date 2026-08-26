//---------------------------------------------------------------------------
//	@filename:
//		CDPHyperOrderConstraints.h
//
//	@doc:
//		Dynamic transitive-closure tracker for GraphSimplifier edge-order
//		constraints. A relation before -> after means the former join edge must
//		be consumed before the latter. This is the unbounded-bitset equivalent
//		of Horn's CircleDetector.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDPHyperOrderConstraints_H
#define GPOPT_CDPHyperOrderConstraints_H

#include <vector>

#include "gpos/common/CBitSet.h"

namespace gpopt
{
using namespace gpos;

class CDPHyperOrderConstraints
{
private:
	CMemoryPool *m_mp;
	std::vector<CBitSet *> m_reachable;

public:
	CDPHyperOrderConstraints(const CDPHyperOrderConstraints &) = delete;
	explicit CDPHyperOrderConstraints(CMemoryPool *mp, ULONG edge_count);
	~CDPHyperOrderConstraints();

	BOOL WouldCreateCycle(ULONG before, ULONG after) const;
	BOOL TryAdd(ULONG before, ULONG after);

	BOOL
	Precedes(ULONG before, ULONG after) const
	{
		GPOS_ASSERT(before < m_reachable.size() &&
					after < m_reachable.size());
		return before != after && m_reachable[before]->Get(after);
	}
};

}  // namespace gpopt

#endif  // !GPOPT_CDPHyperOrderConstraints_H
