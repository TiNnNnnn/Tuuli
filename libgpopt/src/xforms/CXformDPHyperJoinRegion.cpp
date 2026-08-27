//---------------------------------------------------------------------------
//	@filename:
//		CXformDPHyperJoinRegion.cpp
//---------------------------------------------------------------------------
#include "gpopt/xforms/CXformDPHyperJoinRegion.h"

#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CPatternLeaf.h"

using namespace gpopt;

CXformDPHyperJoinRegion::CXformDPHyperJoinRegion(CMemoryPool *mp)
	: CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalInnerJoin(mp),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp))))
{
}

void
CXformDPHyperJoinRegion::Transform(CXformContext *, CXformResult *,
								   CExpression *) const
{
	// The group-expression exploration job consumes this xform before normal
	// transformation scheduling. Keeping Transform side-effect free makes an
	// accidental direct invocation safe.
}
