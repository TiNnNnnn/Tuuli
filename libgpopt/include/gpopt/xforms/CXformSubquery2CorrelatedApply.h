// Execution preparation when optional subquery unnesting is disabled.
#ifndef GPOPT_CXformSubquery2CorrelatedApply_H
#define GPOPT_CXformSubquery2CorrelatedApply_H

#include "gpopt/xforms/CXformSubqueryUnnest.h"

namespace gpopt
{
class CXformSubquery2CorrelatedApply : public CXformSubqueryUnnest
{
public:
	explicit CXformSubquery2CorrelatedApply(CMemoryPool *mp);
	EXformId
	Exfid() const override
	{
		return ExfSubquery2CorrelatedApply;
	}
	const CHAR *
	SzId() const override
	{
		return "CXformSubquery2CorrelatedApply";
	}
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;
	void Transform(CXformContext *, CXformResult *,
				   CExpression *) const override;
};
}  // namespace gpopt
#endif
