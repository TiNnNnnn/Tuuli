//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDSLRule_Union_H
#define GPOPT_CXformDSLRule_Union_H

#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
class CXformDSLRule_Union : public CXformExploration
{
public:
	CXformDSLRule_Union(const CXformDSLRule_Union &) = delete;
	explicit CXformDSLRule_Union(CMemoryPool *mp);
	~CXformDSLRule_Union() override = default;

	EXformId Exfid() const override { return ExfDSLRuleUnion; }
	const CHAR *SzId() const override { return "CXformDSLRule_Union"; }
	EXformPromise Exfp(CExpressionHandle &) const override;
	void Transform(CXformContext *, CXformResult *, CExpression *) const override;
};
}  // namespace gpopt

#endif	// !GPOPT_CXformDSLRule_Union_H
