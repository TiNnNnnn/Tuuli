//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLInstantiator.h
//
//	@doc:
//		Stage ③ of the three-stage rewrite (WeTune Instantiation): build the
//		rule's TARGET expression from the bindings the matcher recorded on the
//		CDSLModel. This is what actually produces the rewritten plan the xform
//		hands to ORCA.
//
//		Key mechanism — equality classes (doc §10). The target fragment names its
//		OWN symbols (t1/p1/a1/...) which the matcher never bound (match runs only
//		over the source). A target symbol is resolved to a concrete artifact via
//		the rule's *Eq constraints: TableEq(t1,t0) says "t1 reuses whatever t0
//		bound to". So instantiation first builds a target-symbol -> source-symbol
//		alias map from the equality constraints, then reads the source symbol's
//		binding out of the model.
//
//		Per-operator target build (structural operators — the set the matcher
//		already supports; Proj/Agg with NEW columns are future work):
//		  Input<t>            -> the relational subtree bound to t (AddRef-graft)
//		  Filter<p a>         -> CLogicalSelect(child, predicate). The predicate is
//		                         the conjunct bound to p, CONJOINED with the
//		                         residual conjuncts the matcher preserved
//		                         (CPredicateUtils::PexprConjunction) — dropping a
//		                         residual = wrong plan.
//		  InnerJoin/LeftJoin  -> the join operator over the two rebuilt children
//		                         plus its (bound) predicate.
//
//		Simplifications (doc §11): reused subtrees/predicates are grafted by
//		AddRef with their real CColRefs already correct, so no
//		PexprCopyWithRemappedColumns is needed for structural rules (it is only
//		required once Proj/Agg introduce fresh columns). We trust the MONSOON EQ
//		proof, so no equivalence re-check.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLInstantiator_H
#define GPOPT_CDSLInstantiator_H

#include "gpos/base.h"
#include "gpos/common/CHashMap.h"

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

// target CDSLSymbol* -> source CDSLSymbol* it aliases (via an *Eq constraint).
// Keys/values unowned (they belong to the rule IR); pointer identity.
using CDSLSymbolAliasMap =
	CHashMap<CDSLSymbol, CDSLSymbol, gpos::HashPtr<CDSLSymbol>,
			 gpos::EqualPtr<CDSLSymbol>, CleanupNULL<CDSLSymbol>,
			 CleanupNULL<CDSLSymbol> >;

//---------------------------------------------------------------------------
//	@class:
//		CDSLInstantiator
//
//	@doc:
//		Builds a rule's target expression from a populated model. Construct per
//		instantiation with the (per-optimization) pool it should allocate in.
//---------------------------------------------------------------------------
class CDSLInstantiator
{
private:
	CMemoryPool *m_mp;

	// target-symbol -> source-symbol alias map, built from the rule's *Eq
	// constraints (owned; released in dtor).
	CDSLSymbolAliasMap *m_phmAlias;

	// populate m_phmAlias from the rule's equality constraints. An *Eq(x,y) links
	// x and y; whichever side was declared on the target aliases the other.
	void BuildAliasMap(const CDSLRule *prule);

	// resolve a (possibly target-side) symbol to the source symbol whose binding
	// it should reuse; returns psym itself if it has no alias (already source).
	const CDSLSymbol *PsymResolve(const CDSLSymbol *psym) const;

	// recursively build the target subtree rooted at pop, reading bindings from
	// pmodel (resolving target symbols through the alias map). Returns NULL if a
	// needed binding is missing or the operator kind is not yet supported.
	CExpression *PexprBuild(const CDSLOp *pop, const CDSLModel *pmodel) const;

	// Input<t>: the bound subtree (AddRef'd).
	CExpression *PexprBuildInput(const CDSLOp *pop,
								 const CDSLModel *pmodel) const;

	// Filter<p a>: Select(child, p-conjunct AND residuals).
	CExpression *PexprBuildFilter(const CDSLOp *pop,
								  const CDSLModel *pmodel) const;

	// InnerJoin/LeftJoin<a a>: join(child0, child1, bound-predicate). The
	// predicate is taken from the SOURCE match if a pred was bound; structural
	// join rules that merely reshape carry the predicate on the model.
	CExpression *PexprBuildJoin(const CDSLOp *pop,
								const CDSLModel *pmodel) const;

public:
	CDSLInstantiator(const CDSLInstantiator &) = delete;

	explicit CDSLInstantiator(CMemoryPool *mp);

	~CDSLInstantiator();

	// build the rule's target expression; NULL if instantiation is not possible
	// (missing binding, unsupported operator). Caller owns the returned ref.
	CExpression *PexprInstantiate(const CDSLRule *prule,
								  const CDSLModel *pmodel);
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLInstantiator_H
