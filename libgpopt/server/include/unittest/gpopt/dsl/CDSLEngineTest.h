//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLEngineTest.h
//
//	@doc:
//		Phase-1 unit tests for the DSL rule engine wiring: the CXformDSLRule_*
//		shells are registered in the xform factory as exploration xforms, and
//		CDSLRuleEngine loads + buckets a rule library by source-root operator.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLEngineTest_H
#define GPOPT_CDSLEngineTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CDSLEngineTest
//
//	@doc:
//		Unittests for the phase-1 engine skeleton + shell registration.
//---------------------------------------------------------------------------
class CDSLEngineTest
{
public:
	static GPOS_RESULT EresUnittest();

	// the DSL shell(s) are registered and classified as exploration xforms
	static GPOS_RESULT EresUnittest_ShellRegistered();

	// CLogicalSelect advertises the shell in its PxfsCandidates set — the exact
	// set the scheduler intersects to route expressions to xforms
	static GPOS_RESULT EresUnittest_SelectDispatches();

	// Union and Union* shells are registered and advertised by their logical
	// operators, completing scheduler dispatch for the generic set-op framework
	static GPOS_RESULT EresUnittest_UnionShellsDispatch();

	// Sort/Limit share the CLogicalLimit shell and scheduler candidate.
	static GPOS_RESULT EresUnittest_LimitShellDispatch();

	// engine buckets a loaded library by source-root EOperatorId
	static GPOS_RESULT EresUnittest_Bucketing();

	// subquery phase routing is an operator capability, independent of rule shape
	static GPOS_RESULT EresUnittest_SubqueryRepresentationCapability();

	// corpus-audit capability metadata mirrors the implemented engine boundary
	static GPOS_RESULT EresUnittest_CapabilityMetadata();

	// three-stage entry points are callable (phase-1 stubs: no rewrite)
	static GPOS_RESULT EresUnittest_StubsCallable();
};	// class CDSLEngineTest
}  // namespace gpopt

#endif	// !GPOPT_CDSLEngineTest_H
