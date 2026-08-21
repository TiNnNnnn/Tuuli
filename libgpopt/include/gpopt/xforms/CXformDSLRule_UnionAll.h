//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDSLRule_UnionAll_H
#define GPOPT_CXformDSLRule_UnionAll_H

#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
class CXformDSLRule_UnionAll : public CXformExploration
{
public:
	CXformDSLRule_UnionAll(const CXformDSLRule_UnionAll &) = delete;
	explicit CXformDSLRule_UnionAll(CMemoryPool *mp);
	~CXformDSLRule_UnionAll() override = default;

	EXformId Exfid() const override { return ExfDSLRuleUnionAll; }
	const CHAR *SzId() const override { return "CXformDSLRule_UnionAll"; }
	EXformPromise Exfp(CExpressionHandle &) const override;
	void Transform(CXformContext *, CXformResult *, CExpression *) const override;
};
}  // namespace gpopt

#endif	// !GPOPT_CXformDSLRule_UnionAll_H
