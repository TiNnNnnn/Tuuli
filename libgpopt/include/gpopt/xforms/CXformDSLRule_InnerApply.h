//---------------------------------------------------------------------------
// Thin Cascade shell routing InnerApply-rooted expressions to the DSL engine.
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDSLRule_InnerApply_H
#define GPOPT_CXformDSLRule_InnerApply_H

#include "gpos/base.h"
#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

class CXformDSLRule_InnerApply : public CXformExploration
{
private:
	CXformDSLRule_InnerApply(const CXformDSLRule_InnerApply &) = delete;

public:
	explicit CXformDSLRule_InnerApply(CMemoryPool *mp);
	~CXformDSLRule_InnerApply() override = default;

	EXformId Exfid() const override { return ExfDSLRuleInnerApply; }
	const CHAR *SzId() const override { return "CXformDSLRule_InnerApply"; }
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;
	void Transform(CXformContext *, CXformResult *, CExpression *) const override;
};
}  // namespace gpopt

#endif  // !GPOPT_CXformDSLRule_InnerApply_H
