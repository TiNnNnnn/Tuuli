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
//		Scope: pure dedup only — a GbAgg carrying aggregate functions is rejected
//		by the Agg matcher (real aggregates are a later milestone).
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

	// the rule must NOT fire when the grouping column is not a key (dedup is not
	// redundant): match succeeds structurally but Unique(t0,a0) gates the check.
	static GPOS_RESULT EresUnittest_RejectsWithoutUnique();

	// a GbAgg that computes an aggregate function (non-empty agg list) is not a
	// pure dedup: the Agg matcher rejects it (out of scope for this milestone).
	static GPOS_RESULT EresUnittest_RejectsNonEmptyAggList();

	// the dedup rule must not match a non-GbAgg root (bare Get / plain Project).
	static GPOS_RESULT EresUnittest_NoFireOnWrongRoot();
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLAggTest_H

// EOF
