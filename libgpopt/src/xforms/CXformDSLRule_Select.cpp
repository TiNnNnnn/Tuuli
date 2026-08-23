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
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogicalFullOuterJoin.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CPatternTree.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

namespace
{
// Apply a LeftJoin-rooted data rule to the equivalent view exposed by a
// null-rejecting Select over FullJoin. The outer Select is not part of the DSL
// source fragment, so retain its predicate around the instantiated target and
// admit the alternative only when it preserves the memo group's output schema.
void
ApplyLeftJoinRuleOverFullJoinSelect(CMemoryPool *mp, CDSLRuleEngine *peng,
								const CDSLRule *prule,
								CExpression *pexprSelect,
								CXformResult *pxfres)
{
	GPOS_ASSERT(COperator::EopLogicalSelect == pexprSelect->Pop()->Eopid());
	if (2 != pexprSelect->Arity())
	{
		return;
	}

	CExpression *pexprFullJoin = (*pexprSelect)[0];
	CExpression *pexprPred = (*pexprSelect)[1];
	if (COperator::EopLogicalFullOuterJoin !=
			pexprFullJoin->Pop()->Eopid() ||
		3 != pexprFullJoin->Arity())
	{
		return;
	}

	for (ULONG ulOuter = 0; ulOuter < 2; ulOuter++)
	{
		CExpression *pexprOuter = (*pexprFullJoin)[ulOuter];
		if (!CPredicateUtils::FNullRejecting(
				mp, pexprPred, pexprOuter->DeriveOutputColumns()))
		{
			continue;
		}

		CExpression *pexprInner = (*pexprFullJoin)[1 - ulOuter];
		CExpression *pexprJoinPred = (*pexprFullJoin)[2];
		pexprOuter->AddRef();
		pexprInner->AddRef();
		pexprJoinPred->AddRef();
		CExpression *pexprLeftJoin = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CLogicalLeftOuterJoin(mp), pexprOuter,
			pexprInner, pexprJoinPred);

		CExpression *pexprTarget = peng->PexprApply(mp, prule, pexprLeftJoin);
		pexprLeftJoin->Release();
		if (nullptr == pexprTarget)
		{
			continue;
		}

		pexprPred->AddRef();
		CExpression *pexprWrapped =
			CUtils::PexprSafeSelect(mp, pexprTarget, pexprPred);
		if (pexprWrapped->DeriveOutputColumns()->Equals(
				pexprSelect->DeriveOutputColumns()))
		{
			pxfres->Add(pexprWrapped);
		}
		else
		{
			pexprWrapped->Release();
		}
	}
}
}  // namespace

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

		if (EdslopLeftJoin == popSrcRoot->Edslop())
		{
			ApplyLeftJoinRuleOverFullJoinSelect(mp, peng, prule, pexpr,
										pxfres);
			continue;
		}

		// Subquery-filter operators can match both the translated Select/scalar-
		// subquery form and the canonical Apply form. Running the same rule in both
		// phases creates competing alternatives for one memo group. When native
		// unnesting is available, defer every such operator to its Apply shell;
		// when it is disabled, retain the Select path so DSL can replace unnesting.
		// Representation adapters that cross EXISTS/IN forms are also registered in
		// the corresponding Apply bucket by CDSLRuleEngine::BucketByRoot.
		if (GPOPT_FENABLED_XFORM(CXform::ExfSelect2Apply) &&
			CDSLOpKindTable::FHasPreUnnestRepresentation(
				popSrcRoot->Edslop()))
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
