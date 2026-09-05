//---------------------------------------------------------------------------
// Thin DSL shell for CTE anchors.
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDSLRule_CTEAnchor_H
#define GPOPT_CXformDSLRule_CTEAnchor_H

#include "gpos/base.h"
#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

class CXformDSLRule_CTEAnchor : public CXformExploration
{
public:
	CXformDSLRule_CTEAnchor(const CXformDSLRule_CTEAnchor &) = delete;
	explicit CXformDSLRule_CTEAnchor(CMemoryPool *mp);
	~CXformDSLRule_CTEAnchor() override = default;

	EXformId Exfid() const override { return ExfDSLRuleCTEAnchor; }
	const CHAR *SzId() const override { return "CXformDSLRule_CTEAnchor"; }
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;
	void Transform(CXformContext *, CXformResult *, CExpression *) const override;
};
}  // namespace gpopt

#endif  // !GPOPT_CXformDSLRule_CTEAnchor_H
