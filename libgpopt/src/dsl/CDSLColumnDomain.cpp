#include "gpopt/dsl/CDSLColumnDomain.h"
#include "gpopt/base/CColRefSetIter.h"

using namespace gpopt;

BOOL
CDSLColumnDomain::FObservable(const CColRef *column)
{
	return !column->IsSystemCol() || CColRef::EUsed == column->GetUsage(true, true);
}

CColRefArray *
CDSLColumnDomain::PdrgpcrOutput(CMemoryPool *mp, const CColRefSet *outputs)
{
	CColRefArray *columns = GPOS_NEW(mp) CColRefArray(mp);
	CColRefSetIter iter(*outputs);
	while (iter.Advance())
	{
		if (FObservable(iter.Pcr())) columns->Append(iter.Pcr());
	}
	return columns;
}
