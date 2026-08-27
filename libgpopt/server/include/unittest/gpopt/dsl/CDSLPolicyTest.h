//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLPolicyTest_H
#define GPOPT_CDSLPolicyTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

class CDSLPolicyTest
{
public:
	static GPOS_RESULT EresUnittest();
	static GPOS_RESULT EresUnittest_CanonicalIdentity();
	static GPOS_RESULT EresUnittest_StrictLoader();
	static GPOS_RESULT EresUnittest_SnapshotDefaultsAndAuto();
};
}  // namespace gpopt

#endif  // !GPOPT_CDSLPolicyTest_H
