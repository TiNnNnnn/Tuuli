//---------------------------------------------------------------------------
//	@filename:
//		CJobJoinEnumeration.cpp
//---------------------------------------------------------------------------
#include "gpopt/search/CJobJoinEnumeration.h"

#include <unordered_map>
#include <vector>

#include "gpos/common/CAutoRef.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/engine/CEngine.h"
#include "gpopt/engine/CHint.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalNAryJoin.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarNAryJoinPredList.h"
#include "gpopt/optimizer/COptimizerConfig.h"
#include "gpopt/search/CGroup.h"
#include "gpopt/search/CGroupExpression.h"
#include "gpopt/search/CGroupProxy.h"
#include "gpopt/search/CJobFactory.h"
#include "gpopt/search/CScheduler.h"
#include "gpopt/search/CSchedulerContext.h"
#include "gpopt/xforms/CDPHyperJoinRegion.h"
#include "gpopt/xforms/CDPHyperPlan.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

namespace
{
class CSubsetGroups
{
	struct SEntry
	{
		CBitSet *m_nodes;
		CGroup *m_group;

		SEntry(CMemoryPool *mp, const CBitSet *nodes, CGroup *group)
			: m_nodes(GPOS_NEW(mp) CBitSet(mp, *nodes)), m_group(group)
		{
		}
		~SEntry()
		{
			m_nodes->Release();
		}
	};

	CMemoryPool *m_mp;
	std::unordered_map<ULONG, std::vector<SEntry *>> m_buckets;
	std::vector<SEntry *> m_entries;

public:
	explicit CSubsetGroups(CMemoryPool *mp) : m_mp(mp)
	{
	}

	~CSubsetGroups()
	{
		for (SEntry *entry : m_entries)
		{
			GPOS_DELETE(entry);
		}
	}

	CGroup *
	Lookup(const CBitSet *nodes) const
	{
		auto bucket = m_buckets.find(nodes->HashValue());
		if (m_buckets.end() == bucket)
		{
			return nullptr;
		}
		for (const SEntry *entry : bucket->second)
		{
			if (entry->m_nodes->Equals(nodes))
			{
				return entry->m_group;
			}
		}
		return nullptr;
	}

	void
	Record(const CBitSet *nodes, CGroup *group)
	{
		CGroup *existing = Lookup(nodes);
		GPOS_ASSERT_IMP(nullptr != existing, existing == group);
		if (nullptr != existing)
		{
			return;
		}
		SEntry *entry = GPOS_NEW(m_mp) SEntry(m_mp, nodes, group);
		m_entries.push_back(entry);
		m_buckets[nodes->HashValue()].push_back(entry);
	}
};

CExpression *
PexprGroupLeaf(CMemoryPool *mp, CGroup *group)
{
	CGroupProxy proxy(group);
	CGroupExpression *gexpr = proxy.PgexprNextLogical(nullptr);
	GPOS_ASSERT(nullptr != gexpr);
	gexpr->Pop()->AddRef();
	return GPOS_NEW(mp) CExpression(mp, gexpr->Pop(), gexpr);
}

CGroupExpression *
PgexprLogicalRepresentative(CGroup *group)
{
	CGroupProxy proxy(group);
	return proxy.PgexprNextLogical(nullptr);
}

void
TraceUnsupported(CGroupExpression *pgexpr, const CHAR *reason,
				 ULONG node_count)
{
	if (GPOS_FTRACE(EopttracePrintXformResults))
	{
		GPOS_TRACE_FORMAT(
			"DPHyper: status=fallback reason=%s group=%d nodes=%d",
			reason, pgexpr->Pgroup()->Id(), node_count);
	}
}
}  // namespace

CJobJoinEnumeration::CJobJoinEnumeration() : m_pgexpr(nullptr)
{
}

CJobJoinEnumeration::~CJobJoinEnumeration() = default;

void
CJobJoinEnumeration::Init(CGroupExpression *pgexpr)
{
	GPOS_ASSERT(!FInit());
	GPOS_ASSERT(nullptr != pgexpr);
	m_pgexpr = pgexpr;
	SetInit();
}

