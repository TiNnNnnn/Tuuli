//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLMatchViewTest_H
#define GPOPT_CDSLMatchViewTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

class CDSLMatchViewTest
{
public:
	static GPOS_RESULT EresUnittest();
	static GPOS_RESULT EresUnittest_AggregateAndOrderViews();
	static GPOS_RESULT EresUnittest_JoinSpineAndCarrierViews();
	static GPOS_RESULT EresUnittest_NullRejectedInnerJoinView();
};
}  // namespace gpopt

#endif  // !GPOPT_CDSLMatchViewTest_H
