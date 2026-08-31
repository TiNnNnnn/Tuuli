//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLModel.h
//
//	@doc:
//		The binding model produced by matching a rule's source template against
//		a real CExpression (WeTune's Model). Maps each DSL symbol (identity =
//		CDSLSymbol*) to the concrete ORCA artifact it was bound to:
//
//		  table  symbol -> CExpression*      (a relational subtree; Input<t>)
//		  attrs  symbol -> CColRefArray*     (ordered columns; join keys, refs)
//		  pred   symbol -> CExpression*      (a single conjunct predicate subtree)
//		  schema symbol -> CColRefArray*     (ordered output columns; Proj/Agg)
//		  func   symbol -> CExpressionArray* (aggregate expressions)
//		  scalar symbol -> CExpression*      (LIMIT count / offset expression)
//		  expr   symbol -> CExpression*      (exact scalar expression list)
//
//		Design notes (see docs/WETUNE_ORCA_PER_OP_THREESTAGE.md):
//		  * Everything stored here is AddRef'd on insertion and Released in the
//		    dtor — the model owns one ref of every artifact it holds. Keys are
//		    NOT owned (they belong to the rule IR).
//		  * Equality-class checking (WeTune TableEq/AttrsEq/...) is done by the
//		    engine's Check phase by comparing the artifacts two symbols in the
//		    same class are bound to (pointer / colref identity).
//
//		PHASE 1: this is the data structure only. The engine's Match/Check/
//		Instantiate that populate and consume it are stubbed; they are filled in
//		in phase 2.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLModel_H
#define GPOPT_CDSLModel_H

#include "gpos/base.h"
#include "gpos/common/CHashMap.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/CWindowFrame.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

// symbol identity -> bound artifact. Key hashed/compared by POINTER identity
// (gpos::HashPtr / gpos::Equals), matching how WeTune compares symbols by
// reference. Keys unowned (CleanupNULL), values own one ref (CleanupRelease).
// Value is the CRefCount base; typed accessors cast.
using CDSLSymbolToRefMap =
	CHashMap<CDSLSymbol, CRefCount, gpos::HashPtr<CDSLSymbol>,
			 gpos::EqualPtr<CDSLSymbol>, CleanupNULL<CDSLSymbol>,
			 CleanupRelease<CRefCount> >;

// IN predicates are additional artifacts keyed by the InSubFilter's attrs
// symbol. They cannot live in m_phmSymToRef because that symbol is already
// bound to its CColRefArray. Values are owned; keys belong to the rule IR.
using CDSLSymbolToExpressionMap =
	CHashMap<CDSLSymbol, CExpression, gpos::HashPtr<CDSLSymbol>,
			 gpos::EqualPtr<CDSLSymbol>, CleanupNULL<CDSLSymbol>,
			 CleanupRelease<CExpression> >;

//---------------------------------------------------------------------------
//	@class:
//		CDSLModel
//
//	@doc:
//		Symbol -> bound-artifact map for one (rule, expression) match attempt.
//		Keyed by CDSLSymbol identity (the builder interns symbols per rule, so a
//		pointer key is the same identity WeTune compares by reference).
//---------------------------------------------------------------------------
class CDSLModel : public CRefCount
{
private:
	CMemoryPool *m_mp;
	CDSLSymbolToRefMap *m_phmSymToRef;
	CDSLSymbolToExpressionMap *m_phmInSubPred;
	CDSLSymbolToExpressionMap *m_phmInSubCarrier;
	CDSLSymbolToExpressionMap *m_phmFilterCarrier;
	CDSLSymbolToExpressionMap *m_phmApplyCarrier;
	CDSLSymbolToExpressionMap *m_phmProjList;
	CDSLSymbolToExpressionMap *m_phmProjLimitShell;
	CDSLSymbolToExpressionMap *m_phmProjAggShell;
	CDSLSymbolToExpressionMap *m_phmAggBinding;
	CDSLSymbolToExpressionMap *m_phmVirtualIdentityProj;
	CDSLSymbolToExpressionMap *m_phmJoinPred;
	CDSLSymbolToExpressionMap *m_phmWindowCarrier;

