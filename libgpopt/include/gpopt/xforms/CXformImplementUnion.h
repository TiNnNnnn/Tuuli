// Direct implementation of UNION DISTINCT, independent of optional rewrites.
#ifndef GPOPT_CXformImplementUnion_H
#define GPOPT_CXformImplementUnion_H

#include "gpopt/xforms/CXformImplementation.h"

namespace gpopt
{
class CXformImplementUnion : public CXformImplementation
{
protected:
	explicit CXformImplementUnion(CExpression *pattern)
		: CXformImplementation(pattern) {}
public:
	explicit CXformImplementUnion(CMemoryPool *mp);
	EXformId
	Exfid() const override
	{
		return ExfImplementUnion;
	}
	const CHAR *
	SzId() const override
	{
		return "CXformImplementUnion";
	}
	EXformPromise
	Exfp(CExpressionHandle &) const override;
	void
	Transform(CXformContext *, CXformResult *, CExpression *) const override;
};
} // namespace gpopt

#endif
