//---------------------------------------------------------------------------
// Cardinality experiment tests.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLStatsExperimentTest_H
#define GPOPT_CDSLStatsExperimentTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

class CDSLStatsExperimentTest
{
public:
	static GPOS_RESULT EresUnittest();
	static GPOS_RESULT EresUnittest_ResolveSPJBoundaries();
	static GPOS_RESULT EresUnittest_StrictInput();
};
}  // namespace gpopt

#endif
