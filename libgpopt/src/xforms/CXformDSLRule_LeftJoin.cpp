//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CXformDSLRule_LeftJoin.cpp
//
//	@doc:
//		Implementation of the CLogicalLeftOuterJoin DSL-rule shell.
//---------------------------------------------------------------------------
#include "gpopt/xforms/CXformDSLRule_LeftJoin.h"

#include "gpos/base.h"

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CPatternTree.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CXformDSLRule_LeftJoin::CXformDSLRule_LeftJoin
//
//	@doc:
//		Ctor — pattern: LeftOuterJoin(rel-tree, rel-tree, predicate-tree). All
//		three children are CPatternTree so the memo binder MATERIALIZES both
//		relational subtrees and the predicate; a CPatternLeaf relational child
//		would bind an arity-0 stub and a rule nesting below the join (or the join
//		matcher's predicate split) could never see it.
//---------------------------------------------------------------------------
CXformDSLRule_LeftJoin::CXformDSLRule_LeftJoin(CMemoryPool *mp)
	: CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalLeftOuterJoin(mp),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)),  // left
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)),  // right
		  GPOS_NEW(mp)
			  CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))  // join predicate
		  ))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformDSLRule_LeftJoin::Exfp
//
//	@doc:
//		Promise. High only when there is at least one rule rooted at
//		CLogicalLeftOuterJoin; otherwise None so the engine skips this shell.
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformDSLRule_LeftJoin::Exfp(CExpressionHandle &  // exprhdl
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
		0 ==
			peng->PdrgpruleForRoot(COperator::EopLogicalLeftOuterJoin)->Size())
	{
		return CXform::ExfpNone;
	}
	return CXform::ExfpHigh;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformDSLRule_LeftJoin::Transform
//
//	@doc:
//		Hand the expression to the engine; for each LeftJoin-rooted rule run the
//		three-stage decision (match && check && instantiate!=NULL) and add every
//		produced alternative. Instantiate re-roots any result so the alternative
//		handed to ORCA is always a freshly-built CExpression.
//---------------------------------------------------------------------------
void
CXformDSLRule_LeftJoin::Transform(CXformContext *pxfctxt,
								  CXformResult *pxfres,
								  CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();
	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	GPOS_ASSERT(nullptr != peng);  // Exfp gated on a non-null engine

	CDSLRuleArray *pdrgprule = peng->PdrgpruleCandidates(
		mp, COperator::EopLogicalLeftOuterJoin, pexpr);

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
