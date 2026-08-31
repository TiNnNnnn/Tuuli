//---------------------------------------------------------------------------
// Cascade shell routing MaxOneRow templates to the shared DSL engine.
//---------------------------------------------------------------------------
#include "gpopt/xforms/CXformDSLRule_MaxOneRow.h"

#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/operators/CLogicalMaxOneRow.h"
#include "gpopt/operators/CPatternTree.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

CXformDSLRule_MaxOneRow::CXformDSLRule_MaxOneRow(CMemoryPool *mp)
	: CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalMaxOneRow(mp),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))))
{
}

CXform::EXformPromise
CXformDSLRule_MaxOneRow::Exfp(CExpressionHandle &) const
{
	if (!GPOS_FTRACE(EopttracePreserveOpsForDSL))
	{
		return ExfpNone;
	}
	CDSLRuleEngine *engine = CDSLRuleEngine::Instance();
	return nullptr != engine && engine->FHasEnabledCBORuleForRoot(
								  COperator::EopLogicalMaxOneRow)
		   ? ExfpHigh
		   : ExfpNone;
}

void
CXformDSLRule_MaxOneRow::Transform(CXformContext *context,
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
		mp, COperator::EopLogicalMaxOneRow, expression);
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
