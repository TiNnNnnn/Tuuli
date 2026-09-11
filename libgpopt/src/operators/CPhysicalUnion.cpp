#include "gpopt/operators/CPhysicalUnion.h"
#include "gpopt/base/CDistributionSpecSingleton.h"
#include "gpopt/operators/CExpressionHandle.h"
using namespace gpopt;

CPhysicalUnion::CPhysicalUnion(CMemoryPool *mp, CColRefArray *output,
							   CColRef2dArray *inputs, BOOL hash)
	: CPhysicalSerialUnionAll(mp, output, inputs), m_hash(hash)
{
	// Global DISTINCT must see all rows, not deduplicate each segment.
	SetDistrRequests(1);
}

BOOL
CPhysicalUnion::Matches(COperator *pop) const
{
	return CPhysicalUnionAll::Matches(pop) &&
		   m_hash == static_cast<CPhysicalUnion *>(pop)->FHash();
}

CColRefSet *
CPhysicalUnion::PcrsRequired(CMemoryPool *mp, CExpressionHandle &, CColRefSet *,
							 ULONG child, CDrvdPropArray *, ULONG)
{
	// Keys discarded by the parent still participate in deduplication.
	return GPOS_NEW(mp) CColRefSet(mp, (*PdrgpdrgpcrInput())[child]);
}

CDistributionSpec *
CPhysicalUnion::PdsRequired(CMemoryPool *mp, CExpressionHandle &,
							CDistributionSpec *, ULONG, CDrvdPropArray *,
							ULONG) const
{
	return GPOS_NEW(mp) CDistributionSpecSingleton();
}

CRewindabilitySpec *
CPhysicalUnion::PrsDerive(CMemoryPool *mp, CExpressionHandle &exprhdl) const
{
	// Dedup/SetOp can rescan their inputs, but cannot mark/restore. Taking the
	// weakest input also prevents a rescannable first child from hiding a
	// non-rescannable later child. ErtNone here would forbid correlated rescans
	// even with a Spool above us: that Spool still needs to rebind this subtree.
	auto type = CRewindabilitySpec::ErtRewindable;
	auto hazard = CRewindabilitySpec::EmhtNoMotion;
	for (ULONG i = 0; i < exprhdl.Arity(); ++i)
	{
		auto *child = exprhdl.Pdpplan(i)->Prs();
		if (child->Ert() > type) type = child->Ert();
		if (child->HasMotionHazard()) hazard = CRewindabilitySpec::EmhtMotion;
	}
	return GPOS_NEW(mp) CRewindabilitySpec(type, hazard);
}
