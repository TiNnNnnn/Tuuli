//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		main.cpp
//
//	@doc:
//		Unit-test runner for the DSL rule engine (libgpopt server tests).
//		Mirrors libgpos/server/src/startup/main.cpp: runs the registered
//		CUnittest suites inside the GPOS task framework.
//
//		Build target: gpopt_dsl_test (see top-level CMakeLists.txt).
//		Run: ./gpopt_dsl_test
//---------------------------------------------------------------------------
#include "gpos/_api.h"
#include "gpos/common/CMainArgs.h"
#include "gpos/test/CUnittest.h"
#include "gpos/types.h"
#include "gpos/error/ILogger.h"
#include "gpos/error/CAutoTrace.h"
#include "gpos/memory/CAutoMemoryPool.h"

#include <cstdio>
#include <cwchar>
#include <string>

#include "gpopt/init.h"
#include "gpopt/mdcache/CMDCache.h"
#include "naucrates/init.h"

#include "unittest/gpopt/dsl/CDSLAggTest.h"
#include "unittest/gpopt/dsl/CDSLConstraintTest.h"
#include "unittest/gpopt/dsl/CDSLEngineTest.h"
#include "unittest/gpopt/dsl/CDSLExistsTest.h"
#include "unittest/gpopt/dsl/CDSLFilterSplitTest.h"
#include "unittest/gpopt/dsl/CDSLFixtureTest.h"
#include "unittest/gpopt/dsl/CDSLInstantiateTest.h"
#include "unittest/gpopt/dsl/CDSLInSubTest.h"
#include "unittest/gpopt/dsl/CDSLJoinElimTest.h"
#include "unittest/gpopt/dsl/CDSLJoinTest.h"
#include "unittest/gpopt/dsl/CDSLMatchTest.h"
#include "unittest/gpopt/dsl/CDSLMatchViewTest.h"
#include "unittest/gpopt/dsl/CDSLOrderLimitTest.h"
#include "unittest/gpopt/dsl/CDSLParserTest.h"
#include "unittest/gpopt/dsl/CDSLPolicyTest.h"
#include "unittest/gpopt/dsl/CDSLStatsExperimentTest.h"
#include "unittest/gpopt/dsl/CDSLProjTest.h"
#include "unittest/gpopt/dsl/CDSLQuantifiedTest.h"
#include "unittest/gpopt/dsl/CDSLTriggerTest.h"
#include "unittest/gpopt/dsl/CDSLUnionTest.h"
#include "unittest/gpopt/xforms/CDPHyperGraphTest.h"

using namespace gpos;
using namespace gpopt;

// Exercise the real task boundary (gpos_exec cannot be nested inside a suite).
// More than 10 MiB of wide entries must reach both output/error sinks in order,
// even with a tiny legacy buffer. Ordinary buffered logging remains supported.
struct STraceSinkTest
{
	ULONG received = 0;
	bool valid = true;
	bool abort_in_sink = false;
	bool fail_in_task = false;
	BOOL cancel_requested = false;
	bool deferred_trace_seen = false;
	bool after_trace = false;
};

static void
TestTraceSink(void *context, const WCHAR *entry)
{
	auto *state = static_cast<STraceSinkTest *>(context);
	if (nullptr != std::wcsstr(entry, L"deferred-trace-cancel"))
	{
		state->deferred_trace_seen = true;
		return;
	}
	if (state->abort_in_sink)
	{
		state->abort_in_sink = false;
		GPOS_ABORT;
	}
	const std::wstring expected = L"trace-stream-" +
								  std::to_wstring(state->received++) + L":" +
								  std::wstring(4096, L'x');
	state->valid &= nullptr != std::wcsstr(entry, expected.c_str());
}

static void *
TestTraceTask(void *arg)
{
	for (ULONG index = 0; index < 800; ++index)
	{
		const std::wstring entry = L"trace-stream-" + std::to_wstring(index) +
								   L":" + std::wstring(4096, L'x');
		if (index % 2 == 0)
		{
			GPOS_TRACE(entry.c_str());
		}
		else
		{
			GPOS_TRACE_ERR(entry.c_str());
		}
	}
	if (static_cast<STraceSinkTest *>(arg)->fail_in_task)
	{
		GPOS_RAISE(CException::ExmaSystem, CException::ExmiOOM);
	}
	return arg;
}

static bool
TestTraceTransport()
{
	WCHAR buffer[64] = {L'!'};
	STraceSinkTest state;
	gpos_exec_params params{};
	params.func = TestTraceTask;
	params.arg = &state;
	params.error_buffer = reinterpret_cast<char *>(buffer);
	params.error_buffer_size = sizeof(buffer);
	params.log_callback = TestTraceSink;
	params.log_context = &state;
	if (gpos_exec(&params) || params.result != &state || !state.valid ||
		state.received != 800 || buffer[0] != L'!')
	{
		std::fprintf(stderr, "Streaming: received=%lu valid=%d buffer=%d\n",
					 static_cast<unsigned long>(state.received), state.valid,
					 int(buffer[0]));
		return false;
	}
	// A sink cancellation must unwind the worker/logger and allow a new task.
	state.abort_in_sink = true;
	bool aborted = false;
	try
	{
		gpos_exec(&params);
	}
	catch (CException &ex)
	{
		aborted =
			GPOS_MATCH_EX(ex, CException::ExmaSystem, CException::ExmiAbort);
		if (!aborted)
		{
			std::fprintf(stderr, "Unexpected cancellation exception: %lu/%lu\n",
						 static_cast<unsigned long>(ex.Major()),
						 static_cast<unsigned long>(ex.Minor()));
		}
	}
	// An ordinary task error must also propagate after prior entries were sent.
	state = STraceSinkTest{};
	state.fail_in_task = true;
	bool failed = false;
	try
	{
		gpos_exec(&params);
	}
	catch (CException &ex)
	{
		failed = GPOS_MATCH_EX(ex, CException::ExmaSystem, CException::ExmiOOM);
	}
	state.fail_in_task = false;
	params.log_callback = nullptr;
	const int buffered_status = gpos_exec(&params);
	if (!aborted || !failed || buffered_status != 0 || buffer[0] == L'!' ||
		buffer[GPOS_ARRAY_SIZE(buffer) - 1] != L'\0')
	{
		std::fprintf(
			stderr,
			"Recovery: aborted=%d failed=%d status=%d first=%d last=%d\n",
			aborted, failed, buffered_status, int(buffer[0]), int(buffer[63]));
		return false;
	}
	return true;
}

