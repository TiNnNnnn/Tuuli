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
#include "gpopt/base/COptCtxt.h"
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
	  m_phmOpidToPrefixIndex(nullptr),
	  m_phmRuleToId(nullptr),
	  m_pdrgpruleEmpty(nullptr)
{
	GPOS_ASSERT(nullptr != mp);
	m_pdrgprule = GPOS_NEW(mp) CDSLRuleArray(mp);
	m_phmOpidToRules = GPOS_NEW(mp) COperatorIdToRuleArrayMap(mp);
	m_phmOpidToPrefixIndex =
		GPOS_NEW(mp) COperatorIdToRulePrefixIndexMap(mp);
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
	m_phmOpidToPrefixIndex->Release();
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
		ULONG rgulOpid[4] = {(ULONG) prule->EopidSrcRoot(), 0, 0, 0};
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
			// Inner-join predicate pushdown may place the ApplyIn in one join
			// input. The InSub matcher reconstructs and validates the original
			// InSub(InnerJoin(...), ...) view without changing the memo tree.
			if (2 == prule->PfragSrc()->PopRoot()->UlChildren() &&
				EdslopInnerJoin ==
					(*prule->PfragSrc()->PopRoot())[0]->Edslop())
			{
				rgulOpid[ulBuckets++] =
					(ULONG) COperator::EopLogicalInnerJoin;
			}
		}
		// Dropping a redundant Proj* uses Select(child, TRUE) as a memo-safe
		// identity-Proj marker. Route ordinary Proj-rooted rules to Select as well
		// so a later DSL rule can consume the generated alternative. The Proj
		// matcher accepts only TRUE over a pure, unsplit dedup GbAgg.
		if (EdslopProj == prule->PfragSrc()->PopRoot()->Edslop() &&
			!prule->PfragSrc()->PopRoot()->FDistinct())
		{
			rgulOpid[ulBuckets++] = (ULONG) COperator::EopLogicalSelect;
		}
		// Inner-join predicate pushdown changes Filter(InnerJoin(...)) into
		// InnerJoin(..., Select(...), ...) before DSL exploration. Route such
		// rules to the InnerJoin shell as well; CDSLFilterMatcher performs the
		// full, constraint-guided representation check.
		if (EdslopFilter == prule->PfragSrc()->PopRoot()->Edslop())
		{
			const CDSLOp *popBase = prule->PfragSrc()->PopRoot();
			while (EdslopFilter == popBase->Edslop() &&
				   1 == popBase->UlChildren())
			{
				popBase = (*popBase)[0];
			}
			if (EdslopInnerJoin == popBase->Edslop())
			{
				rgulOpid[ulBuckets++] =
					(ULONG) COperator::EopLogicalInnerJoin;
			}
		}
		// A null-rejecting Select over FullJoin is semantically a Select over a
		// LeftJoin that preserves the rejected side. Keep this representation
		// adaptation in the DSL shell rather than changing ORCA's native
		// normalizer; the Select shell constructs and validates the temporary
		// LeftJoin view before applying any data rule.
		if (EdslopLeftJoin == prule->PfragSrc()->PopRoot()->Edslop())
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

				CDSLRulePrefixIndex *pindex =
					GPOS_NEW(m_mp) CDSLRulePrefixIndex(m_mp);
				ULONG *pulIndexKey = GPOS_NEW(m_mp) ULONG(ulOpid);
				fInserted =
					m_phmOpidToPrefixIndex->Insert(pulIndexKey, pindex);
				GPOS_ASSERT(fInserted);
				(void) fInserted;
			}

			prule->AddRef();
			pdrgpruleBucket->Append(prule);
			CDSLRulePrefixIndex *pindex =
				m_phmOpidToPrefixIndex->Find(&ulOpid);
			GPOS_ASSERT(nullptr != pindex);
			pindex->Insert(prule, ul, (COperator::EOperatorId) ulOpid);
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

