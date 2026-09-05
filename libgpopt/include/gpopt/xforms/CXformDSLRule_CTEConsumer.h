//---------------------------------------------------------------------------
// Thin DSL shell for CTE consumers.
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDSLRule_CTEConsumer_H
#define GPOPT_CXformDSLRule_CTEConsumer_H

#include "gpos/base.h"
#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

class CXformDSLRule_CTEConsumer : public CXformExploration
{
public:
	CXformDSLRule_CTEConsumer(const CXformDSLRule_CTEConsumer &) = delete;
	explicit CXformDSLRule_CTEConsumer(CMemoryPool *mp);
	~CXformDSLRule_CTEConsumer() override = default;

	EXformId Exfid() const override { return ExfDSLRuleCTEConsumer; }
	const CHAR *SzId() const override { return "CXformDSLRule_CTEConsumer"; }
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;
	void Transform(CXformContext *, CXformResult *, CExpression *) const override;
};
}  // namespace gpopt

#endif  // !GPOPT_CXformDSLRule_CTEConsumer_H
