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

	// the project-list scalar subtree (CScalarProjectList) of a matched
	// CLogicalProject. WeTune's Proj<a s> models projected columns as attrs/schema
	// symbols, but ORCA's project list also carries the computed-column value
	// subtrees, which those symbols do not capture. So the Proj matcher (M1)
	// records the whole list here (opaque, AddRef'd — exactly like Filter records
	// its predicate) and the instantiator AddRef-grafts it over the rebuilt child.
	// Owns one ref; NULL until a Proj match records it.
	CExpression *m_pexprProjList;

	// the whole join-predicate scalar subtree (child[2]) of a matched
	// CLogicalInnerJoin / CLogicalLeftOuterJoin. WeTune's Join<a a> models only the
	// equi-join key columns as attrs symbols, but ORCA's join predicate also carries
	// non-equi conjuncts (and the exact comparison ops) those symbols do not capture.
	// So the join matcher (M2) records the whole predicate here (opaque, AddRef'd —
	// exactly like Proj records its list) and the instantiator AddRef-grafts it onto
	// the rebuilt join. Owns one ref; NULL until a Join match records it.
	CExpression *m_pexprJoinPred;

	// set by the Agg matcher when the matched source root is a pure-dedup
	// CLogicalGbAgg (empty agg list) whose grouping columns form a key — i.e. a
	// redundant SELECT DISTINCT. The instantiator then drops the GbAgg by wrapping
	// the resolved relational child in Select(child, TRUE), mirroring ORCA's own
	// CXformSimplifyGbAgg::FDropGbAgg. No artifact to own — just a bit.
	BOOL m_fDedupDrop;

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
	CColRefArray *PdrgpcrAttrs(const CDSLSymbol *psym) const;
	CColRefArray *PdrgpcrSchema(const CDSLSymbol *psym) const;
	CExpressionArray *PdrgpexprFunc(const CDSLSymbol *psym) const;

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

	//------------------------------------------------------------------
	// project list (Proj match — M1 produces, instantiator consumes)
	//------------------------------------------------------------------

	// record the CScalarProjectList subtree of a matched CLogicalProject. Takes
	// ownership of one ref of pexpr; replaces any previous set.
	void SetProjList(CExpression *pexpr);

	// the project-list subtree recorded by the Proj matcher; NULL if no Proj was
	// matched. Does NOT transfer ownership.
	CExpression *
	PexprProjList() const
	{
		return m_pexprProjList;
	}

	//------------------------------------------------------------------
	// join predicate (Join match — M2 produces, instantiator consumes)
	//------------------------------------------------------------------

	// record the join-predicate subtree (child[2]) of a matched join. Takes
	// ownership of one ref of pexpr; replaces any previous set.
	void SetJoinPred(CExpression *pexpr);

	// the join-predicate subtree recorded by the join matcher; NULL if no join was
	// matched. Does NOT transfer ownership.
	CExpression *
	PexprJoinPred() const
	{
		return m_pexprJoinPred;
	}

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
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLModel_H
