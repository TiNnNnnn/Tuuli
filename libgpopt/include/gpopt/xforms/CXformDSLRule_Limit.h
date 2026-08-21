//---------------------------------------------------------------------------
//	MONSOON DSL rule shell for ORCA's fused order/limit boundary.
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDSLRule_Limit_H
#define GPOPT_CXformDSLRule_Limit_H

#include "gpos/base.h"
#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

class CXformDSLRule_Limit : public CXformExploration
{
private:
	CXformDSLRule_Limit(const CXformDSLRule_Limit &) = delete;

public:
	explicit CXformDSLRule_Limit(CMemoryPool *mp);
	~CXformDSLRule_Limit() override = default;

	EXformId Exfid() const override
	{
		return ExfDSLRuleLimit;
	}
	const CHAR *SzId() const override
	{
		return "CXformDSLRule_Limit";
	}
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;
	void Transform(CXformContext *pxfctxt, CXformResult *pxfres,
		CExpression *pexpr) const override;
};
}  // namespace gpopt

#endif  // !GPOPT_CXformDSLRule_Limit_H
