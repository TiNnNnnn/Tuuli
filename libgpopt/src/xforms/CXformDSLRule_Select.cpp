//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CXformDSLRule_Select.cpp
//
//	@doc:
//		Implementation of the CLogicalSelect DSL-rule shell.
//---------------------------------------------------------------------------
#include "gpopt/xforms/CXformDSLRule_Select.h"

#include "gpos/base.h"

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CPatternLeaf.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CXformDSLRule_Select::CXformDSLRule_Select
//
//	@doc:
//		Ctor — loose pattern: Select(relational-leaf, scalar-leaf).
//---------------------------------------------------------------------------
CXformDSLRule_Select::CXformDSLRule_Select(CMemoryPool *mp)
	: CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalSelect(mp),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)),  // rel
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp))	// pred
		  ))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformDSLRule_Select::Exfp
//
//	@doc:
//		Promise. High only when there is at least one rule rooted at
//		CLogicalSelect; otherwise None so the engine skips this shell entirely.
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformDSLRule_Select::Exfp(CExpressionHandle &	// exprhdl
) const
{
	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	if (nullptr == peng ||
		0 == peng->PdrgpruleForRoot(COperator::EopLogicalSelect)->Size())
	{
		return CXform::ExfpNone;
	}
	return CXform::ExfpHigh;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformDSLRule_Select::Transform
//
//	@doc:
//		Hand the expression to the engine; for each Select-rooted rule run the
//		three-stage decision (match && check && instantiate!=NULL) and add every
//		produced alternative. Instantiate re-roots any operator-eliminating result
//		so the alternative handed to ORCA is always a freshly-built CExpression.
//---------------------------------------------------------------------------
void
CXformDSLRule_Select::Transform(CXformContext *pxfctxt, CXformResult *pxfres,
								CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();
	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	GPOS_ASSERT(nullptr != peng);  // Exfp gated on a non-null engine

	const CDSLRuleArray *pdrgprule =
		peng->PdrgpruleForRoot(COperator::EopLogicalSelect);

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
