// PG's two-input SetOp preserves multiplicities without a logical lowering.
#ifndef GPOPT_CPhysicalSetOp_H
#define GPOPT_CPhysicalSetOp_H
#include "gpopt/operators/CPhysicalUnion.h"
namespace gpopt
{
class CPhysicalSetOp : public CPhysicalUnion
{
  private:
	EOperatorId m_kind;
	COrderSpec *
	Order(CMemoryPool *, CColRefArray *) const;

  public:
	CPhysicalSetOp(CMemoryPool *mp, CColRefArray *output,
				   CColRef2dArray *inputs, BOOL hash, EOperatorId kind)
		: CPhysicalUnion(mp, output, inputs, hash), m_kind(kind)
	{
	}
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalSetOp;
	}
	EOperatorId
	Kind() const
	{
		return m_kind;
	}
	const CHAR *
	SzId() const override
	{
		return FHash() ? "CPhysicalSetOpHash" : "CPhysicalSetOpSorted";
	}
	BOOL
	FInputOrderSensitive() const override
	{
		return true;
	}
	BOOL
	Matches(COperator *pop) const override
	{
		return CPhysicalUnion::Matches(pop) &&
			   m_kind == static_cast<CPhysicalSetOp *>(pop)->Kind();
	}
	COrderSpec *
	PosRequired(CMemoryPool *, CExpressionHandle &, COrderSpec *, ULONG,
				CDrvdPropArray *, ULONG) const override;
	COrderSpec *
	PosDerive(CMemoryPool *, CExpressionHandle &) const override;
	CEnfdProp::EPropEnforcingType
	EpetOrder(CExpressionHandle &, const CEnfdOrder *) const override;
};
} // namespace gpopt
#endif
