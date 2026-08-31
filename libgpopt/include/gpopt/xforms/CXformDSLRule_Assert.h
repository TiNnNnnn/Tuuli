//---------------------------------------------------------------------------
// Cascade shell routing Assert templates to the shared DSL engine.
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDSLRule_Assert_H
#define GPOPT_CXformDSLRule_Assert_H

#include "gpos/base.h"
#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

class CXformDSLRule_Assert : public CXformExploration
{
private:
	CXformDSLRule_Assert(const CXformDSLRule_Assert &) = delete;

public:
	explicit CXformDSLRule_Assert(CMemoryPool *mp);
	~CXformDSLRule_Assert() override = default;

	EXformId Exfid() const override { return ExfDSLRuleAssert; }
	const CHAR *SzId() const override { return "CXformDSLRule_Assert"; }
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;
	void Transform(CXformContext *, CXformResult *, CExpression *) const override;
};
}  // namespace gpopt

#endif  // !GPOPT_CXformDSLRule_Assert_H
