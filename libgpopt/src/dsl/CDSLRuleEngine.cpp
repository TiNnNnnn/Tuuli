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

#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "naucrates/traceflags/traceflags.h"

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
	  m_phmRuleToId(nullptr),
	  m_pdrgpruleEmpty(nullptr)
{
	GPOS_ASSERT(nullptr != mp);
	m_pdrgprule = GPOS_NEW(mp) CDSLRuleArray(mp);
	m_phmOpidToRules = GPOS_NEW(mp) COperatorIdToRuleArrayMap(mp);
	m_phmRuleToId = GPOS_NEW(mp) CDSLRuleToIdMap(mp);
	m_pdrgpruleEmpty = GPOS_NEW(mp) CDSLRuleArray(mp);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLRuleEngine::~CDSLRuleEngine
//---------------------------------------------------------------------------
CDSLRuleEngine::~CDSLRuleEngine()
{
	m_phmOpidToRules->Release();
	m_phmRuleToId->Release();
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
		ULONG rgulOpid[3] = {(ULONG) prule->EopidSrcRoot(), 0, 0};
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
		if (EdslopInSubFilter == prule->PfragSrc()->PopRoot()->Edslop())
		{
			rgulOpid[ulBuckets++] = (ULONG) COperator::EopLogicalSelect;
			// WeTune may canonicalize a correlated EXISTS equality to InSubFilter,
			// while native ORCA unnests it as LeftSemiApply. Route the data rule to
			// that shell as well; the matcher still performs the full shape check.
			rgulOpid[ulBuckets++] =
				(ULONG) COperator::EopLogicalLeftSemiApply;
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
				const ULONG ulRuleId =
					0 == prule->UlSourceLine() ? ul + 1 : prule->UlSourceLine();
				BOOL fInserted = pengine->m_phmRuleToId->Insert(
					prule, GPOS_NEW(mp) ULONG(ulRuleId));
				GPOS_ASSERT(fInserted);
				(void) fInserted;
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

ULONG
CDSLRuleEngine::UlRuleId(const CDSLRule *prule) const
{
	GPOS_ASSERT(nullptr != prule);
	ULONG *pulId = m_phmRuleToId->Find(const_cast<CDSLRule *>(prule));
	return nullptr == pulId ? 0 : *pulId;
}

namespace
{
void
TraceDSLRule(CMemoryPool *mp, ULONG ulRuleId, const CHAR *szStage,
			 const CDSLRule *prule, const CDSLModel *pmodel,
			 const CExpression *pexprSrc, const CExpression *pexprTgt)
{
	if (!GPOS_FTRACE(EopttracePrintDSLRule))
	{
		return;
	}

	CAutoTrace at(mp);
	IOstream &os = at.Os();

	// Machine-readable line for differential testing against WeTune. Keep the
	// established DSL_RULE block below for humans and existing e2e assertions.
	// Keep this record below the trace subsystem's fixed-size line buffer. Full
	// rules and plans can be arbitrarily large and are printed in the human
	// DSL_RULE block below; embedding them here would make the JSON invalid when
	// the trace buffer inserts a physical newline. The compact record also makes
	// accidental publication of machine traces less likely to expose rule text.
	os << "DSL_TRACE {\"kind\":\"application\",\"engine\":\"pgorca\","
		  "\"rule_id\":"
	   << ulRuleId << ",\"status\":\"" << szStage
	   << "\",\"binding_count\":" << (nullptr == pmodel ? 0 : pmodel->Size())
	   << "}" << std::endl;

	os << "DSL_RULE id=" << ulRuleId << " stage=" << szStage;
	if (nullptr != pmodel)
	{
		os << " bindings=" << pmodel->Size();
	}
	os << std::endl << "Rule: ";
	prule->OsPrint(os);
	os << std::endl;
	if (nullptr != pexprTgt)
	{
		os << "Generated:" << std::endl;
		pexprTgt->OsPrint(os);
	}
}
}  // namespace

CExpression *
CDSLRuleEngine::PexprApply(CMemoryPool *mp, const CDSLRule *prule,
						   CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(nullptr != pexpr);

	const ULONG ulRuleId =
		GPOS_FTRACE(EopttracePrintDSLRule) ? UlRuleId(prule) : 0;
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	if (!FMatch(prule, pexpr, pmodel))
	{
		TraceDSLRule(mp, ulRuleId, "match_rejected", prule, pmodel, pexpr,
					 nullptr);
		pmodel->Release();
		return nullptr;
	}
	if (!FCheckConstraints(prule, pmodel, pexpr))
	{
		TraceDSLRule(mp, ulRuleId, "constraint_rejected", prule, pmodel, pexpr,
					 nullptr);
		pmodel->Release();
		return nullptr;
	}

	CExpression *pexprTgt = PexprInstantiate(mp, prule, pmodel);
	TraceDSLRule(mp, ulRuleId,
				 nullptr == pexprTgt ? "instantiate_rejected" : "applied", prule,
				 pmodel, pexpr, pexprTgt);
	pmodel->Release();
	return pexprTgt;
}

//---------------------------------------------------------------------------
//	Three-stage rewrite.
//	Generic recursion + delegated operator matchers, constraint checking, and
//	target instantiation shared by every DSL xform shell.
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
