//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CAutoTrace.cpp
//
//	@doc:
//		Implementation of auto object for creating trace messages
//---------------------------------------------------------------------------

#include "gpos/error/CAutoTrace.h"

#include "gpos/task/CAutoSuspendAbort.h"
#include "gpos/task/ITask.h"

using namespace gpos;

//---------------------------------------------------------------------------
//	@function:
//		CAutoTrace::CAutoTrace
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CAutoTrace::CAutoTrace(CMemoryPool *mp) : m_wstr(mp), m_os(&m_wstr)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CAutoTrace::~CAutoTrace
//
//	@doc:
//		Destructor prints trace message
//
//---------------------------------------------------------------------------
CAutoTrace::~CAutoTrace()
{
	if (0 < m_wstr.Length() && !ITask::Self()->GetErrCtxt()->IsPending())
	{
		// Like CAutoTimer: defer cancellation until outside this noexcept
		// destructor. The next normal abort check still cancels the task.
		CAutoSuspendAbort suspend_abort;
		GPOS_TRACE(m_wstr.GetBuffer());
	}
}

// EOF
