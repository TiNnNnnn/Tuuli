//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//---------------------------------------------------------------------------
#include "gpopt/xforms/CXformDSLRule_UnionAll.h"

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/operators/CLogicalUnionAll.h"
#include "gpopt/operators/CPatternMultiTree.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

CXformDSLRule_UnionAll::CXformDSLRule_UnionAll(CMemoryPool *mp)
	: CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalUnionAll(mp),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternMultiTree(mp))))
{
}

CXform::EXformPromise
CXformDSLRule_UnionAll::Exfp(CExpressionHandle &) const
{
	if (!GPOS_FTRACE(EopttracePreserveOpsForDSL))
	{
		return CXform::ExfpNone;
	}
	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	return nullptr != peng &&
			   0 < peng->PdrgpruleForRoot(COperator::EopLogicalUnionAll)->Size()
		   ? CXform::ExfpHigh
		   : CXform::ExfpNone;
}

void
CXformDSLRule_UnionAll::Transform(CXformContext *pxfctxt,
								  CXformResult *pxfres,
								  CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();
	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	CDSLRuleArray *pdrgprule = peng->PdrgpruleCandidates(
		pxfctxt->Pmp(), COperator::EopLogicalUnionAll, pexpr);
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
