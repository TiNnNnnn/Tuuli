//---------------------------------------------------------------------------
// Thin Cascade shell routing InnerApply-rooted expressions to the DSL engine.
//---------------------------------------------------------------------------
#include "gpopt/xforms/CXformDSLRule_InnerApply.h"

#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/operators/CLogicalInnerApply.h"
#include "gpopt/operators/CPatternTree.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

CXformDSLRule_InnerApply::CXformDSLRule_InnerApply(CMemoryPool *mp)
	: CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalInnerApply(mp),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))))
{
}

CXform::EXformPromise
CXformDSLRule_InnerApply::Exfp(CExpressionHandle &) const
{
	if (!GPOS_FTRACE(EopttracePreserveOpsForDSL))
	{
		return CXform::ExfpNone;
	}

	CDSLRuleEngine *engine = CDSLRuleEngine::Instance();
	return nullptr != engine &&
			   0 < engine->PdrgpruleForRoot(
						 COperator::EopLogicalInnerApply)->Size()
		   ? CXform::ExfpHigh
		   : CXform::ExfpNone;
}

void
CXformDSLRule_InnerApply::Transform(CXformContext *context,
									CXformResult *result,
									CExpression *expression) const
{
	GPOS_ASSERT(nullptr != context);
	GPOS_ASSERT(FPromising(context->Pmp(), this, expression));
	GPOS_ASSERT(FCheckPattern(expression));

	CMemoryPool *mp = context->Pmp();
	CDSLRuleEngine *engine = CDSLRuleEngine::Instance();
	GPOS_ASSERT(nullptr != engine);
	CDSLRuleArray *rules = engine->PdrgpruleCandidates(
		mp, COperator::EopLogicalInnerApply, expression);
	for (ULONG index = 0; index < rules->Size(); ++index)
	{
		CExpression *target =
			engine->PexprApply(mp, (*rules)[index], expression);
		if (nullptr != target)
		{
			result->Add(target);
		}
	}
	rules->Release();
}

// EOF
