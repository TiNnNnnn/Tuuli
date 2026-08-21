//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDSLRule_InSub_H
#define GPOPT_CXformDSLRule_InSub_H

#include "gpos/base.h"

#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

class CXformDSLRule_InSub : public CXformExploration
{
public:
	CXformDSLRule_InSub(const CXformDSLRule_InSub &) = delete;

	explicit CXformDSLRule_InSub(CMemoryPool *mp);
	~CXformDSLRule_InSub() override = default;

	EXformId
	Exfid() const override
	{
		return ExfDSLRuleInSub;
	}

	const CHAR *
	SzId() const override
	{
		return "CXformDSLRule_InSub";
	}

	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;
	void Transform(CXformContext *, CXformResult *, CExpression *) const override;
};
}  // namespace gpopt

#endif	// !GPOPT_CXformDSLRule_InSub_H

