// Shared helpers private to the instantiator implementation.
// Not part of the installed DSL API. No state or duplicate implementations.
#ifndef GPOPT_CDSLInstantiatorUtils_H
#define GPOPT_CDSLInstantiatorUtils_H

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/base/CColRefSet.h"

namespace gpopt
{
namespace dslinstantiator
{
CColRefArray *
PdrgpcrLiveOutput(CMemoryPool *mp, CExpression *pexpr);

BOOL
FColSetContainsArray(const CColRefSet *pcrs,
					 const CColRefArray *pdrgpcr);

CExpression *
PexprRemapPredicate(CMemoryPool *mp, CExpression *pexpr,
				   UlongToColRefMap *mapping);

CExpression *
PexprRebuildComparisons(CMemoryPool *mp, CExpression *pexpr);

const CDSLOp *
PopOnlyBoundInSub(const CDSLOp *pop, const CDSLModel *pmodel,
				  ULONG *pulMatches);
}  // namespace dslinstantiator
}  // namespace gpopt

#endif
