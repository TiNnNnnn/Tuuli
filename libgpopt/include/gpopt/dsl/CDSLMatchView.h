//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLMatchView.h
//
//	@doc:
//		Central, read-only adapters between ORCA's normalized expression shapes
//		and the logical shapes exposed by the rule DSL. Views never modify the
//		memo and never depend on a rule id. Operator matchers retain ownership of
//		symbol binding and semantic checks; this class only decodes or constructs
//		transient equivalent representations.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLMatchView_H
#define GPOPT_CDSLMatchView_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"

#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

class COrderSpec;

class CDSLMatchView
{
public:
	// Non-owning aggregate/HAVING projection of either GbAgg or
	// Select(GbAgg, predicate).
	struct SAggregate
	{
		CExpression *m_pexprAgg;
		CExpression *m_pexprHaving;
	};

	// Non-owning projection of ORCA's fused LogicalLimit representation.
	struct SOrderLimit
	{
		CExpression *m_pexprChild;
		CExpression *m_pexprOffset;
		CExpression *m_pexprCount;
		const COrderSpec *m_pos;
		BOOL m_fHasLimit;
	};

	// Owned route produced by pulling one carrier above a safe join spine.
	struct SJoinSpineRoute
	{
		CExpression *m_pexprRel;
		CExpression *m_pexprCarrier;

		SJoinSpineRoute(CExpression *pexprRel, CExpression *pexprCarrier)
			: m_pexprRel(pexprRel), m_pexprCarrier(pexprCarrier)
		{
		}

		~SJoinSpineRoute()
		{
			m_pexprCarrier->Release();
			m_pexprRel->Release();
		}
	};

	using SJoinSpineRouteArray =
		CDynamicPtrArray<SJoinSpineRoute, CleanupDelete>;

private:
	static SJoinSpineRouteArray *PdrgprouteJoinSpine(
		CMemoryPool *mp, CExpression *pexpr,
		COperator::EOperatorId eopidCarrier, ULONG ulDepth);

public:
	CDSLMatchView() = delete;

	// Decode a direct GbAgg, or (when allowed) Select(GbAgg, HAVING).
	static BOOL FAggregate(CExpression *pexpr, BOOL fAllowHaving,
						 SAggregate *pview);

	// Decode one safe global LogicalLimit, including its fused order property.
	static BOOL FOrderLimit(CExpression *pexpr, SOrderLimit *pview);

	// Recognize the memo-safe identity marker Select(pure-global-dedup, TRUE).
	// The returned expression and grouping array are non-owning.
	static BOOL FDedupIdentity(CExpression *pexpr,
						   CExpression **ppexprDedup,
						   CColRefArray **ppdrgpcrGrouping);

	// A removed redundant Proj* is carried through the memo-free RBO as
	// Select(child, TRUE). When child already has a key, deduplicating all of its
	// output columns is an equivalent nested Proj* view. The returned child is
	// non-owning; the matcher remains responsible for binding its output schema.
	static BOOL FDroppedDedupIdentity(CExpression *pexpr,
							 CExpression **ppexprChild);

	// Peel an unmentioned chain of LogicalLimit shells below a Project. Both
	// returned pointers are non-owning; the first shell is NULL when none exists.
	static CExpression *PexprPeelOrderLimit(CExpression *pexpr,
										 CExpression **ppexprFirstShell);

	// Scalar subquery shape predicates shared by Exists and InSub adapters.
	static BOOL FDirectExists(CExpression *pexpr);
	static BOOL FPlainEqAny(CExpression *pexpr);

	// Clone a Select or LeftSemiApplyIn carrier with a replacement outer
	// relation. The caller owns the returned transient expression.
	static CExpression *PexprRebaseInSubCarrier(CMemoryPool *mp,
										  CExpression *pexprCarrier,
										  CExpression *pexprRel);

	// Expose Select(OuterJoin, predicate) as Select(InnerJoin, predicate) only
	// when the predicate rejects every null-supplying side (the right side of a
	// LeftJoin, or both sides of a FullJoin). The caller owns the returned
	// transient expression; NULL means no safe view.
	static CExpression *PexprNullRejectedInnerJoin(CMemoryPool *mp,
										 CExpression *pexprSelect);

	// Expose an n-ary Union/UnionAll as a binary head plus a UnionAll tail.
	// Set-op associativity makes this view exact, and using a bag-union tail is
	// valid for both outer UNION DISTINCT and UNION ALL. The caller owns the
	// transient expression; NULL means the input is not an n-ary set-op.
	static CExpression *PexprBinarySetOp(CMemoryPool *mp,
									 CExpression *pexprSetOp);

	// Return every LeftJoin view of Select(FullJoin, predicate) for which the
	// predicate rejects NULLs from the side made preserved by the view. The
	// returned array owns each transient LeftJoin expression.
	static CExpressionArray *PdrgpexprNullRejectedLeftJoins(
		CMemoryPool *mp, CExpression *pexprSelect);

	// Return every safe view obtained by pulling one carrier from a join spine.
	// Inner paths are transparent; an outer join is crossed only on its preserved
	// side. Every route owns its reconstructed relation and carrier reference.
	static SJoinSpineRouteArray *PdrgprouteJoinSpine(
		CMemoryPool *mp, CExpression *pexpr,
		COperator::EOperatorId eopidCarrier)
	{
		return PdrgprouteJoinSpine(mp, pexpr, eopidCarrier, 0);
	}
};
}  // namespace gpopt

#endif  // !GPOPT_CDSLMatchView_H
