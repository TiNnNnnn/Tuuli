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

#include "gpos/common/CWallClock.h"
#include "gpos/error/CAutoTrace.h"
#include "gpos/io/COstreamString.h"
#include "gpos/memory/CMemoryPoolManager.h"
#include "gpos/string/CWStringDynamic.h"

#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/dsl/CDSLPolicy.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
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
		ULONG rgulOpid[8] = {(ULONG) prule->EopidSrcRoot()};
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
			// A decorrelated IN/EXISTS equality is represented as a logical
			// semi join. The InSub matcher validates the single-column equality
			// before exposing this post-unnest representation to a data rule.
			rgulOpid[ulBuckets++] =
				(ULONG) COperator::EopLogicalLeftSemiJoin;
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
				// WeTune models IN/EXISTS nodes as members of the Filter chain.
				// Route the post-unnest carriers to the same Filter matcher.
				rgulOpid[ulBuckets++] =
					(ULONG) COperator::EopLogicalLeftSemiApplyIn;
				rgulOpid[ulBuckets++] =
					(ULONG) COperator::EopLogicalLeftSemiJoin;
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
									CExpression *pexpr,
									BOOL fFilterCBO) const
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);
	const ULONG ulOpid = (ULONG) eopid;
	CDSLRulePrefixIndex *pindex = m_phmOpidToPrefixIndex->Find(&ulOpid);
	if (nullptr == pindex)
	{
		return GPOS_NEW(mp) CDSLRuleArray(mp);
	}
	const BOOL fTrace = GPOS_FTRACE(EopttracePrintDSLRule);
	CWallClock timer(fTrace);
	CDSLRuleArray *pdrgprule = pindex->PdrgpruleCandidates(mp, pexpr);
	if (fFilterCBO)
	{
		COptCtxt *poctxt = COptCtxt::PoctxtFromTLS();
		const CDSLPolicySnapshot *snapshot = nullptr == poctxt
			? nullptr
			: poctxt->PdslPolicySnapshot();
		if (nullptr != snapshot)
		{
			CDSLRuleArray *filtered = GPOS_NEW(mp) CDSLRuleArray(mp);
			for (ULONG rule = 0; rule < pdrgprule->Size(); ++rule)
			{
				CDSLRule *candidate = (*pdrgprule)[rule];
				const SDSLRulePolicy *policy = snapshot->Ppolicy(candidate);
				if (nullptr != policy && policy->m_fEnabled &&
					EdslplacementCBO == policy->m_edslplacement)
				{
					candidate->AddRef();
					filtered->Append(candidate);
				}
			}
			pdrgprule->Release();
			pdrgprule = filtered;
		}
	}
	if (fTrace)
	{
		COptCtxt::PoctxtFromTLS()->RecordDSLCandidateTiming(
			timer.ElapsedUS(), pdrgprule->Size());
	}
	return pdrgprule;
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
	const BOOL fTrace = GPOS_FTRACE(EopttracePrintDSLRule);
	CWallClock timer(fTrace);
	CExpressionArray *pdrgpexpr =
		pindex->PdrgpexprBindings(mp, pgexprRoot);
	if (fTrace)
	{
		COptCtxt::PoctxtFromTLS()->RecordDSLBindingTiming(
			timer.ElapsedUS(), pdrgpexpr->Size());
	}
	return pdrgpexpr;
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
	EdsltraceDuplicate,
	EdsltraceBudgetExhausted,
	EdsltraceBudgetSkipped
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
		case EdsltraceBudgetExhausted:
			return "budget_exhausted";
		case EdsltraceBudgetSkipped:
			return "budget_skipped";
	}
	GPOS_ASSERT(!"invalid DSL trace stage");
	return "invalid";
}

