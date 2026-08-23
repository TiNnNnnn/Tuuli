//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		COptCtxt.h
//
//	@doc:
//		Optimizer context object; contains all global objects pertaining to
//		one optimization
//---------------------------------------------------------------------------
#ifndef GPOPT_COptCtxt_H
#define GPOPT_COptCtxt_H

#include "gpos/base.h"
#include "gpos/common/CHashMapIter.h"
#include "gpos/task/CTaskLocalStorageObject.h"

#include "gpopt/base/CCTEInfo.h"
#include "gpopt/base/CColumnFactory.h"
#include "gpopt/base/IComparator.h"
#include "gpopt/base/SPartSelectorInfo.h"
#include "gpopt/mdcache/CMDAccessor.h"

namespace gpopt
{
using namespace gpos;

// hash maps ULONG -> array of ULONGs
using UlongToBitSetMap =
	CHashMap<ULONG, CBitSet, gpos::HashValue<ULONG>, gpos::Equals<ULONG>,
			 CleanupDelete<ULONG>, CleanupRelease<CBitSet>>;

struct SDSLRuleTraceCounters
{
	ULONG m_stage_attempts[7];
	ULONG m_bound_symbols;

	SDSLRuleTraceCounters()
		: m_stage_attempts{0, 0, 0, 0, 0, 0, 0}, m_bound_symbols(0)
	{
	}

	ULONG
	UlAttempts() const
	{
		ULONG ulAttempts = 0;
		for (ULONG ul = 0; ul < 7; ul++)
		{
			ulAttempts += m_stage_attempts[ul];
		}
		return ulAttempts;
	}
};

using UlongToDSLRuleTraceCountersMap =
	CHashMap<ULONG, SDSLRuleTraceCounters, gpos::HashValue<ULONG>,
			 gpos::Equals<ULONG>, CleanupDelete<ULONG>,
			 CleanupDelete<SDSLRuleTraceCounters>>;

using UlongToDSLRuleTraceCountersMapIter =
	CHashMapIter<ULONG, SDSLRuleTraceCounters, gpos::HashValue<ULONG>,
				 gpos::Equals<ULONG>, CleanupDelete<ULONG>,
				 CleanupDelete<SDSLRuleTraceCounters>>;

using UlongToUlongMap =
	CHashMap<ULONG, ULONG, gpos::HashValue<ULONG>, gpos::Equals<ULONG>,
			 CleanupDelete<ULONG>, CleanupDelete<ULONG>>;

// forward declarations
class CColRefSet;
class COptimizerConfig;
class ICostModel;
class IConstExprEvaluator;

//---------------------------------------------------------------------------
//	@class:
//		COptCtxt
//
//	@doc:
//		"Optimizer Context" is a container of global objects (mostly
//		singletons) that are needed by the optimizer.
//
//		A COptCtxt object is instantiated in COptimizer::PdxlnOptimize() via
//		COptCtxt::PoctxtCreate() and stored as a task local object. The global
//		information contained in it can be accessed by calling
//		COptCtxt::PoctxtFromTLS(), instead of passing a pointer to it all
//		around. For example to get the global CMDAccessor:
//			CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
//
//---------------------------------------------------------------------------
class COptCtxt : public CTaskLocalStorageObject
{
private:
	// shared memory pool
	CMemoryPool *m_mp;

	// column factory
	CColumnFactory *m_pcf;

	// metadata accessor;
	CMDAccessor *m_pmda;

	// cost model
	ICostModel *m_cost_model;

	// constant expression evaluator
	IConstExprEvaluator *m_pceeval;

	// comparator between IDatum instances
	IComparator *m_pcomp;

	// atomic counter for generating part index ids
	ULONG m_auPartId;

	// global CTE information
	CCTEInfo *m_pcteinfo;

	// system columns required in query output
	CColRefArray *m_pdrgpcrSystemCols;

	// optimizer configurations
	COptimizerConfig *m_optimizer_config;

	// whether or not we are optimizing a DML query
	BOOL m_fDMLQuery;

	// value for the first valid part id
	static ULONG m_ulFirstValidPartId;

	// if there are master only tables in the query
	BOOL m_has_master_only_tables;

	// does the query contain any volatile functions or
	// functions that read/modify SQL data
	BOOL m_has_volatile_func{false};

