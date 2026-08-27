//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CJobGroupExpressionExploration.cpp
//
//	@doc:
//		Implementation of group expression exploration job
//---------------------------------------------------------------------------

#include "gpopt/search/CJobGroupExpressionExploration.h"

#include <unordered_set>

#include "gpos/common/CAutoRef.h"

#include "gpopt/engine/CEngine.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogical.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CLogicalJoin.h"
#include "gpopt/operators/CLogicalNAryJoin.h"
#include "gpopt/search/CGroup.h"
#include "gpopt/search/CGroupExpression.h"
#include "gpopt/search/CGroupProxy.h"
#include "gpopt/search/CJobFactory.h"
#include "gpopt/search/CJobGroupExploration.h"
#include "gpopt/search/CJobJoinEnumeration.h"
#include "gpopt/search/CJobTransformation.h"
#include "gpopt/search/CScheduler.h"
#include "gpopt/search/CSchedulerContext.h"
#include "gpopt/xforms/CXformFactory.h"


using namespace gpopt;

namespace
{
void
ClearDPHyperJoinEnumeration(CXformSet *xform_set,
							COperator::EOperatorId op_id)
{
	switch (op_id)
	{
		case COperator::EopLogicalInnerJoin:
			(void) xform_set->ExchangeClear(
				CXform::ExfInnerJoinCommutativity);
			(void) xform_set->ExchangeClear(CXform::ExfJoinAssociativity);
			(void) xform_set->ExchangeClear(CXform::ExfInnerJoinSemiJoinSwap);
			(void) xform_set->ExchangeClear(
				CXform::ExfInnerJoinAntiSemiJoinSwap);
			(void) xform_set->ExchangeClear(
				CXform::ExfInnerJoinAntiSemiJoinNotInSwap);
			break;
		case COperator::EopLogicalLeftOuterJoin:
			(void) xform_set->ExchangeClear(CXform::ExfLeftJoin2RightJoin);
			break;
		case COperator::EopLogicalLeftSemiJoin:
			(void) xform_set->ExchangeClear(CXform::ExfSemiJoinSemiJoinSwap);
			(void) xform_set->ExchangeClear(
				CXform::ExfSemiJoinAntiSemiJoinSwap);
			(void) xform_set->ExchangeClear(
				CXform::ExfSemiJoinAntiSemiJoinNotInSwap);
			(void) xform_set->ExchangeClear(CXform::ExfSemiJoinInnerJoinSwap);
			break;
		case COperator::EopLogicalLeftAntiSemiJoin:
			(void) xform_set->ExchangeClear(
				CXform::ExfAntiSemiJoinAntiSemiJoinSwap);
			(void) xform_set->ExchangeClear(
				CXform::ExfAntiSemiJoinAntiSemiJoinNotInSwap);
			(void) xform_set->ExchangeClear(
				CXform::ExfAntiSemiJoinSemiJoinSwap);
			(void) xform_set->ExchangeClear(
				CXform::ExfAntiSemiJoinInnerJoinSwap);
			break;
		case COperator::EopLogicalFullOuterJoin:
			(void) xform_set->ExchangeClear(
				CXform::ExfFullJoinCommutativity);
			break;
		default:
			GPOS_ASSERT(!"Unsupported DPHyper join-region root");
	}
}

void
ConfigureNAryFallback(CXformSet *xform_set, BOOL use_greedy)
{
	(void) xform_set->ExchangeClear(CXform::ExfExpandNAryJoin);
	(void) xform_set->ExchangeClear(CXform::ExfExpandNAryJoinMinCard);
	(void) xform_set->ExchangeClear(CXform::ExfExpandNAryJoinDP);
	(void) xform_set->ExchangeClear(CXform::ExfExpandNAryJoinGreedy);
	(void) xform_set->ExchangeClear(CXform::ExfExpandNAryJoinDPv2);
	// Search-stage intersection normally leaves exactly one native policy.
	// DPHyper fallback owns this decision and must therefore restore the safe
	// non-DP policy explicitly.
	(void) xform_set->ExchangeSet(
		use_greedy ? CXform::ExfExpandNAryJoinGreedy
				   : CXform::ExfExpandNAryJoin);
}

BOOL
FHasPendingJoinIngress(CMemoryPool *mp, CGroup *group,
					   std::unordered_set<CGroup *> *visited)
{
	if (group->FScalar() || !visited->insert(group).second)
	{
		return false;
	}
	CGroupProxy proxy(group);
	CGroupExpression *gexpr = nullptr;
	while (nullptr != (gexpr = proxy.PgexprNextLogical(gexpr)))
	{
		if (nullptr != dynamic_cast<CLogicalApply *>(gexpr->Pop()))
		{
			return true;
		}
		CExpressionHandle exprhdl(mp);
		exprhdl.Attach(gexpr);
		for (ULONG child = 0; child < gexpr->Arity(); ++child)
		{
			CGroup *child_group = (*gexpr)[child];
			if (child_group->FScalar())
			{
				if (exprhdl.DeriveHasSubquery(child))
				{
					return true;
				}
			}
			else if (FHasPendingJoinIngress(mp, child_group, visited))
			{
				return true;
			}
		}
	}
	return false;
}

BOOL
FHasPendingJoinIngress(CMemoryPool *mp, CGroupExpression *gexpr)
{
	std::unordered_set<CGroup *> visited;
	for (ULONG child = 0; child < gexpr->Arity(); ++child)
	{
		CGroup *child_group = (*gexpr)[child];
		if (!child_group->FScalar() &&
			FHasPendingJoinIngress(mp, child_group, &visited))
		{
			return true;
		}
	}
	return false;
}

BOOL
FScheduleDPHyperBeforeChildren(CSchedulerContext *psc,
							 CGroupExpression *gexpr, CJob *parent)
{
	if (CGroupExpression::EdphUnrequested != gexpr->DPHyperStatus())
	{
		return false;
	}
	CAutoRef<CXformSet> candidates(
		CLogical::PopConvert(gexpr->Pop())
			->PxfsCandidates(psc->GetGlobalMemoryPool()));
	candidates->Intersection(CXformFactory::Pxff()->PxfsExploration());
	candidates->Intersection(psc->Peng()->PxfsCurrentStage());
	if (!candidates->Get(CXform::ExfExpandNAryJoinDPHyper))
	{
		// The stage or xform-disable policy declined ownership. Marked joins
		// retain all native join rewrites because no success status is published.
		return false;
	}
	if (FHasPendingJoinIngress(psc->GetGlobalMemoryPool(), gexpr))
	{
		// Explore subquery/Apply components first so their join-producing
		// alternatives become visible to the maximal-region extractor. The
		// parent still runs DPHyper before scheduling any of its own native xforms.
		return false;
	}
	gexpr->SetDPHyperStatus(CGroupExpression::EdphScheduled);
	CJobJoinEnumeration::ScheduleJob(psc, gexpr, parent);
	return true;
}
}  // namespace