void
TraceDSLRule(CMemoryPool *mp, ULONG ulRuleId, EDslTraceStage edsltrace,
			 const CDSLRule *prule, const CDSLModel *pmodel,
			 const CExpression *pexprSrc, const CExpression *pexprTgt,
			 const CDSLConstraint *pconFailed = nullptr,
			 ULONG ulFailed = gpos::ulong_max, ULONG ulMatchUs = 0,
			 ULONG ulConstraintUs = 0, ULONG ulInstantiateUs = 0)
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
		poctxt->RecordDSLRuleTiming(ulRuleId, ulMatchUs, ulConstraintUs,
								 ulInstantiateUs);
	}
	// Keep machine trace cardinality identical in compact and verbose modes.
	// Reprinting every Cascades attempt can fill the task's fixed trace buffer
	// before ProcessTraceFlags() emits the authoritative rule summaries. Verbose
	// mode enriches the first rule/stage event; full native xform flooding is a
	// separate, explicitly diagnostic runner option.
	const BOOL fFirstEvent =
		nullptr == poctxt ||
		poctxt->FMarkDSLTraceEvent(ulRuleId, (ULONG) edsltrace);
	if (!fFirstEvent)
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
	   << "\",\"rule_hash\":\"" << prule->SzIdentity() << "\"";
	if (nullptr != poctxt && nullptr != poctxt->PdslPolicySnapshot())
	{
		const SDSLRulePolicy *policy =
			poctxt->PdslPolicySnapshot()->Ppolicy(prule);
		if (nullptr != policy)
		{
			os << ",\"placement\":\""
			   << SzDSLPlacement(policy->m_edslplacement)
			   << "\",\"phase\":\"" << SzDSLPhase(policy->m_edslphase)
			   << "\",\"priority\":" << policy->m_iPriority;
		}
	}
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

	// Rule identity also keys query-level budgets, so it must remain stable even
	// when trace emission is disabled.
	const ULONG ulRuleId = UlRuleId(prule);
	COptCtxt *poctxt = COptCtxt::PoctxtFromTLS();
	const SDSLRulePolicy *policy = nullptr == poctxt->PdslPolicySnapshot()
		? nullptr
		: poctxt->PdslPolicySnapshot()->Ppolicy(prule);
	if (nullptr != policy &&
		(!policy->m_fEnabled || EdslplacementCBO != policy->m_edslplacement))
	{
		return nullptr;
	}
	if (poctxt->FDSLAlternativeBudgetExhausted(ulRuleId))
	{
		// This binding was deliberately not inspected. Keep it distinct from
		// budget_exhausted, which follows a complete source match and target build.
		TraceDSLRule(mp, ulRuleId, EdsltraceBudgetSkipped, prule, nullptr,
					 pexpr, nullptr);
		return nullptr;
	}
	CDSLRewriteDecision *pdecision = PdecisionEvaluate(mp, prule, pexpr);
	CDSLModel *pmodel = pdecision->Pmodel();
	CExpression *pexprTgt = pdecision->PexprTarget();
	const ULONG ulMatchUs = pdecision->UlMatchUs();
	const ULONG ulConstraintUs = pdecision->UlConstraintUs();
	const ULONG ulInstantiateUs = pdecision->UlInstantiateUs();
	if (EdsldecisionMatchRejected == pdecision->Status())
	{
		TraceDSLRule(mp, ulRuleId, EdsltraceMatchRejected, prule, pmodel, pexpr,
					 nullptr, nullptr, gpos::ulong_max, ulMatchUs);
		GPOS_DELETE(pdecision);
		return nullptr;
	}
	if (EdsldecisionConstraintRejected == pdecision->Status())
	{
		TraceDSLRule(mp, ulRuleId, EdsltraceConstraintRejected, prule, pmodel,
					 pexpr, nullptr, pdecision->PconFailed(),
					 pdecision->UlFailedConstraint(), ulMatchUs, ulConstraintUs);
		GPOS_DELETE(pdecision);
		return nullptr;
	}
	if (EdsldecisionDuplicate == pdecision->Status())
	{
		// A representational adapter can rebuild a rule's target into the exact
		// source tree (for example when an output-column Project must remain over
		// a collapsed dedup layer). Re-inserting it cannot add an alternative and
		// may repeatedly fire as native xforms enumerate equivalent children.
		TraceDSLRule(mp, ulRuleId, EdsltraceDuplicate, prule, pmodel, pexpr,
					 pexprTgt, nullptr, gpos::ulong_max, ulMatchUs,
					 ulConstraintUs, ulInstantiateUs);
		GPOS_DELETE(pdecision);
		return nullptr;
	}
	if (EdsldecisionReady == pdecision->Status() &&
		!poctxt->FReserveDSLAlternative(ulRuleId))
	{
		TraceDSLRule(mp, ulRuleId, EdsltraceBudgetExhausted, prule, pmodel,
					 pexpr, pexprTgt, nullptr, gpos::ulong_max, ulMatchUs,
					 ulConstraintUs, ulInstantiateUs);
		GPOS_DELETE(pdecision);
		return nullptr;
	}
	TraceDSLRule(mp, ulRuleId,
				 EdsldecisionInstantiateRejected == pdecision->Status()
					 ? EdsltraceInstantiateRejected
					 : EdsltraceApplied,
				 prule,
				 pmodel, pexpr, pexprTgt, nullptr, gpos::ulong_max, ulMatchUs,
				 ulConstraintUs, ulInstantiateUs);
	CExpression *pexprResult = pdecision->PexprDetachTarget();
	GPOS_DELETE(pdecision);
	return pexprResult;
}

