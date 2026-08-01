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

	// a Select-shaped input does NOT fire a Proj-rooted rule (operator-identity
	// gate) — the trigger/no-trigger discriminator.
	static GPOS_RESULT EresUnittest_NoFireOnWrongRoot();
};	// class CDSLProjTest
}  // namespace gpopt

#endif	// !GPOPT_CDSLProjTest_H
