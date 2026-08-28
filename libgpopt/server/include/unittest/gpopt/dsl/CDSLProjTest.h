//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLProjTest.h
//
//	@doc:
//		Three-stage tests for the Proj operator (docs/DSL_WETUNE_ALIGNMENT.md M1).
//		Migrates the SEMANTICS of WeTune's Proj match/instantiate: bind the
//		projected-column symbols against a CLogicalProject, and instantiate a
//		target Project whose output columns equal the source's (output-column
//		invariant, design §八.3).
//
//		M1 covers plain Proj -> CLogicalProject. Proj* (DISTINCT -> CLogicalGbAgg)
//		and computed/new-column projections are the next milestone.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLProjTest_H
#define GPOPT_CDSLProjTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

class CDSLProjTest
{
public:
	static GPOS_RESULT EresUnittest();

	// Proj<a0 s0>(Input<t0>) matches a live CLogicalProject; <a0> binds the
	// projected columns, <t0> the relational child subtree.
	static GPOS_RESULT EresUnittest_MatchBindsProjectedColumns();

	// identity Proj rule instantiates a CLogicalProject whose output columns
	// equal the source's (output-column invariant).
	static GPOS_RESULT EresUnittest_InstantiatePreservesOutput();

	// Target Proj attrs may select an equivalent join-key column. The scalar
	// project expression must change while its defined output/schema stays fixed.
	static GPOS_RESULT EresUnittest_InstantiateRebindsTargetAttrs();
	static GPOS_RESULT EresUnittest_JoinKeySubsetFollowsAttrsEq();
	static GPOS_RESULT EresUnittest_PreservesHiddenLimitShell();

	// A memo-safe Select(TRUE) produced by dedup removal exposes an identity Proj
	// view over its pure Proj* child, allowing a second DSL rule to consume it.
	static GPOS_RESULT EresUnittest_TrivialSelectContinuesDedupChain();

	// A nested Proj* can consume a Select(TRUE) dedup-removal marker only when
	// the marker child proves uniqueness on its complete output.
	static GPOS_RESULT EresUnittest_DroppedDedupFeedsParentProject();

	// A nested Proj* may consume a complete Global dedup carrying minimal-group
	// provenance, while the same expression remains forbidden for a root-level
	// Proj* elimination rule.
	static GPOS_RESULT EresUnittest_NestedProjStarConsumesGeneratedDedup();

	// A proved nested-Proj* collapse accepts grouping subsets and rejects
	// aggregate-bearing or Local GbAgg implementation stages.
	static GPOS_RESULT EresUnittest_CollapseGbAggRuleBoundaries();

	// A proved nested Project rule composes pass-through aliases only when the
	// inner scalar list is explicitly error-free and deterministic.
	static GPOS_RESULT EresUnittest_CollapseIdentityProject();

	// Compute captures the exact ProjectList independently of Proj's relational
	// projection view, and can reuse it without changing LET semantics.
	static GPOS_RESULT EresUnittest_ComputeExactRoundTrip();
	static GPOS_RESULT EresUnittest_CollapseIndependentCompute();
	static GPOS_RESULT EresUnittest_SplitPartiallyIndependentCompute();

	// a non-trivial Select does NOT fire a Proj-rooted rule.
	static GPOS_RESULT EresUnittest_NoFireOnWrongRoot();
};	// class CDSLProjTest
}  // namespace gpopt

#endif	// !GPOPT_CDSLProjTest_H