	// does the query have replicated tables
	BOOL m_has_replicated_tables;

	// does this plan have a direct dispatchable filter
	CExpressionArray *m_direct_dispatchable_filters;

	// mappings of dynamic scan -> partition indexes (after static elimination)
	// this is mainetained here to avoid dependencies on optimization order
	// between dynamic scans/partition selectors and remove the assumption
	// of one being optimized before the other. Instead, we populate the
	// partitions during optimization of the dynamic scans, and populate
	// the partitions for the corresponding partition selector in
	// ExprToDXL. We could possibly do this in DXLToPlstmt, but we would be
	// making an assumption about the order the scan vs partition selector
	// is translated, and would also need information from the append's
	// child dxl nodes.
	UlongToBitSetMap *m_scanid_to_part_map;

	// unique id per partition selector in the memo
	ULONG m_selector_id_counter;

	// Currently-deriving join's combined equivalence classes (borrowed
	// from CPropConstraint, NOT ref-counted here).  Set by
	// CJoinStatsProcessor::DeriveJoinStats via the RAII guard
	// CJoinECScope, read by ApplyForeignKeyAdjustment to do EC-aware
	// FK matching.  Nullptr outside a join's stats derivation scope.
	// Lifetime: borrowed from the exprhdl's CPropConstraint, which lives
	// at least as long as the join's stats derivation call.
	CColRefSetArray *m_join_equiv_classes{nullptr};

	// detailed info (filter expr, stats etc) per partition selector
	// (required by CDynamicPhysicalScan for recomputing statistics for DPE)
	SPartSelectorInfo *m_part_selector_info;

	// Compact DSL trace events already emitted in this optimization. The key is
	// rule_id * 5 + stage, so repeated Cascades attempts do not exhaust the trace
	// buffer. Verbose tracing bypasses this set.
	CBitSet *m_dsl_trace_events;

	// Uncompacted per-rule counters used to explain search-space growth.
	UlongToDSLRuleTraceCountersMap *m_dsl_rule_trace_counters;

	ULONG m_ulDSLGeneratedAlternatives;

	UlongToUlongMap *m_dsl_generated_alternatives_by_rule;

public:
	COptCtxt(COptCtxt &) = delete;

	// Mark one compact DSL rule/stage event. Returns true only for its first
	// occurrence in this optimization context.
	BOOL
	FMarkDSLTraceEvent(ULONG ulRuleId, ULONG ulStage)
	{
		GPOS_ASSERT(ulStage < 7);
		return !m_dsl_trace_events->ExchangeSet(ulRuleId * 7 + ulStage);
	}

	void RecordDSLRuleTrace(ULONG ulRuleId, ULONG ulStage,
						ULONG ulBoundSymbols);

	// Reserve one query-level DSL alternative. A zero configured maximum means
	// unlimited. The counter is query-local because COptCtxt lives in TLS.
	BOOL FReserveDSLAlternative(ULONG ulRuleId);

	// Return true once either the query-wide or per-rule generation budget has
	// already been consumed. This permits an early exit before matching work.
	BOOL FDSLAlternativeBudgetExhausted(ULONG ulRuleId);

	UlongToDSLRuleTraceCountersMap *
	PdrgDSLRuleTraceCounters() const
	{
		return m_dsl_rule_trace_counters;
	}

	// ctor
	COptCtxt(CMemoryPool *mp, CColumnFactory *col_factory,
			 CMDAccessor *md_accessor, IConstExprEvaluator *pceeval,
			 COptimizerConfig *optimizer_config);

	// dtor
	~COptCtxt() override;

	// memory pool accessor
	CMemoryPool *
	Pmp() const
	{
		return m_mp;
	}

	// optimizer configurations
	COptimizerConfig *
	GetOptimizerConfig() const
	{
		return m_optimizer_config;
	}

	// are we optimizing a DML query
	BOOL
	FDMLQuery() const
	{
		return m_fDMLQuery;
	}

	// set the DML flag
	void
	MarkDMLQuery(BOOL fDMLQuery)
	{
		m_fDMLQuery = fDMLQuery;
	}

	void
	SetHasMasterOnlyTables()
	{
		m_has_master_only_tables = true;
	}

