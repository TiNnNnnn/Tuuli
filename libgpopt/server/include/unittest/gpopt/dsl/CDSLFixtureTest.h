//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLFixtureTest.h
//
//	@doc:
//		Smoke tests for CDSLTestFixture: it stands up a live optimizer context,
//		builds logical expressions, and their derived properties + conjunct
//		split behave as expected. Guards the base every phase-2 test builds on.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLFixtureTest_H
#define GPOPT_CDSLFixtureTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

class CDSLFixtureTest
{
public:
	static GPOS_RESULT EresUnittest();

	// fixture stands up opt-ctxt + builds Get; output columns derive
	static GPOS_RESULT EresUnittest_GetDerivesColumns();

	// Select over Get with AND predicate; conjuncts flatten to the right count
	static GPOS_RESULT EresUnittest_SelectConjuncts();

	// InnerJoin builds and derives the union of child output columns
	static GPOS_RESULT EresUnittest_JoinDerivesColumns();
};	// class CDSLFixtureTest
}  // namespace gpopt

#endif	// !GPOPT_CDSLFixtureTest_H
