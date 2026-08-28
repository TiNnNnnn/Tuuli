//---------------------------------------------------------------------------
// Thin DSL shell for post-unnest ALL comparisons.
//---------------------------------------------------------------------------
#include "gpopt/xforms/CXformDSLRule_All.h"

#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CPatternNode.h"
#include "gpopt/operators/CPatternTree.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

CXformDSLRule_All::CXformDSLRule_All(CMemoryPool *mp)
	: CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CPatternNode(mp, CPatternNode::EmtMatchAllApply),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))))
{
}

CXform::EXformPromise
CXformDSLRule_All::Exfp(CExpressionHandle &exprhdl) const
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
CXformDSLRule_All::Transform(CXformContext *pxfctxt, CXformResult *pxfres,
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
