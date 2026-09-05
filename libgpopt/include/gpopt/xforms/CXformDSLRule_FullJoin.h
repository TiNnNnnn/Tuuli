#ifndef GPOPT_CXformDSLRule_FullJoin_H
#define GPOPT_CXformDSLRule_FullJoin_H

#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
class CXformDSLRule_FullJoin : public CXformExploration
{
public:
	CXformDSLRule_FullJoin(const CXformDSLRule_FullJoin &) = delete;
	explicit CXformDSLRule_FullJoin(CMemoryPool *mp);
	~CXformDSLRule_FullJoin() override = default;
	EXformId Exfid() const override { return ExfDSLRuleFullJoin; }
	const CHAR *SzId() const override { return "CXformDSLRule_FullJoin"; }
	EXformPromise Exfp(CExpressionHandle &) const override;
	void Transform(CXformContext *, CXformResult *, CExpression *) const override;
};
}  // namespace gpopt

#endif
