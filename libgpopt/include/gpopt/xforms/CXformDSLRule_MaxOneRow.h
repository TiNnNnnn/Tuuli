//---------------------------------------------------------------------------
// Cascade shell routing MaxOneRow templates to the shared DSL engine.
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDSLRule_MaxOneRow_H
#define GPOPT_CXformDSLRule_MaxOneRow_H

#include "gpos/base.h"
#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

class CXformDSLRule_MaxOneRow : public CXformExploration
{
private:
	CXformDSLRule_MaxOneRow(const CXformDSLRule_MaxOneRow &) = delete;

public:
	explicit CXformDSLRule_MaxOneRow(CMemoryPool *mp);
	~CXformDSLRule_MaxOneRow() override = default;

	EXformId Exfid() const override { return ExfDSLRuleMaxOneRow; }
	const CHAR *SzId() const override { return "CXformDSLRule_MaxOneRow"; }
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;
	void Transform(CXformContext *, CXformResult *, CExpression *) const override;
};
}  // namespace gpopt

#endif  // !GPOPT_CXformDSLRule_MaxOneRow_H
