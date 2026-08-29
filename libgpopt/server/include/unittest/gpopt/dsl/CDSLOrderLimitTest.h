#ifndef GPOPT_CDSLOrderLimitTest_H
#define GPOPT_CDSLOrderLimitTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

class CDSLOrderLimitTest
{
public:
	static GPOS_RESULT EresUnittest();
	static GPOS_RESULT EresUnittest_FusedLimitSortRoundTrip();
	static GPOS_RESULT EresUnittest_SortOverLimitStaysNested();
	static GPOS_RESULT EresUnittest_PlainLimitRejectsHiddenOrder();
	static GPOS_RESULT EresUnittest_OffsetOnlyLimitRoundTrip();
	static GPOS_RESULT EresUnittest_NonDefaultNullOrderRejects();
	static GPOS_RESULT EresUnittest_TargetScalarConstants();
};
}  // namespace gpopt

#endif  // !GPOPT_CDSLOrderLimitTest_H
