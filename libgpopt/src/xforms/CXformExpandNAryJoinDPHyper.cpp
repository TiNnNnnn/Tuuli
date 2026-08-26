//---------------------------------------------------------------------------
//	@filename:
//		CXformExpandNAryJoinDPHyper.cpp
//---------------------------------------------------------------------------
#include "gpopt/xforms/CXformExpandNAryJoinDPHyper.h"

#include "gpopt/operators/CLogicalNAryJoin.h"
#include "gpopt/operators/CPatternMultiLeaf.h"
#include "gpopt/operators/CPatternTree.h"

using namespace gpopt;

CXformExpandNAryJoinDPHyper::CXformExpandNAryJoinDPHyper(CMemoryPool *mp)
	: CXformExploration(
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalNAryJoin(mp),
			  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternMultiLeaf(mp)),
			  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))))
{
}

void
CXformExpandNAryJoinDPHyper::Transform(CXformContext *, CXformResult *,
									   CExpression *) const
{
	// The group-expression exploration job consumes this xform before normal
	// transformation scheduling. Keeping Transform side-effect free makes an
	// accidental direct invocation safe.
}