	void
	SetHasVolatileFunc()
	{
		m_has_volatile_func = true;
	}

	void
	SetHasReplicatedTables()
	{
		m_has_replicated_tables = true;
	}

	void
	AddDirectDispatchableFilterCandidate(CExpression *filter_expression)
	{
		filter_expression->AddRef();
		m_direct_dispatchable_filters->Append(filter_expression);
	}

	BOOL
	HasMasterOnlyTables() const
	{
		return m_has_master_only_tables;
	}

	BOOL
	HasVolatileFunc() const
	{
		return m_has_volatile_func;
	}

	BOOL
	HasReplicatedTables() const
	{
		return m_has_replicated_tables;
	}

	CExpressionArray *
	GetDirectDispatchableFilters() const
	{
		return m_direct_dispatchable_filters;
	}

	BOOL
	OptimizeDMLQueryWithSingletonSegment() const
	{
		// A DML statement can be optimized by enforcing a gather motion on segment instead of master,
		// whenever a singleton execution is needed.
		// This optmization can not be applied if the query contains any of the following:
		// (1). master-only tables
		// (2). a volatile function
		return !GPOS_FTRACE(EopttraceDisableNonMasterGatherForDML) &&
			   FDMLQuery() && !HasMasterOnlyTables() && !HasVolatileFunc();
	}

	// column factory accessor
	CColumnFactory *
	Pcf() const
	{
		return m_pcf;
	}

	// metadata accessor
	CMDAccessor *
	Pmda() const
	{
		return m_pmda;
	}

	// cost model accessor
	ICostModel *
	GetCostModel() const
	{
		return m_cost_model;
	}

	// constant expression evaluator
	IConstExprEvaluator *
	Pceeval()
	{
		return m_pceeval;
	}

	// comparator
	const IComparator *
	Pcomp()
	{
		return m_pcomp;
	}

	// cte info
	CCTEInfo *
	Pcteinfo()
	{
		return m_pcteinfo;
	}

	// equivalence classes of the join currently being stats-derived;
	// see m_join_equiv_classes.
	CColRefSetArray *
	GetJoinEquivClasses() const
	{
		return m_join_equiv_classes;
	}
	// Returns the previous value so a RAII scope can restore it.
	CColRefSetArray *
	SetJoinEquivClasses(CColRefSetArray *new_value)
	{
		CColRefSetArray *prev = m_join_equiv_classes;
		m_join_equiv_classes = new_value;
		return prev;
	}

	// return a new part index id
	ULONG
	UlPartIndexNextVal()
	{
		return m_auPartId++;
	}

	ULONG
	NextPartSelectorId()
	{
		return m_selector_id_counter++;
	}

	// required system columns
	CColRefArray *
	PdrgpcrSystemCols() const
	{
		return m_pdrgpcrSystemCols;
	}

	void AddPartForScanId(ULONG scanid, ULONG index);

	CBitSet *
	GetPartitionsForScanId(ULONG scanid)
	{
		return m_scanid_to_part_map->Find(&scanid);
	}

	BOOL AddPartSelectorInfo(ULONG selector_id, SPartSelectorInfoEntry *entry);

	const SPartSelectorInfoEntry *GetPartSelectorInfo(ULONG selector_id) const;

	// set required system columns
	void
	SetReqdSystemCols(CColRefArray *pdrgpcrSystemCols)
	{
		GPOS_ASSERT(nullptr != pdrgpcrSystemCols);

		CRefCount::SafeRelease(m_pdrgpcrSystemCols);
		m_pdrgpcrSystemCols = pdrgpcrSystemCols;
	}

	// factory method
	static COptCtxt *PoctxtCreate(CMemoryPool *mp, CMDAccessor *md_accessor,
								  IConstExprEvaluator *pceeval,
								  COptimizerConfig *optimizer_config);

	// shorthand to retrieve opt context from TLS
	inline static COptCtxt *
	PoctxtFromTLS()
	{
		return reinterpret_cast<COptCtxt *>(
			ITask::Self()->GetTls().Get(CTaskLocalStorage::EtlsidxOptCtxt));
	}

	// return true if all enforcers are enabled
	static BOOL FAllEnforcersEnabled();

};	// class COptCtxt
}  // namespace gpopt


#endif	// !GPOPT_COptCtxt_H

// EOF
