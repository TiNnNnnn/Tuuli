//---------------------------------------------------------------------------
//	@filename:
//		CDPHyperJoinRegion.h
//
//	@doc:
//		Binary or NAry join region adapted to a DPHyp hypergraph. Predicate and
//		CD-C eligibility are represented independently from the enumerator so
//		graph construction and Memo materialization can be tested separately.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDPHyperJoinRegion_H
#define GPOPT_CDPHyperJoinRegion_H

#include <utility>
#include <vector>

#include "gpopt/operators/CExpression.h"
#include "gpopt/xforms/CDPHyperGraph.h"

namespace gpopt
{
using namespace gpos;

class CJoinRegionSpec;

// Exact graph identity used to suppress repeated DPHyper ownership only
// inside the same Memo equivalence group. The hash is only a diagnostic
// summary; equality compares atoms, predicates and hyperedge topology exactly.
class CDPHyperGraphFingerprint
{
private:
	struct SConflictRule
	{
		CBitSet *m_activate;
		CBitSet *m_required;
	};

	struct SEdge
	{
		COperator::EOperatorId m_join_type;
		CBitSet *m_left;
		CBitSet *m_right;
		std::vector<SConflictRule> m_conflict_rules;
	};

	CMemoryPool *m_mp;
	CExpressionArray *m_atoms;
	CExpressionArray *m_predicates;
	std::vector<SEdge> m_edges;
	std::vector<CBitSet *> m_dependencies;
	ULONG m_hash;

public:
	CDPHyperGraphFingerprint(const CDPHyperGraphFingerprint &) = delete;
	CDPHyperGraphFingerprint(CMemoryPool *mp, CExpressionArray *atoms,
							 CExpressionArray *predicates,
							 const CDPHyperGraph *graph,
							 const CJoinRegionSpec *spec);
	~CDPHyperGraphFingerprint();

	BOOL Matches(const CDPHyperGraphFingerprint *other) const;

	ULONG
	HashValue() const
	{
		return m_hash;
	}
};

class CDPHyperJoinRegion
{
public:
	struct SApplicableEdge
	{
		ULONG m_edge_id;
		BOOL m_swapped;
	};

	struct SJoinRequest
	{
		COperator::EOperatorId m_join_type;
		BOOL m_swapped;
		BOOL m_dependency_directional;
		std::vector<ULONG> m_edge_ids;
	};

private:
	CMemoryPool *m_mp;
	CExpressionArray *m_components;
	CExpressionArray *m_conjuncts;
	std::vector<CBitSet *> m_predicate_covers;
	std::vector<BOOL> m_equality_edges;
	std::vector<std::pair<CBitSet *, CBitSet *>> m_skeleton_edges;
	const CJoinRegionSpec *m_spec;
	CDPHyperGraph *m_graph;
	ULONG m_edge_budget;
	ULONG m_generated_edges;
	ULONG m_cartesian_edges;
	BOOL m_edge_budget_exhausted;

	CBitSet *PbsPredicateCover(CExpression *predicate) const;
	BOOL AddPredicatePartitions(ULONG predicate_id, const CBitSet *cover);
	BOOL AddPredicatePartitionsRecursive(ULONG predicate_id,
								 const std::vector<ULONG> &nodes, ULONG pos,
								 CBitSet *left, CBitSet *right);
	BOOL FPredicateCrosses(const CBitSet *left, const CBitSet *right) const;
	BOOL AddMissingSkeletonEdges();
	BOOL AddCartesianComponentEdges();
	BOOL FDependencyApplicable(const CBitSet *left, const CBitSet *right,
							 BOOL *swapped, BOOL *directional) const;

public:
	CDPHyperJoinRegion(const CDPHyperJoinRegion &) = delete;

	CDPHyperJoinRegion(CMemoryPool *mp, CExpressionArray *components,
					   CExpressionArray *conjuncts, ULONG edge_budget);
	CDPHyperJoinRegion(CMemoryPool *mp, const CJoinRegionSpec *spec,
					   ULONG edge_budget);
	~CDPHyperJoinRegion();

	// Build once. False means the exact generalized-edge expansion exceeded
	// its budget; callers must leave the existing ORCA enumerator enabled.
	BOOL Build();

	CExpression *PexprPredicate(const CBitSet *left, const CBitSet *right,
							 BOOL include_residual) const;
	CExpression *PexprAllPredicates() const;
	CDPHyperGraphFingerprint *Pfp() const;
	BOOL FPairApplicable(const CBitSet *left, const CBitSet *right,
						 ULONG edge_id, BOOL *swapped = nullptr) const;
	std::vector<SApplicableEdge> ApplicableEdges(const CBitSet *left,
											 const CBitSet *right) const;
	BOOL FBuildJoinRequest(const CBitSet *left, const CBitSet *right,
						   SJoinRequest *request) const;
	CExpression *PexprPredicate(const SJoinRequest &request) const;
	BOOL FEqualityEdge(ULONG edge_id) const;
	BOOL FHasEqualityPredicate(const CBitSet *left,
							   const CBitSet *right) const;

	CDPHyperGraph *
	MutableGraph()
	{
		return m_graph;
	}

	const CDPHyperGraph *
	Graph() const
	{
		return m_graph;
	}

	const CBitSet *
	PredicateCover(ULONG predicate_id) const
	{
		GPOS_ASSERT(predicate_id < m_predicate_covers.size());
		return m_predicate_covers[predicate_id];
	}

	CExpression *
	Component(ULONG node_id) const
	{
		return (*m_components)[node_id];
	}

	ULONG
	NodeCount() const
	{
		return m_components->Size();
	}

	ULONG
	PredicateCount() const
	{
		return m_conjuncts->Size();
	}

	ULONG
	GeneratedEdgeCount() const
	{
		return m_generated_edges;
	}

	ULONG
	CartesianEdgeCount() const
	{
		return m_cartesian_edges;
	}

	BOOL
	EdgeBudgetExhausted() const
	{
		return m_edge_budget_exhausted;
	}
};

}  // namespace gpopt

#endif  // !GPOPT_CDPHyperJoinRegion_H
