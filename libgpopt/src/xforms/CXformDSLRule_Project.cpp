//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CXformDSLRule_Project.cpp
//
//	@doc:
//		Implementation of the CLogicalProject DSL-rule shell.
//---------------------------------------------------------------------------
#include "gpopt/xforms/CXformDSLRule_Project.h"

#include "gpos/base.h"

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CPatternTree.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CXformDSLRule_Project::CXformDSLRule_Project
//
//	@doc:
//		Ctor — pattern: Project(relational-tree, project-list-tree). Both children
//		are CPatternTree so the memo binder MATERIALIZES the whole relational
//		subtree AND the project list, letting a DSL rule of ANY depth rooted at
//		Project recurse into them — in particular the join-elimination rule
//		Proj(Join(Input,Input)) -> Proj(Input) (WeTune rules.txt 180/205), whose
//		Join child a CPatternLeaf would leave as an unmaterialized arity-0 stub.
//---------------------------------------------------------------------------
CXformDSLRule_Project::CXformDSLRule_Project(CMemoryPool *mp)
	: CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalProject(mp),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)),  // rel
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))	// prjl
		  ))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformDSLRule_Project::Exfp
//
//	@doc:
//		Promise. High only when there is at least one rule rooted at
//		CLogicalProject; otherwise None so the engine skips this shell entirely.
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformDSLRule_Project::Exfp(CExpressionHandle &	 // exprhdl
) const
{
	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	if (nullptr == peng ||
		0 == peng->PdrgpruleForRoot(COperator::EopLogicalProject)->Size())
	{
		return CXform::ExfpNone;
	}
	return CXform::ExfpHigh;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformDSLRule_Project::Transform
//
//	@doc:
//		Hand the expression to the engine; for each Project-rooted rule run the
//		three-stage decision (match && check && instantiate!=NULL) and add every
//		produced alternative. Instantiate re-roots any operator-eliminating result
//		so the alternative handed to ORCA is always a freshly-built CExpression.
//---------------------------------------------------------------------------
void
CXformDSLRule_Project::Transform(CXformContext *pxfctxt, CXformResult *pxfres,
								 CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();
	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	GPOS_ASSERT(nullptr != peng);  // Exfp gated on a non-null engine

	const CDSLRuleArray *pdrgprule =
		peng->PdrgpruleForRoot(COperator::EopLogicalProject);

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
