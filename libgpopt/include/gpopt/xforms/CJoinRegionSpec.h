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

class CJoinRegionSpec
{
public:
	class CEdge
	{
	private:
		COperator::EOperatorId m_join_type;
		CBitSet *m_left;
		CBitSet *m_right;
		CExpression *m_predicate;

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

		CExpression *
		Predicate() const
		{
			return m_predicate;
		}
	};

private:
	CMemoryPool *m_mp;
	CExpressionArray *m_atoms;
	std::vector<CEdge *> m_edges;
	BOOL m_built;
	BOOL m_pure_inner;

	static BOOL FSupportedBinaryJoin(COperator::EOperatorId op_id);
	CBitSet *PbsCollect(CExpression *expr);

public:
	CJoinRegionSpec(const CJoinRegionSpec &) = delete;
	explicit CJoinRegionSpec(CMemoryPool *mp);
	~CJoinRegionSpec();

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
};

}  // namespace gpopt

#endif  // !GPOPT_CJoinRegionSpec_H
