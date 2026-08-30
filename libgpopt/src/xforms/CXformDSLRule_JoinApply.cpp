//---------------------------------------------------------------------------
// Thin Cascade shell routing join-producing Apply expressions to the DSL engine.
//---------------------------------------------------------------------------
#include "gpopt/xforms/CXformDSLRule_JoinApply.h"

#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CPatternNode.h"
#include "gpopt/operators/CPatternTree.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

CXformDSLRule_JoinApply::CXformDSLRule_JoinApply(CMemoryPool *mp)
	: CXformExploration(GPOS_NEW(mp) CExpression(
		  mp,
		  GPOS_NEW(mp) CPatternNode(mp, CPatternNode::EmtMatchJoinApply),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))))
{
}

CXform::EXformPromise
CXformDSLRule_JoinApply::Exfp(CExpressionHandle &exprhdl) const
{
	if (!GPOS_FTRACE(EopttracePreserveOpsForDSL))
	{
		return CXform::ExfpNone;
	}
	CDSLRuleEngine *engine = CDSLRuleEngine::Instance();
	return nullptr != engine &&
			   0 < engine->PdrgpruleForRoot(exprhdl.Pop()->Eopid())->Size()
		   ? CXform::ExfpHigh
		   : CXform::ExfpNone;
}

void
CXformDSLRule_JoinApply::Transform(CXformContext *context,
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
		mp, expression->Pop()->Eopid(), expression);
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