static void *
TestDeferredTraceCancel(void *arg)
{
	auto *state = static_cast<STraceSinkTest *>(arg);
	CAutoMemoryPool amp;
	{
		CAutoTrace trace(amp.Pmp());
		trace.Os() << "deferred-trace-cancel";
		state->cancel_requested = true;
	}
	state->after_trace = true;
	GPOS_CHECK_ABORT;
	return nullptr;
}

static bool
TestTraceDestructorCancel()
{
	STraceSinkTest state;
	gpos_exec_params params{};
	params.func = TestDeferredTraceCancel;
	params.arg = &state;
	params.abort_requested = &state.cancel_requested;
	params.log_callback = TestTraceSink;
	params.log_context = &state;
	try
	{
		gpos_exec(&params);
	}
	catch (CException &ex)
	{
		return state.after_trace && state.deferred_trace_seen &&
			GPOS_MATCH_EX(ex, CException::ExmaSystem, CException::ExmiAbort);
	}
	return false;
}

// static array of all DSL-engine unittest routines
static gpos::CUnittest rgut[] = {
	GPOS_UNITTEST_STD(CDSLParserTest),
	GPOS_UNITTEST_STD(CDSLPolicyTest),
	GPOS_UNITTEST_STD(CDSLStatsExperimentTest),
	GPOS_UNITTEST_STD(CDSLEngineTest),
	GPOS_UNITTEST_STD(CDSLFixtureTest),
	GPOS_UNITTEST_STD(CDSLMatchTest),
	GPOS_UNITTEST_STD(CDSLMatchViewTest),
	GPOS_UNITTEST_STD(CDSLOrderLimitTest),
	GPOS_UNITTEST_STD(CDSLFilterSplitTest),
	GPOS_UNITTEST_STD(CDSLConstraintTest),
	GPOS_UNITTEST_STD(CDSLInstantiateTest),
	GPOS_UNITTEST_STD(CDSLProjTest),
	GPOS_UNITTEST_STD(CDSLJoinTest),
	GPOS_UNITTEST_STD(CDSLJoinElimTest),
	GPOS_UNITTEST_STD(CDSLAggTest),
	GPOS_UNITTEST_STD(CDSLExistsTest),
	GPOS_UNITTEST_STD(CDSLInSubTest),
	GPOS_UNITTEST_STD(CDSLQuantifiedTest),
	GPOS_UNITTEST_STD(CDSLTriggerTest),
	GPOS_UNITTEST_STD(CDSLUnionTest),
	GPOS_UNITTEST_STD(CDPHyperGraphTest),
};

static void *
PvExec(void *arg)
{
	// initialise DXL + metadata cache so xform/engine tests (phase 1/2) have a
	// live context; harmless for the pure-parser tests.
	InitDXL();
	CMDCache::Init();

	GPOS_RESULT eres = GPOS_OK;
	CHAR *suite = static_cast<CHAR *>(arg);
	if (nullptr == suite)
	{
		eres = CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
	}
	else
	{
		eres = GPOS_FAILED;
		for (ULONG test = 0; test < GPOS_ARRAY_SIZE(rgut); ++test)
		{
			if (rgut[test].Equals(suite))
			{
				eres = CUnittest::EresExecute(&rgut[test], 1);
				break;
			}
		}
	}

	CMDCache::Shutdown();
	return (void *) (eres == GPOS_OK ? nullptr : (void *) 1);
}

int
main(int argc, const char **argv)
{
	struct gpos_init_params gpos_params = {nullptr};
	gpos_init(&gpos_params);
	gpdxl_init();
	gpopt_init();
	if (!TestTraceDestructorCancel() || !TestTraceTransport())
	{
		std::fprintf(stderr, "Task trace transport regression failed\n");
		return 1;
	}

	gpos_exec_params params;
	params.func = PvExec;
	params.arg =
		1 < argc ? const_cast<CHAR *>(static_cast<const CHAR *>(argv[1])) : nullptr;
	params.result = nullptr;
	params.stack_start = &params;
	params.error_buffer = nullptr;
	params.error_buffer_size = -1;
	params.abort_requested = nullptr;

	// gpos_exec returns framework status; the test verdict is in params.result
	// (non-null => a suite failed).
	INT framework_status = gpos_exec(&params);
	INT test_failed = (nullptr != params.result) ? 1 : 0;

	gpopt_terminate();
	gpdxl_terminate();
	gpos_terminate();

	return (0 != framework_status) ? framework_status : test_failed;
}
