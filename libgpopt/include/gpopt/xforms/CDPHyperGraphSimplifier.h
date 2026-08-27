//---------------------------------------------------------------------------
//	@filename:
//		CDPHyperGraphSimplifier.h
//
//	@doc:
//		Budget-driven DPHyper graph simplification: rank neighboring edge-order
//		constraints by cost, apply the most beneficial constraints, probe with
//		the real enumerator, then binary-search the minimum required constraints.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDPHyperGraphSimplifier_H
#define GPOPT_CDPHyperGraphSimplifier_H

#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <stack>
#include <vector>

#include "gpopt/xforms/CDPHyperGraph.h"
#include "gpopt/xforms/CDPHyperOrderConstraints.h"
#include "gpopt/xforms/CDPHyperPlan.h"

namespace gpopt
{
using namespace gpos;

class CDPHyperGraphSimplifier
{
public:
	using EdgeFilter = std::function<BOOL(ULONG)>;
	using JoinCost =
		std::function<BOOL(const CBitSet *, const CBitSet *, DOUBLE *)>;

private:
	struct SEdgeChange
	{
		ULONG m_edge;
		CBitSet *m_old_left;
		CBitSet *m_old_right;
		CBitSet *m_new_left;
		CBitSet *m_new_right;

		SEdgeChange(CMemoryPool *mp, ULONG edge, const CBitSet *old_left,
					const CBitSet *old_right, const CBitSet *new_left,
					const CBitSet *new_right);
		~SEdgeChange();
	};

	struct SSimplificationStep
	{
		DOUBLE m_benefit{0.0};
		ULONG m_before{0};
		ULONG m_after{0};
		std::shared_ptr<SEdgeChange> m_change;
	};

	struct SBestSimplification
	{
		INT m_best_neighbor{-1};
		std::optional<SSimplificationStep> m_step;
		BOOL m_in_queue{false};
	};

	struct SBestCompare
	{
		BOOL operator()(const SBestSimplification *left,
						const SBestSimplification *right) const;
	};

	CMemoryPool *m_mp;
	CDPHyperGraph *m_graph;
	ULONG m_pair_budget;
	CDPHyperPlan::PairFilter m_pair_filter;
	EdgeFilter m_edge_filter;
	JoinCost m_join_cost;
	CDPHyperOrderConstraints m_order;
	std::vector<SBestSimplification *> m_best;
	std::priority_queue<SBestSimplification *,
						std::vector<SBestSimplification *>, SBestCompare>
		m_queue;
	std::stack<SSimplificationStep> m_applied;
	std::stack<SSimplificationStep> m_unapplied;

	static BOOL FSubset(const CBitSet *subset, const CBitSet *superset);
	static CBitSet *PbsUnion(CMemoryPool *mp, const CBitSet *left,
						   const CBitSet *right);
	const CDPHyperGraph::SEdge *Edge(ULONG edge) const;
	CBitSet *PbsUsedNodes(ULONG edge) const;
	BOOL FTryGetSuperset(const CBitSet *left, const CBitSet *right,
						 const CBitSet **superset) const;

	void ExtractJoinDependencies();
	void InitFirstStep();
	void ProcessNeighbors(ULONG edge, ULONG begin, ULONG end);
	std::optional<SSimplificationStep> MakeStep(ULONG edge1, ULONG edge2);
	std::optional<SSimplificationStep> ThreeLeftJoin(
		const CBitSet *set1, ULONG edge1, const CBitSet *set2, ULONG edge2,
		const CBitSet *set3);
	std::optional<SSimplificationStep> ThreeRightJoin(
		const CBitSet *set1, ULONG edge1, const CBitSet *set2, ULONG edge2,
		const CBitSet *set3);
	SSimplificationStep OrderJoin(const SSimplificationStep &edge1_before2,
								  const SSimplificationStep &edge2_before1,
								  ULONG edge1, ULONG edge2);
	std::shared_ptr<SEdgeChange> Pchange(ULONG edge,
										const CBitSet *new_left,
										const CBitSet *new_right);

	BOOL TrySetStep(const SSimplificationStep &step,
					SBestSimplification *best, ULONG neighbor);
	void UpdateQueue(SBestSimplification *best);
	std::optional<SSimplificationStep> FetchStep();
	BOOL ApplyStep();
	BOOL UnapplyStep();
	BOOL ApplySteps(ULONG count);
	void ModifyEdge(const SSimplificationStep &step, BOOL apply);
	BOOL EnumerationExceedsBudget() const;

public:
	CDPHyperGraphSimplifier(const CDPHyperGraphSimplifier &) = delete;
	CDPHyperGraphSimplifier(CMemoryPool *mp, CDPHyperGraph *graph,
							ULONG pair_budget,
							CDPHyperPlan::PairFilter pair_filter,
							EdgeFilter edge_filter, JoinCost join_cost);
	~CDPHyperGraphSimplifier();

	// Returns true only when a simplified graph was found whose complete
	// enumeration stays within the pair budget. On false, all applied graph
	// changes are rolled back.
	BOOL Simplify();
	void Restore();

	ULONG
	AppliedStepCount() const
	{
		return m_applied.size();
	}
};

}  // namespace gpopt

#endif  // !GPOPT_CDPHyperGraphSimplifier_H
