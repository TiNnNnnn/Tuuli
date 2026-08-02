//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLJoinMatcher.h
//
//	@doc:
//		Stage ① symbol binding for the join operators (see
//		docs/DSL_WETUNE_ALIGNMENT.md M2).
//
//		DSL   : InnerJoin<a a>(left, right) / LeftJoin<a a>(left, right)
//		        — first <a> = left join columns, second <a> = right join columns.
//		ORCA  : CLogicalInnerJoin / CLogicalLeftOuterJoin(left, right, joinpred)
//		        where child[0]=left rel, child[1]=right rel, child[2]=scalar
//		        conjunctive join predicate.
//
//		Approach (engine-side; ORCA core untouched), mirroring CDSLProjMatcher:
//		  1. Identity/arity gate: live node is the matching join with arity 3.
//		  2. Flatten child[2] into a conjunct set (CPredicateUtils::PdrgpexprConjuncts).
//		     For each PLAIN-EQUALITY conjunct (both sides CScalarIdent), extract the
//		     two CColRefs and split them into left/right key sets by which side's
//		     column belongs to the left subtree's output columns. NON-equi conjuncts
//		     are recorded as residual (CDSLModel::SetResidualConjuncts) so the
//		     instantiator preserves them — dropping a predicate = wrong plan.
//		  3. Bind the first <a> to the left key columns, the second <a> to the right
//		     key columns (CDSLModel::FBind, arrays — same as Filter's <a>).
//		  4. Recurse child[0] and child[1] back through the generic matcher.
//		  5. Record the WHOLE predicate subtree (child[2]) on the model
//		     (CDSLModel::SetJoinPred) so the instantiator can graft the exact
//		     equi + non-equi predicate onto the rebuilt join.
//
//		Like Filter/Proj, join carries scalar structure (the predicate) that only
//		operator-specific code reads, so CDSLMatcher intercepts the join op kinds
//		and routes here rather than going through generic child recursion.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLJoinMatcher_H
#define GPOPT_CDSLJoinMatcher_H

#include "gpos/base.h"

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

// fwd decl — the generic matcher the relational children recurse back into
class CDSLMatcher;

//---------------------------------------------------------------------------
//	@class:
//		CDSLJoinMatcher
//
//	@doc:
//		Matches a DSL InnerJoin/LeftJoin<a a> template against an ORCA
//		CLogicalInnerJoin / CLogicalLeftOuterJoin. Constructed per match attempt
//		with the transient pool and a back-reference to the generic matcher (so the
//		two relational children can recurse). Owns no state beyond those.
//---------------------------------------------------------------------------
class CDSLJoinMatcher
{
private:
	CMemoryPool *m_mp;

	// generic matcher to recurse the relational children into (not owned)
	const CDSLMatcher *m_pmatcher;

	// split the join predicate's conjuncts into left/right equi-key columns and
	// residual (non-equi) conjuncts. pexprLeftRel is the left relational child (its
	// output columns decide which key side each equi column belongs to). Appends
	// keys to pdrgpcrLeft/pdrgpcrRight (caller-owned) and residuals to
	// pdrgpexprResidual (caller-owned; each element AddRef'd). Returns false if a
	// plain-equality conjunct's columns cannot be assigned to exactly one side.
	BOOL FSplitPredicate(CExpression *pexprPred, CExpression *pexprLeftRel,
						 CColRefArray *pdrgpcrLeft, CColRefArray *pdrgpcrRight,
						 CExpressionArray *pdrgpexprResidual) const;

public:
	CDSLJoinMatcher(const CDSLJoinMatcher &) = delete;

	CDSLJoinMatcher(CMemoryPool *mp, const CDSLMatcher *pmatcher)
		: m_mp(mp), m_pmatcher(pmatcher)
	{
		GPOS_ASSERT(nullptr != mp);
		GPOS_ASSERT(nullptr != pmatcher);
	}

	// match a join-rooted DSL template against a live join. Returns true iff the
	// join-key symbols bound consistently and both relational children matched.
	// Populates pmodel with the <a>/<a> key bindings, residual conjuncts, the
	// child bindings, and the recorded join predicate.
	BOOL FMatch(const CDSLOp *popJoin, CExpression *pexprJoin,
				CDSLModel *pmodel) const;
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLJoinMatcher_H
