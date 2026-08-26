//---------------------------------------------------------------------------
//	@filename:
//		CJoinRegionSpec.h
//
//	@doc:
//		Transient description of a binary logical-join region.  The descriptor
//		preserves the original join skeleton without introducing an optimizer
//		operator, so both DSL matching and Memo insertion continue to see the
//		real binary tree.
//---------------------------------------------------------------------------
#ifndef GPOPT_CJoinRegionSpec_H
#define GPOPT_CJoinRegionSpec_H

#include <vector>

#include "gpos/common/CBitSet.h"

#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

class CColRefSet;

class CJoinRegionSpec
{
public:
	class CConflictRule
	{
	private:
		CBitSet *m_activate;
		CBitSet *m_required;

	public:
		CConflictRule(const CConflictRule &) = delete;
		CConflictRule(CMemoryPool *mp, const CBitSet *activate,
					  const CBitSet *required);
		~CConflictRule();

		const CBitSet *
		Activate() const
		{
			return m_activate;
		}

		const CBitSet *
		Required() const
		{
			return m_required;
		}
	};

	class CEdge
	{
		friend class CJoinRegionSpec;

	private:
		CMemoryPool *m_mp;
		COperator::EOperatorId m_join_type;
		CBitSet *m_left;
		CBitSet *m_right;
		CBitSet *m_all;
		CBitSet *m_ses;
		CBitSet *m_tes;
		CExpression *m_predicate;
		std::vector<CConflictRule *> m_conflict_rules;

	public:
		CEdge(const CEdge &) = delete;
		CEdge(CMemoryPool *mp, COperator::EOperatorId join_type,
			  const CBitSet *left, const CBitSet *right,
			  CExpression *predicate);
		~CEdge();

		COperator::EOperatorId
		JoinType() const
		{
			return m_join_type;
		}

		const CBitSet *
		Left() const
		{
			return m_left;
		}

		const CBitSet *
		Right() const
		{
			return m_right;
		}

		const CBitSet *
		All() const
		{
			return m_all;
		}

		const CBitSet *
		SES() const
		{
			return m_ses;
		}

		const CBitSet *
		TES() const
		{
			return m_tes;
		}

		const std::vector<CConflictRule *> &
		ConflictRules() const
		{
			return m_conflict_rules;
		}

		CExpression *
		Predicate() const
		{
			return m_predicate;
		}

		// Test whether this original join edge may connect the candidate inputs.
		// Non-inner joins preserve the orientation of their TES partitions.
		BOOL FApplicable(const CBitSet *left, const CBitSet *right) const;
	};

private:
	CMemoryPool *m_mp;
	CExpressionArray *m_atoms;
	std::vector<CEdge *> m_edges;
	std::vector<CBitSet *> m_dependencies;
	BOOL m_built;
	BOOL m_pure_inner;
	BOOL m_cdc_supported;
	BOOL m_external_dependencies;

	static BOOL FSupportedBinaryJoin(COperator::EOperatorId op_id);
	static BOOL FCDCSupportedJoin(COperator::EOperatorId op_id);
	static BOOL FInnerJoin(COperator::EOperatorId op_id);
	CBitSet *PbsCollect(CExpression *expr);
	CBitSet *PbsPredicateCover(CExpression *predicate) const;
	CColRefSet *PcrsNodes(const CBitSet *nodes) const;
	BOOL FNullRejecting(const CEdge *edge, const CBitSet *nodes) const;
	BOOL FAssociative(const CEdge *left, const CEdge *right) const;
	BOOL FLeftAsscom(const CEdge *left, const CEdge *right) const;
	BOOL FRightAsscom(const CEdge *left, const CEdge *right) const;
	CBitSet *PbsIntersectIfNotDegenerate(const CBitSet *used,
									 const CBitSet *available) const;
	void AddConflictRule(CEdge *edge, const CBitSet *activate,
					 const CBitSet *required);
	void AbsorbConflictRules(CEdge *edge);
	void BuildEligibility(CEdge *edge);
	void BuildDependencies();

public:
	CJoinRegionSpec(const CJoinRegionSpec &) = delete;
	explicit CJoinRegionSpec(CMemoryPool *mp);
	~CJoinRegionSpec();

	// Copy an expression tree while marking every maximal binary InnerJoin
	// region, or every CD-C-supported mixed join region when requested. Query
	// preprocessing and DSL alternatives share this path so region ownership
	// does not depend on which phase produced the tree.
	static CExpression *PexprMarkDPHyperRegions(
		CMemoryPool *mp, CExpression *expr, BOOL include_complex = false,
		BOOL parent_is_join = false);

	// Build once.  Unsupported relational subtrees are opaque atoms.  The root
	// itself must be a supported three-child logical join.
	BOOL Build(CExpression *root);

	CExpression *
	Atom(ULONG node_id) const
	{
		GPOS_ASSERT(node_id < m_atoms->Size());
		return (*m_atoms)[node_id];
	}

	const CEdge *
	Edge(ULONG edge_id) const
	{
		GPOS_ASSERT(edge_id < m_edges.size());
		return m_edges[edge_id];
	}

	ULONG
	NodeCount() const
	{
		return m_atoms->Size();
	}

	ULONG
	EdgeCount() const
	{
		return m_edges.size();
	}

	BOOL
	PureInner() const
	{
		return m_pure_inner;
	}

	BOOL
	CDCSupported() const
	{
		return m_cdc_supported;
	}

	const CBitSet *
	Dependencies(ULONG node_id) const
	{
		GPOS_ASSERT(node_id < m_dependencies.size());
		return m_dependencies[node_id];
	}

	BOOL
	HasDependencies() const
	{
		for (const CBitSet *dependencies : m_dependencies)
		{
			if (0 < dependencies->Size())
			{
				return true;
			}
		}
		return false;
	}

	BOOL
	HasExternalDependencies() const
	{
		return m_external_dependencies;
	}
};

}  // namespace gpopt

#endif  // !GPOPT_CJoinRegionSpec_H
