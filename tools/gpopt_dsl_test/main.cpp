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
#include "unittest/gpopt/dsl/CDSLProjTest.h"
#include "unittest/gpopt/dsl/CDSLTriggerTest.h"
#include "unittest/gpopt/dsl/CDSLUnionTest.h"
#include "unittest/gpopt/xforms/CDPHyperGraphTest.h"

using namespace gpos;
using namespace gpopt;

// static array of all DSL-engine unittest routines
static gpos::CUnittest rgut[] = {
	GPOS_UNITTEST_STD(CDSLParserTest),
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
