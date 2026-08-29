//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLConstraintTest.h
//
//	@doc:
//		Tests for the structural-constraint checker (CDSLConstraintChecker, task
//		#26). Migrates the SEMANTICS of WeTune's constraint checks embedded in
//		InstantiationTest.test0 plus the dedicated constraint/ tests
//		(docs/WETUNE_TEST_MIGRATION.md §6): each constraint gets an ADMIT +
//		REJECT pair.
//
//		Approach: build a Get over a synthetic table (optionally with a unique key
//		/ nullable columns), then MANUALLY bind the rule's table + attrs symbols
//		into a CDSLModel and run CDSLConstraintChecker::FCheck directly — this
//		isolates the check phase from match. WeTune asserts an empty match result
//		on constraint failure; here we assert FCheck returns true (admit) / false
//		(reject).
//
//		Reference(FK): pending — the programmatic fixture carries no FK metadata
//		(IMDRelation::ForeignKeyAt is only populated from a live relcache); see
//		CDSLConstraintChecker.h. Enabled with base B / live PG.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLConstraintTest_H
#define GPOPT_CDSLConstraintTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

class CDSLConstraintTest
{
public:
	static GPOS_RESULT EresUnittest();

	// AttrsSub(a,t): admit when a's columns ⊆ t's output; reject otherwise
	static GPOS_RESULT EresUnittest_AttrsSubAdmit();
	static GPOS_RESULT EresUnittest_AttrsSubReject();
	static GPOS_RESULT EresUnittest_AttrsSubAttrsAdmit();
	static GPOS_RESULT EresUnittest_AttrsSubAttrsReject();

	// Unique(t,a): admit when a is a key of t; reject when t has no such key
	static GPOS_RESULT EresUnittest_UniqueAdmit();
	static GPOS_RESULT EresUnittest_UniqueAdmitOnFixedKey();
	static GPOS_RESULT EresUnittest_UniqueAdmitThroughJoin();
	static GPOS_RESULT EresUnittest_UniqueReject();

	// NotNull(t,a): admit when a's columns are non-nullable; reject when nullable
	static GPOS_RESULT EresUnittest_NotNullAdmit();
	static GPOS_RESULT EresUnittest_NotNullThroughLeftJoin();
	static GPOS_RESULT EresUnittest_NotNullReject();
};	// class CDSLConstraintTest
}  // namespace gpopt

#endif	// !GPOPT_CDSLConstraintTest_H
