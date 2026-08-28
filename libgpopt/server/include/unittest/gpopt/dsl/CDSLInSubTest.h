//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//--------------------------------------------------------------------------
#ifndef GPOPT_CDSLInSubTest_H
#define GPOPT_CDSLInSubTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

class CDSLInSubTest
{
public:
	static GPOS_RESULT EresUnittest();
	static GPOS_RESULT EresUnittest_PreApplyCorpusElimination();
	static GPOS_RESULT EresUnittest_PostApplyCorpusElimination();
	static GPOS_RESULT EresUnittest_PreApplyRepeatedInElimination();
	static GPOS_RESULT EresUnittest_CorrelatedExistsCanonicalization();
	static GPOS_RESULT EresUnittest_PostApplyCorrelatedExistsCanonicalization();
	static GPOS_RESULT EresUnittest_PostApplyRepeatedInElimination();
	static GPOS_RESULT EresUnittest_PushedDownJoinRemap();
	static GPOS_RESULT EresUnittest_DecorrelatedSemiJoinRemap();
	static GPOS_RESULT EresUnittest_SemiJoinToInnerJoin();
	static GPOS_RESULT EresUnittest_SemiJoinComputedKeyToInnerJoin();
	static GPOS_RESULT EresUnittest_RejectsCorrelatedSemiJoinView();
	static GPOS_RESULT EresUnittest_RejectsSameSideSemiJoinPredicate();
	static GPOS_RESULT EresUnittest_InSubAsSimpleFilterCarrier();
	static GPOS_RESULT EresUnittest_PreApplyBelowBinaryJoinSpineRemap();
	static GPOS_RESULT EresUnittest_RejectsNullSupplyingRoute();
	static GPOS_RESULT EresUnittest_RejectsDifferentTable();
	static GPOS_RESULT EresUnittest_PostApplyIdentity();
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLInSubTest_H
