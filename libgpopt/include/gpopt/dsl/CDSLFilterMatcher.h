//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLFilterMatcher.h
//
//	@doc:
//		The hardest part of stage ① (see docs/WETUNE_ORCA_PER_OP_THREESTAGE.md §2
//		and docs/WETUNE_TEST_MIGRATION.md §3): reconciling WeTune's single-
//		predicate Filter CHAIN with ORCA's single CLogicalSelect carrying a
//		conjunctive predicate CScalarBoolOp(And).
//
//		The mismatch:
//		  WeTune : Filter<p2 a2>(Filter<p1 a1>(Filter<p0 a0>(base)))  — a chain of
//		           single-predicate Filters, matched holistically with subset /
//		           reorder / merge (FilterMatcher / FilterChain / FilterAssignments).
//		  ORCA   : CLogicalSelect(base, And(c0, c1, c2, ...)), or adjacent
//		           Select nodes retained in separate Memo groups.
//
//		Approach (engine-side; ORCA core untouched):
//		  1. MATCH  — peel the DSL Filter chain (root, following child[0], while
//		     the op is a Filter) down to its non-Filter base op. Flatten the
//		     predicates from only as many adjacent Select levels as the source
//		     template declares, then flatten them to a conjunct SET via
//		     CPredicateUtils::PdrgpexprConjuncts. Assign each DSL Filter's pred
//		     symbol <p> to a DISTINCT conjunct and its attrs symbol <a> to that
//		     conjunct's used columns — a subset match with reordering
//		     (backtracking), mirroring WeTune's grouped/free sub-matchers.
//		  2. BASE   — the chain's base op recurses through the generic matcher
//		     against the Select's relational child[0].
//		  3. RESIDUAL — conjuncts not consumed by any DSL Filter are recorded on
//		     the model (CDSLModel::SetResidualConjuncts) so the instantiator (#27)
//		     carries them through unchanged. Dropping a conjunct = wrong plan.
//
//		Key simplification vs WeTune (doc §2 / §11): we do NOT reproduce
//		NormalizeFilter's lexicographic chain ordering; matching a conjunct SET
//		makes predicate order irrelevant by construction.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLFilterMatcher_H
#define GPOPT_CDSLFilterMatcher_H

#include "gpos/base.h"

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

// fwd decl — the generic matcher the base op recurses back into
class CDSLMatcher;

//---------------------------------------------------------------------------
//	@class:
//		CDSLFilterMatcher
//
//	@doc:
//		Matches a DSL Filter chain against an ORCA CLogicalSelect. For an inner
//		join whose single-side Filter was already pushed into an input, it consumes
//		the equivalent pre-pushdown Select(Join) view from CDSLMatchView. Constructed per
//		match attempt with the transient pool and a back-reference to the generic
//		matcher (so the chain base can recurse). Owns no state beyond those.
//---------------------------------------------------------------------------
class CDSLFilterMatcher
{
private:
	CMemoryPool *m_mp;

	// generic matcher to recurse the chain's base op into (not owned)
	const CDSLMatcher *m_pmatcher;

	// complete rule (not owned), used only to constrain ambiguous conjunct
	// assignments by source-side AttrsEq/PredicateEq declarations.
	const CDSLRule *m_prule;

	// collect the maximal run of Filter ops starting at popFilterRoot, following
	// child[0], into rgpopFilters (non-owning; the rule IR outlives the match).
	// Writes the count to *pulFilters and returns the first non-Filter op (the
	// chain base; never NULL for a valid Filter-rooted template). rgpopFilters
	// must hold at least ulCapacity entries; returns NULL if the chain is longer.
	const CDSLOp *PopCollectChain(const CDSLOp *popFilterRoot,
								  const CDSLOp **rgpopFilters, ULONG ulCapacity,
								  ULONG *pulFilters) const;

	// try to assign DSL Filters[ulFilter..] to conjuncts, backtracking without
	// mutating the model. Distinct conjuncts are preferred, but a conjunct may be
	// reused as the normalized view of Filter(p, Filter(p, child)). rgfUsed marks
	// conjuncts consumed at least once and rgulAssigned stores each selection.
	BOOL FAssign(const CDSLOp **rgpopFilters, ULONG ulFilters, ULONG ulFilter,
				 CExpressionArray *pdrgpexprConj, const CDSLOp *popBase,
				 CExpression *pexprBase, BOOL *rgfUsed,
				 ULONG *rgulAssigned) const;

	// Check source-side equality constraints between a proposed Filter binding
	// and filters already assigned on this branch.
	BOOL FAssignmentCompatible(const CDSLOp **rgpopFilters, ULONG ulFilter,
						   CExpressionArray *pdrgpexprConj,
						   const ULONG *rgulAssigned, const CDSLOp *popBase,
						   CExpression *pexprBase,
						   CExpression *pexprCandidate) const;

	// If a source AttrsEq connects this Filter's attrs to a direct Join key,
	// reject conjunct candidates that use different columns before committing
	// bindings. This extends constraint-aware backtracking across the Filter/base
	// boundary without adding rollback state to CDSLModel.
	BOOL FBaseAssignmentCompatible(const CDSLOp *popFilter,
								   const CDSLOp *popBase,
								   CExpression *pexprBase,
								   CExpression *pexprCandidate) const;

	// Bind Filter<p,local[,outer]>. The legacy vector receives every dependency;
	// the extended vectors partition dependencies by the actual child output.
	BOOL FBindFilterSymbols(const CDSLOp *popFilter, CExpression *pexprConj,
							CExpression *pexprBase,
							CDSLModel *pmodel) const;

	// after a successful assignment, record the conjuncts left unused on the
	// model as residual (AddRef'd), for the instantiator to preserve.
	void RecordResidual(CExpressionArray *pdrgpexprConj, const BOOL *rgfUsed,
						CDSLModel *pmodel) const;

	// Match Filter(InnerJoin(...)) against a Select pushed anywhere along a safe
	// inner/preserved join spine. Only a conjunct selected by source
	// AttrsEq(Filter.attrs, Join.side_keys) is eligible.
	BOOL FMatchPushedDownInnerJoin(const CDSLOp *popFilterRoot,
								 CExpression *pexprJoin,
								 CDSLModel *pmodel) const;

	// Match WeTune's SimpleFilter view of a single-column IN/semi-join carrier.
	BOOL FMatchSubqueryCarrier(const CDSLOp *popFilterRoot,
							 CExpression *pexprCarrier,
							 CDSLModel *pmodel) const;

public:
	CDSLFilterMatcher(const CDSLFilterMatcher &) = delete;

	CDSLFilterMatcher(CMemoryPool *mp, const CDSLMatcher *pmatcher,
					  const CDSLRule *prule)
		: m_mp(mp), m_pmatcher(pmatcher), m_prule(prule)
	{
		GPOS_ASSERT(nullptr != mp);
		GPOS_ASSERT(nullptr != pmatcher);
	}

	// Match a Filter-rooted DSL template against a live CLogicalSelect or the
	// equivalent pushed-down inner-join view. Returns true iff the whole chain
	// (and its base) matched; populates pmodel with the <p>/<a> bindings,
	// residual conjuncts, and the base subtree bindings.
	BOOL FMatch(const CDSLOp *popFilterRoot, CExpression *pexpr,
				CDSLModel *pmodel) const;
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLFilterMatcher_H
