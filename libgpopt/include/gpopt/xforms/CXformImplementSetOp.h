#ifndef GPOPT_CXformImplementSetOp_H
#define GPOPT_CXformImplementSetOp_H
#include "gpopt/operators/CPatternMultiLeaf.h"
#include "gpopt/operators/CPatternNode.h"
#include "gpopt/xforms/CXformImplementUnion.h"
namespace gpopt
{
class CXformImplementSetOp : public CXformImplementUnion
{
  public:
	explicit CXformImplementSetOp(CMemoryPool *mp)
		: CXformImplementUnion(GPOS_NEW(mp) CExpression(
			  mp,
			  GPOS_NEW(mp)
				  CPatternNode(mp, CPatternNode::EmtMatchIntersectOrDifference),
			  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternMultiLeaf(mp))))
	{
	}
	EXformId
	Exfid() const override
	{
		return ExfImplementSetOp;
	}
	const CHAR *
	SzId() const override
	{
		return "CXformImplementSetOp";
	}
};
} // namespace gpopt
#endif
