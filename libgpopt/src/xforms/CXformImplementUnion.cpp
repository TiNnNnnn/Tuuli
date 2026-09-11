#include "gpopt/xforms/CXformImplementUnion.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/cost/ICostModel.h"
#include "gpopt/operators/CLogicalUnion.h"
#include "gpopt/operators/CPatternMultiLeaf.h"
#include "gpopt/operators/CPhysicalUnion.h"

using namespace gpopt;

CXformImplementUnion::CXformImplementUnion(CMemoryPool *mp)
	: CXformImplementation(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalUnion(mp),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternMultiLeaf(mp))))
{
}

CXform::EXformPromise
CXformImplementUnion::Exfp(CExpressionHandle &) const
{
	// Composite Append/dedup costing is currently supplied by the PG backend.
	return COptCtxt::PoctxtFromTLS()->GetCostModel()->Ecmt() ==
				   ICostModel::EcmtPG
			   ? ExfpHigh
			   : ExfpNone;
}

void
CXformImplementUnion::Transform(CXformContext *context, CXformResult *result,
								CExpression *expr) const
{
	CMemoryPool *mp = context->Pmp();
	auto *logical = CLogicalUnion::PopConvert(expr->Pop());
	auto *output = logical->PdrgpcrOutput();
	auto *inputs = logical->PdrgpdrgpcrInput();
	for (BOOL hash : {false, true})
	{
		if (hash ? (output->Size() == 0 || !CUtils::IsHashable(output) ||
					!CUtils::FComparisonPossible(output, IMDType::EcmptEq))
				 : (output->Size() > 0 &&
					!CUtils::FComparisonPossible(output, IMDType::EcmptL)))
		{
			continue;
		}
		auto *children = GPOS_NEW(mp) CExpressionArray(mp);
		for (ULONG i = 0; i < expr->Arity(); ++i)
		{
			(*expr)[i]->AddRef();
			children->Append((*expr)[i]);
		}
		output->AddRef();
		inputs->AddRef();
		result->Add(GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CPhysicalUnion(mp, output, inputs, hash),
			children));
	}
}
