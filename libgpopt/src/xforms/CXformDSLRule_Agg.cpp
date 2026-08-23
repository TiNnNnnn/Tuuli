//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CXformDSLRule_Agg.cpp
//
//	@doc:
//		Implementation of the CLogicalGbAgg DSL-rule shell.
//---------------------------------------------------------------------------
#include "gpopt/xforms/CXformDSLRule_Agg.h"

#include "gpos/base.h"

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CPatternTree.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CXformDSLRule_Agg::CXformDSLRule_Agg
//
//	@doc:
//		Ctor — pattern: GbAgg(relational-tree, agg-project-list-tree). Both children
//		are CPatternTree so the memo binder MATERIALIZES the whole relational
//		subtree AND the aggregate project list, letting the engine inspect the agg
//		list (a pure dedup has an empty one) and recurse the relational child. A
//		CPatternLeaf would leave arity-0 stubs — same lesson as the join/proj shells.
//---------------------------------------------------------------------------
CXformDSLRule_Agg::CXformDSLRule_Agg(CMemoryPool *mp)
	: CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalGbAgg(mp),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)),  // rel
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))	// agg list
		  ))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformDSLRule_Agg::Exfp
//
//	@doc:
//		Promise. High only when there is at least one rule rooted at
//		CLogicalGbAgg; otherwise None so the engine skips this shell entirely.
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformDSLRule_Agg::Exfp(CExpressionHandle &	 // exprhdl
) const
{
	// master switch: pg_orca.enable_dsl_rule (trace flag EopttracePreserveOpsForDSL)
	// gates whether DSL rules fire at all. Off => behave as native ORCA.
	if (!GPOS_FTRACE(EopttracePreserveOpsForDSL))
	{
		return CXform::ExfpNone;
	}

	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	if (nullptr == peng ||
		0 == peng->PdrgpruleForRoot(COperator::EopLogicalGbAgg)->Size())
	{
		return CXform::ExfpNone;
	}
	return CXform::ExfpHigh;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformDSLRule_Agg::Transform
//
//	@doc:
//		Hand the expression to the engine; for each GbAgg-rooted rule run the
//		three-stage decision (match && check && instantiate!=NULL) and add every
//		produced alternative. Instantiate re-roots any operator-eliminating result
//		so the alternative handed to ORCA is always a freshly-built CExpression.
//---------------------------------------------------------------------------
void
CXformDSLRule_Agg::Transform(CXformContext *pxfctxt, CXformResult *pxfres,
							 CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();
	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	GPOS_ASSERT(nullptr != peng);  // Exfp gated on a non-null engine

	CDSLRuleArray *pdrgprule = peng->PdrgpruleCandidates(
		mp, COperator::EopLogicalGbAgg, pexpr);

	const ULONG ulRules = pdrgprule->Size();
	for (ULONG ul = 0; ul < ulRules; ul++)
	{
		const CDSLRule *prule = (*pdrgprule)[ul];

		CExpression *pexprTgt = peng->PexprApply(mp, prule, pexpr);
		if (nullptr != pexprTgt)
		{
			// trust chain: rule carries a WeTune EQ proof, so ORCA does not
			// re-verify equivalence (see design §七).
			pxfres->Add(pexprTgt);
		}
	}
	pdrgprule->Release();
}

// EOF
