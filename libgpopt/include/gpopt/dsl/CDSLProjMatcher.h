//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLProjMatcher.h
//
//	@doc:
//		Stage ① symbol binding for the Proj operator (see
//		docs/WETUNE_ORCA_PER_OP_THREESTAGE.md and docs/DSL_WETUNE_ALIGNMENT.md M1).
//
//		DSL   : Proj<a s>(base)         — <a> attrs (projected columns), <s> schema.
//		ORCA  : CLogicalProject(base, CScalarProjectList(prEl0, prEl1, ...))  where
//		        each CScalarProjectElement::Pcr() defines one projected CColRef.
//
//		Approach (engine-side; ORCA core untouched), mirroring CDSLFilterMatcher:
//		  1. Identity gate is done by the generic matcher (Eopid == EopLogical
//		     Project) BEFORE it delegates here; we only bind symbols + recurse.
//		  2. Collect the project-list's defined CColRefs (child[1]'s elements'
//		     Pcr()) into an ordered CColRefArray and bind it to <a> (attrs) AND
//		     <s> (schema). WeTune's schema symbol names the projection's output
//		     column schema; over placeholder columns the two coincide, so M1 binds
//		     both to the same projected-column set (Agg's distinct schema handling
//		     is future work).
//		  3. The relational child (child[0]) recurses back through the generic
//		     matcher.
//
//		This is deliberately NOT folded into the generic FMatchChildren recursion:
//		like Filter, Proj carries scalar structure (the project list) that only
//		operator-specific code knows how to read, so CDSLMatcher intercepts
//		EdslopProj and routes here.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLProjMatcher_H
#define GPOPT_CDSLProjMatcher_H

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
//		CDSLProjMatcher
//
//	@doc:
//		Matches a DSL Proj<a s> template against an ORCA CLogicalProject.
//		Constructed per match attempt with the transient pool and a back-reference
//		to the generic matcher (so the relational child can recurse). Owns no state
//		beyond those.
//---------------------------------------------------------------------------
class CDSLProjMatcher
{
private:
	CMemoryPool *m_mp;

	// generic matcher to recurse the relational child into (not owned)
	const CDSLMatcher *m_pmatcher;

	// gather the columns the project list DEFINES / outputs (each
	// CScalarProjectElement's Pcr()) — WeTune's valuesOf, bound to schema <s>.
	// Caller owns the returned ref. NULL if child[1] is not a project list.
	CColRefArray *PdrgpcrSchema(CExpression *pexprProjList) const;

	// gather the columns the project elements' value expressions REFERENCE
	// (DeriveUsedColumns), de-duplicated in first-seen order — WeTune's
	// valueRefsOf, bound to attrs <a>. Differs from the schema for computed
	// columns. Caller owns the returned ref. NULL if not a project list.
	CColRefArray *PdrgpcrAttrs(CExpression *pexprProjList) const;

	// A dedup-drop target is represented in the memo as Select(child, TRUE).
	// When child is a pure global dedup GbAgg, expose that marker as the
	// equivalent identity Proj so a following DSL rule can consume it.
	BOOL FMatchTrivialSelectOverDedup(const CDSLOp *popProj,
								 CExpression *pexprSelect,
								 CDSLModel *pmodel) const;

	// ORCA omits WeTune's input-column projection below an aggregate. Derive
	// that projection from grouping columns and aggregate arguments, then match
	// the structured DSL child against the aggregate input.
	BOOL FMatchProjectOverAgg(const CDSLOp *popProj,
						  CExpression *pexprProject,
						  CDSLModel *pmodel) const;

	// ORCA represents a column-pruning Proj(InSub(...)) at the SemiJoin/Apply/
	// Select group itself. Expose an identity Project view whose attrs/schema are
	// that group's output columns; the nested InSub matcher remains the semantic
	// gate for the carrier.
	BOOL FMatchIdentityOverInSub(const CDSLOp *popProj,
								 CExpression *pexprCarrier,
								 CDSLModel *pmodel) const;

public:
	CDSLProjMatcher(const CDSLProjMatcher &) = delete;

	CDSLProjMatcher(CMemoryPool *mp, const CDSLMatcher *pmatcher)
		: m_mp(mp), m_pmatcher(pmatcher)
	{
		GPOS_ASSERT(nullptr != mp);
		GPOS_ASSERT(nullptr != pmatcher);
	}

	// Match a Proj-rooted DSL template against a live CLogicalProject, or against
	// the memo-safe Select(TRUE, pure-dedup) identity view produced by an earlier
	// DSL dedup drop. Returns true iff symbols and the relational child match.
	BOOL FMatch(const CDSLOp *popProj, CExpression *pexprProject,
				CDSLModel *pmodel) const;
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLProjMatcher_H
