//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLMatcher.h
//
//	@doc:
//		Stage ① of the three-stage rewrite (WeTune Match.java): match a rule's
//		source template (a CDSLOp tree) against a live logical CExpression,
//		binding the template's placeholder symbols into a CDSLModel.
//
//		This class owns ONLY the GENERIC skeleton, mirroring WeTune's
//		Match.matchOne dispatch:
//		  * Input<t> — an OPAQUE relational placeholder: matches ANY subtree and
//		    binds the table symbol to it (no node-type check), exactly like
//		    WeTune's INPUT branch. This is the recursion's leaf.
//		  * every other operator — an IDENTITY gate: the DSL op's mapped ORCA
//		    EOperatorId must equal pexpr->Pop()->Eopid(); then its own symbols are
//		    bound and its RELATIONAL children are matched positionally.
//
//		ORCA-to-DSL representation differences are decoded centrally by
//		CDSLMatchView. Per-operator SYMBOL binding and semantic validation remain
//		delegated to dedicated collaborators (see
//		docs/WETUNE_ORCA_PER_OP_THREESTAGE.md):
//		  * Filter <p a>              -> CDSLFilterMatcher   (conjunct split; #25)
//		  * InnerJoin/LeftJoin <a a>  -> join-key extraction (#27)
//		  * Proj <a s> / Agg <a a f s p> -> attrs/schema binding (#27)
//		Until those land, FBindOpSymbols is a documented no-op seam for them, so
//		the skeleton + Input recursion is independently testable.
//
//		Column/subtree AddRef discipline lives in CDSLModel::FBind (it AddRefs the
//		artifact it stores); the matcher hands raw pointers to FBind.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLMatcher_H
#define GPOPT_CDSLMatcher_H

#include "gpos/base.h"

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

class COrderSpec;

//---------------------------------------------------------------------------
//	@class:
//		CDSLMatcher
//
//	@doc:
//		Recursive structural matcher (template CDSLOp tree <-> live CExpression).
//		Stateless apart from the pool it allocates transient work in; safe to
//		construct per match attempt.
//---------------------------------------------------------------------------
class CDSLMatcher
{
private:
	CMemoryPool *m_mp;
	// Complete rule owning the source template. Optional for low-level matcher
	// tests, but present in the rule engine so operator matchers can honor
	// source-side constraints while exploring ambiguous bindings.
	const CDSLRule *m_prule;

	// Input<t>: bind the single table symbol to the whole subtree (any
	// relational subtree qualifies; no node-type check — WeTune INPUT branch).
	BOOL FMatchInput(const CDSLOp *pop, CExpression *pexpr,
					 CDSLModel *pmodel) const;

	// Empty<t>: match an actual zero-row ConstTableGet and bind its schema
	// carrier to the table symbol.
	BOOL FMatchEmpty(const CDSLOp *pop, CExpression *pexpr,
					 CDSLModel *pmodel) const;

	// bind THIS op's positional symbols against pexpr. Generic ops (Input handled
	// separately, symbol-free Union) need nothing; Filter/Join/Proj/Agg symbol
	// binding is delegated to dedicated collaborators (see class doc). Returns
	// false only on a hard structural incompatibility.
	BOOL FBindOpSymbols(const CDSLOp *pop, CExpression *pexpr,
						CDSLModel *pmodel) const;

	// match the DSL op's relational children positionally against pexpr's leading
	// children (scalar children of pexpr come after and are consumed by symbol
	// binding, not here).
	BOOL FMatchChildren(const CDSLOp *pop, CExpression *pexpr,
						CDSLModel *pmodel) const;

	// Sort/Limit share CLogicalLimit in ORCA. Match the standalone forms and
	// virtually split a fused node into Limit(Sort(child)).
	BOOL FMatchOrderLimit(const CDSLOp *pop, CExpression *pexpr,
						 CDSLModel *pmodel) const;
	BOOL FMatchSortView(const CDSLOp *popSort, CExpression *pexprChild,
						  const COrderSpec *pos, CDSLModel *pmodel) const;

	// Bind a SequenceProject without flattening its per-window order/frame
	// arrays. Partition columns are exposed as Attrs; the remaining metadata is
	// preserved as opaque, equality-comparable artifacts.
	BOOL FMatchWindow(const CDSLOp *pop, CExpression *pexpr,
					 CDSLModel *pmodel) const;

	// Assert<p,a>(child): bind the exact predicate and its dependency columns,
	// and keep the live operator as the error-contract carrier.
	BOOL FMatchAssert(const CDSLOp *pop, CExpression *pexpr,
					 CDSLModel *pmodel) const;

public:
	CDSLMatcher(const CDSLMatcher &) = delete;

	explicit CDSLMatcher(CMemoryPool *mp, const CDSLRule *prule = nullptr)
		: m_mp(mp), m_prule(prule)
	{
		GPOS_ASSERT(nullptr != mp);
	}

	const CDSLRule *
	Prule() const
	{
		return m_prule;
	}

	// match one DSL op subtree against one live expression, populating pmodel.
	// Returns true iff the whole subtree matched and every symbol bound
	// consistently (FBind rejects incompatible re-binds -> equality classes).
	BOOL FMatch(const CDSLOp *pop, CExpression *pexpr, CDSLModel *pmodel) const;
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLMatcher_H
