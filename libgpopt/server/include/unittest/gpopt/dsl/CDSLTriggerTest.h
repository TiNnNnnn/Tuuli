//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLTriggerTest.h
//
//	@doc:
//		End-to-end RULE-TRIGGERING tests (task #28). Migrates the SEMANTICS of
//		WeTune's OptimizerTest (docs/WETUNE_TEST_MIGRATION.md §5): given an input
//		shape + a rule, does the rule FIRE, and — for the trigger/no-trigger
//		pair — is it correctly withheld when a precondition fails?
//
//		WeTune's OptimizerTest.doTest runs the full optimizer and asserts the
//		result set CONTAINS an expected SQL (anyMatch). The pgorca analog of a
//		"fire" is exactly the decision the CXformDSLRule_Select shell makes per
//		rule: FMatch && FCheckConstraints && (PexprInstantiate != NULL). These
//		tests replicate that fire predicate over base "A" (programmatic metadata),
//		which is faithful because the shell's engine entry points delegate 1:1 to
//		the matcher / checker / instantiator these tests drive directly. ORCA's
//		memo dispatch of the shell itself is already proven in phase 1
//		(ExfDSLRuleSelect in CLogicalSelect::PxfsCandidates); the heavier
//		minidump base (B) would only re-cover that wiring.
//
//		The trigger/no-trigger pair (KeyPresent vs KeyAbsent) is the direct analog
//		of WeTune OptimizerTest test3/test4: same rule, same input shape, differ
//		ONLY in whether the constraint's metadata precondition holds — proving the
//		check stage actually gates triggering.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLTriggerTest_H
#define GPOPT_CDSLTriggerTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

class CDSLTriggerTest
{
public:
	static GPOS_RESULT EresUnittest();

	// a Filter-rooted rule fires on a matching Select(Get, atom): match hits,
	// constraints hold, instantiate yields a Select — anyMatch analog.
	static GPOS_RESULT EresUnittest_FiresOnMatchingSelect();

	// the same rule does NOT fire against a bare Get (root-shape mismatch — the
	// matcher's identity gate rejects before any binding).
	static GPOS_RESULT EresUnittest_NoFireOnWrongRoot();

	// constraint gates triggering (WeTune test3/test4 analog), FIRE case: a
	// Unique(t0,a0) rule fires when a0's bound column is the table's key.
	static GPOS_RESULT EresUnittest_ConstraintGates_KeyPresent();

	// ... and the NO-FIRE case: the same rule + same input shape is withheld
	// when the table carries no such key (check rejects; instantiate never runs).
	static GPOS_RESULT EresUnittest_ConstraintGates_KeyAbsent();

	// a rule that fires over a multi-conjunct Select produces a structurally
	// valid rewrite: the fired target preserves every conjunct (no dropped
	// predicate) and keeps the source's output columns.
	static GPOS_RESULT EresUnittest_FiredTargetPreservesResiduals();
};	// class CDSLTriggerTest
}  // namespace gpopt

#endif	// !GPOPT_CDSLTriggerTest_H