	// Every matched Union/Union* expression, in source-tree traversal order.
	// The operator owns the ordered output-column array and one ordered input
	// array per branch; keeping the complete expression lets instantiation
	// preserve those mappings even when target constraints reorder branches.
	CExpressionArray *m_pdrgpexprUnionBindings;
	// UnionAll tails created only by the n-ary-to-binary associative match view.
	// Marking them prevents target construction from flattening a genuine
	// nested UnionAll that happened to be bound to an ordinary Input symbol.
	CExpressionArray *m_pdrgpexprNaryUnionTails;

	// conjuncts of a matched Filter/Select that the rule did NOT consume — they
	// must be carried through to the instantiated target unchanged (dropping a
	// predicate = wrong plan). Populated by the filter matcher (#25), consumed by
	// the instantiator (#27). Owns one ref of each conjunct; NULL until a filter
	// match records residuals.
	CExpressionArray *m_pdrgpexprResidual;

	// conjuncts surrounding a ScalarSubqueryExists in its source Select. The
	// EXISTS matcher consumes only the existential conjunct; these predicates
	// must remain as a Select above the instantiated LeftSemiApply. Kept separate
	// from Filter/Join residuals because an Exists subtree may contain either.
	CExpressionArray *m_pdrgpexprExistsResidual;

	// Other conjuncts beside matched IN/ANY subqueries in their source Select.
	// Each IN comparison predicate is stored separately in m_phmInSubPred under
	// that InSubFilter node's attrs symbol, allowing nested/repeated IN rules.
	CExpressionArray *m_pdrgpexprInSubResidual;

	// Project-list scalar subtrees (CScalarProjectList), keyed by each Proj's
	// schema symbol. A rule may contain sibling or nested Proj nodes, so a single
	// global slot would let a later match overwrite an earlier one. Values are
	// opaque and AddRef'd; they preserve computed-column value subtrees that
	// attrs/schema bindings alone cannot represent.
	// set by the Agg matcher when the matched source root is a pure-dedup
	// CLogicalGbAgg (empty agg list) whose grouping columns form a key — i.e. a
	// redundant SELECT DISTINCT. The instantiator then drops the GbAgg by wrapping
	// the resolved relational child in Select(child, TRUE), mirroring ORCA's own
	// CXformSimplifyGbAgg::FDropGbAgg. No artifact to own — just a bit.
	BOOL m_fDedupDrop;

	// Original GbAgg when a Proj* source is matched against WeTune's virtual
	// dedup projection below a DISTINCT aggregate. The target removes that
	// virtual Proj*, so instantiation rebuilds this aggregate with DQA flags off.
	CExpression *m_pexprDistinctAgg;

public:
	CDSLModel(const CDSLModel &) = delete;

	explicit CDSLModel(CMemoryPool *mp);

	~CDSLModel() override;

	// the pool this model (and any transient work a matcher does for it) lives
	// in — the per-optimization xform pool, NOT the engine's long-lived pool.
	CMemoryPool *
	Pmp() const
	{
		return m_mp;
	}

	// bind a symbol to an artifact; AddRef's pval and takes ownership of that
	// ref. Returns false if the symbol was already bound to a DIFFERENT value
	// (WeTune's incompatible-reassignment failure); rebinding to the SAME value
	// is a no-op success.
	BOOL FBind(const CDSLSymbol *psym, CRefCount *pval);

	// look up a bound artifact (NULL if unbound). Does NOT AddRef.
	CRefCount *PvalLookup(const CDSLSymbol *psym) const;

	// typed convenience accessors; NULL if unbound. Do NOT AddRef.
	CExpression *PexprTable(const CDSLSymbol *psym) const;
	CExpression *PexprPred(const CDSLSymbol *psym) const;
	CExpression *PexprScalar(const CDSLSymbol *psym) const;
	CExpression *PexprExpr(const CDSLSymbol *psym) const;
	CColRefArray *PdrgpcrAttrs(const CDSLSymbol *psym) const;
	CColRefArray *PdrgpcrSchema(const CDSLSymbol *psym) const;
	CExpressionArray *PdrgpexprFunc(const CDSLSymbol *psym) const;
	COrderSpecArray *PdrgposOrder(const CDSLSymbol *psym) const;
	CExpression *PexprWindow(const CDSLSymbol *psym) const;
	CWindowFrameArray *PdrgpwfFrame(const CDSLSymbol *psym) const;

