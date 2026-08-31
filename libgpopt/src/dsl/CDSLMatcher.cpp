//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLMatcher.cpp
//
//	@doc:
//		Implementation of the generic recursive matcher (see CDSLMatcher.h).
//		Mirrors WeTune Match.matchOne's dispatch skeleton.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLMatcher.h"

#include "gpos/base.h"

#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/dsl/CDSLAggMatcher.h"
#include "gpopt/dsl/CDSLExistsMatcher.h"
#include "gpopt/dsl/CDSLFilterMatcher.h"
#include "gpopt/dsl/CDSLInSubMatcher.h"
#include "gpopt/dsl/CDSLJoinMatcher.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/dsl/CDSLProjMatcher.h"
#include "gpopt/dsl/CDSLQuantifiedMatcher.h"
#include "gpopt/dsl/CDSLUnionMatcher.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/operators/CLogicalConstTableGet.h"
#include "naucrates/md/IMDType.h"

using namespace gpopt;

namespace
{
BOOL
FDefaultOrderDirection(const COrderSpec *pos, EDslSortDir edslsort)
{
	if (nullptr == pos || pos->IsEmpty() || EdslsortNone == edslsort)
	{
		return false;
	}

	const IMDType::ECmpType ecmpt =
		(EdslsortAsc == edslsort) ? IMDType::EcmptL : IMDType::EcmptG;
	const COrderSpec::ENullTreatment ent =
		(EdslsortAsc == edslsort) ? COrderSpec::EntLast
								  : COrderSpec::EntFirst;
	for (ULONG ul = 0; ul < pos->UlSortColumns(); ul++)
	{
		const CColRef *pcr = pos->Pcr(ul);
		IMDId *pmdid = pcr->RetrieveType()->GetMdidForCmpType(ecmpt);
		if (!IMDId::IsValid(pmdid) ||
			!pmdid->Equals(pos->GetMdIdSortOp(ul)) || ent != pos->Ent(ul))
		{
			return false;
		}
	}
	return true;
}

}  // namespace

BOOL
CDSLMatcher::FMatchSortView(const CDSLOp *popSort,
							CExpression *pexprChild, const COrderSpec *pos,
							CDSLModel *pmodel) const
{
	GPOS_ASSERT(EdslopSort == popSort->Edslop());
	if (1 != popSort->UlChildren() || nullptr == popSort->Pdrgpsym() ||
		1 != popSort->Pdrgpsym()->Size() ||
		!FDefaultOrderDirection(pos, popSort->Edslsort()))
	{
		return false;
	}

	CColRefArray *pdrgpcr = GPOS_NEW(m_mp) CColRefArray(m_mp);
	for (ULONG ul = 0; ul < pos->UlSortColumns(); ul++)
	{
		pdrgpcr->Append(const_cast<CColRef *>(pos->Pcr(ul)));
	}
	BOOL fBound = pmodel->FBind((*popSort->Pdrgpsym())[0], pdrgpcr);
	pdrgpcr->Release();
	return fBound && FMatch((*popSort)[0], pexprChild, pmodel);
}

