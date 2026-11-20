// Observable DSL rows retain ordinary columns and explicitly used system columns.
#ifndef GPOPT_CDSLColumnDomain_H
#define GPOPT_CDSLColumnDomain_H

#include "gpopt/base/CColRefSet.h"

namespace gpopt
{
class CExpression;
class CDSLColumnDomain
{
public:
	// Consumes target and preserves the paths of its relational inputs.
	static CExpression *PexprRestoreJoinKeys(CMemoryPool *mp, CExpression *source, CExpression *target);
	// Owned domain of a bound relation or an explicit attribute/schema vector.
	static CColRefSet *PcrsBinding(CMemoryPool *mp, CRefCount *binding);
	static BOOL FObservable(const CColRef *column);
	static CColRefArray *PdrgpcrOutput(CMemoryPool *mp, const CColRefSet *outputs);
};
}
#endif
