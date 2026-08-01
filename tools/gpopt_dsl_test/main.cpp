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

#include "gpopt/init.h"
#include "gpopt/mdcache/CMDCache.h"
#include "naucrates/init.h"

#include "unittest/gpopt/dsl/CDSLEngineTest.h"
#include "unittest/gpopt/dsl/CDSLParserTest.h"

using namespace gpos;
using namespace gpopt;

// static array of all DSL-engine unittest routines
static gpos::CUnittest rgut[] = {
	GPOS_UNITTEST_STD(CDSLParserTest),
	GPOS_UNITTEST_STD(CDSLEngineTest),
};

static void *
PvExec(void *)
{
	// initialise DXL + metadata cache so xform/engine tests (phase 1/2) have a
	// live context; harmless for the pure-parser tests.
	InitDXL();
	CMDCache::Init();

	GPOS_RESULT eres = CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));

	CMDCache::Shutdown();
	return (void *) (eres == GPOS_OK ? nullptr : (void *) 1);
}

int
main(int, const char **)
{
	struct gpos_init_params gpos_params = {nullptr};
	gpos_init(&gpos_params);
	gpdxl_init();
	gpopt_init();

	gpos_exec_params params;
	params.func = PvExec;
	params.arg = nullptr;
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