BOOL
CDSLMatcher::FMatchOrderLimit(const CDSLOp *pop, CExpression *pexpr,
							  CDSLModel *pmodel) const
{
	CDSLMatchView::SOrderLimit view;
	if (!CDSLMatchView::FOrderLimit(pexpr, &view))
	{
		return false;
	}

	if (EdslopSort == pop->Edslop())
	{
		return !view.m_fHasLimit &&
			FMatchSortView(pop, view.m_pexprChild, view.m_pos, pmodel);
	}

	GPOS_ASSERT(EdslopLimit == pop->Edslop());
	if (!view.m_fHasLimit || 1 != pop->UlChildren() ||
		nullptr == pop->Pdrgpsym() || 2 != pop->Pdrgpsym()->Size())
	{
		return false;
	}

	// DSL positional order is Limit<count offset>; ORCA child order is
	// relational, offset, count.
	if (!pmodel->FBind((*pop->Pdrgpsym())[0], view.m_pexprCount) ||
		!pmodel->FBind((*pop->Pdrgpsym())[1], view.m_pexprOffset))
	{
		return false;
	}

	const CDSLOp *popChild = (*pop)[0];
	if (EdslopSort == popChild->Edslop())
	{
		// One fused ORCA node is the canonical view Limit(Sort(child)).
		return FMatchSortView(popChild, view.m_pexprChild, view.m_pos, pmodel);
	}

	// A plain DSL Limit carries no ordering. Do not silently consume an order
	// property which the target side would be unable to reconstruct.
	return view.m_pos->IsEmpty() &&
		   FMatch(popChild, view.m_pexprChild, pmodel);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLMatcher::FMatchInput
//
//	@doc:
//		Input<t> is an opaque table placeholder. WeTune's INPUT branch binds the
//		table symbol to whatever plan node sits there WITHOUT checking its type
//		— any relational subtree qualifies. So we bind the single <t> symbol to
//		the whole pexpr subtree (FBind AddRefs it). If <t> is already bound (same
//		symbol appears again under an equality class), FBind enforces it points at
//		the SAME subtree.
//---------------------------------------------------------------------------
BOOL
CDSLMatcher::FMatchInput(const CDSLOp *pop, CExpression *pexpr,
						 CDSLModel *pmodel) const
{
	GPOS_ASSERT(EdslopInput == pop->Edslop());

	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	// Input declares exactly one table symbol <t> (validated at parse time).
	if (nullptr == pdrgpsym || 1 != pdrgpsym->Size())
	{
		return false;
	}

	const CDSLSymbol *psymTable = (*pdrgpsym)[0];
	return pmodel->FBind(psymTable, pexpr);
}

BOOL
CDSLMatcher::FMatchEmpty(const CDSLOp *pop, CExpression *pexpr,
					 CDSLModel *pmodel) const
{
	GPOS_ASSERT(EdslopEmpty == pop->Edslop());
	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	if (nullptr == pdrgpsym || 1 != pdrgpsym->Size() ||
		COperator::EopLogicalConstTableGet != pexpr->Pop()->Eopid() ||
		0 != pexpr->Arity())
	{
		return false;
	}
	CLogicalConstTableGet *popConst =
		CLogicalConstTableGet::PopConvert(pexpr->Pop());
	return 0 == popConst->Pdrgpdrgpdatum()->Size() &&
		pmodel->FBind((*pdrgpsym)[0], pexpr);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLMatcher::FBindOpSymbols
//
//	@doc:
//		Bind the positional symbols of a non-Input operator against pexpr.
//
//		The generic skeleton knows how to bind NOTHING structural on its own:
//		  * symbol-free operators (Union / Exists) bind nothing here — success.
//		  * InnerJoin / LeftJoin / Proj / Agg carry symbols whose binding needs
//		    operator-specific structural knowledge (join-key extraction,
//		    project-list / group-by columns). Those are delegated to dedicated
//		    binding in a later component (#27):
//		        Join    <a a>            -> join-key binding
//		        Proj    <a s>            -> attrs/schema binding
//		        Agg     <a a f s p>      -> agg symbol binding
//		  * Filter <p a> is NOT handled here at all — it is intercepted earlier in
//		    FMatch and routed to CDSLFilterMatcher (#25), because a DSL Filter
//		    chain maps to a single ORCA Select, not a per-node match.
//
//		Until #27 lands this is a NO-OP seam: an operator with symbols still
//		matches structurally (identity + children), it simply leaves those
//		symbols unbound. That is deliberately safe for the skeleton's own tests
//		(Input, Union, bare identity) and is filled in by the later component
//		without touching the recursion here.
//---------------------------------------------------------------------------
BOOL
CDSLMatcher::FBindOpSymbols(const CDSLOp *pop,
							CExpression *,	// pexpr
							CDSLModel *		// pmodel
) const
{
	const ULONG ulSyms =
		(nullptr == pop->Pdrgpsym()) ? 0 : pop->Pdrgpsym()->Size();
	if (0 == ulSyms)
	{
		// Union / Exists and friends: purely structural, nothing to bind.
		return true;
	}

	// Operator carries symbols but no collaborator has claimed it yet. The
	// skeleton leaves them unbound (see doc). This is intentionally permissive;
	// #25/#27 replace this with real binding.
	return true;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLMatcher::FMatchChildren
//
//	@doc:
//		Match the DSL op's relational children positionally. Every ORCA logical
//		operator we map to lays out its relational inputs FIRST (indices
//		[0, UlChildren)) with scalar children (predicates, project lists) after;
//		the DSL op's UlChildren() is exactly that relational arity (WeTune
//		OpKind.numPredecessors). Scalar children are consumed by symbol binding,
//		not walked here.
//---------------------------------------------------------------------------
BOOL
CDSLMatcher::FMatchChildren(const CDSLOp *pop, CExpression *pexpr,
							CDSLModel *pmodel) const
{
	const ULONG ulChildren = pop->UlChildren();

	// the live expression must have at least as many children as the template
	// has relational children (it may have more: trailing scalar children).
	if (pexpr->Arity() < ulChildren)
	{
		return false;
	}

	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		if (!FMatch((*pop)[ul], (*pexpr)[ul], pmodel))
		{
			return false;
		}
	}
	return true;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLMatcher::FMatch
//
//	@doc:
//		Match one DSL op subtree against one live expression. Dispatch mirrors
//		WeTune Match.matchOne: Input is the opaque leaf; everything else is an
//		operator-identity gate followed by symbol binding + child recursion.
//---------------------------------------------------------------------------
BOOL
CDSLMatcher::FMatch(const CDSLOp *pop, CExpression *pexpr,
					CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != pop);
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != pmodel);

	// Input<t>: opaque subtree placeholder — bind and stop (no identity check,
	// no child recursion).
	if (EdslopInput == pop->Edslop())
	{
		return FMatchInput(pop, pexpr, pmodel);
	}
	if (EdslopEmpty == pop->Edslop())
	{
		return FMatchEmpty(pop, pexpr, pmodel);
	}

	// Filter chain: a DSL single-predicate Filter (chain) matches ONE ORCA
	// CLogicalSelect whose conjunctive predicate is split into a conjunct set.
	// Delegated to the filter matcher, which recurses the chain's base op back
	// into this matcher (see CDSLFilterMatcher). This is the hardest mismatch
	// (doc §2) and is why Filter does not go through the generic child recursion.
	if (EdslopFilter == pop->Edslop())
	{
		CDSLFilterMatcher fm(m_mp, this, m_prule);
		return fm.FMatch(pop, pexpr, pmodel);
	}

	// Proj<a s>: bind the projected-column symbols against the live
	// CLogicalProject's project list, then recurse the relational child. Like
	// Filter, Proj carries scalar structure (the project list) that only
	// operator-specific code reads, so it does not go through generic child
	// recursion (see CDSLProjMatcher, doc M1).
	//
	// Proj* (deduplicated projection) has NO CLogicalProject counterpart in ORCA
	// — SELECT DISTINCT becomes a CLogicalGbAgg (empty agg list). So a DISTINCT
	// Proj routes to the Agg matcher instead; a plain Proj to the Proj matcher.
	if (EdslopProj == pop->Edslop())
	{
		if (pop->FDistinct())
		{
			CDSLAggMatcher am(m_mp, this);
			return am.FMatch(pop, pexpr, pmodel);
		}
		CDSLProjMatcher pm(m_mp, this);
		return pm.FMatch(pop, pexpr, pmodel);
	}

	// Compute<e a s> names ORCA's actual ComputeScalar/LET node. It shares the
	// Project shell with Proj, but deliberately bypasses every Proj compatibility
	// view and captures the complete scalar project list under <e>.
	if (EdslopCompute == pop->Edslop())
	{
		CDSLProjMatcher pm(m_mp, this);
		return pm.FMatchCompute(pop, pexpr, pmodel);
	}

	// Corpus Agg<a a f s p> and the six-symbol extension route to the Agg
	// matcher, including ORCA's Select-over-GbAgg representation of HAVING.
	if (EdslopAgg == pop->Edslop())
	{
		CDSLAggMatcher am(m_mp, this);
		return am.FMatch(pop, pexpr, pmodel);
	}

	// EXISTS in a filter context is normalized by ORCA into a LeftSemiApply.
	// Its uncorrelated inner LIMIT 1 is an implementation detail hidden from the
	// two-child DSL operator.
	if (EdslopExists == pop->Edslop() ||
		EdslopNotExists == pop->Edslop())
	{
		CDSLExistsMatcher em(m_mp, this);
		return em.FMatch(pop, pexpr, pmodel);
	}

	if (EdslopInSubFilter == pop->Edslop())
	{
		CDSLInSubMatcher ism(m_mp, this);
		return ism.FMatch(pop, pexpr, pmodel);
	}

	if (EdslopAny == pop->Edslop() || EdslopAll == pop->Edslop())
	{
		CDSLQuantifiedMatcher qm(m_mp, this);
		return qm.FMatch(pop, pexpr, pmodel);
	}

	if (EdslopUnion == pop->Edslop())
	{
		CDSLUnionMatcher um(m_mp, this);
		return um.FMatch(pop, pexpr, pmodel);
	}

	if (EdslopSort == pop->Edslop() || EdslopLimit == pop->Edslop())
	{
		return FMatchOrderLimit(pop, pexpr, pmodel);
	}

	// InnerJoin/LeftJoin<a a>: bind the equi-join key columns to the two <a>
	// symbols, keep non-equi conjuncts as residual, and recurse both relational
	// children. Like Filter/Proj, the join predicate (child[2]) is scalar structure
	// only operator-specific code reads, so join does not go through generic child
	// recursion (see CDSLJoinMatcher, doc M2).
	if (EdslopInnerJoin == pop->Edslop() || EdslopLeftJoin == pop->Edslop() ||
		EdslopSemiJoin == pop->Edslop() ||
		EdslopSemiApply == pop->Edslop() ||
		EdslopAntiJoin == pop->Edslop() ||
		EdslopAntiApply == pop->Edslop() ||
		EdslopAntiJoinNotIn == pop->Edslop() ||
		EdslopAntiApplyNotIn == pop->Edslop() ||
		EdslopInnerApply == pop->Edslop() ||
		EdslopLeftOuterApply == pop->Edslop())
	{
		CDSLJoinMatcher jm(m_mp, this, m_prule);
		return jm.FMatch(pop, pexpr, pmodel);
	}

	// operator-identity gate for directly represented logical operators.
	const COperator::EOperatorId eopidTemplate = pop->Eopid();
	if (COperator::EopSentinel == eopidTemplate ||
		eopidTemplate != pexpr->Pop()->Eopid())
	{
		return false;
	}

	// bind this node's own symbols (delegated per operator), then recurse into
	// its relational children.
	if (!FBindOpSymbols(pop, pexpr, pmodel))
	{
		return false;
	}
	return FMatchChildren(pop, pexpr, pmodel);
}

// EOF
