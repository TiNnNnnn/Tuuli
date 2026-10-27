// Observable DSL rows retain ordinary columns and explicitly used system columns.
#ifndef GPOPT_CDSLColumnDomain_H
#define GPOPT_CDSLColumnDomain_H

#include "gpopt/base/CColRefSet.h"

namespace gpopt
{
class CDSLColumnDomain
{
public:
	static BOOL FObservable(const CColRef *column);
	static CColRefArray *PdrgpcrOutput(CMemoryPool *mp, const CColRefSet *outputs);
};
}
#endif