CDSLRuleArray *
CDSLRuleEngine::PdrgpruleCandidates(CMemoryPool *mp,
									COperator::EOperatorId eopid,
									CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);
	const ULONG ulOpid = (ULONG) eopid;
	CDSLRulePrefixIndex *pindex = m_phmOpidToPrefixIndex->Find(&ulOpid);
	if (nullptr == pindex)
	{
		return GPOS_NEW(mp) CDSLRuleArray(mp);
	}
	return pindex->PdrgpruleCandidates(mp, pexpr);
}

CExpressionArray *
CDSLRuleEngine::PdrgpexprBindings(CMemoryPool *mp,
								  COperator::EOperatorId eopid,
								  CGroupExpression *pgexprRoot) const
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pgexprRoot);
	const ULONG ulOpid = (ULONG) eopid;
	CDSLRulePrefixIndex *pindex = m_phmOpidToPrefixIndex->Find(&ulOpid);
	if (nullptr == pindex)
	{
		return GPOS_NEW(mp) CExpressionArray(mp);
	}
	return pindex->PdrgpexprBindings(mp, pgexprRoot);
}

ULONG
CDSLRuleEngine::UlRuleId(const CDSLRule *prule) const
{
	GPOS_ASSERT(nullptr != prule);
	ULONG *pulId = m_phmRuleToId->Find(const_cast<CDSLRule *>(prule));
	return nullptr == pulId ? 0 : *pulId;
}

BOOL
CDSLRuleEngine::FHasOrdinaryProjSourceRoot() const
{
	for (ULONG ul = 0; ul < m_pdrgprule->Size(); ul++)
	{
		const CDSLOp *popRoot = (*m_pdrgprule)[ul]->PfragSrc()->PopRoot();
		if (EdslopProj == popRoot->Edslop() && !popRoot->FDistinct())
		{
			return true;
		}
	}
	return false;
}

namespace
{
BOOL
FContainsDSLOperator(const CDSLOp *pop, EDslOpKind edslop)
{
	if (pop->Edslop() == edslop)
	{
		return true;
	}
	for (ULONG ul = 0; ul < pop->UlChildren(); ul++)
	{
		if (FContainsDSLOperator((*pop)[ul], edslop))
		{
			return true;
		}
	}
	return false;
}
}  // namespace

BOOL
CDSLRuleEngine::FHasSourceOperator(EDslOpKind edslop) const
{
	for (ULONG ul = 0; ul < m_pdrgprule->Size(); ul++)
	{
		if (FContainsDSLOperator(
				(*m_pdrgprule)[ul]->PfragSrc()->PopRoot(), edslop))
		{
			return true;
		}
	}
	return false;
}

