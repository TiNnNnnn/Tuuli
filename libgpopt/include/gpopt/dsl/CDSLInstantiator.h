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

	// Target column vectors materialized by AttrsIntersect. Values are ordered
	// CColRefArray objects owned by this per-instantiation cache.
	mutable CDSLSymbolToRefMap *m_phmDerivedCols;

	// Target predicates materialized by multi-output predicate algebra such as
	// PredicateDomainSplit. Values are owned by this instantiation.
	mutable CDSLSymbolToExpressionMap *m_phmDerivedPreds;

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

	// Resolve a scalar alias to an owned expression, or materialize a target
	// symbol declared by ScalarOne/ScalarZero.
	CExpression *PexprResolveScalar(const CDSLSymbol *psym,
								 const CDSLModel *pmodel) const;

	// Resolve a bound/aliased predicate to an owned expression, or lazily build
	// a target predicate declared by PredicateAnd.
	CExpression *PexprResolvePredicate(const CDSLSymbol *psym,
									const CDSLModel *pmodel,
									ULONG ulDepth = 0) const;

	// Atomically partition one conjunction and populate its two predicate plus
	// four dependency-vector outputs. Returns false for mixed three-domain atoms
	// or when no genuine external dependency exists.
	BOOL FMaterializePredicateDomainSplit(const CDSLConstraint *pcon,
									   const CDSLModel *pmodel,
									   ULONG ulDepth) const;

	// Resolve a bound/aliased attrs or schema vector. AttrsIntersect derives an
	// ordered subset, AttrsUnion a stable duplicate-free union, AttrsEmpty
	// materializes a target-only empty attrs vector, and OutputAttrs derives the
	// complete output vector of a keyed relation. SchemaFromAttrs explicitly
	// carries a resolved attrs vector into the schema namespace.
	CColRefArray *PdrgpcrResolveCols(const CDSLSymbol *psym,
									 const CDSLModel *pmodel,
									 ULONG ulDepth = 0) const;

	// Materialize child-dependent minimal grouping metadata when the rule
	// explicitly declares MinimalGrouping(group,schema). Returns an owned array,
	// or NULL when this target aggregate has no such property declaration.
	CColRefArray *PdrgpcrMinimalGrouping(const CDSLSymbol *psymGroup,
									const CDSLSymbol *psymSchema,
									const CDSLModel *pmodel) const;

	// Resolve an expression-list symbol to an owned CScalarProjectList. Besides
	// direct/ExprListEq bindings, target symbols may be defined by ExprConcat and
	// are
	// evaluated lazily from their source operands.
	CExpression *PexprResolveExpr(const CDSLSymbol *psym,
								 const CDSLModel *pmodel,
								 ULONG ulDepth = 0) const;

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

	// Empty<t>: build a zero-row ConstTableGet with the output columns of the
	// resolved table/subtree binding.
	CExpression *PexprBuildEmpty(const CDSLOp *pop,
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

	// Remap an exact source predicate over a rebuilt binary target. Every used
	// column must be supplied unambiguously by exactly one target child.
	CExpression *PexprRemapPredicateToChildren(
		const CDSLOp *popLeft, CExpression *pexprLeft,
		const CDSLOp *popRight, CExpression *pexprRight,
		CExpression *pexprSourcePred, const CDSLModel *pmodel) const;

	// Flatten a target Filter chain into one Select whose conjunction contains
	// each target predicate plus matcher residuals exactly once.
	CExpression *PexprBuildFilter(const CDSLOp *pop,
								  const CDSLModel *pmodel) const;

	// InnerJoin/LeftJoin<a a [a s] [p a a]>: join both rebuilt children with
	// the bound predicate. Explicit residual bindings are validated against
	// their declared per-child dependencies before the exact source predicate is
	// remapped.
	CExpression *PexprBuildJoin(const CDSLOp *pop,
								const CDSLModel *pmodel) const;

	// Proj<a s>: Project(child, project-list). Rebuilds the SOURCE-matched list
	// over the target child, remapping source attrs to the target attrs while
	// preserving the project element output/schema columns.
	CExpression *PexprBuildProj(const CDSLOp *pop,
								const CDSLModel *pmodel) const;

	// Compute<e a s>: rebuild an exact ORCA ComputeScalar/LET node from the
	// captured expression list. The child keeps all of its columns; <a> names
	// expression dependencies and <s> the newly-defined columns.
	CExpression *PexprBuildCompute(const CDSLOp *pop,
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

	// InSubFilter<a [a p a a]>(outer,inner): rebuild equality-only ApplyIn or
	// an extended residual-aware LeftSemiJoin from the exact captured predicate.
	CExpression *PexprBuildInSub(const CDSLOp *pop,
								 const CDSLModel *pmodel) const;

	// Any/All<p a>(outer,inner): rebuild a quantified Apply from the exact
	// comparison predicate. ALL uses ORCA's inverse-witness predicate internally.
	CExpression *PexprBuildQuantified(const CDSLOp *pop,
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
	CExpression *PexprBuildWindow(const CDSLOp *pop,
								   const CDSLModel *pmodel) const;
	CExpression *PexprBuildRowNumber(const CDSLOp *pop,
								  const CDSLModel *pmodel) const;

	// AssertMaxOneRow(child): ORCA's canonical executable implementation of
	// the scalar-subquery cardinality contract.
	CExpression *PexprBuildAssertMaxOneRow(const CDSLOp *pop,
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

	// Evaluate every constructive output of one already-validated constraint and
	// bind it into the match model. This gives constraint checking and target
	// construction one implementation of restricted LET chains.
	BOOL FMaterializeConstraintOutputs(const CDSLRule *prule,
									 const CDSLConstraint *pcon,
									 CDSLModel *pmodel);

	// build the rule's target expression; NULL if instantiation is not possible
	// (missing binding, unsupported operator). Caller owns the returned ref.
	CExpression *PexprInstantiate(const CDSLRule *prule,
								  const CDSLModel *pmodel);
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLInstantiator_H
