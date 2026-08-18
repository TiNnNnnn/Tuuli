//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLAggMatcher.h
//
//	@doc:
//		Stage ① symbol binding for the dedup (SELECT DISTINCT) form of the Agg
//		operator (see docs/DSL_WETUNE_ALIGNMENT.md — Agg phase 1).
//
//		DSL   : Proj*<a s>(base)   — a DEDUPLICATED projection. In ORCA there is no
//		        "distinct" flag on CLogicalProject; SELECT DISTINCT cols becomes a
//		        CLogicalGbAgg whose grouping columns are `cols` and whose aggregate
//		        project list is EMPTY. (A bare Agg<...> template also routes here.)
//		ORCA  : CLogicalGbAgg(base, CScalarProjectList())  with Pdrgpcr() = grouping.
//
//		Scope of THIS matcher: pure dedup only — it rejects any GbAgg that carries
//		aggregate functions (non-empty project list). Real aggregate functions
//		(Agg_count/max/min, column remapping, HAVING) are a later milestone; an
//		Agg-with-funcs rule simply fails to match here and does not fire.
//
//		Approach (engine-side; ORCA core untouched), mirroring CDSLProjMatcher:
//		  1. Identity + arity gate: the live node must be a CLogicalGbAgg of arity 2
//		     with an EMPTY agg list (child[1] arity 0).
//		  2. Bind the grouping columns (CLogicalGbAgg::Pdrgpcr()) to BOTH <a> (attrs)
//		     and <s> (schema): for a pure dedup the referenced, output and grouping
//		     columns coincide.
//		  3. Recurse the relational child (child[0]) through the generic matcher.
//		  4. Flag the model with SetDedupDrop() so the instantiator drops the GbAgg
//		     by wrapping the resolved child in Select(child, TRUE), exactly like
//		     ORCA's own CXformSimplifyGbAgg::FDropGbAgg.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLAggMatcher_H
#define GPOPT_CDSLAggMatcher_H

#include "gpos/base.h"

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

// fwd decl — the generic matcher the relational child recurses back into
class CDSLMatcher;

//---------------------------------------------------------------------------
//	@class:
//		CDSLAggMatcher
//
//	@doc:
//		Matches a DSL Proj*<a s> (or Agg) dedup template against an ORCA
//		CLogicalGbAgg with an empty aggregate list. Constructed per match attempt
//		with the transient pool and a back-reference to the generic matcher (so the
//		relational child can recurse). Owns no state beyond those.
//---------------------------------------------------------------------------
class CDSLAggMatcher
{
private:
	CMemoryPool *m_mp;

	// generic matcher to recurse the relational child into (not owned)
	const CDSLMatcher *m_pmatcher;

	// copy the GbAgg's grouping columns (Pdrgpcr()) into a fresh ordered
	// CColRefArray — WeTune's valuesOf/valueRefsOf coincide for a pure dedup.
	// Caller owns the returned ref; the colrefs themselves are owned by the
	// column factory and are NOT AddRef'd individually (CColRefArray uses
	// CleanupNULL). Never NULL for a valid GbAgg.
	CColRefArray *PdrgpcrGrouping(CExpression *pexprAgg) const;

public:
	CDSLAggMatcher(const CDSLAggMatcher &) = delete;

	CDSLAggMatcher(CMemoryPool *mp, const CDSLMatcher *pmatcher)
		: m_mp(mp), m_pmatcher(pmatcher)
	{
		GPOS_ASSERT(nullptr != mp);
		GPOS_ASSERT(nullptr != pmatcher);
	}

	// match a dedup Agg/Proj*-rooted DSL template against a live CLogicalGbAgg.
	// Returns true iff the GbAgg is a pure dedup (empty agg list), the grouping
	// columns bound consistently, and the relational child matched. Sets the
	// model's dedup-drop flag on success.
	BOOL FMatch(const CDSLOp *popAgg, CExpression *pexprAgg,
				CDSLModel *pmodel) const;
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLAggMatcher_H
