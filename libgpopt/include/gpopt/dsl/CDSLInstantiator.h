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
//		Per-operator target build (the logical subset ORCA can represent):
//		  Input<t>            -> the relational subtree bound to t (AddRef-graft)
//		  Filter<p a>         -> CLogicalSelect(child, predicate). The predicate is
//		                         the conjunct bound to p, CONJOINED with the
//		                         residual conjuncts the matcher preserved
//		                         (CPredicateUtils::PexprConjunction) — dropping a
//		                         residual = wrong plan.
//		  InnerJoin/LeftJoin  -> the join operator over the two rebuilt children
//		                         plus its (bound) predicate.
//		  Proj/Agg/Exists/InSub/Union
//		                       -> operator-specific builders preserve scalar and
//		                          ordered-column metadata captured by the matcher.
//
//		Simplifications (doc §11): reused subtrees/predicates are grafted by
//		AddRef with their real CColRefs already correct, so no per-node column
//		remapping is needed for structural rules (that is only required once
//		Proj/Agg introduce fresh columns). ONE exception: an operator-eliminating
//		rule (e.g. Filter(Input<t0>) -> Input<t1>) yields a target whose ROOT is a
//		reused memo subtree, which violates Cascades' "result root must be freshly
//		built" contract; PexprFreshRoot copies only its operator and keeps the
//		already-bound children. We
//		trust the MONSOON EQ proof, so no equivalence re-check.
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

