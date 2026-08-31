//---------------------------------------------------------------------------
// Cascade shell routing SequenceProject/Window templates to the DSL engine.
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDSLRule_Window_H
#define GPOPT_CXformDSLRule_Window_H

#include "gpos/base.h"
#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

class CXformDSLRule_Window : public CXformExploration
{
private:
	CXformDSLRule_Window(const CXformDSLRule_Window &) = delete;

public:
	explicit CXformDSLRule_Window(CMemoryPool *mp);
	~CXformDSLRule_Window() override = default;

	EXformId Exfid() const override { return ExfDSLRuleWindow; }
	const CHAR *SzId() const override { return "CXformDSLRule_Window"; }
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;
	void Transform(CXformContext *, CXformResult *, CExpression *) const override;
};
}  // namespace gpopt

#endif  // !GPOPT_CXformDSLRule_Window_H
