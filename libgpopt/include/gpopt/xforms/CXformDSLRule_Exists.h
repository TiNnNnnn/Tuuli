//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CXformDSLRule_Exists.h
//
//	@doc:
//		Thin DSL-rule exploration shell rooted at CLogicalLeftSemiApply, ORCA's
//		filter-context representation of EXISTS.
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDSLRule_Exists_H
#define GPOPT_CXformDSLRule_Exists_H

#include "gpos/base.h"

#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

class CXformDSLRule_Exists : public CXformExploration
{
public:
	CXformDSLRule_Exists(const CXformDSLRule_Exists &) = delete;

	explicit CXformDSLRule_Exists(CMemoryPool *mp);
	~CXformDSLRule_Exists() override = default;

	EXformId
	Exfid() const override
	{
		return ExfDSLRuleExists;
	}

	const CHAR *
	SzId() const override
	{
		return "CXformDSLRule_Exists";
	}

	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;

	void Transform(CXformContext *, CXformResult *,
				   CExpression *) const override;
};
}  // namespace gpopt

#endif	// !GPOPT_CXformDSLRule_Exists_H

