//---------------------------------------------------------------------------
//	@filename:
//		CXformDPHyperJoinRegion.h
//
//	@doc:
//		Identity and stage-control xform for the specialized DPHyper Cascades
//		join-region job. CJobGroupExpressionExploration intercepts this xform
//		and schedules CJobJoinEnumeration; it is never executed as a
//		binding-at-a-time xform.
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDPHyperJoinRegion_H
#define GPOPT_CXformDPHyperJoinRegion_H

#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

class CXformDPHyperJoinRegion : public CXformExploration
{
public:
	explicit CXformDPHyperJoinRegion(CMemoryPool *mp);
	~CXformDPHyperJoinRegion() override = default;

	EXformId
	Exfid() const override
	{
		return ExfDPHyperJoinRegion;
	}

	const CHAR *
	SzId() const override
	{
		return "CXformDPHyperJoinRegion";
	}

	EXformPromise
	Exfp(CExpressionHandle &) const override
	{
		return ExfpNone;
	}

	void Transform(CXformContext *, CXformResult *, CExpression *) const override;
};

}  // namespace gpopt

#endif  // !GPOPT_CXformDPHyperJoinRegion_H