// State transition diagram for group expression exploration job state machine;
//
// +-----------------------+   eevExploringChildren
// |    estInitialized:    | -----------------------+
// | EevtExploreChildren() |                        |
// |                       | <----------------------+
// +-----------------------+
//   |
//   | eevChildrenExplored
//   v
// +-----------------------+   eevExploringSelf
// | estChildrenExplored:  | -----------------------+
// |   EevtExploreSelf()   |                        |
// |                       | <----------------------+
// +-----------------------+
//   |
//   | eevSelfExplored
//   v
// +-----------------------+
// |   estSelfExplored:    |
// |    EevtFinalize()     |
// +-----------------------+
//   |
//   | eevFinalized
//   v
// +-----------------------+
// |     estCompleted      |
// +-----------------------+
//
const CJobGroupExpressionExploration::EEvent
	rgeev[CJobGroupExpressionExploration::estSentinel]
		 [CJobGroupExpressionExploration::estSentinel] = {
			 {// estInitialized
			  CJobGroupExpressionExploration::eevExploringChildren,
			  CJobGroupExpressionExploration::eevChildrenExplored,
			  CJobGroupExpressionExploration::eevSentinel,
			  CJobGroupExpressionExploration::eevSentinel},
			 {// estChildrenExplored
			  CJobGroupExpressionExploration::eevSentinel,
			  CJobGroupExpressionExploration::eevExploringSelf,
			  CJobGroupExpressionExploration::eevSelfExplored,
			  CJobGroupExpressionExploration::eevSentinel},
			 {// estSelfExplored
			  CJobGroupExpressionExploration::eevSentinel,
			  CJobGroupExpressionExploration::eevSentinel,
			  CJobGroupExpressionExploration::eevSentinel,
			  CJobGroupExpressionExploration::eevFinalized},
			 {// estCompleted
			  CJobGroupExpressionExploration::eevSentinel,
			  CJobGroupExpressionExploration::eevSentinel,
			  CJobGroupExpressionExploration::eevSentinel,
			  CJobGroupExpressionExploration::eevSentinel},
};

