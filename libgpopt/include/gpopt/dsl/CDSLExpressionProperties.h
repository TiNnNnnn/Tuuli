// Shared native expression properties. No rule bindings or scheduling state.
#ifndef GPOPT_CDSLExpressionProperties_H
#define GPOPT_CDSLExpressionProperties_H

#include "gpos/base.h"
#include "gpopt/base/CMaxCard.h"

namespace gpopt
{
class CExpression;
class CColRef;
class CColRefSet;

namespace dslproperties
{
// Inputs are borrowed. Unknown operators are not evidence of error freedom.
// aggregateInput must belong to the aggregate's actual relational input.
BOOL FScalarTreeProvablyErrorFree(CExpression *pexpr,
	const CMaxCard &aggregateInput = CMaxCard());
BOOL FScalarTreeProvablyDeterministic(CExpression *pexpr);
BOOL FRelationalTreeProvablyErrorFree(CExpression *pexpr,
	BOOL deterministic = false);

// Sufficient evidence that early termination and full evaluation agree.
// Also accepts scalar operands using the same totality/repeatability checks.
BOOL FQueryDemandInsensitive(CExpression *pexpr);
// A fixed natural-number slice bound, allowing only audited total casts.
BOOL FNonnegativeLimitBound(CExpression *expr);

// Conservative frame footprint for the supported native lowering. Unlike
// DeriveUsedColumns, retains reads owned by nested relational scopes. Caller
// owns the result; nullptr means unknown, never an empty footprint. This is
// structural evidence only, not source admissibility or receipt authorization.
CColRefSet *PcrsFrameReads(gpos::CMemoryPool *mp, CExpression *pexpr);

// Follow only relational edges that cannot introduce NULL for this column.
BOOL FExpressionProvesNotNull(gpos::CMemoryPool *mp, CExpression *pexpr,
	const CColRef *pcr);
}  // namespace dslproperties
}  // namespace gpopt

#endif  // !GPOPT_CDSLExpressionProperties_H
