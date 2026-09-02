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

#include "gpopt/dsl/CDSLExpressionDefinitions.h"
#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CPatternTree.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CXformDSLRule_Select::CXformDSLRule_Select
//
//	@doc:
//		Ctor — pattern: Select(relational-tree, scalar-tree). Both children are
//		CPatternTree (not CPatternLeaf) so the memo binder MATERIALIZES the whole
//		relational subtree and the predicate, letting a DSL rule of ANY depth
//		rooted at Select (e.g. Select(Join(...)) ) recurse into them. A leaf would
//		bind an unmaterialized group stub (arity 0) and a nested rule could never
//		match. See CXformExpandFullOuterJoin for the same tree-on-relational-child
//		idiom.
//---------------------------------------------------------------------------
CXformDSLRule_Select::CXformDSLRule_Select(CMemoryPool *mp)
	: CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalSelect(mp),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)),  // rel
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))	// pred
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
	// master switch: pg_orca.enable_dsl_rule (trace flag EopttracePreserveOpsForDSL)
	// gates whether DSL rules fire at all. Off => behave as native ORCA.
	if (!GPOS_FTRACE(EopttracePreserveOpsForDSL))
	{
		return CXform::ExfpNone;
	}

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

	CDSLRuleArray *pdrgprule = peng->PdrgpruleCandidates(
		mp, COperator::EopLogicalSelect, pexpr);

	const ULONG ulRules = pdrgprule->Size();
	for (ULONG ul = 0; ul < ulRules; ul++)
	{
		const CDSLRule *prule = (*pdrgprule)[ul];
		const CDSLOp *popSrcRoot = prule->PfragSrc()->PopRoot();
		const CDSLExpressionDefinitions *pexprdefs = prule->Pexprdefs();
		BOOL fNestedAll = false;
		if (1 < pexprdefs->UlDefinitions())
		{
			for (ULONG ulDef = 0; ulDef < pexprdefs->UlDefinitions(); ulDef++)
			{
				fNestedAll = fNestedAll ||
					EdslexprExprListAll == pexprdefs->PdefAt(ulDef)->Edslexpr();
			}
		}

		// Subquery-filter operators can match both the translated Select/scalar-
		// subquery form and the canonical Apply form. Running the same rule in both
		// phases creates competing alternatives for one memo group. When native
		// unnesting is available, defer every such operator to its Apply shell;
		// when it is disabled, retain the Select path so DSL can replace unnesting.
		// Representation adapters that cross EXISTS/IN forms are also registered in
		// the corresponding Apply bucket by CDSLRuleEngine::BucketByRoot. Native ALL
		// lowering also owns nested definition chains because its intermediate
		// projection columns cannot share a memo group with the atomic DSL chain.
		if (GPOPT_FENABLED_XFORM(CXform::ExfSelect2Apply) &&
			(CDSLOpKindTable::FHasPreUnnestRepresentation(
				 popSrcRoot->Edslop()) ||
			 fNestedAll))
		{
			continue;
		}

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
