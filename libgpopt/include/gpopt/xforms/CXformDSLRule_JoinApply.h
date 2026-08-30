//---------------------------------------------------------------------------
// Thin Cascade shell routing join-producing Apply expressions to the DSL engine.
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDSLRule_JoinApply_H
#define GPOPT_CXformDSLRule_JoinApply_H

#include "gpos/base.h"
#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

class CXformDSLRule_JoinApply : public CXformExploration
{
private:
	CXformDSLRule_JoinApply(const CXformDSLRule_JoinApply &) = delete;

public:
	explicit CXformDSLRule_JoinApply(CMemoryPool *mp);
	~CXformDSLRule_JoinApply() override = default;

	EXformId Exfid() const override { return ExfDSLRuleJoinApply; }
	const CHAR *SzId() const override { return "CXformDSLRule_JoinApply"; }
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;
	void Transform(CXformContext *, CXformResult *, CExpression *) const override;
};
}  // namespace gpopt

#endif  // !GPOPT_CXformDSLRule_JoinApply_H
