//---------------------------------------------------------------------------
//	@filename:
//		CXformExpandNAryJoinDPHyper.h
//
//	@doc:
//		Identity and stage-control xform for the specialized DPHyper Cascades
//		job. CJobGroupExpressionExploration intercepts this xform and schedules
//		CJobJoinEnumeration; it is never executed as a binding-at-a-time xform.
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformExpandNAryJoinDPHyper_H
#define GPOPT_CXformExpandNAryJoinDPHyper_H

#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

class CXformExpandNAryJoinDPHyper : public CXformExploration
{
public:
	explicit CXformExpandNAryJoinDPHyper(CMemoryPool *mp);
	~CXformExpandNAryJoinDPHyper() override = default;

	EXformId
	Exfid() const override
	{
		return ExfExpandNAryJoinDPHyper;
	}

	const CHAR *
	SzId() const override
	{
		return "CXformExpandNAryJoinDPHyper";
	}

	EXformPromise
	Exfp(CExpressionHandle &) const override
	{
		return ExfpNone;
	}

	void Transform(CXformContext *, CXformResult *, CExpression *) const override;
};

}  // namespace gpopt

#endif  // !GPOPT_CXformExpandNAryJoinDPHyper_H
