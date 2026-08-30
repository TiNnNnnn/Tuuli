//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLJoinTest.h
//
//	@doc:
//		Three-stage tests for the join operators (docs/DSL_WETUNE_ALIGNMENT.md M2).
//		Migrates the SEMANTICS of WeTune's join match/instantiate: bind the
//		equi-join key columns to the two <a> symbols, keep non-equi conjuncts as
//		residual, recurse both children, and instantiate a target join that grafts
//		the exact predicate (equi + non-equi) — output-column invariant preserved.
//
//		M2 covers InnerJoin / LeftJoin / SemiJoin structural rewrites (base A)
//		and the Reference/FK check (which needs live relcache metadata — base C;
//		here we
//		only assert it REJECTS on the FK-less programmatic fixture, so a
//		Reference-guarded rule does not fire).
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLJoinTest_H
#define GPOPT_CDSLJoinTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

class CDSLJoinTest
{
public:
	static GPOS_RESULT EresUnittest();

	// InnerJoin<a0 a1>(Input<t0>,Input<t1>) matches a live CLogicalInnerJoin;
	// <a0> binds the left join column(s), <a1> the right, children bind the two
	// relational subtrees, and the predicate is recorded on the model.
	static GPOS_RESULT EresUnittest_MatchBindsJoinKeys();

	// identity join rule instantiates a CLogicalInnerJoin whose output columns
	// equal the source's, reusing the two child subtrees and the join predicate.
	static GPOS_RESULT EresUnittest_InstantiatePreservesJoin();
	static GPOS_RESULT EresUnittest_ExtendedOutputPreservesCommutedJoin();
	static GPOS_RESULT EresUnittest_NestedJoinPredicatesStayLocal();

	// a join predicate "t0.c0 = t1.c0 AND <non-equi>" binds the equi conjunct as a
	// key and preserves the non-equi conjunct as residual; the instantiated join's
	// predicate is complete.
	static GPOS_RESULT EresUnittest_NonEquiPredicateResidual();
	static GPOS_RESULT EresUnittest_PredicateOnlyJoin();
	static GPOS_RESULT EresUnittest_ExplicitSemiJoinBindsCompletePredicate();
	static GPOS_RESULT EresUnittest_UncorrelatedSemiApplyBuildsSemiJoin();
	static GPOS_RESULT EresUnittest_UncorrelatedAntiApplyBuildsAntiJoin();
	static GPOS_RESULT EresUnittest_SemiJoinBuildsUncorrelatedSemiApply();
	static GPOS_RESULT EresUnittest_PredicateAndBuildsSemiJoinCondition();
	static GPOS_RESULT EresUnittest_PredicateAndBuildsAntiJoinCondition();

	// PredicateFalse gates a direct constant-FALSE LeftJoin and Empty builds a
	// zero-row right input with the original output schema.
	static GPOS_RESULT EresUnittest_FalseLeftJoinBuildsEmptyInput();

	// a join-rooted rule does NOT fire on a Select (operator-identity gate).
	static GPOS_RESULT EresUnittest_NoFireOnWrongRoot();

	// a Reference-guarded rule does NOT fire on the FK-less programmatic fixture
	// (FCheckReference cannot confirm the FK => reject). Live FK verification is
	// exercised at base C (see doc M2 verification §C).
	static GPOS_RESULT EresUnittest_ReferenceRejectsWithoutFK();

	// Reference(R,a,R,a) is a reflexive inclusion dependency when the referred
	// binding is the complete base relation, but not when that side is filtered.
	static GPOS_RESULT EresUnittest_ReferenceAcceptsReflexiveBaseColumn();
	static GPOS_RESULT EresUnittest_ReferenceRejectsFilteredReflexiveTarget();
};	// class CDSLJoinTest
}  // namespace gpopt

#endif	// !GPOPT_CDSLJoinTest_H
