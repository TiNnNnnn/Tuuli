//---------------------------------------------------------------------------
// Thin DSL shell for CTE consumers.
//---------------------------------------------------------------------------
#include "gpopt/xforms/CXformDSLRule_CTEConsumer.h"

#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalCTEConsumer.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

CXformDSLRule_CTEConsumer::CXformDSLRule_CTEConsumer(CMemoryPool *mp)
	: CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalCTEConsumer(mp)))
{
}

CXform::EXformPromise
CXformDSLRule_CTEConsumer::Exfp(CExpressionHandle &exprhdl) const
{
	if (!GPOS_FTRACE(EopttracePreserveOpsForDSL))
	{
		return CXform::ExfpNone;
	}
	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	return nullptr != peng &&
			   0 < peng->PdrgpruleForRoot(exprhdl.Pop()->Eopid())->Size()
		   ? CXform::ExfpHigh
		   : CXform::ExfpNone;
}

void
CXformDSLRule_CTEConsumer::Transform(CXformContext *pxfctxt,
									 CXformResult *pxfres,
									 CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));
	CMemoryPool *mp = pxfctxt->Pmp();
	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	GPOS_ASSERT(nullptr != peng);
	CDSLRuleArray *pdrgprule = peng->PdrgpruleCandidates(
		mp, pexpr->Pop()->Eopid(), pexpr);
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
