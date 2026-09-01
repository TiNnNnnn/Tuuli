//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLAggTest.h
//
//	@doc:
//		Dedup / DISTINCT-elimination three-stage tests (base "A").
//
//		Migrates the SEMANTICS of the WeTune identity "a deduplicated projection
//		equals a plain projection when the deduplicated columns are unique" — i.e.
//		SELECT DISTINCT k FROM t = SELECT k FROM t when k is a key. In ORCA a
//		SELECT DISTINCT is a CLogicalGbAgg with an EMPTY aggregate project list;
//		this rule drops that GbAgg. It is the DSL analogue of ORCA's own
//		CXformSimplifyGbAgg::FDropGbAgg (grouping cols contain a key => drop the
//		GbAgg, wrapping the child in Select(child, TRUE)).
//
//		Rule (Proj* = deduplicated projection, routed to the GbAgg bucket):
//		    Proj*<a0 s0>(Input<t0>)
//		      | Input<t2>
//		      | AttrsSub(a0,t0);Unique(t0,a0);TableEq(t2,t0)
//
//		Also covers the repository rule corpus' five-symbol real Agg matching and
//		target reconstruction. The newer six-symbol/function-qualified spelling is
//		covered as a compatibility extension by parser tests.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLAggTest_H
#define GPOPT_CDSLAggTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CDSLAggTest
//
//	@doc:
//		Unit tests for dedup elimination (GbAgg dedup -> Select over child).
//---------------------------------------------------------------------------
class CDSLAggTest
{
public:
	static GPOS_RESULT EresUnittest();

	// the dedup rule matches a live dedup GbAgg over a Get whose grouping column
	// is the unique key, and flags the model for a dedup drop.
	static GPOS_RESULT EresUnittest_MatchBindsDedupGbAgg();

	// end-to-end fire: match + check + instantiate produce Select(Get) — the GbAgg
	// is gone, the child Get is reused (pointer identity), output cols preserved.
	static GPOS_RESULT EresUnittest_InstantiateProducesSelectOverChild();

	// The corpus spelling keeps an explicit plain Proj target. Since its source
	// Proj* has no ORCA project list, instantiate it as a pass-through dedup drop.
	static GPOS_RESULT EresUnittest_InstantiateDedupToPlainProj();

	// AttrsIntersect derives the ordered outer-only grouping vector needed when
	// a source DISTINCT groups columns from both sides of a join.
	static GPOS_RESULT EresUnittest_InstantiateIntersectedGrouping();

	// the rule must NOT fire when the grouping column is not a key (dedup is not
	// redundant): match succeeds structurally but Unique(t0,a0) gates the check.
	static GPOS_RESULT EresUnittest_RejectsWithoutUnique();

	// a non-DISTINCT aggregate does not represent WeTune's inner Proj* and must
	// therefore still be rejected by the dedup matcher.
	static GPOS_RESULT EresUnittest_RejectsNonEmptyAggList();

	// WeTune's Agg(Proj*) representation matches ORCA's DISTINCT aggregate and
	// instantiation removes only the aggregate DISTINCT flag.
	static GPOS_RESULT EresUnittest_InstantiateDistinctAggregateToPlain();

	// The representation adapter remains gated by the rule's Unique constraint.
	static GPOS_RESULT EresUnittest_DistinctAggregateRejectsWithoutUnique();

	// Bare Agg<a a f s p> binds the five corpus symbols against a real GbAgg.
	static GPOS_RESULT EresUnittest_MatchBindsRealAgg();

	// A corpus-format Agg identity rule reconstructs a valid GbAgg and infers its
	// aggregate output columns from schema - groupByAttrs.
	static GPOS_RESULT EresUnittest_InstantiateRealAgg();

	// OutputAttrs validates a source full-row grouping and lazily derives the
	// corresponding target grouping from a keyed relational child.
	static GPOS_RESULT EresUnittest_InstantiateOutputAttrsGrouping();

	// SchemaFromAttrs supplies the schema half of a target-only Proj* from the
	// same derived keyed-output vector.
	static GPOS_RESULT EresUnittest_InstantiateSchemaFromAttrs();

	// Constraint-local values compose through the shared checker/materializer.
	static GPOS_RESULT EresUnittest_ConstraintLocalValueChain();

	// MinimalGrouping derives the same FD-backed metadata as
	// CXformSimplifyGbAgg without changing the full semantic grouping, and does
	// not rematch an already annotated aggregate.
	static GPOS_RESULT EresUnittest_MinimalGroupingMetadata();

	// Copying a split-like Global aggregate preserves the constructor domain
	// which permits duplicate-generating partial aggregate state.
	static GPOS_RESULT EresUnittest_CopySplitGlobalGbAgg();

	// ORCA Select(GbAgg,HAVING) binds p to the real predicate and reconstructs
	// the same two-node shape end to end.
	static GPOS_RESULT EresUnittest_HavingRoundTrip();

	// A correlated Filter commutes above Agg only when its non-outer dependency
	// vector is non-empty and contained in the existing grouping keys.
	static GPOS_RESULT EresUnittest_AggFilterCommuteGroupingGuard();

	// The proved grouped external-correlation rule composes aggregate and join
	// constraints in one RuleIR and must remain parser/loader compatible.
	static GPOS_RESULT EresUnittest_AggExternalSemiApplyRuleRoundTrip();

	// Equality-only correlation keys not already grouped are pulled into both
	// target grouping and schema; non-equality correlations are rejected.
	static GPOS_RESULT EresUnittest_AggCorrelationComposition();

	// Function-specific templates do not match a different aggregate kind.
	static GPOS_RESULT EresUnittest_RejectsWrongAggFunction();

	// the dedup rule must not match a non-GbAgg root (bare Get / plain Project).
	static GPOS_RESULT EresUnittest_NoFireOnWrongRoot();
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLAggTest_H

// EOF