class COrderSpec;

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

	// Rule currently being instantiated (not owned). Used to associate a target
	// Filter's predicate template with the attrs vector of its source Filter.
	const CDSLRule *m_prule;

	// Source Input symbols already materialized while walking the target. A
	// second target occurrence is a fresh relational occurrence and therefore
	// needs independent CColRefs, even when TableEq points at the same source.
	CDSLSymbolArray *m_pdrgpsymBuiltInputs;

	// populate m_phmAlias from the rule's equality constraints. An *Eq(x,y) links
	// x and y; whichever side was declared on the target aliases the other.
	void BuildAliasMap(const CDSLRule *prule);

	// resolve a (possibly target-side) symbol to the source symbol whose binding
	// it should reuse; returns psym itself if it has no alias (already source).
	const CDSLSymbol *PsymResolve(const CDSLSymbol *psym) const;

	// Find the source Filter that owns psymPred, then copy its bound predicate
	// while remapping source Filter attrs to the target Filter attrs. Returns
	// NULL when the vectors are incompatible.
	const CDSLOp *PopSourceFilterForPredicate(
		const CDSLOp *pop, const CDSLSymbol *psymPred) const;
	CExpression *PexprBuildFilterPredicate(const CDSLOp *popFilter,
										 const CDSLModel *pmodel) const;
	CExpression *PexprBuildFilterCarrier(const CDSLOp *popFilter,
									 const CDSLModel *pmodel,
									 CExpression *pexprOuter) const;

	// Find the source Proj that owns a schema symbol. Target Proj attrs can name
	// a different (but constraint-equivalent) input vector, so its saved scalar
	// project list must be rebound from this source Proj's attrs.
	const CDSLOp *PopSourceProjForSchema(
		const CDSLOp *pop, const CDSLSymbol *psymSchema) const;
	CExpression *PexprRemapProjectList(const CDSLSymbol *psymTargetAttrs,
									 const CDSLSymbol *psymSchema,
									 const CDSLModel *pmodel) const;

	// recursively build the target subtree rooted at pop, reading bindings from
	// pmodel (resolving target symbols through the alias map). Returns NULL if a
	// needed binding is missing or the operator kind is not yet supported.
	CExpression *PexprBuild(const CDSLOp *pop, const CDSLModel *pmodel) const;

	// Input<t>: the bound subtree (AddRef'd).
	CExpression *PexprBuildInput(const CDSLOp *pop,
								 const CDSLModel *pmodel) const;

	// Map one source CColRef through the target template/built-expression pair.
	// Besides direct Input copies, this follows every matched SetOp's ordered
	// output-to-input correspondence. This is the shared positional adapter used
	// by predicate rebinding and newly constructed SetOp input maps.
	CColRef *PcrMapToTarget(const CDSLOp *popTarget,
						   CExpression *pexprTarget,
						   CColRef *pcrSource,
						   const CDSLModel *pmodel) const;
	CColRefArray *PdrgpcrMapToTarget(const CDSLOp *popTarget,
								CExpression *pexprTarget,
								const CColRefArray *pdrgpcrSource,
								const CDSLModel *pmodel) const;

	// Flatten a target Filter chain into one Select whose conjunction contains
	// each target predicate plus matcher residuals exactly once.
	CExpression *PexprBuildFilter(const CDSLOp *pop,
								  const CDSLModel *pmodel) const;

	// InnerJoin/LeftJoin<a a>: join(child0, child1, bound-predicate). The
	// predicate is taken from the SOURCE match if a pred was bound; structural
	// join rules that merely reshape carry the predicate on the model.
	CExpression *PexprBuildJoin(const CDSLOp *pop,
								const CDSLModel *pmodel) const;

	// Proj<a s>: Project(child, project-list). Rebuilds the SOURCE-matched list
	// over the target child, remapping source attrs to the target attrs while
	// preserving the project element output/schema columns.
	CExpression *PexprBuildProj(const CDSLOp *pop,
								const CDSLModel *pmodel) const;

	// Rebuild a Global CLogicalGbAgg for corpus Agg<a a f s p> or the extended
	// Agg<a a a f s p>. The corpus form infers aggregate outputs from schema minus
	// grouping columns. A non-TRUE HAVING is Select(GbAgg, predicate).
	CExpression *PexprBuildAgg(const CDSLOp *pop,
							   const CDSLModel *pmodel) const;

	// Exists(left,right): rebuild ORCA's LeftSemiApply representation. An
	// uncorrelated right input receives the same LIMIT 1 normalization used by
	// CSubqueryHandler.
	CExpression *PexprBuildExists(const CDSLOp *pop,
								  const CDSLModel *pmodel) const;

	// InSubFilter<a>(outer,inner): rebuild a LeftSemiApplyIn using the exact
	// equality predicate captured from ScalarSubqueryAny/ApplyIn.
	CExpression *PexprBuildInSub(const CDSLOp *pop,
								 const CDSLModel *pmodel) const;

	// Union/Union*: rebuild a binary logical set-op while preserving the
	// source match's ordered output-to-input column mapping. Target TableEq /
	// SchemaEq aliases may reorder the two branches.
	CExpression *PexprBuildUnion(const CDSLOp *pop,
								 const CDSLModel *pmodel) const;

	// Sort is represented as a count-less CLogicalLimit. Limit consumes a
	// directly nested Sort and fuses both into one CLogicalLimit, mirroring the
	// query translator's ORDER BY + LIMIT representation.
	CExpression *PexprBuildSort(const CDSLOp *pop,
								const CDSLModel *pmodel) const;
	CExpression *PexprBuildLimit(const CDSLOp *pop,
								 const CDSLModel *pmodel) const;
	COrderSpec *PosBuildSort(const CDSLOp *pop,
							 const CDSLModel *pmodel,
							 CExpression *pexprChild) const;

	// Cascades requires an xform result ROOT to be a freshly-built CExpression
	// (Pgexpr()==NULL). Operator-eliminating rules build a target whose root is a
	// reused memo subtree; re-root it via an identity PexprCopyWithRemappedColumns
	// (fresh nodes, colrefs unchanged). Consumes pexpr, returns the fresh-rooted
	// expression (or pexpr unchanged if it was already fresh / NULL).
	CExpression *PexprFreshRoot(CExpression *pexpr) const;

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
