//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//---------------------------------------------------------------------------
#include "gpopt/xforms/CXformDSLRule_SetOp.h"

#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/operators/CLogicalDifference.h"
#include "gpopt/operators/CLogicalDifferenceAll.h"
#include "gpopt/operators/CLogicalIntersect.h"
#include "gpopt/operators/CLogicalIntersectAll.h"
#include "gpopt/operators/CPatternMultiTree.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

namespace
{
COperator *
PopSetOpPattern(CMemoryPool *mp, COperator::EOperatorId eopid)
{
	switch (eopid)
	{
		case COperator::EopLogicalIntersect:
			return GPOS_NEW(mp) CLogicalIntersect(mp);
		case COperator::EopLogicalIntersectAll:
			return GPOS_NEW(mp) CLogicalIntersectAll(mp);
		case COperator::EopLogicalDifference:
			return GPOS_NEW(mp) CLogicalDifference(mp);
		case COperator::EopLogicalDifferenceAll:
			return GPOS_NEW(mp) CLogicalDifferenceAll(mp);
		default:
			GPOS_ASSERT(!"unsupported DSL set-op root");
			return nullptr;
	}
}
}  // namespace

CXformDSLRule_SetOp::CXformDSLRule_SetOp(CMemoryPool *mp,
									 COperator::EOperatorId eopid,
									 EXformId exfid, const CHAR *szId)
	: CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, PopSetOpPattern(mp, eopid),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternMultiTree(mp)))),
	  m_exfid(exfid),
	  m_eopid(eopid),
	  m_szId(szId)
{
}

CXform::EXformPromise
CXformDSLRule_SetOp::Exfp(CExpressionHandle &) const
{
	if (!GPOS_FTRACE(EopttracePreserveOpsForDSL))
		return CXform::ExfpNone;
	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	return nullptr != peng && 0 < peng->PdrgpruleForRoot(m_eopid)->Size()
		? CXform::ExfpHigh
		: CXform::ExfpNone;
}

void
CXformDSLRule_SetOp::Transform(CXformContext *pxfctxt,
								CXformResult *pxfres,
								CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	CDSLRuleArray *pdrgprule =
		peng->PdrgpruleCandidates(pxfctxt->Pmp(), m_eopid, pexpr);
	for (ULONG ul = 0; ul < pdrgprule->Size(); ul++)
	{
		CExpression *pexprTgt =
			peng->PexprApply(pxfctxt->Pmp(), (*pdrgprule)[ul], pexpr);
		if (nullptr != pexprTgt)
			pxfres->Add(pexprTgt);
	}
	pdrgprule->Release();
}