#ifdef GPOS_DEBUG

// names for states
const WCHAR rgwszStates[CJobGroupExpressionExploration::estSentinel]
					   [GPOPT_FSM_NAME_LENGTH] = {
						   GPOS_WSZ_LIT("initialized"),
						   GPOS_WSZ_LIT("children explored"),
						   GPOS_WSZ_LIT("self explored"),
						   GPOS_WSZ_LIT("completed")};

// names for events
const WCHAR rgwszEvents[CJobGroupExpressionExploration::eevSentinel]
					   [GPOPT_FSM_NAME_LENGTH] = {
						   GPOS_WSZ_LIT("exploring children groups"),
						   GPOS_WSZ_LIT("explored children groups"),
						   GPOS_WSZ_LIT("applying exploration xforms"),
						   GPOS_WSZ_LIT("applied exploration xforms"),
						   GPOS_WSZ_LIT("finalized")};

#endif	// GPOS_DEBUG


//---------------------------------------------------------------------------
//	@function:
//		CJobGroupExpressionExploration::CJobGroupExpressionExploration
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CJobGroupExpressionExploration::CJobGroupExpressionExploration() = default;


//---------------------------------------------------------------------------
//	@function:
//		CJobGroupExpressionExploration::~CJobGroupExpressionExploration
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CJobGroupExpressionExploration::~CJobGroupExpressionExploration() = default;


//---------------------------------------------------------------------------
//	@function:
//		CJobGroupExpressionExploration::Init
//
//	@doc:
//		Initialize job
//
//---------------------------------------------------------------------------
void
CJobGroupExpressionExploration::Init(CGroupExpression *pgexpr)
{
	CJobGroupExpression::Init(pgexpr);
	GPOS_ASSERT(pgexpr->Pop()->FLogical());


	m_jsm.Init(rgeev
#ifdef GPOS_DEBUG
			   ,
			   rgwszStates, rgwszEvents
#endif	// GPOS_DEBUG
	);

	// set job actions
	m_jsm.SetAction(estInitialized, EevtExploreChildren);
	m_jsm.SetAction(estChildrenExplored, EevtExploreSelf);
	m_jsm.SetAction(estSelfExplored, EevtFinalize);

	CJob::SetInit();
}


