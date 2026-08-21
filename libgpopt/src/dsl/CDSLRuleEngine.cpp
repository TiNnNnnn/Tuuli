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

#include "gpos/io/COstreamString.h"
#include "gpos/memory/CMemoryPoolManager.h"
#include "gpos/string/CWStringDynamic.h"

#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLMatcher.h"

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
		ULONG rgulOpid[2] = {(ULONG) prule->EopidSrcRoot(), 0};
		ULONG ulBuckets = 1;
		// HAVING is Select(GbAgg,predicate) in ORCA, while the DSL source root
		// remains Agg. Route Agg-rooted rules to both physical expression shapes.
		if (EdslopAgg == prule->PfragSrc()->PopRoot()->Edslop())
		{
			rgulOpid[ulBuckets++] = (ULONG) COperator::EopLogicalSelect;
		}
		// Before CXformSelect2Apply runs, a DSL Exists source is rooted at an
		// ORCA Select whose scalar predicate is CScalarSubqueryExists. Put it in
		// the Select bucket so the DSL can perform the unnesting independently.
		if (EdslopExists == prule->PfragSrc()->PopRoot()->Edslop())
		{
			rgulOpid[ulBuckets++] = (ULONG) COperator::EopLogicalSelect;
		}

		for (ULONG ulBucket = 0; ulBucket < ulBuckets; ulBucket++)
		{
			const ULONG ulOpid = rgulOpid[ulBucket];
			CDSLRuleArray *pdrgpruleBucket =
				m_phmOpidToRules->Find(&ulOpid);
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
		//TODO: load the rule from postgresql sytem table instead of from static file.
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
//	Three-stage rewrite.
//	Match: real (phase 2, #24 — generic recursion + Input, delegated symbol
//	binding). Check / Instantiate remain phase-2 stubs (#26 / #27).
//---------------------------------------------------------------------------
BOOL
CDSLRuleEngine::FMatch(const CDSLRule *prule, CExpression *pexpr,
					   CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != pmodel);

	// match the source fragment's root template against the live expression.
	// The matcher allocates any transient work in the model's (per-optimization)
	// pool — NOT the engine's long-lived library pool.
	CDSLMatcher matcher(pmodel->Pmp());
	CDSLOp *pop_src_root = prule->PfragSrc()->PopRoot();
	return matcher.FMatch(pop_src_root, pexpr, pmodel);
}

BOOL
CDSLRuleEngine::FCheckConstraints(const CDSLRule *prule,
								  const CDSLModel *pmodel,
								  CExpression *  // pexpr (unused: constraints are
											     // checked against bound model)
) const
{
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(nullptr != pmodel);

	// structural constraints (AttrsSub/Unique/NotNull/Reference) verified against
	// the bound model + live metadata; equality-class constraints hold by
	// construction (FBind enforced them during match). See CDSLConstraintChecker.
	CDSLConstraintChecker checker(pmodel->Pmp());
	return checker.FCheck(prule, pmodel);
}

CExpression *
CDSLRuleEngine::PexprInstantiate(CMemoryPool *mp, const CDSLRule *prule,
								 const CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(nullptr != pmodel);

	// build the target expression from the bound model; reused subtrees/preds are
	// AddRef-grafted, residual conjuncts merged, target symbols resolved through
	// the rule's equality classes. See CDSLInstantiator.
	CDSLInstantiator instantiator(mp);
	return instantiator.PexprInstantiate(prule, pmodel);
}

// EOF
