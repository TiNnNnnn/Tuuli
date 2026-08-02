//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CXformDSLRule_InnerJoin.cpp
//
//	@doc:
//		Implementation of the CLogicalInnerJoin DSL-rule shell.
//---------------------------------------------------------------------------
#include "gpopt/xforms/CXformDSLRule_InnerJoin.h"

#include "gpos/base.h"

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPatternTree.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CXformDSLRule_InnerJoin::CXformDSLRule_InnerJoin
//
//	@doc:
//		Ctor — loose pattern: InnerJoin(rel-leaf, rel-leaf, predicate-leaf).
//---------------------------------------------------------------------------
CXformDSLRule_InnerJoin::CXformDSLRule_InnerJoin(CMemoryPool *mp)
	: CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalInnerJoin(mp),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)),  // left
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)),  // right
		  GPOS_NEW(mp)
			  CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))  // join predicate
		  ))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformDSLRule_InnerJoin::Exfp
//
//	@doc:
//		Promise. High only when there is at least one rule rooted at
//		CLogicalInnerJoin; otherwise None so the engine skips this shell entirely.
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformDSLRule_InnerJoin::Exfp(CExpressionHandle &  // exprhdl
) const
{
	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	if (nullptr == peng ||
		0 == peng->PdrgpruleForRoot(COperator::EopLogicalInnerJoin)->Size())
	{
		return CXform::ExfpNone;
	}
	return CXform::ExfpHigh;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformDSLRule_InnerJoin::Transform
//
//	@doc:
//		Hand the expression to the engine; for each InnerJoin-rooted rule run the
//		three-stage decision (match && check && instantiate!=NULL) and add every
//		produced alternative. Instantiate re-roots any result so the alternative
//		handed to ORCA is always a freshly-built CExpression.
//---------------------------------------------------------------------------
void
CXformDSLRule_InnerJoin::Transform(CXformContext *pxfctxt,
								   CXformResult *pxfres,
								   CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();
	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	GPOS_ASSERT(nullptr != peng);  // Exfp gated on a non-null engine

	const CDSLRuleArray *pdrgprule =
		peng->PdrgpruleForRoot(COperator::EopLogicalInnerJoin);

	const ULONG ulRules = pdrgprule->Size();
	for (ULONG ul = 0; ul < ulRules; ul++)
	{
		const CDSLRule *prule = (*pdrgprule)[ul];

		CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
		if (peng->FMatch(prule, pexpr, pmodel) &&
			peng->FCheckConstraints(prule, pmodel, pexpr))
		{
			CExpression *pexprTgt = peng->PexprInstantiate(mp, prule, pmodel);
			if (nullptr != pexprTgt)
			{
				// trust chain: rule carries a WeTune EQ proof, so ORCA does not
				// re-verify equivalence (see design §七).
				pxfres->Add(pexprTgt);
			}
		}
		pmodel->Release();
	}
}

// EOF
