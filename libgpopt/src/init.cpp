//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		init.cpp
//
//	@doc:
//		Implementation of initialization and termination functions for
//		libgpopt.
//---------------------------------------------------------------------------

#include "gpopt/init.h"

#include "gpos/_api.h"
#include "gpos/task/CWorker.h"

#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/exception.h"
#include "gpopt/mdcache/CMDCache.h"
#include "gpopt/xforms/CXformFactory.h"
#include "naucrates/init.h"

#include <cstdlib>	// getenv

using namespace gpos;
using namespace gpopt;

static CMemoryPool *mp = nullptr;


//---------------------------------------------------------------------------
//      @function:
//              gpopt_init
//
//      @doc:
//              Initialize gpopt library. To enable memory allocations
//              via a custom allocator, pass in non-NULL fnAlloc/fnFree
//              allocation/deallocation functions. If either of the parameters
//              are NULL, gpopt with be initialized with the default allocator.
//
//---------------------------------------------------------------------------
void
gpopt_init()
{
	mp = CMemoryPoolManager::CreateMemoryPool();

	gpopt::EresExceptionInit(mp);

	CXformFactory::Init();

	// Initialize the MONSOON DSL rule engine. The rule-library path comes from
	// the MONSOON_DSL_RULES env var for now (a GUC can replace this later);
	// unset => the engine loads empty and every DSL shell safely no-ops.
	CDSLRuleEngine::Init(std::getenv("MONSOON_DSL_RULES"));
}

//---------------------------------------------------------------------------
//      @function:
//              gpopt_terminate
//
//      @doc:
//              Destroy the memory pool
//
//---------------------------------------------------------------------------
void
gpopt_terminate()
{
#ifdef GPOS_DEBUG
	CDSLRuleEngine::Shutdown();

	CMDCache::Shutdown();

	CMemoryPoolManager::Destroy(mp);

	CXformFactory::Shutdown();
#endif	// GPOS_DEBUG
}

// EOF
