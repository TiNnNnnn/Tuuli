#include "gpopt/xforms/CXformDSLRule_FullJoin.h"

#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/operators/CLogicalFullOuterJoin.h"
#include "gpopt/operators/CPatternTree.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

CXformDSLRule_FullJoin::CXformDSLRule_FullJoin(CMemoryPool *mp)
	: CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalFullOuterJoin(mp),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))))
{
}

CXform::EXformPromise
CXformDSLRule_FullJoin::Exfp(CExpressionHandle &) const
{
	CDSLRuleEngine *engine = CDSLRuleEngine::Instance();
	return GPOS_FTRACE(EopttracePreserveOpsForDSL) && nullptr != engine &&
			   0 < engine->PdrgpruleForRoot(
					   COperator::EopLogicalFullOuterJoin)->Size()
		? ExfpHigh
		: ExfpNone;
}

void
CXformDSLRule_FullJoin::Transform(CXformContext *context,
								  CXformResult *result,
								  CExpression *expr) const
{
	CDSLRuleEngine *engine = CDSLRuleEngine::Instance();
	GPOS_ASSERT(nullptr != engine);
	CDSLRuleArray *rules = engine->PdrgpruleCandidates(
		context->Pmp(), COperator::EopLogicalFullOuterJoin, expr);
	for (ULONG i = 0; i < rules->Size(); ++i)
	{
		CExpression *target = engine->PexprApply(context->Pmp(), (*rules)[i], expr);
		if (nullptr != target)
		{
			result->Add(target);
		}
	}
	rules->Release();
}
