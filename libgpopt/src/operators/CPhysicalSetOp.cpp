#include "gpopt/operators/CPhysicalSetOp.h"
#include "gpopt/operators/CExpressionHandle.h"
using namespace gpopt;

COrderSpec *
CPhysicalSetOp::Order(CMemoryPool *mp, CColRefArray *columns) const
{
	auto *order = GPOS_NEW(mp) COrderSpec(mp);
	if (!FHash())
	{
		for (ULONG i = 0; i < columns->Size(); ++i)
		{
			auto *column = (*columns)[i];
			auto *mdid =
				column->RetrieveType()->GetMdidForCmpType(IMDType::EcmptL);
			mdid->AddRef();
			order->Append(mdid, column, COrderSpec::EntLast);
		}
	}
	return order;
}

COrderSpec *
CPhysicalSetOp::PosRequired(CMemoryPool *mp, CExpressionHandle &, COrderSpec *,
							ULONG child, CDrvdPropArray *, ULONG) const
{
	return Order(mp, (*PdrgpdrgpcrInput())[child]);
}

COrderSpec *
CPhysicalSetOp::PosDerive(CMemoryPool *mp, CExpressionHandle &) const
{
	return Order(mp, PdrgpcrOutput());
}

CEnfdProp::EPropEnforcingType
CPhysicalSetOp::EpetOrder(CExpressionHandle &exprhdl,
						  const CEnfdOrder *order) const
{
	return order->FCompatible(CDrvdPropPlan::Pdpplan(exprhdl.Pdp())->Pos())
			   ? CEnfdProp::EpetUnnecessary
			   : CEnfdProp::EpetRequired;
}
