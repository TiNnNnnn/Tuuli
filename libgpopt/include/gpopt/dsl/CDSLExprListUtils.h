//---------------------------------------------------------------------------
//	MONSOON DSL expression-list algebra
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLExprListUtils_H
#define GPOPT_CDSLExprListUtils_H

#include "gpos/base.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

class CDSLExprListUtils
{
public:
	CDSLExprListUtils() = delete;

	static BOOL FProjectList(const CExpression *pexpr);
	static BOOL FConcatSafe(CExpression *pexprUpper,
							CExpression *pexprLower);
	static BOOL FDepsDisjoint(CMemoryPool *mp, CExpression *pexprList,
							 CColRefArray *pdrgpcrSchema);

	// Merge two evaluation layers. Movable ordinary upper elements precede the
	// upper SRF cohort, followed by lower, matching ORCA's canonical SRF order.
	// Caller owns the returned ProjectList.
	static CExpression *PexprConcat(CMemoryPool *mp, CExpression *pexprUpper,
								 CExpression *pexprLower);

	// Partition an upper list around the columns defined by lower. Independent
	// elements move into merged=(movable upper)++lower; dependent elements remain
	// in residual. SRFs move as one cohort only when lower has no SRF, preserving
	// row-expansion layers. Caller owns both returned lists.
	static BOOL FSplit(CMemoryPool *mp, CExpression *pexprUpper,
					   CExpression *pexprLower, CExpression **ppexprMerged,
					   CExpression **ppexprResidual);
};
}  // namespace gpopt

#endif  // !GPOPT_CDSLExprListUtils_H