//---------------------------------------------------------------------------
//	@function:
//		CJobGroupExpressionExploration::ScheduleApplicableTransformations
//
//	@doc:
//		Schedule transformation jobs for all applicable xforms
//
//---------------------------------------------------------------------------
void
CJobGroupExpressionExploration::ScheduleApplicableTransformations(
	CSchedulerContext *psc)
{
	GPOS_ASSERT(!FXformsScheduled());

	// get all applicable xforms
	COperator *pop = m_pgexpr->Pop();
	CXformSet *xform_set =
		CLogical::PopConvert(pop)->PxfsCandidates(psc->GetGlobalMemoryPool());

	// intersect them with required xforms and schedule jobs
	xform_set->Intersection(CXformFactory::Pxff()->PxfsExploration());
	xform_set->Intersection(psc->Peng()->PxfsCurrentStage());
	if (COperator::EopLogicalNAryJoin != pop->Eopid())
	{
		CLogicalJoin *join = dynamic_cast<CLogicalJoin *>(pop);
		if (nullptr != join && join->FDPHyperRegionMember() &&
			!join->FDPHyperRegionRoot() &&
			CGroupExpression::EdphSucceeded == m_pgexpr->DPHyperStatus() &&
			!GPOS_FTRACE(EopttraceDPHyperShadow))
		{
			ClearDPHyperJoinEnumeration(xform_set, pop->Eopid());
		}
	}
	if (xform_set->Get(CXform::ExfExpandNAryJoinDPHyper))
	{
		// DPHyper is a whole-region job, not a binding-at-a-time xform. Remove
		// its marker from normal scheduling while preserving search-stage and
		// xform-disable control through the candidate-set intersections above.
		(void) xform_set->ExchangeClear(
			CXform::ExfExpandNAryJoinDPHyper);
		if (COperator::EopLogicalNAryJoin == pop->Eopid() &&
			CXform::ExfExpandNAryJoinDPHyper == m_pgexpr->ExfidOrigin())
		{
			// A failed binary DPHyper attempt materializes this NAryJoin solely
			// as an exact bridge to the greedy enumerator. Do not recurse into
			// DPHyper or re-enter any exhaustive native DP enumerator.
			ConfigureNAryFallback(xform_set, true /*use_greedy*/);
			ScheduleTransformations(psc, xform_set);
			xform_set->Release();
			SetXformsScheduled();
			return;
		}
		if (CGroupExpression::EdphUnrequested ==
			m_pgexpr->DPHyperStatus())
		{
			m_pgexpr->SetDPHyperStatus(CGroupExpression::EdphScheduled);
			CJobJoinEnumeration::ScheduleJob(psc, m_pgexpr, this);
			xform_set->Release();
			return;
		}
		GPOS_ASSERT(CGroupExpression::EdphScheduled !=
					m_pgexpr->DPHyperStatus());
		if (CGroupExpression::EdphFallback == m_pgexpr->DPHyperStatus() &&
			COperator::EopLogicalNAryJoin == pop->Eopid() &&
			!GPOS_FTRACE(EopttraceDPHyperShadow))
		{
			CLogicalNAryJoin *nary = CLogicalNAryJoin::PopConvert(pop);
			CExpressionHandle exprhdl(psc->GetGlobalMemoryPool());
			exprhdl.Attach(m_pgexpr);
			const BOOL use_greedy = !nary->HasOuterJoinChildren() &&
									!exprhdl.HasOuterRefs();
			// Greedy only models inner joins. Preserve query order for outer or
			// correlated NAryJoin fallback, which is semantic rather than a
			// search-space budget decision.
			ConfigureNAryFallback(xform_set, use_greedy);
		}
		if ((CGroupExpression::EdphSucceeded ==
				 m_pgexpr->DPHyperStatus() ||
			 CGroupExpression::EdphNativeFallback ==
				 m_pgexpr->DPHyperStatus()) &&
			!GPOS_FTRACE(EopttraceDPHyperShadow))
		{
			// Keep one join-enumeration owner: successful DPHyper suppresses the
			// native ingress owner, while a materialized native fallback suppresses
			// the original binary owner and lets its NAryJoin bridge enumerate.
			if (CGroupExpression::EdphNativeFallback ==
				m_pgexpr->DPHyperStatus())
			{
				if (COperator::EopLogicalNAryJoin != pop->Eopid())
				{
					ClearDPHyperJoinEnumeration(xform_set, pop->Eopid());
				}
			}
			else if (COperator::EopLogicalNAryJoin == pop->Eopid())
			{
				(void) xform_set->ExchangeClear(CXform::ExfExpandNAryJoin);
				(void) xform_set->ExchangeClear(CXform::ExfExpandNAryJoinMinCard);
				(void) xform_set->ExchangeClear(CXform::ExfExpandNAryJoinDP);
				(void) xform_set->ExchangeClear(CXform::ExfExpandNAryJoinGreedy);
				(void) xform_set->ExchangeClear(CXform::ExfExpandNAryJoinDPv2);
			}
			else
			{
				ClearDPHyperJoinEnumeration(xform_set, pop->Eopid());
			}
		}
	}
	ScheduleTransformations(psc, xform_set);
	xform_set->Release();

	SetXformsScheduled();
}


