#include "gpopt/xforms/CXformSubquery2CorrelatedApply.h"

#include "gpopt/base/COptCtxt.h"
#include "gpopt/cost/ICostModel.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPatternNode.h"
#include "gpopt/operators/CPatternTree.h"

using namespace gpopt;

CXformSubquery2CorrelatedApply::CXformSubquery2CorrelatedApply(CMemoryPool *mp)
	: CXformSubqueryUnnest(GPOS_NEW(mp) CExpression(
		  mp,
		  GPOS_NEW(mp) CPatternNode(mp, CPatternNode::EmtMatchUnarySubquery),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))))
{
}

CXform::EXformPromise
CXformSubquery2CorrelatedApply::Exfp(CExpressionHandle &exprhdl) const
{
	if (COptCtxt::PoctxtFromTLS()->GetCostModel()->Ecmt() != ICostModel::EcmtPG)
		return ExfpNone;
	EXformId original = ExfInvalid;
	switch (exprhdl.Pop()->Eopid())
	{
		case COperator::EopLogicalSelect:
			original = ExfSelect2Apply;
			break;
		case COperator::EopLogicalProject:
			original = ExfProject2Apply;
			break;
		case COperator::EopLogicalGbAgg:
			original = ExfGbAgg2Apply;
			break;
		case COperator::EopLogicalSequenceProject:
			original = ExfSequenceProject2Apply;
			break;
		default:
			return ExfpNone;
	}
	// The original transform already offers correlated execution. Preserve its
	// native search space; do not infer executable coverage from DSL presence.
	if (GPOPT_FENABLED_XFORM(original))
		return ExfpNone;
	return CXformSubqueryUnnest::Exfp(exprhdl);
}

void
CXformSubquery2CorrelatedApply::Transform(CXformContext *context,
										  CXformResult *result,
										  CExpression *expr) const
{
	GPOS_ASSERT(FPromising(context->Pmp(), this, expr));
	GPOS_ASSERT(FCheckPattern(expr));
	CExpression *lowered = PexprSubqueryUnnest(context->Pmp(), expr,
											   true /*fEnforceCorrelatedApply*/,
											   false /*fNormalize*/);
	if (nullptr != lowered)
		result->Add(lowered);
}
