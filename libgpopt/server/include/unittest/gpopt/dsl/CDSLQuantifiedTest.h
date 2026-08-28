//---------------------------------------------------------------------------
// Tests for generic ANY / ALL DSL matching and instantiation.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLQuantifiedTest_H
#define GPOPT_CDSLQuantifiedTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

class CDSLQuantifiedTest
{
public:
	static GPOS_RESULT EresUnittest();
	static GPOS_RESULT EresUnittest_PreUnnestAnyDistinctDrop();
	static GPOS_RESULT EresUnittest_PreUnnestAllDistinctDrop();
	static GPOS_RESULT EresUnittest_PostUnnestAllRestoresPredicate();
	static GPOS_RESULT EresUnittest_PostUnnestCorrelatedPreservesCarrier();
	static GPOS_RESULT EresUnittest_PolarityIsolation();
	static GPOS_RESULT EresUnittest_ConstantOuterDependencies();
};
}  // namespace gpopt

#endif  // !GPOPT_CDSLQuantifiedTest_H
