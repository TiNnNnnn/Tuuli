// Direct cardinality checking without a row_number/Assert logical rewrite.
#ifndef GPOPT_CXformImplementMaxOneRow_H
#define GPOPT_CXformImplementMaxOneRow_H
#include "gpopt/xforms/CXformImplementation.h"
namespace gpopt
{
class CXformImplementMaxOneRow : public CXformImplementation
{
  public:
	explicit CXformImplementMaxOneRow(CMemoryPool *);
	EXformId
	Exfid() const override
	{
		return ExfImplementMaxOneRow;
	}
	const CHAR *
	SzId() const override
	{
		return "CXformImplementMaxOneRow";
	}
	EXformPromise
	Exfp(CExpressionHandle &) const override;
	void
	Transform(CXformContext *, CXformResult *, CExpression *) const override;
};
} // namespace gpopt
#endif
