//---------------------------------------------------------------------------
//	@filename:
//		CDPHyperGraphTest.h
//---------------------------------------------------------------------------
#ifndef GPOPT_CDPHyperGraphTest_H
#define GPOPT_CDPHyperGraphTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

class CDPHyperGraphTest
{
public:
	static GPOS_RESULT EresUnittest();
	static GPOS_RESULT EresUnittest_Chain();
	static GPOS_RESULT EresUnittest_Star();
	static GPOS_RESULT EresUnittest_Hyperedge();
	static GPOS_RESULT EresUnittest_Disconnected();
	static GPOS_RESULT EresUnittest_DynamicBitset();
	static GPOS_RESULT EresUnittest_ExhaustiveSimpleGraphs();
	static GPOS_RESULT EresUnittest_DifferentialHypergraphs();
	static GPOS_RESULT EresUnittest_AtomicBudget();
	static GPOS_RESULT EresUnittest_JoinRegion();
};
}  // namespace gpopt

#endif  // !GPOPT_CDPHyperGraphTest_H
