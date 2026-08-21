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
#include "gpopt/dsl/CDSLJoinMatcher.h"
#include "gpopt/dsl/CDSLProjMatcher.h"

using namespace gpopt;

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

	// Filter chain: a DSL single-predicate Filter (chain) matches ONE ORCA
	// CLogicalSelect whose conjunctive predicate is split into a conjunct set.
	// Delegated to the filter matcher, which recurses the chain's base op back
	// into this matcher (see CDSLFilterMatcher). This is the hardest mismatch
	// (doc §2) and is why Filter does not go through the generic child recursion.
	if (EdslopFilter == pop->Edslop())
	{
		CDSLFilterMatcher fm(m_mp, this);
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
	if (EdslopExists == pop->Edslop())
	{
		CDSLExistsMatcher em(m_mp, this);
		return em.FMatch(pop, pexpr, pmodel);
	}

	// InnerJoin/LeftJoin<a a>: bind the equi-join key columns to the two <a>
	// symbols, keep non-equi conjuncts as residual, and recurse both relational
	// children. Like Filter/Proj, the join predicate (child[2]) is scalar structure
	// only operator-specific code reads, so join does not go through generic child
	// recursion (see CDSLJoinMatcher, doc M2).
	if (EdslopInnerJoin == pop->Edslop() || EdslopLeftJoin == pop->Edslop())
	{
		CDSLJoinMatcher jm(m_mp, this);
		return jm.FMatch(pop, pexpr, pmodel);
	}

	// operator-identity gate. Operators with no direct ORCA logical counterpart
	// (Sort/Limit -> EopSentinel; deferred, see doc §9) never match here.
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
