//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLRuleEngine.cpp
//
//	@doc:
//		Implementation of the shared rule engine (see CDSLRuleEngine.h).
//		Phase 1: real lifecycle + loading + bucketing; stubbed three-stage.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLRuleEngine.h"

#include "gpos/error/CAutoTrace.h"
#include "gpos/io/COstreamString.h"
#include "gpos/memory/CMemoryPoolManager.h"
#include "gpos/string/CWStringDynamic.h"

using namespace gpopt;

// global instance
CDSLRuleEngine *CDSLRuleEngine::m_instance = nullptr;

//---------------------------------------------------------------------------
//	@function:
//		CDSLRuleEngine::CDSLRuleEngine
//---------------------------------------------------------------------------
CDSLRuleEngine::CDSLRuleEngine(CMemoryPool *mp)
	: m_mp(mp),
	  m_pdrgprule(nullptr),
	  m_phmOpidToRules(nullptr),
	  m_pdrgpruleEmpty(nullptr)
{
	GPOS_ASSERT(nullptr != mp);
	m_pdrgprule = GPOS_NEW(mp) CDSLRuleArray(mp);
	m_phmOpidToRules = GPOS_NEW(mp) COperatorIdToRuleArrayMap(mp);
	m_pdrgpruleEmpty = GPOS_NEW(mp) CDSLRuleArray(mp);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLRuleEngine::~CDSLRuleEngine
//---------------------------------------------------------------------------
CDSLRuleEngine::~CDSLRuleEngine()
{
	m_phmOpidToRules->Release();
	m_pdrgprule->Release();
	m_pdrgpruleEmpty->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLRuleEngine::BucketByRoot
//
//	@doc:
//		Group the loaded library by each rule's source-root EOperatorId. Each
//		bucket AddRef's the rules it lists (the master m_pdrgprule keeps the
//		other ref).
//---------------------------------------------------------------------------
void
CDSLRuleEngine::BucketByRoot()
{
	const ULONG ulRules = m_pdrgprule->Size();
	for (ULONG ul = 0; ul < ulRules; ul++)
	{
		CDSLRule *prule = (*m_pdrgprule)[ul];
		const ULONG ulOpid = (ULONG) prule->EopidSrcRoot();

		CDSLRuleArray *pdrgpruleBucket = m_phmOpidToRules->Find(&ulOpid);
		if (nullptr == pdrgpruleBucket)
		{
			pdrgpruleBucket = GPOS_NEW(m_mp) CDSLRuleArray(m_mp);
			ULONG *pulKey = GPOS_NEW(m_mp) ULONG(ulOpid);
			BOOL fInserted =
				m_phmOpidToRules->Insert(pulKey, pdrgpruleBucket);
			GPOS_ASSERT(fInserted);
			(void) fInserted;
		}

		prule->AddRef();
		pdrgpruleBucket->Append(prule);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLRuleEngine::Init
//
//	@doc:
//		Create the global instance and load the rule library. Missing/empty file
//		=> an empty engine (shells no-op), which is the safe default for phase 1.
//---------------------------------------------------------------------------
void
CDSLRuleEngine::Init(const CHAR *szPath)
{
	if (nullptr != m_instance)
	{
		// idempotent
		return;
	}

	CMemoryPool *mp = CMemoryPoolManager::CreateMemoryPool();
	CDSLRuleEngine *pengine = GPOS_NEW(mp) CDSLRuleEngine(mp);

	if (nullptr != szPath)
	{
		CDSLRuleLoader::SLoadStats stats;
		CWStringDynamic strErrs(mp);
		CDSLRuleArray *pdrgprule = CDSLRuleLoader::PdrgpdslruleLoadFile(
			mp, szPath, true /*fEqOnly*/, &stats, &strErrs);

		if (nullptr != pdrgprule)
		{
			// move the loaded rules into the engine-owned array
			const ULONG ulLoaded = pdrgprule->Size();
			for (ULONG ul = 0; ul < ulLoaded; ul++)
			{
				CDSLRule *prule = (*pdrgprule)[ul];
				prule->AddRef();
				pengine->m_pdrgprule->Append(prule);
			}
			pdrgprule->Release();
		}

		{
			CAutoTrace at(mp);
			at.Os() << "[CDSLRuleEngine] loaded rules from '" << szPath
					<< "': admitted=" << stats.ul_admitted
					<< " skipped=" << stats.ul_skipped
					<< " failed=" << stats.ul_failed;
			if (0 < strErrs.Length())
			{
				at.Os() << "; first errors: " << strErrs.GetBuffer();
			}
		}
	}

	pengine->BucketByRoot();
	m_instance = pengine;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLRuleEngine::Shutdown
//---------------------------------------------------------------------------
void
CDSLRuleEngine::Shutdown()
{
	CDSLRuleEngine *pengine = m_instance;
	if (nullptr == pengine)
	{
		return;
	}

	CMemoryPool *mp = pengine->m_mp;
	m_instance = nullptr;
	GPOS_DELETE(pengine);
	CMemoryPoolManager::Destroy(mp);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLRuleEngine::PdrgpruleForRoot
//---------------------------------------------------------------------------
const CDSLRuleArray *
CDSLRuleEngine::PdrgpruleForRoot(COperator::EOperatorId eopid) const
{
	const ULONG ulOpid = (ULONG) eopid;
	CDSLRuleArray *pdrgprule = m_phmOpidToRules->Find(&ulOpid);
	if (nullptr == pdrgprule)
	{
		return m_pdrgpruleEmpty;
	}
	return pdrgprule;
}

//---------------------------------------------------------------------------
//	Three-stage rewrite — PHASE 1 STUBS.
//	Implemented in phase 2 (see docs/WETUNE_ORCA_PER_OP_THREESTAGE.md).
//---------------------------------------------------------------------------
BOOL
CDSLRuleEngine::FMatch(const CDSLRule *,	 // prule
					   CExpression *,		 // pexpr
					   CDSLModel *			 // pmodel
) const
{
	// phase 2: recursively compare CDSLOp.Eopid() with pexpr->Pop()->Eopid(),
	// bind symbols into *pmodel, split CScalarBoolOp(And) into conjuncts for
	// filter matching.
	return false;
}

BOOL
CDSLRuleEngine::FCheckConstraints(const CDSLRule *,	 // prule
								  const CDSLModel *,  // pmodel
								  CExpression *		  // pexpr
) const
{
	// phase 2: Unique/NotNull/Reference/AttrsSub via CExpressionHandle +
	// CMDAccessor; equality-class compatibility over the bound model.
	return false;
}

CExpression *
CDSLRuleEngine::PexprInstantiate(CMemoryPool *,		  // mp
								 const CDSLRule *,	  // prule
								 const CDSLModel *	  // pmodel
) const
{
	// phase 2: build target CExpression, AddRef-graft reused subtrees, merge
	// residual conjuncts via CPredicateUtils, remap columns with
	// PexprCopyWithRemappedColumns.
	return nullptr;
}

// EOF