void
CDSLRuleEngine::TraceRBOOutcome(
	CMemoryPool *mp, const CDSLRule *prule,
	const SDSLRulePolicy *policy,
	const CDSLRewriteDecision *pdecision, CExpression *pexprSource,
	CExpression *pexprTarget, const CHAR *szStatus, const CHAR *szReason,
	const CDSLRule *pruleSelected) const
{
	if (!GPOS_FTRACE(EopttracePrintDSLRule))
		return;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(nullptr != pexprSource);
	GPOS_ASSERT(nullptr != szStatus);

	CAutoTrace trace(mp);
	IOstream &os = trace.Os();
	os << "DSL_TRACE {\"kind\":\"application\",\"engine\":\"pgorca\","
		  "\"rule_id\":"
	   << UlRuleId(prule) << ",\"status\":\"" << szStatus
	   << "\",\"rule_hash\":\"" << prule->SzIdentity()
	   << "\",\"placement\":\"rbo\"";
	if (nullptr != policy)
	{
		os << ",\"phase\":\"" << SzDSLPhase(policy->m_edslphase)
		   << "\",\"priority\":" << policy->m_iPriority
		   << ",\"effect\":\"" << SzDSLEffect(policy->m_edsleffect)
		   << "\",\"order\":\"" << SzDSLOrder(policy->m_edslorder)
		   << "\"";
	}
	const ULONG sourceFingerprint = nullptr == pdecision
		? CExpression::HashValue(pexprSource)
		: pdecision->UlSourceFingerprint();
	const ULONG targetFingerprint = nullptr == pdecision
		? (nullptr == pexprTarget ? 0 : CExpression::HashValue(pexprTarget))
		: pdecision->UlTargetFingerprint();
	os << ",\"source_fingerprint\":" << sourceFingerprint;
	if (nullptr != pexprTarget)
		os << ",\"target_fingerprint\":" << targetFingerprint;
	if (nullptr != szReason)
		os << ",\"reason\":\"" << szReason << "\"";
	if (nullptr != pruleSelected)
	{
		os << ",\"selected_rule_id\":" << UlRuleId(pruleSelected)
		   << ",\"selected_rule_hash\":\""
		   << pruleSelected->SzIdentity() << "\"";
	}
	if (nullptr != pdecision)
	{
		os << ",\"match_us\":" << pdecision->UlMatchUs()
		   << ",\"constraint_us\":" << pdecision->UlConstraintUs()
		   << ",\"instantiate_us\":" << pdecision->UlInstantiateUs();
		if (nullptr != pdecision->PconFailed())
		{
			os << ",\"failed_constraint\":\""
			   << CDSLConstraintKindTable::SzName(
					  pdecision->PconFailed()->Edslcon())
			   << "\",\"failed_constraint_index\":"
			   << pdecision->UlFailedConstraint();
		}
	}
	os << "}" << std::endl;
}

CDSLRewriteDecision *
CDSLRuleEngine::PdecisionEvaluate(CMemoryPool *mp, const CDSLRule *prule,
								  CExpression *pexpr,
								  BOOL fFingerprint) const
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(nullptr != pexpr);
	const CDSLOp *popSource = prule->PfragSrc()->PopRoot();
	if (EdslopLeftJoin != popSource->Edslop())
	{
		return PdecisionEvaluateDirect(mp, prule, pexpr, fFingerprint);
	}

	CExpressionArray *pdrgpexprView =
		CDSLMatchView::PdrgpexprNullRejectedLeftJoins(mp, pexpr);
	if (0 == pdrgpexprView->Size())
	{
		pdrgpexprView->Release();
		return PdecisionEvaluateDirect(mp, prule, pexpr, fFingerprint);
	}

	CDSLRewriteDecision *pdecisionRejected = nullptr;
	for (ULONG ul = 0; ul < pdrgpexprView->Size(); ul++)
	{
		CDSLRewriteDecision *pdecision = PdecisionEvaluateDirect(
			mp, prule, (*pdrgpexprView)[ul], false /*fingerprint view*/);
		if (EdsldecisionReady == pdecision->Status() ||
			EdsldecisionDuplicate == pdecision->Status())
		{
			CExpression *pexprTarget = pdecision->PexprDetachTarget();
			(*pexpr)[1]->AddRef();
			CExpression *pexprWrapped =
				CUtils::PexprSafeSelect(mp, pexprTarget, (*pexpr)[1]);
			const BOOL fSchemaPreserved =
				pexprWrapped->DeriveOutputColumns()->Equals(
					pexpr->DeriveOutputColumns());
			const EDslRewriteDecisionStatus status = !fSchemaPreserved
				? EdsldecisionInstantiateRejected
				: (pexprWrapped->Matches(pexpr) ? EdsldecisionDuplicate
											 : EdsldecisionReady);
			if (!fSchemaPreserved)
			{
				pexprWrapped->Release();
				pexprWrapped = nullptr;
			}
			CDSLRewriteDecision *pdecisionWrapped = GPOS_NEW(mp)
				CDSLRewriteDecision(
					pdecision->PmodelDetach(), pexprWrapped, status, nullptr,
					gpos::ulong_max, pdecision->UlMatchUs(),
					pdecision->UlConstraintUs(),
					pdecision->UlInstantiateUs(),
					fFingerprint ? CExpression::HashValue(pexpr) : 0,
					nullptr == pexprWrapped || !fFingerprint
						? 0
						: CExpression::HashValue(pexprWrapped));
			GPOS_DELETE(pdecision);
			GPOS_DELETE(pdecisionRejected);
			pdrgpexprView->Release();
			return pdecisionWrapped;
		}
		GPOS_DELETE(pdecisionRejected);
		pdecisionRejected = pdecision;
	}
	pdrgpexprView->Release();
	GPOS_ASSERT(nullptr != pdecisionRejected);
	CDSLRewriteDecision *pdecisionResult = GPOS_NEW(mp) CDSLRewriteDecision(
		pdecisionRejected->PmodelDetach(), nullptr,
		pdecisionRejected->Status(), pdecisionRejected->PconFailed(),
		pdecisionRejected->UlFailedConstraint(),
		pdecisionRejected->UlMatchUs(), pdecisionRejected->UlConstraintUs(),
		pdecisionRejected->UlInstantiateUs(),
		fFingerprint ? CExpression::HashValue(pexpr) : 0, 0);
	GPOS_DELETE(pdecisionRejected);
	return pdecisionResult;
}

