//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLRuleEngine.h
//
//	@doc:
//		The shared, stateless-reentrant engine that owns the loaded rule library
//		and drives the three-stage rewrite (match -> check -> instantiate) for
//		the thin per-root-operator xform shells (CXformDSLRule_*).
//
//		Design (see docs/DSL_XFORM_ENGINE_DESIGN.md §五 and
//		docs/WETUNE_ORCA_PER_OP_THREESTAGE.md):
//		  * One process-global singleton with its OWN long-lived CMemoryPool that
//		    holds the parsed CDSLRule library — loaded once, reused across every
//		    optimization (rules carry no per-optimization CColRef, by construction
//		    of the template IR).
//		  * Rules are bucketed by their source-root ORCA EOperatorId; a shell for
//		    operator X asks RulesForRoot(X) for just its rules.
//		  * Match/Check/Instantiate operate on the LIVE optimizer context passed
//		    in via the xform's CMemoryPool (COptCtxt::PoctxtFromTLS()).
//
//		PHASE 1: singleton lifecycle + rule loading + bucketing + RulesForRoot()
//		are real. Match/Check/Instantiate are STUBS (log + return no rewrite);
//		they are implemented in phase 2.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLRuleEngine_H
#define GPOPT_CDSLRuleEngine_H

#include "gpos/base.h"
#include "gpos/common/CHashMap.h"

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRuleLoader.h"
#include "gpopt/dsl/CDSLRulePrefixIndex.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/COperator.h"

namespace gpopt
{
using namespace gpos;

// EOperatorId (enum) -> its rule bucket (a non-owning list of CDSLRule*).
// Key hashed by value. Bucket arrays own one ref of each rule they list.
using COperatorIdToRuleArrayMap =
	CHashMap<ULONG, CDSLRuleArray, gpos::HashValue<ULONG>, gpos::Equals<ULONG>,
			 CleanupDelete<ULONG>, CleanupRelease<CDSLRuleArray> >;

// EOperatorId -> immutable source-template prefix trie for that shell bucket.
using COperatorIdToRulePrefixIndexMap =
	CHashMap<ULONG, CDSLRulePrefixIndex, gpos::HashValue<ULONG>,
			 gpos::Equals<ULONG>, CleanupDelete<ULONG>,
			 CleanupDelete<CDSLRulePrefixIndex> >;

// Rule pointer -> physical one-based line in the source rule file. Keys are
// owned by m_pdrgprule; values are owned by this map. Rules not loaded from a
// file fall back to their stable admitted-library ordinal.
using CDSLRuleToIdMap =
	CHashMap<CDSLRule, ULONG, gpos::HashPtr<CDSLRule>,
			 gpos::EqualPtr<CDSLRule>, CleanupNULL<CDSLRule>,
			 CleanupDelete<ULONG> >;

//---------------------------------------------------------------------------
//	@class:
//		CDSLRuleEngine
//
//	@doc:
//		Process-global rule engine singleton. Thread-safe to READ after Init()
//		(the library is immutable once loaded).
//---------------------------------------------------------------------------
class CDSLRuleEngine
{
private:
	// global instance
	static CDSLRuleEngine *m_instance;

	// long-lived pool owning the loaded rule library + buckets
	CMemoryPool *m_mp;

	// the whole admitted library (owns one ref per rule)
	CDSLRuleArray *m_pdrgprule;

	// source-root EOperatorId -> rules with that root
	COperatorIdToRuleArrayMap *m_phmOpidToRules;

	// same buckets organized as variable-depth source-template prefix tries
	COperatorIdToRulePrefixIndexMap *m_phmOpidToPrefixIndex;

	// admitted rule pointer -> physical source line (or fallback ordinal)
	CDSLRuleToIdMap *m_phmRuleToId;

	// empty bucket returned for roots with no rules (avoids NULL checks in
	// shells); owned, allocated once.
	CDSLRuleArray *m_pdrgpruleEmpty;

	// private ctor
	explicit CDSLRuleEngine(CMemoryPool *mp);

	// bucket a freshly loaded library by source-root EOperatorId
	void BucketByRoot();

public:
	CDSLRuleEngine(const CDSLRuleEngine &) = delete;

	~CDSLRuleEngine();

	//------------------------------------------------------------------
	// lifecycle
	//------------------------------------------------------------------

	// initialize the global instance and load the rule library from szPath
	// (one rule DSL per line, EQ-only). If szPath is NULL or the file is
	// missing, the engine initializes EMPTY (no rules) — shells then no-op.
	// Idempotent: a second call is ignored.
	static void Init(const CHAR *szPath);

	// destroy the global instance (releases the library + its pool)
	static void Shutdown();

	// global accessor; NULL before Init()
	static CDSLRuleEngine *
	Instance()
	{
		return m_instance;
	}

	//------------------------------------------------------------------
	// dispatch
	//------------------------------------------------------------------

	// rules whose SOURCE root maps to eopid (never NULL; empty bucket if none)
	const CDSLRuleArray *PdrgpruleForRoot(COperator::EOperatorId eopid) const;

	// Candidate rules selected by the source-template prefix trie. Caller owns
	// the returned array; rule order remains the physical rule-file order.
	CDSLRuleArray *PdrgpruleCandidates(CMemoryPool *mp,
									 COperator::EOperatorId eopid,
									 CExpression *pexpr) const;

	// total admitted rules (diagnostics)
	ULONG UlRules() const { return m_pdrgprule->Size(); }

	// Stable physical source line. Returns zero only for a pointer that is not
	// owned by this engine.
	ULONG UlRuleId(const CDSLRule *prule) const;

	// Whether any loaded rule needs an ordinary (non-DISTINCT) Proj at the
	// source root. QueryContext uses this to preserve the otherwise implicit
	// top-level SQL projection as a memo-visible identity Project.
	BOOL FHasOrdinaryProjSourceRoot() const;

	// Whether any loaded source fragment contains the requested DSL operator at
	// any depth. Preprocessing uses this capability query to avoid irreversibly
	// deleting shapes that a data rule must inspect in the memo.
	BOOL FHasSourceOperator(EDslOpKind edslop) const;

	//------------------------------------------------------------------
	// three-stage rewrite
	//------------------------------------------------------------------

	// ①: match rule's source template against pexpr, populating *pmodel.
	BOOL FMatch(const CDSLRule *prule, CExpression *pexpr,
				CDSLModel *pmodel) const;

	// ②: check the rule's constraints against the bound model / live metadata.
	BOOL FCheckConstraints(const CDSLRule *prule, const CDSLModel *pmodel,
						   CExpression *pexpr,
						   const CDSLConstraint **ppconFailed = nullptr,
						   ULONG *pulFailed = nullptr) const;

	// ③: instantiate the rule's target template under the bound model, in mp.
	CExpression *PexprInstantiate(CMemoryPool *mp, const CDSLRule *prule,
								  const CDSLModel *pmodel) const;

	// Run the complete match -> check -> instantiate pipeline for one rule.
	// When pg_orca.trace_dsl_rule is enabled, this is also the single attribution
	// point used by every operator shell. Caller owns the returned expression.
	CExpression *PexprApply(CMemoryPool *mp, const CDSLRule *prule,
							CExpression *pexpr) const;
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLRuleEngine_H
