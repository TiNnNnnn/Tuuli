//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLExistsTest.h
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLExistsTest_H
#define GPOPT_CDSLExistsTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

class CDSLExistsTest
{
public:
	static GPOS_RESULT EresUnittest();
	static GPOS_RESULT EresUnittest_CorpusAggProjRoundTrip();
	static GPOS_RESULT EresUnittest_PreApplyCorpusAggProjRoundTrip();
	static GPOS_RESULT EresUnittest_PreApplyPreservesResidual();
	static GPOS_RESULT EresUnittest_PreApplyNotExistsDistinctDrop();
	static GPOS_RESULT EresUnittest_PostApplyNotExistsDistinctDrop();
	static GPOS_RESULT EresUnittest_ExistsPolarityIsolation();
	static GPOS_RESULT EresUnittest_PredicateSemiJoinRoundTrip();
	static GPOS_RESULT EresUnittest_ExpressionDefinedExistence();
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLExistsTest_H
