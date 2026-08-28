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
ClearNativeJoinEnumerationXform(CXformSet *xform_set,
								CXform::EXformId exfid)
{
	GPOS_ASSERT(CJobJoinEnumeration::FReplacesNativeXform(exfid));
	(void) xform_set->ExchangeClear(exfid);
}

void
ClearDPHyperJoinEnumeration(CXformSet *xform_set,
							COperator::EOperatorId op_id)
{
	// The operator switch documents which region root owns each candidate. The
	// central membership predicate is also consumed by replacement inventory, so
	// search behavior and audit classification cannot drift independently.
	switch (op_id)
	{
		case COperator::EopLogicalInnerJoin:
			ClearNativeJoinEnumerationXform(
				xform_set,
				CXform::ExfInnerJoinCommutativity);
			ClearNativeJoinEnumerationXform(
				xform_set, CXform::ExfJoinAssociativity);
			ClearNativeJoinEnumerationXform(
				xform_set, CXform::ExfInnerJoinSemiJoinSwap);
			ClearNativeJoinEnumerationXform(
				xform_set,
				CXform::ExfInnerJoinAntiSemiJoinSwap);
			ClearNativeJoinEnumerationXform(
				xform_set,
				CXform::ExfInnerJoinAntiSemiJoinNotInSwap);
			break;
		case COperator::EopLogicalLeftOuterJoin:
			ClearNativeJoinEnumerationXform(
				xform_set, CXform::ExfLeftJoin2RightJoin);
			break;
		case COperator::EopLogicalLeftSemiJoin:
			ClearNativeJoinEnumerationXform(
				xform_set, CXform::ExfSemiJoinSemiJoinSwap);
			ClearNativeJoinEnumerationXform(
				xform_set,
				CXform::ExfSemiJoinAntiSemiJoinSwap);
			ClearNativeJoinEnumerationXform(
				xform_set,
				CXform::ExfSemiJoinAntiSemiJoinNotInSwap);
			ClearNativeJoinEnumerationXform(
				xform_set, CXform::ExfSemiJoinInnerJoinSwap);
			break;
		case COperator::EopLogicalLeftAntiSemiJoin:
			ClearNativeJoinEnumerationXform(
				xform_set,
				CXform::ExfAntiSemiJoinAntiSemiJoinSwap);
			ClearNativeJoinEnumerationXform(
				xform_set,
				CXform::ExfAntiSemiJoinAntiSemiJoinNotInSwap);
			ClearNativeJoinEnumerationXform(
				xform_set,
				CXform::ExfAntiSemiJoinSemiJoinSwap);
			ClearNativeJoinEnumerationXform(
				xform_set,
				CXform::ExfAntiSemiJoinInnerJoinSwap);
			break;
		case COperator::EopLogicalFullOuterJoin:
			ClearNativeJoinEnumerationXform(
				xform_set,
				CXform::ExfFullJoinCommutativity);
			break;
		default:
			GPOS_ASSERT(!"Unsupported DPHyper join-region root");
	}
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
	if (!candidates->Get(CXform::ExfDPHyperJoinRegion))
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
	CLogicalJoin *join = dynamic_cast<CLogicalJoin *>(pop);
	if (nullptr != join && join->FDPHyperRegionMember() &&
		!join->FDPHyperRegionRoot() &&
		CGroupExpression::EdphSucceeded == m_pgexpr->DPHyperStatus() &&
		!GPOS_FTRACE(EopttraceDPHyperShadow))
	{
		ClearDPHyperJoinEnumeration(xform_set, pop->Eopid());
	}
	if (xform_set->Get(CXform::ExfDPHyperJoinRegion))
	{
		GPOS_ASSERT(nullptr != dynamic_cast<CLogicalJoin *>(pop));
		// DPHyper is a whole-region job, not a binding-at-a-time xform. Remove
		// its marker from normal scheduling while preserving search-stage and
		// xform-disable control through the candidate-set intersections above.
		(void) xform_set->ExchangeClear(
			CXform::ExfDPHyperJoinRegion);
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
		if ((CGroupExpression::EdphSucceeded ==
				 m_pgexpr->DPHyperStatus() ||
			 CGroupExpression::EdphNativeFallback ==
				 m_pgexpr->DPHyperStatus()) &&
			!GPOS_FTRACE(EopttraceDPHyperShadow))
		{
			// Successful DPHyper and its direct binary Greedy fallback each own
			// join enumeration for the preserved input region.
			ClearDPHyperJoinEnumeration(xform_set, pop->Eopid());
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
