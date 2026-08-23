//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CXformDSLRule_Exists.cpp
//---------------------------------------------------------------------------
#include "gpopt/xforms/CXformDSLRule_Exists.h"

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/operators/CLogicalLeftSemiApply.h"
#include "gpopt/operators/CPatternTree.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

CXformDSLRule_Exists::CXformDSLRule_Exists(CMemoryPool *mp)
	: CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalLeftSemiApply(mp),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))))
{
}

CXform::EXformPromise
CXformDSLRule_Exists::Exfp(CExpressionHandle &) const
{
	if (!GPOS_FTRACE(EopttracePreserveOpsForDSL))
	{
		return CXform::ExfpNone;
	}

	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	if (nullptr == peng ||
		0 == peng->PdrgpruleForRoot(
					 COperator::EopLogicalLeftSemiApply)->Size())
	{
		return CXform::ExfpNone;
	}
	return CXform::ExfpHigh;
}

void
CXformDSLRule_Exists::Transform(CXformContext *pxfctxt,
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
		mp, COperator::EopLogicalLeftSemiApply, pexpr);
	for (ULONG ul = 0; ul < pdrgprule->Size(); ul++)
	{
		const CDSLRule *prule = (*pdrgprule)[ul];
		CExpression *pexprTgt = peng->PexprApply(mp, prule, pexpr);
		if (nullptr != pexprTgt)
		{
			pxfres->Add(pexprTgt);
		}
	}
	pdrgprule->Release();
}

// EOF
