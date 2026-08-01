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
//		  * Filter / InnerJoin / LeftJoin / Proj / Agg carry symbols whose binding
//		    needs operator-specific structural knowledge (conjunct split, join-key
//		    extraction, project-list / group-by columns). Those are delegated to
//		    dedicated collaborators in their own files:
//		        Filter  <p a>            -> CDSLFilterMatcher       (#25)
//		        Join    <a a>            -> join-key binding         (#27)
//		        Proj    <a s>            -> attrs/schema binding      (#27)
//		        Agg     <a a f s p>      -> agg symbol binding        (#27)
//
//		Until those land this is a NO-OP seam: an operator with symbols still
//		matches structurally (identity + children), it simply leaves those
//		symbols unbound. That is deliberately safe for the skeleton's own tests
//		(Input, Union, bare identity) and is filled in by the later components
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
