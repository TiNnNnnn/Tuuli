//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLInSubMatcher.h
//--------------------------------------------------------------------------
#ifndef GPOPT_CDSLInSubMatcher_H
#define GPOPT_CDSLInSubMatcher_H

#include "gpos/base.h"

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

class CDSLMatcher;

// Match WeTune's plain InSubFilter<a>(outer,inner) both before ORCA subquery
// unnesting (Select + ScalarSubqueryAny) and after it (LeftSemiApplyIn).
class CDSLInSubMatcher
{
private:
	CMemoryPool *m_mp;
	const CDSLMatcher *m_pmatcher;

	BOOL FBindOuterAttrs(const CDSLOp *pop, CExpression *pexprScalar,
					 CDSLModel *pmodel) const;

	// Match one DSL inner child against a live inner relation. pcrProjected is
	// the subquery output column recorded by either ScalarSubqueryAny or Apply.
	// Handles PostgreSQL's pass-through Proj folding normalization uniformly in
	// both representations.
	BOOL FMatchInner(const CDSLOp *popInner, CExpression *pexprInner,
					 const CColRef *pcrProjected, CDSLModel *pmodel) const;

	// Reconstruct the equality predicate represented by ScalarSubqueryAny.
	// Caller owns the returned expression.
	CExpression *PexprComparison(CExpression *pexprAny) const;

	// ORCA can retain a correlated EXISTS whose inner predicate is a single
	// cross-relation equality, while WeTune canonicalizes the same SQL to
	// InSubFilter. Match that representation without depending on the native
	// subquery-to-Apply xforms. The adapter materializes the equality's implied
	// NOT NULL filters on both inputs, so eliminating the InSub cannot lose SQL's
	// NULL filtering semantics.
	BOOL FMatchCorrelatedExists(const CDSLOp *pop, CExpression *pexpr,
							 CDSLModel *pmodel) const;

	// Inner-join predicate pushdown can turn InSub(Join(left,right),inner)
	// into Join(ApplyIn(left,inner),right). Reconstruct the former only as a
	// transient matcher view; the live memo expression remains unchanged.
	BOOL FMatchPushedDownInnerJoin(const CDSLOp *pop, CExpression *pexpr,
								 CDSLModel *pmodel) const;

public:
	CDSLInSubMatcher(const CDSLInSubMatcher &) = delete;

	CDSLInSubMatcher(CMemoryPool *mp, const CDSLMatcher *pmatcher)
		: m_mp(mp), m_pmatcher(pmatcher)
	{
		GPOS_ASSERT(nullptr != mp);
		GPOS_ASSERT(nullptr != pmatcher);
	}

	BOOL FMatch(const CDSLOp *pop, CExpression *pexpr,
				CDSLModel *pmodel) const;
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLInSubMatcher_H
