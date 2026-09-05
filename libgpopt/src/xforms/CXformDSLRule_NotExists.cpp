//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//---------------------------------------------------------------------------
#include "gpopt/xforms/CXformDSLRule_NotExists.h"

#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CPatternNode.h"
#include "gpopt/operators/CPatternTree.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

CXformDSLRule_NotExists::CXformDSLRule_NotExists(CMemoryPool *mp)
	: CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CPatternNode(
				  mp, CPatternNode::EmtMatchNotExistsApplyOrAntiSemiJoin),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))))
{
}

CXform::EXformPromise
CXformDSLRule_NotExists::Exfp(CExpressionHandle &exprhdl) const
{
	if (!GPOS_FTRACE(EopttracePreserveOpsForDSL))
	{
		return CXform::ExfpNone;
	}

	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	if (nullptr == peng ||
		0 == peng->PdrgpruleForRoot(exprhdl.Pop()->Eopid())->Size())
	{
		return CXform::ExfpNone;
	}
	return CXform::ExfpHigh;
}

void
CXformDSLRule_NotExists::Transform(CXformContext *pxfctxt,
								   CXformResult *pxfres,
								   CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();
	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	GPOS_ASSERT(nullptr != peng);

	CDSLRuleArray *pdrgprule =
		peng->PdrgpruleCandidates(mp, pexpr->Pop()->Eopid(), pexpr);
	for (ULONG ul = 0; ul < pdrgprule->Size(); ul++)
	{
		CExpression *pexprTgt =
			peng->PexprApply(mp, (*pdrgprule)[ul], pexpr);
		if (nullptr != pexprTgt)
		{
			pxfres->Add(pexprTgt);
		}
	}
	pdrgprule->Release();
}

// EOF
