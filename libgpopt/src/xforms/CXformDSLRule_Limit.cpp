//---------------------------------------------------------------------------
//	MONSOON DSL rule shell for ORCA's fused order/limit boundary.
//---------------------------------------------------------------------------
#include "gpopt/xforms/CXformDSLRule_Limit.h"

#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CPatternTree.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

CXformDSLRule_Limit::CXformDSLRule_Limit(CMemoryPool *mp)
	: CXformExploration(
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalLimit(mp),
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)),
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)),
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))))
{
}

CXform::EXformPromise
CXformDSLRule_Limit::Exfp(CExpressionHandle &exprhdl) const
{
	if (!GPOS_FTRACE(EopttracePreserveOpsForDSL))
	{
		return ExfpNone;
	}
	CLogicalLimit *popLimit = CLogicalLimit::PopConvert(exprhdl.Pop());
	if (!popLimit->FGlobal() || popLimit->IsTopLimitUnderDMLorCTAS())
	{
		return ExfpNone;
	}
	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	return nullptr != peng
			&& 0 < peng->PdrgpruleForRoot(COperator::EopLogicalLimit)->Size()
		? ExfpHigh
		: ExfpNone;
}

void
CXformDSLRule_Limit::Transform(CXformContext *pxfctxt,
							   CXformResult *pxfres,
							   CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	GPOS_ASSERT(nullptr != peng);
	CDSLRuleArray *pdrgprule = peng->PdrgpruleCandidates(
		pxfctxt->Pmp(), COperator::EopLogicalLimit, pexpr);
	for (ULONG ul = 0; ul < pdrgprule->Size(); ul++)
	{
		CExpression *pexprTgt
			= peng->PexprApply(pxfctxt->Pmp(), (*pdrgprule)[ul], pexpr);
		if (nullptr != pexprTgt)
		{
			pxfres->Add(pexprTgt);
		}
	}
	pdrgprule->Release();
}

// EOF