void
CJobJoinEnumeration::ScheduleJob(CSchedulerContext *psc,
								 CGroupExpression *pgexpr, CJob *parent)
{
	CJob *job = psc->Pjf()->PjCreate(CJob::EjtJoinEnumeration);
	CJobJoinEnumeration *enumeration = PjConvert(job);
	enumeration->Init(pgexpr);
	psc->Psched()->Add(enumeration, parent);
}

BOOL
CJobJoinEnumeration::FExecute(CSchedulerContext *psc)
{
	GPOS_ASSERT(FInit());
	const BOOL success = FEnumerate(psc);
	m_pgexpr->SetDPHyperStatus(
		success ? CGroupExpression::EdphSucceeded
				: CGroupExpression::EdphFallback);
	return true;
}

BOOL
CJobJoinEnumeration::FEnumerate(CSchedulerContext *psc)
{
	if (COperator::EopLogicalNAryJoin != m_pgexpr->Pop()->Eopid() ||
		m_pgexpr->Arity() < 3)
	{
		TraceUnsupported(m_pgexpr, "not_nary_region", 0);
		return false;
	}
	CLogicalNAryJoin *nary = CLogicalNAryJoin::PopConvert(m_pgexpr->Pop());
	const ULONG node_count = m_pgexpr->Arity() - 1;
	if (nary->HasOuterJoinChildren())
	{
		TraceUnsupported(m_pgexpr, "non_inner_join", node_count);
		return false;
	}

	CMemoryPool *mp = psc->GetGlobalMemoryPool();
	CExpressionArray *components = GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG node = 0; node < node_count; ++node)
	{
		CGroupExpression *child_gexpr =
			PgexprLogicalRepresentative((*m_pgexpr)[node]);
		if (nullptr == child_gexpr)
		{
			components->Release();
			TraceUnsupported(m_pgexpr, "missing_component", node_count);
			return false;
		}
		child_gexpr->Pop()->AddRef();
		CExpression *component =
			GPOS_NEW(mp) CExpression(mp, child_gexpr->Pop(), child_gexpr);
		// Sibling-correlated/LATERAL regions require directed dependency edges.
		// Until those are represented, conservatively retain DPv2 for the region.
		if (0 < component->DeriveOuterReferences()->Size())
		{
			component->Release();
			components->Release();
			TraceUnsupported(m_pgexpr, "lateral_dependency", node_count);
			return false;
		}
		components->Append(component);
	}

	CExpression *scalar = (*m_pgexpr)[node_count]->PexprScalarRep();
	if (nullptr == scalar ||
		COperator::EopScalarNAryJoinPredList == scalar->Pop()->Eopid())
	{
		components->Release();
		TraceUnsupported(m_pgexpr,
					 nullptr == scalar ? "missing_predicate"
								   : "non_inner_predicate_list",
					 node_count);
		return false;
	}
	CExpressionArray *conjuncts =
		CPredicateUtils::PdrgpexprConjuncts(mp, scalar);
	CHint *hint =
		COptCtxt::PoctxtFromTLS()->GetOptimizerConfig()->GetHint();
	CDPHyperJoinRegion region(mp, components, conjuncts,
							 hint->UlDPHyperEdgeBudget());
	components->Release();
	conjuncts->Release();
	if (!region.Build())
	{
		if (GPOS_FTRACE(EopttracePrintXformResults))
		{
			GPOS_TRACE_FORMAT(
				"DPHyper: status=fallback reason=edge_budget group=%d "
				"nodes=%d edges=%d budget=%d",
				m_pgexpr->Pgroup()->Id(), node_count,
				region.GeneratedEdgeCount(), hint->UlDPHyperEdgeBudget());
		}
		return false;
	}

	CDPHyperPlan plan(mp, hint->UlDPHyperPairBudget());
	CDPHyperEnumerator enumerator(mp, region.Graph(), &plan);
	if (enumerator.Enumerate() || !plan.Complete(node_count))
	{
		if (GPOS_FTRACE(EopttracePrintXformResults))
		{
			GPOS_TRACE_FORMAT(
				"DPHyper: status=fallback reason=%s group=%d nodes=%d "
				"pairs=%d budget=%d",
				plan.BudgetExhausted() ? "pair_budget" : "disconnected",
				m_pgexpr->Pgroup()->Id(), node_count, plan.PairCount(),
				hint->UlDPHyperPairBudget());
		}
		return false;
	}

	// Enumeration above is side-effect free. From this point onward all child
	// subsets are known to exist and pairs are topologically ordered, so Memo
	// insertion cannot expose a partial DPHyper result due to a graph/budget
	// failure.
	CSubsetGroups subset_groups(mp);
	for (ULONG node = 0; node < node_count; ++node)
	{
		CAutoRef<CBitSet> singleton(GPOS_NEW(mp) CBitSet(mp));
		(void) singleton->ExchangeSet(node);
		subset_groups.Record(singleton.Value(), (*m_pgexpr)[node]);
	}

	CEngine *engine = psc->Peng();
	for (const CDPHyperPlan::SPair *pair : plan.Pairs())
	{
		CGroup *left_group = subset_groups.Lookup(pair->m_left);
		CGroup *right_group = subset_groups.Lookup(pair->m_right);
		GPOS_ASSERT(nullptr != left_group && nullptr != right_group);

		CAutoRef<CBitSet> joined(
			GPOS_NEW(mp) CBitSet(mp, *pair->m_left));
		joined->Union(pair->m_right);
		const BOOL full = joined->Size() == node_count;
		CGroup *target = full ? m_pgexpr->Pgroup()
							  : subset_groups.Lookup(joined.Value());

		CExpression *predicate = region.PexprPredicate(
			pair->m_left, pair->m_right, full /*include residual*/);
		CExpression *join = CUtils::PexprLogicalJoin<CLogicalInnerJoin>(
			mp, PexprGroupLeaf(mp, left_group),
			PexprGroupLeaf(mp, right_group), predicate,
			CXform::ExfExpandNAryJoinDPHyper);
		target = engine->PgroupInsert(
			target, join, CXform::ExfExpandNAryJoinDPHyper, m_pgexpr,
			!full /*intermediate*/);
		join->Release();
		GPOS_ASSERT(nullptr != target);
		subset_groups.Record(joined.Value(), target);

		// DPHyp reports an unordered CSG-CMP pair once. InnerJoin is physically
		// asymmetric, so retain both orientations in the same equivalence group.
		predicate = region.PexprPredicate(pair->m_right, pair->m_left,
										 full /*include residual*/);
		join = CUtils::PexprLogicalJoin<CLogicalInnerJoin>(
			mp, PexprGroupLeaf(mp, right_group),
			PexprGroupLeaf(mp, left_group), predicate,
			CXform::ExfExpandNAryJoinDPHyper);
		CGroup *reverse_target = engine->PgroupInsert(
			target, join, CXform::ExfExpandNAryJoinDPHyper, m_pgexpr,
			!full /*intermediate*/);
		join->Release();
		GPOS_ASSERT(reverse_target == target);
	}
	if (GPOS_FTRACE(EopttracePrintXformResults))
	{
		GPOS_TRACE_FORMAT(
			"DPHyper: status=applied group=%d nodes=%d edges=%d "
			"cartesian_edges=%d pairs=%d subsets=%d mode=%s",
			m_pgexpr->Pgroup()->Id(), node_count,
			region.GeneratedEdgeCount(), region.CartesianEdgeCount(),
			plan.PairCount(), plan.SeenCount(),
			GPOS_FTRACE(EopttraceDPHyperShadow) ? "shadow" : "replacement");
	}
	return true;
}

#ifdef GPOS_DEBUG
IOstream &
CJobJoinEnumeration::OsPrint(IOstream &os) const
{
	return os << "DPHyper join enumeration for group expression "
			  << (nullptr == m_pgexpr ? GPOPT_INVALID_GEXPR_ID : m_pgexpr->Id());
}
#endif
