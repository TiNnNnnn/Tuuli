//---------------------------------------------------------------------------
//	MONSOON DSL Union tests
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLUnionTest_H
#define GPOPT_CDSLUnionTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

class CDSLUnionTest
{
public:
	static GPOS_RESULT EresUnittest();
	static GPOS_RESULT EresUnittest_MatchAndDistinctGate();
	static GPOS_RESULT EresUnittest_RejectsNarySetOp();
	static GPOS_RESULT EresUnittest_InstantiatePreservesColumnMaps();
	static GPOS_RESULT EresUnittest_SwapsBranchesByConstraints();
	static GPOS_RESULT EresUnittest_RejectsRemapAcrossOptimizerGbAgg();
	static GPOS_RESULT EresUnittest_CorpusTwoProjects();
	static GPOS_RESULT EresUnittest_CorpusNestedDistinctProjects();
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLUnionTest_H
