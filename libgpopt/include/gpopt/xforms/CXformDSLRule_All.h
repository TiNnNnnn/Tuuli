//---------------------------------------------------------------------------
// Thin DSL shell for post-unnest ALL comparisons.
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDSLRule_All_H
#define GPOPT_CXformDSLRule_All_H

#include "gpos/base.h"
#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

class CXformDSLRule_All : public CXformExploration
{
public:
	CXformDSLRule_All(const CXformDSLRule_All &) = delete;
	explicit CXformDSLRule_All(CMemoryPool *mp);
	~CXformDSLRule_All() override = default;

	EXformId Exfid() const override { return ExfDSLRuleAll; }
	const CHAR *SzId() const override { return "CXformDSLRule_All"; }
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;
	void Transform(CXformContext *, CXformResult *, CExpression *) const override;
};
}  // namespace gpopt

#endif  // !GPOPT_CXformDSLRule_All_H
