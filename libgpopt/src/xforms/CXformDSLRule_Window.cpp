//---------------------------------------------------------------------------
// Cascade shell routing SequenceProject/Window templates to the DSL engine.
//---------------------------------------------------------------------------
#include "gpopt/xforms/CXformDSLRule_Window.h"

#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/operators/CLogicalSequenceProject.h"
#include "gpopt/operators/CPatternTree.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

CXformDSLRule_Window::CXformDSLRule_Window(CMemoryPool *mp)
	: CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalSequenceProject(mp),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))))
{
}

CXform::EXformPromise
CXformDSLRule_Window::Exfp(CExpressionHandle &) const
{
	if (!GPOS_FTRACE(EopttracePreserveOpsForDSL))
	{
		return ExfpNone;
	}
	CDSLRuleEngine *engine = CDSLRuleEngine::Instance();
	return nullptr != engine && engine->FHasEnabledCBORuleForRoot(
								  COperator::EopLogicalSequenceProject)
		   ? ExfpHigh
		   : ExfpNone;
}

void
CXformDSLRule_Window::Transform(CXformContext *context, CXformResult *result,
								 CExpression *expression) const
{
	GPOS_ASSERT(nullptr != context);
	GPOS_ASSERT(FPromising(context->Pmp(), this, expression));
	GPOS_ASSERT(FCheckPattern(expression));

	CMemoryPool *mp = context->Pmp();
	CDSLRuleEngine *engine = CDSLRuleEngine::Instance();
	GPOS_ASSERT(nullptr != engine);
	CDSLRuleArray *rules = engine->PdrgpruleCandidates(
		mp, COperator::EopLogicalSequenceProject, expression);
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