	ULONG Size() const { return m_phmSymToRef->Size(); }

	//------------------------------------------------------------------
	// residual conjuncts (filter split — #25 produces, #27 consumes)
	//------------------------------------------------------------------

	// record the conjuncts of a matched Filter/Select that the rule did not
	// consume. Takes ownership of pdrgpexpr (one ref); replaces any previous set.
	// Each element is expected to already carry the ref the array holds.
	void SetResidualConjuncts(CExpressionArray *pdrgpexpr);

	// the residual conjuncts recorded by the filter matcher; NULL if none were
	// recorded (i.e. no filter was split, or every conjunct was consumed). Does
	// NOT transfer ownership.
	CExpressionArray *
	PdrgpexprResidual() const
	{
		return m_pdrgpexprResidual;
	}

	// record/access predicates adjacent to the consumed EXISTS conjunct.
	void SetExistsResidualConjuncts(CExpressionArray *pdrgpexpr);

	CExpressionArray *
	PdrgpexprExistsResidual() const
	{
		return m_pdrgpexprExistsResidual;
	}

	// Record the exact comparison predicate for one source InSubFilter attrs
	// symbol. Takes ownership of pexpr. Returns false on an incompatible repeat.
	BOOL FSetInSubPred(const CDSLSymbol *psymAttrs, CExpression *pexpr);

	// Look up the predicate for a resolved source attrs symbol. No AddRef.
	CExpression *PexprInSubPred(const CDSLSymbol *psymAttrs) const;

	// Record the post-unnest Apply/SemiJoin representation used to match one
	// InSubFilter. Target construction preserves that phase representation.
	BOOL FSetInSubCarrier(const CDSLSymbol *psymAttrs, CExpression *pexpr);
	CExpression *PexprInSubCarrier(const CDSLSymbol *psymAttrs) const;

	// A WeTune SimpleFilter may be represented by a relational subquery-filter
	// carrier in ORCA. Record it by the Filter predicate symbol so target
	// construction can move the whole carrier instead of losing the subquery.
	BOOL FSetFilterCarrier(const CDSLSymbol *psymPred, CExpression *pexpr);
	CExpression *PexprFilterCarrier(const CDSLSymbol *psymPred) const;

	// A scalar subquery in a Select predicate is exposed as the same InnerApply
	// view produced by ORCA's subquery handler. Keep that carrier by predicate
	// symbol so target construction can preserve required-inner-column and
	// origin-subquery metadata instead of manufacturing a pattern Apply.
	BOOL FSetApplyCarrier(const CDSLSymbol *psymPred, CExpression *pexpr);
	CExpression *PexprApplyCarrier(const CDSLSymbol *psymPred) const;

	void SetInSubResidualConjuncts(CExpressionArray *pdrgpexpr);

	CExpressionArray *
	PdrgpexprInSubResidual() const
	{
		return m_pdrgpexprInSubResidual;
	}

	//------------------------------------------------------------------
	// project list (Proj match — M1 produces, instantiator consumes)
	//------------------------------------------------------------------

	// Record the CScalarProjectList subtree for one matched Proj schema symbol.
	// Takes ownership of one ref of pexpr; duplicate keys are rejected.
	BOOL FSetProjList(const CDSLSymbol *psymSchema, CExpression *pexpr);

	// The project-list subtree for one Proj schema symbol; NULL if it was not
	// matched. Does NOT transfer ownership.
	CExpression *PexprProjList(const CDSLSymbol *psymSchema) const;

