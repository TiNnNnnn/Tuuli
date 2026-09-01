//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLInstantiateTest.h
//
//	@doc:
//		End-to-end three-stage tests for the target builder (CDSLInstantiator,
//		task #27): match -> (constraints) -> instantiate. Migrates the SEMANTICS
//		of WeTune InstantiationTest (docs/WETUNE_TEST_MIGRATION.md §2 test0/1/4):
//		the rewritten target is built from the source's bindings, residual
//		conjuncts are preserved, and — the subtlest invariant (design §八.3) —
//		the target's output columns equal the source's.
//
//		Focus is on STRUCTURAL rules (Input / Filter), the operator set the
//		matcher + instantiator support. Join instantiation is pending join-key
//		binding; Proj/Agg (new columns) are future work.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLInstantiateTest_H
#define GPOPT_CDSLInstantiateTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

class CDSLInstantiateTest
{
public:
	static GPOS_RESULT EresUnittest();

	// identity-shaped Filter rule over a single-conjunct Select: instantiate
	// yields a Select whose output columns == the source's.
	static GPOS_RESULT EresUnittest_FilterIdentityPreservesOutput();

	// single DSL Filter over a 3-conjunct Select: the instantiated target Select
	// re-conjoins the bound conjunct + 2 residuals (no predicate dropped).
	static GPOS_RESULT EresUnittest_ResidualConjunctsPreserved();

	// A multi-Filter target becomes one Select; repeated bound predicates and
	// residuals occur once in its conjunction.
	static GPOS_RESULT EresUnittest_TargetFilterChainFlattened();

	// A target-only predicate defined by PredicateAnd is constructed for Filter
	// just as it is for Join/Exists, with exact declared dependencies.
	static GPOS_RESULT EresUnittest_DerivedFilterConjunction();

	// A Filter pushed through a nested InnerJoin input is exposed as the
	// equivalent source view; target attrs remap it to the opposite root key.
	static GPOS_RESULT EresUnittest_PushedFilterPredicateRemapped();

	// PredicateDomainSplit atomically derives residual/external predicates and
	// their exact dependency partitions from two current relation domains.
	static GPOS_RESULT EresUnittest_PredicateDomainSplit();

	// A single conjunct spanning both current domains and an external domain is
	// not separable and must make target instantiation fail.
	static GPOS_RESULT EresUnittest_PredicateDomainSplitRejectsMixedAtom();

	// the reused base subtree is grafted (the target's Get child is the same
	// bound subtree the source matched).
	static GPOS_RESULT EresUnittest_BaseSubtreeReused();
};	// class CDSLInstantiateTest
}  // namespace gpopt

#endif	// !GPOPT_CDSLInstantiateTest_H
