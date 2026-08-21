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
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLExistsTest_H

