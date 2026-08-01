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

	ULONG Size() const { return m_phmSymToRef->Size(); }
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLModel_H
