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

	// total admitted rules (diagnostics)
	ULONG UlRules() const { return m_pdrgprule->Size(); }

	//------------------------------------------------------------------
	// three-stage rewrite (PHASE 2 — currently stubs)
	//------------------------------------------------------------------

	// ①: match rule's source template against pexpr, populating *pmodel.
	// PHASE 1 STUB: always returns false.
	BOOL FMatch(const CDSLRule *prule, CExpression *pexpr,
				CDSLModel *pmodel) const;

	// ②: check the rule's constraints against the bound model / live metadata.
	// PHASE 1 STUB: always returns false.
	BOOL FCheckConstraints(const CDSLRule *prule, const CDSLModel *pmodel,
						   CExpression *pexpr) const;

	// ③: instantiate the rule's target template under the bound model, in mp.
	// PHASE 1 STUB: always returns NULL.
	CExpression *PexprInstantiate(CMemoryPool *mp, const CDSLRule *prule,
								  const CDSLModel *pmodel) const;
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLRuleEngine_H
