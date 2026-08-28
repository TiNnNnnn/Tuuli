//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDSLRule_NotExists_H
#define GPOPT_CXformDSLRule_NotExists_H

#include "gpos/base.h"

#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

// Thin DSL-rule exploration shell rooted at CLogicalLeftAntiSemiApply,
// ORCA's filter-context representation of NOT EXISTS.
class CXformDSLRule_NotExists : public CXformExploration
{
public:
	CXformDSLRule_NotExists(const CXformDSLRule_NotExists &) = delete;

	explicit CXformDSLRule_NotExists(CMemoryPool *mp);
	~CXformDSLRule_NotExists() override = default;

	EXformId
	Exfid() const override
	{
		return ExfDSLRuleNotExists;
	}

	const CHAR *
	SzId() const override
	{
		return "CXformDSLRule_NotExists";
	}

	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;

	void Transform(CXformContext *, CXformResult *,
				   CExpression *) const override;
};
}  // namespace gpopt

#endif	// !GPOPT_CXformDSLRule_NotExists_H
