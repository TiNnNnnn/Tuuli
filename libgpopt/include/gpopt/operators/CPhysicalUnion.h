// Complete-row distinct Append, implemented by aggregate/sort or LIMIT 1.
#ifndef GPOPT_CPhysicalUnion_H
#define GPOPT_CPhysicalUnion_H
#include "gpopt/operators/CPhysicalSerialUnionAll.h"
namespace gpopt
{
class CPhysicalUnion : public CPhysicalSerialUnionAll
{
private:
	BOOL m_hash;

public:
	CPhysicalUnion(CMemoryPool *, CColRefArray *, CColRef2dArray *, BOOL hash);
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalUnion;
	}
	const CHAR *
	SzId() const override
	{
		return m_hash ? "CPhysicalUnionHash" : "CPhysicalUnionSorted";
	}
	BOOL
	FHash() const
	{
		return m_hash;
	}
	BOOL
	Matches(COperator *) const override;
	CColRefSet *
	PcrsRequired(CMemoryPool *, CExpressionHandle &, CColRefSet *, ULONG,
				 CDrvdPropArray *, ULONG) override;
	CDistributionSpec *
	PdsRequired(CMemoryPool *, CExpressionHandle &, CDistributionSpec *, ULONG,
				CDrvdPropArray *, ULONG) const override;
	CRewindabilitySpec *
	PrsDerive(CMemoryPool *, CExpressionHandle &) const override;
};
} // namespace gpopt
#endif