namespace
{
enum EDslTraceStage
{
	EdsltraceMatchRejected = 0,
	EdsltraceConstraintRejected,
	EdsltraceInstantiateRejected,
	EdsltraceApplied,
	EdsltraceDuplicate
};

const CHAR *
SzDSLTraceStage(EDslTraceStage edsltrace)
{
	switch (edsltrace)
	{
		case EdsltraceMatchRejected:
			return "match_rejected";
		case EdsltraceConstraintRejected:
			return "constraint_rejected";
		case EdsltraceInstantiateRejected:
			return "instantiate_rejected";
		case EdsltraceApplied:
			return "applied";
		case EdsltraceDuplicate:
			return "duplicate";
	}
	GPOS_ASSERT(!"invalid DSL trace stage");
	return "invalid";
}

void
TraceDSLRule(CMemoryPool *mp, ULONG ulRuleId, EDslTraceStage edsltrace,
			 const CDSLRule *prule, const CDSLModel *pmodel,
			 const CExpression *pexprSrc, const CExpression *pexprTgt,
			 const CDSLConstraint *pconFailed = nullptr,
			 ULONG ulFailed = gpos::ulong_max)
{
	if (!GPOS_FTRACE(EopttracePrintDSLRule))
	{
		return;
	}

	const CHAR *szStage = SzDSLTraceStage(edsltrace);
	const BOOL fVerbose = GPOS_FTRACE(EopttracePrintXformResults);
	COptCtxt *poctxt = COptCtxt::PoctxtFromTLS();
	if (nullptr != poctxt)
	{
		poctxt->RecordDSLRuleTrace(
			ulRuleId, (ULONG) edsltrace,
			nullptr == pmodel ? 0 : pmodel->Size());
	}
	if (!fVerbose && nullptr != poctxt &&
		!poctxt->FMarkDSLTraceEvent(ulRuleId, (ULONG) edsltrace))
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
	   << ulRuleId << ",\"status\":\"" << szStage << "\"";
	if (fVerbose)
	{
		os << ",\"binding_count\":"
		   << (nullptr == pmodel ? 0 : pmodel->Size());
	}
	// Constraint identity is compact and essential for classifying corpus
	// misses. Emit it even without the very expensive full xform trace.
	if (nullptr != pconFailed)
	{
		os << ",\"failed_constraint\":\""
		   << CDSLConstraintKindTable::SzName(pconFailed->Edslcon())
		   << "\",\"failed_constraint_index\":" << ulFailed;
	}
	os << "}" << std::endl;

	// Rejected candidates can number in the thousands during Cascades
	// exploration. Their compact record above is sufficient to classify the
	// failed stage and map back to the source rule by id. Reprinting the full
	// rule for every rejection can exhaust the trace buffer and truncate a later
	// JSON record. Keep verbose rule text and generated plans for actual rewrites.
	if (nullptr == pexprTgt || !fVerbose)
	{
		return;
	}

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
		TraceDSLRule(mp, ulRuleId, EdsltraceMatchRejected, prule, pmodel, pexpr,
					 nullptr);
		pmodel->Release();
		return nullptr;
	}
	const CDSLConstraint *pconFailed = nullptr;
	ULONG ulFailed = gpos::ulong_max;
	if (!FCheckConstraints(prule, pmodel, pexpr, &pconFailed, &ulFailed))
	{
		TraceDSLRule(mp, ulRuleId, EdsltraceConstraintRejected, prule, pmodel, pexpr,
					 nullptr, pconFailed, ulFailed);
		pmodel->Release();
		return nullptr;
	}

	CExpression *pexprTgt = PexprInstantiate(mp, prule, pmodel);
	if (nullptr != pexprTgt && pexprTgt->Matches(pexpr))
	{
		// A representational adapter can rebuild a rule's target into the exact
		// source tree (for example when an output-column Project must remain over
		// a collapsed dedup layer). Re-inserting it cannot add an alternative and
		// may repeatedly fire as native xforms enumerate equivalent children.
		TraceDSLRule(mp, ulRuleId, EdsltraceDuplicate, prule, pmodel, pexpr,
					 pexprTgt);
		pexprTgt->Release();
		pmodel->Release();
		return nullptr;
	}
	TraceDSLRule(mp, ulRuleId,
				 nullptr == pexprTgt ? EdsltraceInstantiateRejected
									 : EdsltraceApplied,
				 prule,
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
	CDSLMatcher matcher(pmodel->Pmp(), prule);
	CDSLOp *pop_src_root = prule->PfragSrc()->PopRoot();
	return matcher.FMatch(pop_src_root, pexpr, pmodel);
}

BOOL
CDSLRuleEngine::FCheckConstraints(const CDSLRule *prule,
								  const CDSLModel *pmodel,
								  CExpression *,  // pexpr unused: constraints are
												  // checked against bound model
								  const CDSLConstraint **ppconFailed,
								  ULONG *pulFailed
) const
{
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(nullptr != pmodel);

	// structural constraints (AttrsSub/Unique/NotNull/Reference) verified against
	// the bound model + live metadata; equality-class constraints hold by
	// construction (FBind enforced them during match). See CDSLConstraintChecker.
	CDSLConstraintChecker checker(pmodel->Pmp());
	return checker.FCheck(prule, pmodel, ppconFailed, pulFailed);
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
