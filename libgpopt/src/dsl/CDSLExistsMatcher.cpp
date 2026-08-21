//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLExistsMatcher.cpp
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLExistsMatcher.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CScalarConst.h"
#include "naucrates/base/IDatumInt2.h"
#include "naucrates/base/IDatumInt4.h"
#include "naucrates/base/IDatumInt8.h"

using namespace gpopt;
using namespace gpnaucrates;

namespace
{
BOOL
FScalarConstIntOne(CExpression *pexpr)
{
	if (COperator::EopScalarConst != pexpr->Pop()->Eopid())
	{
		return false;
	}

	IDatum *pdatum = CScalarConst::PopConvert(pexpr->Pop())->GetDatum();
	if (pdatum->IsNull())
	{
		return false;
	}

	switch (pdatum->GetDatumType())
	{
		case IMDType::EtiInt2:
			return 1 == dynamic_cast<IDatumInt2 *>(pdatum)->Value();
		case IMDType::EtiInt4:
			return 1 == dynamic_cast<IDatumInt4 *>(pdatum)->Value();
		case IMDType::EtiInt8:
			return 1 == dynamic_cast<IDatumInt8 *>(pdatum)->Value();
		default:
			return false;
	}
}
}  // namespace

BOOL
CDSLExistsMatcher::FExistsLimitOne(CExpression *pexpr) const
{
	if (COperator::EopLogicalLimit != pexpr->Pop()->Eopid() ||
		3 != pexpr->Arity())
	{
		return false;
	}

	CLogicalLimit *popLimit = CLogicalLimit::PopConvert(pexpr->Pop());
	return popLimit->FGlobal() && popLimit->FHasCount() &&
		   CUtils::FHasZeroOffset(pexpr) && FScalarConstIntOne((*pexpr)[2]);
}

BOOL
CDSLExistsMatcher::FMatch(const CDSLOp *pop, CExpression *pexpr,
						  CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != pop);
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != pmodel);
	GPOS_ASSERT(EdslopExists == pop->Edslop());

	if (2 != pop->UlChildren() || nullptr == pop->Pdrgpsym() ||
		0 != pop->Pdrgpsym()->Size() || 3 != pexpr->Arity() ||
		COperator::EopLogicalLeftSemiApply != pexpr->Pop()->Eopid() ||
		!CUtils::FScalarConstTrue((*pexpr)[2]))
	{
		return false;
	}

	CLogicalApply *popApply =
		dynamic_cast<CLogicalApply *>(pexpr->Pop());
	if (nullptr == popApply ||
		COperator::EopScalarSubqueryExists != popApply->EopidOriginSubq())
	{
		return false;
	}

	CExpression *pexprInner = (*pexpr)[1];
	if (FExistsLimitOne(pexprInner))
	{
		pexprInner = (*pexprInner)[0];
	}

	return m_pmatcher->FMatch((*pop)[0], (*pexpr)[0], pmodel) &&
		   m_pmatcher->FMatch((*pop)[1], pexprInner, pmodel);
}

// EOF
