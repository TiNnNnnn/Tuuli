#include "gpopt/xforms/CXformImplementApply.h"
#include "gpopt/cost/ICostModel.h"
#include "gpopt/operators/CPatternNode.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPhysicalInnerNLJoin.h"
#include "gpopt/operators/CPhysicalLeftOuterNLJoin.h"
#include "gpopt/operators/CPhysicalLeftSemiNLJoin.h"
#include "gpopt/operators/CPhysicalLeftAntiSemiNLJoin.h"
#include "gpopt/operators/CPhysicalLeftAntiSemiNLJoinNotIn.h"
#include "gpopt/xforms/CXformUtils.h"
using namespace gpopt;

CXformImplementApply::CXformImplementApply(CMemoryPool *mp)
	: CXformImplementation(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CPatternNode(mp, CPatternNode::EmtMatchRegularApply),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp))))
{
}

CXform::EXformPromise
CXformImplementApply::Exfp(CExpressionHandle &exprhdl) const
{
	// PG's regular NLJ translator binds inner outer-refs through PARAM_EXEC.
	// Do not change the GPDB backend's correlated execution/distribution path.
	if (COptCtxt::PoctxtFromTLS()->GetCostModel()->Ecmt() != ICostModel::EcmtPG)
		return ExfpNone;
	return CXformUtils::ExfpLogicalJoin2PhysicalJoin(exprhdl);
}

void
CXformImplementApply::Transform(CXformContext *context, CXformResult *result,
								CExpression *expr) const
{
	// Keep the original children and predicate: no decorrelation, scalarizing,
	// or fresh logical groups. CorrelatedApply (SubPlan) is a different domain.
	switch (expr->Pop()->Eopid())
	{
	case COperator::EopLogicalInnerApply:
		CXformUtils::ImplementNLJoin<CPhysicalInnerNLJoin>(context, result,
														   expr);
		break;
	case COperator::EopLogicalLeftOuterApply:
		CXformUtils::ImplementNLJoin<CPhysicalLeftOuterNLJoin>(context, result,
															   expr);
		break;
	case COperator::EopLogicalLeftSemiApply:
	case COperator::EopLogicalLeftSemiApplyIn:
		CXformUtils::ImplementNLJoin<CPhysicalLeftSemiNLJoin>(context, result,
															  expr);
		break;
	case COperator::EopLogicalLeftAntiSemiApply:
		CXformUtils::ImplementNLJoin<CPhysicalLeftAntiSemiNLJoin>(context,
																  result, expr);
		break;
	case COperator::EopLogicalLeftAntiSemiApplyNotIn:
		CXformUtils::ImplementNLJoin<CPhysicalLeftAntiSemiNLJoinNotIn>(
			context, result, expr);
		break;
	default:
		GPOS_ASSERT(!"Unexpected ordinary Apply type");
	}
}