	// ORCA has no column-pruning logical Project. A Proj(InSub(...)) rule may
	// therefore be scheduled directly on the equivalent SemiJoin/Apply/Select
	// group. Record that the source Project was an implicit identity view so
	// target construction removes only this virtual shell.
	BOOL FSetVirtualIdentityProj(const CDSLSymbol *psymSchema,
								 CExpression *pexprCarrier);
	BOOL FVirtualIdentityProj(const CDSLSymbol *psymSchema) const;

	// ORCA can place an unmentioned Sort/Limit shell between Project and its
	// relational child. Record it by Project schema so target construction can
	// preserve the shell after rewriting the child.
	BOOL FSetProjLimitShell(const CDSLSymbol *psymSchema,
						 CExpression *pexpr);
	CExpression *PexprProjLimitShell(const CDSLSymbol *psymSchema) const;

	// Project(GbAgg(...)) can represent a DSL projection over the aggregate's
	// required input columns. Preserve the whole live shell around a rewritten
	// aggregate input.
	BOOL FSetProjAggShell(const CDSLSymbol *psymSchema, CExpression *pexpr);
	CExpression *PexprProjAggShell(const CDSLSymbol *psymSchema) const;

	// Preserve optimizer-only metadata when a target Agg keeps the same full
	// grouping set and relational child.  The source expression is keyed by its
	// schema symbol so SchemaEq aliases resolve to the correct matched Agg.
	BOOL FSetAggBinding(const CDSLSymbol *psymSchema, CExpression *pexpr);
	CExpression *PexprAggBinding(const CDSLSymbol *psymSchema) const;

	//------------------------------------------------------------------
	// set-op mappings (Union match produces, instantiator consumes)
	//------------------------------------------------------------------

	// Add one matched CLogicalUnion/CLogicalUnionAll expression. AddRefs it.
	void AddUnionBinding(CExpression *pexprUnion);
	void AddNaryUnionTail(CExpression *pexprUnionAll);
	BOOL FIsNaryUnionTail(CExpression *pexpr) const;

	// Matched Union expressions; does not transfer ownership.
	CExpressionArray *
	PdrgpexprUnionBindings() const
	{
		return m_pdrgpexprUnionBindings;
	}

	//------------------------------------------------------------------
	// join predicate (Join match — M2 produces, instantiator consumes)
	//------------------------------------------------------------------

	// Record one source Join predicate under both of that node's attrs symbols.
	// This keeps nested Join predicates independent while allowing target-side
	// AttrsEq aliases (including swaps) to find the source predicate.
	BOOL FSetJoinPred(const CDSLSymbol *psymLeftAttrs,
					  const CDSLSymbol *psymRightAttrs, CExpression *pexpr);

	// Return the predicate shared by a resolved attrs pair, or NULL if the pair
	// did not come from one matched source Join. Does not AddRef.
	CExpression *PexprJoinPred(const CDSLSymbol *psymLeftAttrs,
						   const CDSLSymbol *psymRightAttrs) const;

	// Preserve the exact ORCA SequenceProject shell by its window-items symbol.
	// Partition/order/frame symbols remain independently bound for constraints;
	// this carrier prevents target construction from approximating optimizer
	// metadata that the surface DSL intentionally treats as opaque values.
	BOOL FSetWindowCarrier(const CDSLSymbol *psymWindow, CExpression *pexpr);
	CExpression *PexprWindowCarrier(const CDSLSymbol *psymWindow) const;

	//------------------------------------------------------------------
	// dedup drop (Agg match — Agg matcher produces, instantiator consumes)
	//------------------------------------------------------------------

	// mark the matched source root as a redundant dedup GbAgg to be dropped
	// (grouping cols form a key, no agg functions). The instantiator wraps the
	// resolved child in Select(child, TRUE).
	void
	SetDedupDrop()
	{
		m_fDedupDrop = true;
	}

	// whether the Agg matcher flagged a redundant dedup GbAgg for elimination.
	BOOL
	FDedupDrop() const
	{
		return m_fDedupDrop;
	}

	BOOL FSetDistinctAgg(CExpression *pexprAgg);

	CExpression *
	PexprDistinctAgg() const
	{
		return m_pexprDistinctAgg;
	}
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLModel_H
