// Direct relational Apply execution, independent of optional decorrelation.
#ifndef GPOPT_CXformImplementApply_H
#define GPOPT_CXformImplementApply_H
#include "gpopt/xforms/CXformImplementation.h"
namespace gpopt
{
class CXformImplementApply : public CXformImplementation
{
  public:
	explicit CXformImplementApply(CMemoryPool *);
	EXformId
	Exfid() const override
	{
		return ExfImplementApply;
	}
	const CHAR *
	SzId() const override
	{
		return "CXformImplementApply";
	}
	EXformPromise
	Exfp(CExpressionHandle &) const override;
	void
	Transform(CXformContext *, CXformResult *, CExpression *) const override;
};
} // namespace gpopt
#endif