CDSLRewriteDecision *
CDSLRuleEngine::PdecisionEvaluateDirect(CMemoryPool *mp,
										const CDSLRule *prule,
										CExpression *pexpr,
										BOOL fFingerprint) const
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(nullptr != pexpr);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	const BOOL fTrace = GPOS_FTRACE(EopttracePrintDSLRule);
	CWallClock stageTimer(fTrace);
	// Fingerprints are required by the RBO cycle guard, but computing them for
	// every rejected Cascades binding would add work to the legacy CBO path.
	const ULONG ulSourceFingerprint =
		fFingerprint ? CExpression::HashValue(pexpr) : 0;
	if (!FMatch(prule, pexpr, pmodel))
	{
		const ULONG ulMatchUs = fTrace ? stageTimer.ElapsedUS() : 0;
		return GPOS_NEW(mp) CDSLRewriteDecision(
			pmodel, nullptr, EdsldecisionMatchRejected, nullptr,
			gpos::ulong_max, ulMatchUs, 0, 0, ulSourceFingerprint, 0);
	}
	const ULONG ulMatchUs = fTrace ? stageTimer.ElapsedUS() : 0;
	if (fTrace)
		stageTimer.Restart();
	const CDSLConstraint *pconFailed = nullptr;
	ULONG ulFailed = gpos::ulong_max;
	if (!FCheckConstraints(prule, pmodel, pexpr, &pconFailed, &ulFailed))
	{
		const ULONG ulConstraintUs = fTrace ? stageTimer.ElapsedUS() : 0;
		return GPOS_NEW(mp) CDSLRewriteDecision(
			pmodel, nullptr, EdsldecisionConstraintRejected, pconFailed,
			ulFailed, ulMatchUs, ulConstraintUs, 0, ulSourceFingerprint, 0);
	}
	const ULONG ulConstraintUs = fTrace ? stageTimer.ElapsedUS() : 0;
	if (fTrace)
		stageTimer.Restart();
	CExpression *pexprTarget = PexprInstantiate(mp, prule, pmodel);
	const ULONG ulInstantiateUs = fTrace ? stageTimer.ElapsedUS() : 0;
	if (nullptr == pexprTarget)
	{
		return GPOS_NEW(mp) CDSLRewriteDecision(
			pmodel, nullptr, EdsldecisionInstantiateRejected, nullptr,
			gpos::ulong_max, ulMatchUs, ulConstraintUs, ulInstantiateUs,
			ulSourceFingerprint, 0);
	}
	const ULONG ulTargetFingerprint =
		fFingerprint ? CExpression::HashValue(pexprTarget) : 0;
	const EDslRewriteDecisionStatus status = pexprTarget->Matches(pexpr)
		? EdsldecisionDuplicate
		: EdsldecisionReady;
	return GPOS_NEW(mp) CDSLRewriteDecision(
		pmodel, pexprTarget, status, nullptr, gpos::ulong_max, ulMatchUs,
		ulConstraintUs, ulInstantiateUs, ulSourceFingerprint,
		ulTargetFingerprint);
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
