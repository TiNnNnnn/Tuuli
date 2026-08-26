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

#include "gpopt/search/CJob.h"

namespace gpopt
{
using namespace gpos;

class CGroupExpression;

class CJobJoinEnumeration : public CJob
{
private:
	CGroupExpression *m_pgexpr;

	BOOL FEnumerate(CSchedulerContext *psc);

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
