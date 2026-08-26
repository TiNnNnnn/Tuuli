//---------------------------------------------------------------------------
//	@filename:
//		CDPHyperPlan.h
//
//	@doc:
//		Atomic, budget-aware result of a DPHyp enumeration. No Memo state is
//		changed while this object is populated; callers materialize the recorded
//		pairs only after Complete() succeeds.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDPHyperPlan_H
#define GPOPT_CDPHyperPlan_H

#include <unordered_map>
#include <vector>

#include "gpopt/xforms/CDPHyperGraph.h"

namespace gpopt
{
using namespace gpos;

class CDPHyperPlan : public IDPHyperReceiver
{
public:
	struct SPair
	{
		CBitSet *m_left;
		CBitSet *m_right;
		ULONG m_connecting_edge;

		SPair(CMemoryPool *mp, const CBitSet *left, const CBitSet *right,
			  ULONG connecting_edge);
		~SPair();
	};

private:
	CMemoryPool *m_mp;
	ULONG m_pair_budget;
	BOOL m_budget_exhausted;
	std::unordered_map<ULONG, std::vector<CBitSet *>> m_seen;
	std::unordered_map<ULONG, std::vector<SPair *>> m_pairs_by_union;
	std::vector<SPair *> m_pairs;
	ULONG m_seen_count;

	BOOL RecordSeen(const CBitSet *nodes);
	BOOL HasPair(const CBitSet *left, const CBitSet *right,
				 ULONG union_hash) const;

public:
	CDPHyperPlan(const CDPHyperPlan &) = delete;

	CDPHyperPlan(CMemoryPool *mp, ULONG pair_budget);
	~CDPHyperPlan() override;

	BOOL FoundSingleNode(ULONG node_id) override;
	BOOL HasSeen(const CBitSet *nodes) const override;
	BOOL FoundSubgraphPair(const CBitSet *left, const CBitSet *right,
						   ULONG edge_id) override;

	BOOL Complete(ULONG node_count) const;

	BOOL
	BudgetExhausted() const
	{
		return m_budget_exhausted;
	}

	ULONG
	PairCount() const
	{
		return m_pairs.size();
	}

	ULONG
	SeenCount() const
	{
		return m_seen_count;
	}

	const std::vector<SPair *> &
	Pairs() const
	{
		return m_pairs;
	}
};

}  // namespace gpopt

#endif  // !GPOPT_CDPHyperPlan_H
