//---------------------------------------------------------------------------
//	@filename:
//		CJobJoinEnumeration.h
//
//	@doc:
//		Specialized Cascades exploration job for whole join regions. Unlike a
//		local xform binding, it enumerates CSG-CMP pairs once and inserts every
//		legal binary InnerJoin alternative into shared subset groups in Memo.
//---------------------------------------------------------------------------
#ifndef GPOPT_CJobJoinEnumeration_H
#define GPOPT_CJobJoinEnumeration_H

#include <vector>

#include "gpopt/search/CJob.h"

namespace gpopt
{
using namespace gpos;

class CGroupExpression;
class CGroup;
class CDPHyperJoinRegion;
class CJoinRegionSpec;

class CJobJoinEnumeration : public CJob
{
private:
	CGroupExpression *m_pgexpr;
	BOOL m_materialized;
	BOOL m_native_fallback_materialized;
	std::vector<CGroup *> m_intermediate_groups;

	BOOL FEnumerate(CSchedulerContext *psc);
	BOOL FEnumerateRegion(CSchedulerContext *psc,
						  CDPHyperJoinRegion *region,
						  const std::vector<CGroup *> &component_groups,
						  const CJoinRegionSpec *spec,
						  const std::vector<CGroup *> &skeleton_groups);
	void MaterializeNativeFallback(
		CSchedulerContext *psc, CDPHyperJoinRegion *region,
		const std::vector<CGroup *> &component_groups);

public:
	CJobJoinEnumeration(const CJobJoinEnumeration &) = delete;

	CJobJoinEnumeration();
	~CJobJoinEnumeration() override;

	void Init(CGroupExpression *pgexpr);
	static void ScheduleJob(CSchedulerContext *psc, CGroupExpression *pgexpr,
						CJob *parent);

	BOOL FExecute(CSchedulerContext *psc) override;

#ifdef GPOS_DEBUG
	IOstream &OsPrint(IOstream &os) const override;
#endif

	static CJobJoinEnumeration *
	PjConvert(CJob *job)
	{
		GPOS_ASSERT(nullptr != job);
		GPOS_ASSERT(EjtJoinEnumeration == job->Ejt());
		return dynamic_cast<CJobJoinEnumeration *>(job);
	}
};

}  // namespace gpopt

#endif  // !GPOPT_CJobJoinEnumeration_H
