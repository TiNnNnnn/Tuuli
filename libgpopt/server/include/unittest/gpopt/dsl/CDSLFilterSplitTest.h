//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLFilterSplitTest.h
//
//	@doc:
//		Tests for the Filter-chain <-> conjunctive-Select matcher (CDSLFilterMatcher,
//		task #25) — the hardest part of stage ①. Migrates the SEMANTICS of WeTune
//		FilterChainTest / FilterAssignmentTest / FilterMatchTest
//		(docs/WETUNE_TEST_MIGRATION.md §3): conjunct flatten, 1:1 exact match,
//		N:M subset + reorder, and residual (unconsumed conjunct) preservation.
//
//		WeTune asserts predicate .toString(); here we assert which conjunct each
//		DSL pred symbol bound to + that unmatched conjuncts survive on the model.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLFilterSplitTest_H
#define GPOPT_CDSLFilterSplitTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

class CDSLFilterSplitTest
{
public:
	static GPOS_RESULT EresUnittest();

	// FilterChainTest: 1 DSL Filter over a 3-conjunct Select — flatten to 3,
	// bind p0 to one conjunct, 2 conjuncts survive as residual.
	static GPOS_RESULT EresUnittest_SingleFilterSplitsAndKeepsResidual();

	// FilterAssignmentTest.testSimple: 3 DSL Filters over exactly 3 conjuncts —
	// full 1:1 cover, no residual, every pred symbol distinct-bound.
	static GPOS_RESULT EresUnittest_FullCoverNoResidual();

	// FilterMatchTest.test0: 2 DSL Filters over 3 conjuncts — subset, one
	// conjunct left as residual; attrs symbols bind to the conjunct's columns.
	static GPOS_RESULT EresUnittest_SubsetMatchWithResidual();

	// Ambiguous conjunct order must backtrack until source AttrsEq is satisfied;
	// a failed candidate must not poison the final model bindings.
	static GPOS_RESULT EresUnittest_ConstraintAwareBacktracking();

	// Extended Filter dependencies compare local and outer partitions
	// independently; different correlations do not invalidate equal local deps.
	static GPOS_RESULT EresUnittest_CorrelatedDependencyPartitions();

	// A single explicit correlation-aware Filter binds the whole normalized
	// conjunction, so an absorbing target cannot lose sibling predicates.
	static GPOS_RESULT EresUnittest_CorrelatedFilterBindsWholePredicate();

	// A PredicateDomainSplit source Filter binds every predicate from adjacent
	// physical Select nodes and exposes their common relational base.
	static GPOS_RESULT EresUnittest_CorrelatedFilterCollectsSelectChain();

	// Without an explicit whole-chain constraint, a correlation-aware Filter
	// consumes one Select boundary and leaves the child subtree opaque.
	static GPOS_RESULT EresUnittest_CorrelatedFilterKeepsSelectBoundary();

	// ORCA normalizes Filter(p,Filter(p,x)) to one conjunct; both placeholders
	// may bind that same conjunct and it is consumed only once.
	static GPOS_RESULT EresUnittest_NormalizedDuplicateFilterMatchesOnce();
};	// class CDSLFilterSplitTest
}  // namespace gpopt

#endif	// !GPOPT_CDSLFilterSplitTest_H