//---------------------------------------------------------------------------
//	@function:
//		CJobGroupExpressionExploration::ScheduleChildGroupsJobs
//
//	@doc:
//		Schedule exploration jobs for all child groups
//
//---------------------------------------------------------------------------
void
CJobGroupExpressionExploration::ScheduleChildGroupsJobs(CSchedulerContext *psc)
{
	GPOS_ASSERT(!FChildrenScheduled());

	ULONG arity = m_pgexpr->Arity();

	for (ULONG i = 0; i < arity; i++)
	{
		CJobGroupExploration::ScheduleJob(psc, (*(m_pgexpr))[i], this);
	}

	SetChildrenScheduled();
}


//---------------------------------------------------------------------------
//	@function:
//		CJobGroupExpressionExploration::EevtExploreChildren
//
//	@doc:
//		Explore child groups
//
//---------------------------------------------------------------------------
CJobGroupExpressionExploration::EEvent
CJobGroupExpressionExploration::EevtExploreChildren(CSchedulerContext *psc,
													CJob *pjOwner)
{
	// get a job pointer
	CJobGroupExpressionExploration *pjgee = PjConvert(pjOwner);
	if (!pjgee->FChildrenScheduled())
	{
		if (FScheduleDPHyperBeforeChildren(psc, pjgee->m_pgexpr, pjgee))
		{
			return eevExploringChildren;
		}
		pjgee->m_pgexpr->SetState(CGroupExpression::estExploring);
		pjgee->ScheduleChildGroupsJobs(psc);

		return eevExploringChildren;
	}
	else
	{
		return eevChildrenExplored;
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CJobGroupExpressionExploration::EevtExploreSelf
//
//	@doc:
//		Explore group expression
//
//---------------------------------------------------------------------------
CJobGroupExpressionExploration::EEvent
CJobGroupExpressionExploration::EevtExploreSelf(CSchedulerContext *psc,
												CJob *pjOwner)
{
	// get a job pointer
	CJobGroupExpressionExploration *pjgee = PjConvert(pjOwner);
	if (!pjgee->FXformsScheduled())
	{
		pjgee->ScheduleApplicableTransformations(psc);
		return eevExploringSelf;
	}
	else
	{
		return eevSelfExplored;
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CJobGroupExpressionExploration::EevtFinalize
//
//	@doc:
//		Finalize exploration
//
//---------------------------------------------------------------------------
CJobGroupExpressionExploration::EEvent
CJobGroupExpressionExploration::EevtFinalize(CSchedulerContext *,  //psc
											 CJob *pjOwner)
{
	// get a job pointer
	CJobGroupExpressionExploration *pjgee = PjConvert(pjOwner);
	pjgee->m_pgexpr->SetState(CGroupExpression::estExplored);

	return eevFinalized;
}


//---------------------------------------------------------------------------
//	@function:
//		CJobGroupExpressionExploration::FExecute
//
//	@doc:
//		Main job function
//
//---------------------------------------------------------------------------
BOOL
CJobGroupExpressionExploration::FExecute(CSchedulerContext *psc)
{
	GPOS_ASSERT(FInit());

	return m_jsm.FRun(psc, this);
}


//---------------------------------------------------------------------------
//	@function:
//		CJobGroupExpressionExploration::ScheduleJob
//
//	@doc:
//		Schedule a new group expression exploration job
//
//---------------------------------------------------------------------------
void
CJobGroupExpressionExploration::ScheduleJob(CSchedulerContext *psc,
											CGroupExpression *pgexpr,
											CJob *pjParent)
{
	CJob *pj = psc->Pjf()->PjCreate(CJob::EjtGroupExpressionExploration);

	// initialize job
	CJobGroupExpressionExploration *pjege = PjConvert(pj);
	pjege->Init(pgexpr);
	psc->Psched()->Add(pjege, pjParent);
}

#ifdef GPOS_DEBUG

//---------------------------------------------------------------------------
//	@function:
//		CJobGroupExpressionExploration::OsPrint
//
//	@doc:
//		Print function
//
//---------------------------------------------------------------------------
IOstream &
CJobGroupExpressionExploration::OsPrint(IOstream &os) const
{
	m_jsm.OsHistory(os);
	return CJob::OsPrint(os);
}

#endif	// GPOS_DEBUG

// EOF
