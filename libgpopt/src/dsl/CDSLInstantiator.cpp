//---------------------------------------------------------------------------
// MONSOON DSL rule engine — CDSLInstantiator.cpp
// Lifecycle, shared inputs, target dispatch and Memo root/provenance contracts.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLInstantiator.h"
#include "CDSLInstantiatorUtils.h"

#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalConstTableGet.h"
#include "gpopt/operators/CLogicalCTEAnchor.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarAggFunc.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarValuesList.h"
#include "gpopt/xforms/CXformUtils.h"

using namespace gpopt;
using namespace gpopt::dslinstantiator;

namespace
{
BOOL
FContainsInputSymbol(const CDSLOp *pop, const CDSLSymbol *psym)
{
	if (EdslopInput == pop->Edslop() && nullptr != pop->Pdrgpsym() &&
		1 == pop->Pdrgpsym()->Size() && (*pop->Pdrgpsym())[0] == psym)
	{
		return true;
	}
	for (ULONG ul = 0; ul < pop->UlChildren(); ul++)
	{
		if (FContainsInputSymbol((*pop)[ul], psym))
		{
			return true;
		}
	}
	return false;
}

// Rebuild a matched GbAgg while removing only aggregate DISTINCT semantics.
// The relational child, grouping metadata, output columns, ordinary/direct
// arguments and ORDER BY metadata are preserved; the DISTINCT flag and its
// sort-group metadata are cleared on freshly built aggregate operators.
CExpression *
PexprWithoutDistinctAgg(CMemoryPool *mp, CExpression *pexprAgg)
{
	GPOS_ASSERT(COperator::EopLogicalGbAgg == pexprAgg->Pop()->Eopid());
	CLogicalGbAgg *popGbAgg = CLogicalGbAgg::PopConvert(pexprAgg->Pop());
	CExpression *pexprOldList = (*pexprAgg)[1];
	CExpressionArray *pdrgpexprNewElems =
		GPOS_NEW(mp) CExpressionArray(mp);

	for (ULONG ul = 0; ul < pexprOldList->Arity(); ul++)
	{
		CExpression *pexprOldElem = (*pexprOldList)[ul];
		CExpression *pexprOldFunc = (*pexprOldElem)[0];
		CScalarAggFunc *popOldFunc =
			CScalarAggFunc::PopConvert(pexprOldFunc->Pop());
		if (!popOldFunc->IsDistinct())
		{
			pexprOldElem->AddRef();
			pdrgpexprNewElems->Append(pexprOldElem);
			continue;
		}

		popOldFunc->MDId()->AddRef();
		popOldFunc->GetArgTypes()->AddRef();
		IMDId *pmdidResolved = nullptr;
		if (popOldFunc->FHasAmbiguousReturnType())
		{
			pmdidResolved = popOldFunc->MdidType();
			pmdidResolved->AddRef();
		}
		CScalarAggFunc *popNewFunc = CUtils::PopAggFunc(
			mp, popOldFunc->MDId(),
			GPOS_NEW(mp)
				CWStringConst(mp, popOldFunc->PstrAggFunc()->GetBuffer()),
			false /*is_distinct*/, popOldFunc->Eaggfuncstage(),
			popOldFunc->FSplit(), pmdidResolved, popOldFunc->AggKind(),
			popOldFunc->GetArgTypes(), popOldFunc->FRepSafe(),
			popOldFunc->IsAggStar());
		CExpressionArray *pdrgpexprArgs = GPOS_NEW(mp) CExpressionArray(mp);
		for (ULONG ulArg = 0; ulArg < pexprOldFunc->Arity(); ulArg++)
		{
			if (EaggfuncIndexDistinct == ulArg)
			{
				pdrgpexprArgs->Append(GPOS_NEW(mp) CExpression(
					mp, GPOS_NEW(mp) CScalarValuesList(mp),
					GPOS_NEW(mp) CExpressionArray(mp)));
				continue;
			}
			CExpression *pexprArg = (*pexprOldFunc)[ulArg];
			pexprArg->AddRef();
			pdrgpexprArgs->Append(pexprArg);
		}
		CExpression *pexprNewFunc =
			GPOS_NEW(mp) CExpression(mp, popNewFunc, pdrgpexprArgs);
		pexprOldElem->Pop()->AddRef();
		pdrgpexprNewElems->Append(GPOS_NEW(mp) CExpression(
			mp, pexprOldElem->Pop(), pexprNewFunc));
	}

	CExpression *pexprNewList = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprNewElems);
	// The adapter only matches the original user-level global aggregate. Keep
	// its constructor form and stage: passing a null minimal-group array to the
	// split-aggregate overload would silently turn it into the full group set.
	GPOS_ASSERT(nullptr == popGbAgg->PdrgpcrMinimal());
	popGbAgg->Pdrgpcr()->AddRef();
	CColRefArray *pdrgpcrArgDQA = popGbAgg->PdrgpcrArgDQA();
	if (nullptr != pdrgpcrArgDQA)
	{
		pdrgpcrArgDQA->AddRef();
	}
	CLogicalGbAgg *popNewAgg = GPOS_NEW(mp) CLogicalGbAgg(
		mp, popGbAgg->Pdrgpcr(), popGbAgg->Egbaggtype(),
		popGbAgg->FGeneratesDuplicates(), pdrgpcrArgDQA,
		popGbAgg->AggStage());
	(*pexprAgg)[0]->AddRef();
	return GPOS_NEW(mp) CExpression(mp, popNewAgg, (*pexprAgg)[0],
									pexprNewList);
}
}  // namespace

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::CDSLInstantiator
//---------------------------------------------------------------------------
CDSLInstantiator::CDSLInstantiator(CMemoryPool *mp)
	: m_mp(mp),
	  m_phmAlias(nullptr),
	  m_phmDerivedCols(nullptr),
	  m_phmDerivedPreds(nullptr),
	  m_prule(nullptr),
	  m_pdrgpsymBuiltInputs(nullptr),
	  m_pinput_origins(nullptr)
{
	GPOS_ASSERT(nullptr != mp);
	m_phmAlias = GPOS_NEW(mp) CDSLSymbolAliasMap(mp);
	m_phmDerivedCols = GPOS_NEW(mp) CDSLSymbolToRefMap(mp);
	m_phmDerivedPreds = GPOS_NEW(mp) CDSLSymbolToExpressionMap(mp);
	m_pdrgpsymBuiltInputs = GPOS_NEW(mp) CDSLSymbolArray(mp);
}

void
CDSLInstantiator::IndexTargetInputs(const CDSLOp *pop,
									 const std::string &path)
{
	if (EdslopInput == pop->Edslop())
	{
		m_target_input_paths.emplace(pop, path);
		return;
	}
	for (ULONG child = 0; child < pop->UlChildren(); ++child)
	{
		IndexTargetInputs((*pop)[child],
						  path + "/" + std::to_string(child));
	}
}

void
CDSLInstantiator::RecordBuiltInput(const CDSLOp *pop,
								const CExpression *pexpr) const
{
	if (nullptr == m_pinput_origins || nullptr == pexpr)
	{
		return;
	}
	auto path = m_target_input_paths.find(pop);
	if (m_target_input_paths.end() != path)
	{
		m_built_input_roots.emplace_back(pexpr, path->second);
	}
}

BOOL
CDSLInstantiator::FFindExpressionPath(const CExpression *root,
								   const CExpression *target,
								   std::string *path) const
{
	if (root == target)
	{
		return true;
	}
	for (ULONG child = 0; child < root->Arity(); ++child)
	{
		const size_t length = path->size();
		path->append("/").append(std::to_string(child));
		if (FFindExpressionPath((*root)[child], target, path))
		{
			return true;
		}
		path->resize(length);
	}
	return false;
}

void
CDSLInstantiator::CollectTargetInputOrigins(const CExpression *root)
{
	if (nullptr == m_pinput_origins || nullptr == root)
	{
		return;
	}
	for (const auto &input : m_built_input_roots)
	{
		std::string expressionPath("r");
		if (FFindExpressionPath(root, input.first, &expressionPath))
		{
			m_pinput_origins->push_back({input.second, expressionPath});
		}
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::~CDSLInstantiator
//---------------------------------------------------------------------------
CDSLInstantiator::~CDSLInstantiator()
{
	for (auto &entry : m_input_col_maps)
	{
		entry.second->Release();
	}
	m_pdrgpsymBuiltInputs->Release();
	m_phmDerivedPreds->Release();
	m_phmDerivedCols->Release();
	m_phmAlias->Release();
}

BOOL
CDSLInstantiator::FPrepareSharedInputs(const CDSLRule *prule,
									   const CDSLModel *pmodel)
{
	std::unordered_map<const CDSLSymbol *, ULONG> cteBySource;
	for (ULONG ul = 0; ul < prule->Pdrgpcon()->Size(); ul++)
	{
		const CDSLConstraint *pcon = (*prule->Pdrgpcon())[ul];
		if (EdslconTableShared != pcon->Edslcon())
		{
			continue;
		}
		const CDSLSymbol *targets[] = {
			(*pcon->Pdrgpsym())[0], (*pcon->Pdrgpsym())[1]};
		const CDSLSymbol *source = PsymResolve(targets[0]);
		if (source != PsymResolve(targets[1]) ||
			EdslsideSource != source->Eside() ||
			nullptr == pmodel->PexprTable(source))
		{
			return false;
		}
		for (const CDSLSymbol *target : targets)
		{
			if (!FContainsInputSymbol(prule->PfragTgt()->PopRoot(), target))
			{
				return false;
			}
		}

		auto sourceIt = cteBySource.find(source);
		if (sourceIt == cteBySource.end())
		{
			const ULONG id = COptCtxt::PoctxtFromTLS()->Pcteinfo()->next_id();
			sourceIt = cteBySource.emplace(source, id).first;
			m_shared_sources.push_back(source);
			m_shared_cte_ids.push_back(id);
		}
		for (const CDSLSymbol *target : targets)
		{
			auto inserted =
				m_shared_cte_by_target.emplace(target, sourceIt->second);
			if (!inserted.second && inserted.first->second != sourceIt->second)
			{
				return false;
			}
		}
	}
	for (ULONG ul = 0; ul < m_shared_sources.size(); ul++)
	{
		CExpression *pexprSource = pmodel->PexprTable(m_shared_sources[ul]);
		CColRefArray *pdrgpcrOutput = PdrgpcrLiveOutput(m_mp, pexprSource);
		(void) CXformUtils::PexprAddCTEProducer(
			m_mp, m_shared_cte_ids[ul], pdrgpcrOutput, pexprSource);
		pdrgpcrOutput->Release();
	}
	return true;
}

CExpression *
CDSLInstantiator::PexprFinalizeSharedInputs(CExpression *pexpr) const
{
	if (nullptr == pexpr)
	{
		return nullptr;
	}
	for (ULONG ul = m_shared_cte_ids.size(); 0 < ul; ul--)
	{
		pexpr = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalCTEAnchor(
				m_mp, m_shared_cte_ids[ul - 1]), pexpr);
	}
	return pexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildInput
//
//	@doc:
//		Input<t>: reuse the relational subtree bound to t (resolved through the
//		alias map). AddRef-graft it into the target.
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildInput(const CDSLOp *pop,
								  const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	if (nullptr == pdrgpsym || 1 != pdrgpsym->Size())
	{
		return nullptr;
	}
	const CDSLSymbol *psymTarget = (*pdrgpsym)[0];
	const CDSLSymbol *psymTable = PsymResolve(psymTarget);
	CExpression *pexpr = pmodel->PexprTable(psymTable);
	if (nullptr == pexpr)
	{
		return nullptr;
	}
	BOOL fAlreadyBuilt = false;
	for (ULONG ul = 0; ul < m_pdrgpsymBuiltInputs->Size(); ul++)
	{
		if ((*m_pdrgpsymBuiltInputs)[ul] == psymTable)
		{
			fAlreadyBuilt = true;
			break;
		}
	}
	if (!fAlreadyBuilt)
	{
		const_cast<CDSLSymbol *>(psymTable)->AddRef();
		m_pdrgpsymBuiltInputs->Append(
			const_cast<CDSLSymbol *>(psymTable));
	}
	auto shared = m_shared_cte_by_target.find(psymTarget);
	if (shared != m_shared_cte_by_target.end())
	{
		CColRefArray *pdrgpcrFrom = PdrgpcrLiveOutput(m_mp, pexpr);
		CColRefArray *pdrgpcrConsumer = pdrgpcrFrom;
		if (fAlreadyBuilt)
		{
			UlongToColRefMap *phm = GPOS_NEW(m_mp) UlongToColRefMap(m_mp);
			pdrgpcrConsumer = CUtils::PdrgpcrCopy(
				m_mp, pdrgpcrFrom, false /* fAllComputed */, phm);
			m_input_col_maps.emplace(pop, phm);
			pdrgpcrFrom->Release();
		}
		CExpression *pexprConsumer = CXformUtils::PexprCTEConsumer(
			m_mp, shared->second, pdrgpcrConsumer);
		RecordBuiltInput(pop, pexprConsumer);
		return pexprConsumer;
	}
	if (!fAlreadyBuilt)
	{
		pexpr->AddRef();
		RecordBuiltInput(pop, pexpr);
		return pexpr;
	}

	// A repeated target occurrence denotes another range variable. Reusing the
	// same CColRefs would conflate both occurrences and, inside a SetOp, violate
	// the per-input column identity contract. Copy the complete subtree using a
	// fresh output-column map, as native ORCA distribution xforms do.
	CColRefArray *pdrgpcrFrom =
		pexpr->DeriveOutputColumns()->Pdrgpcr(m_mp);
	UlongToColRefMap *phm = GPOS_NEW(m_mp) UlongToColRefMap(m_mp);
	CColRefArray *pdrgpcrTo =
		CUtils::PdrgpcrCopy(m_mp, pdrgpcrFrom, false, phm);
	CExpression *pexprCopy = pexpr->PexprCopyWithRemappedColumns(
		m_mp, phm, true /*must_exist*/);
	pdrgpcrTo->Release();
	m_input_col_maps.emplace(pop, phm);
	pdrgpcrFrom->Release();
	RecordBuiltInput(pop, pexprCopy);
	return pexprCopy;
}

CExpression *
CDSLInstantiator::PexprBuildEmpty(const CDSLOp *pop,
								  const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	if (nullptr == pdrgpsym || 1 != pdrgpsym->Size())
	{
		return nullptr;
	}
	const CDSLSymbol *psymTable = PsymResolve((*pdrgpsym)[0]);
	CExpression *pexprSchemaSource = pmodel->PexprTable(psymTable);
	if (nullptr == pexprSchemaSource)
	{
		return nullptr;
	}
	CColRefArray *pdrgpcrOutput =
		pexprSchemaSource->DeriveOutputColumns()->Pdrgpcr(m_mp);
	return GPOS_NEW(m_mp) CExpression(
		m_mp,
		GPOS_NEW(m_mp) CLogicalConstTableGet(
			m_mp, pdrgpcrOutput, GPOS_NEW(m_mp) IDatum2dArray(m_mp)));
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuild
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuild(const CDSLOp *pop, const CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != pop);

	switch (pop->Edslop())
	{
		case EdslopInput:
			return PexprBuildInput(pop, pmodel);
		case EdslopEmpty:
			return PexprBuildEmpty(pop, pmodel);
		case EdslopFilter:
			return PexprBuildFilter(pop, pmodel);
		case EdslopProj:
			return PexprBuildProj(pop, pmodel);
		case EdslopCompute:
			return PexprBuildCompute(pop, pmodel);
		case EdslopAgg:
			return PexprBuildAgg(pop, pmodel);
		case EdslopExists:
		case EdslopNotExists:
			return PexprBuildExists(pop, pmodel);
		case EdslopInSubFilter:
			return PexprBuildInSub(pop, pmodel);
		case EdslopAny:
		case EdslopAll:
			return PexprBuildQuantified(pop, pmodel);
		case EdslopUnion:
		case EdslopIntersect:
		case EdslopExcept:
			return PexprBuildUnion(pop, pmodel);
		case EdslopSort:
			return PexprBuildSort(pop, pmodel);
		case EdslopLimit:
			return PexprBuildLimit(pop, pmodel);
		case EdslopWindowRows:
		case EdslopWindowFrame:
			return PexprBuildWindow(pop, pmodel);
		case EdslopRowNumber:
			return PexprBuildRowNumber(pop, pmodel);
		case EdslopAssertMaxOneRow:
			return PexprBuildAssertMaxOneRow(pop, pmodel);
		case EdslopMaxOneRow:
			// MaxOneRow is an input/source contract. Targets use its executable
			// AssertMaxOneRow representation so no logical placeholder survives.
			return nullptr;
		case EdslopInnerJoin:
		case EdslopLeftJoin:
		case EdslopFullJoin:
		case EdslopSemiJoin:
		case EdslopSemiApply:
		case EdslopAntiJoin:
		case EdslopAntiApply:
		case EdslopAntiJoinNotIn:
		case EdslopAntiApplyNotIn:
		case EdslopInnerApply:
		case EdslopLeftOuterApply:
			return PexprBuildJoin(pop, pmodel);
		default:
			return nullptr;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprFreshRoot
//
//	@doc:
//		Cascades contract (CEngine::PgroupInsert): an xform result ROOT must be a
//		freshly-built CExpression (Pgexpr()==NULL); a memo-extracted node as root
//		trips the "A valid group is expected" assertion. CHILDREN may freely reuse
//		memo subtrees. Operator-eliminating rules (e.g. Filter(Input<t0>) ->
//		Input<t1>) build a target whose root IS a reused memo subtree, so we must
//		re-root it. Copy only the root operator and keep its memo-bound children.
//		Deep-copying the subtree would make CEngine recursively insert every level
//		again; equivalent-group merging can then turn an ordinary ancestor/child
//		chain into a circular memo dependency. An empty column map preserves the
//		root's CColRefs and therefore its output-column invariant. Fresh-rooted
//		targets (Filter/Join) are returned as-is.
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprFreshRoot(CExpression *pexpr)
{
	if (nullptr == pexpr || nullptr == pexpr->Pgexpr())
	{
		// already a freshly-built root (or NULL) — nothing to do.
		return pexpr;
	}

	// Re-root via an identity operator remap (empty mapping => colrefs pass
	// through), while grafting the existing memo-bound children unchanged.
	UlongToColRefMap *colref_mapping = GPOS_NEW(m_mp) UlongToColRefMap(m_mp);
	COperator *popFresh = pexpr->Pop()->PopCopyWithRemappedColumns(
		m_mp, colref_mapping, false /*must_exist*/);
	colref_mapping->Release();

	CExpressionArray *pdrgpexprChildren =
		GPOS_NEW(m_mp) CExpressionArray(m_mp, pexpr->Arity());
	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		CExpression *pexprChild = (*pexpr)[ul];
		pexprChild->AddRef();
		pdrgpexprChildren->Append(pexprChild);
	}
	CExpression *pexprFresh = GPOS_NEW(m_mp)
		CExpression(m_mp, popFresh, pdrgpexprChildren);
	for (auto &input : m_built_input_roots)
	{
		if (input.first == pexpr)
		{
			input.first = pexprFresh;
		}
	}
	pexpr->Release();
	return pexprFresh;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprInstantiate
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprInstantiate(const CDSLRule *prule,
								   const CDSLModel *pmodel,
								   CDSLTargetInputOriginArray *inputOrigins)
{
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(nullptr != pmodel);

	m_prule = prule;
	m_pinput_origins = inputOrigins;
	if (nullptr != m_pinput_origins)
	{
		m_pinput_origins->clear();
		IndexTargetInputs(prule->PfragTgt()->PopRoot(), "r");
	}
	BuildAliasMap(prule);
	if (!FPrepareSharedInputs(prule, pmodel))
	{
		return nullptr;
	}
	const CDSLOp *popSrcRoot = prule->PfragSrc()->PopRoot();
	const CDSLOp *popTgtRoot = prule->PfragTgt()->PopRoot();
	const BOOL fVirtualDqaSource =
		EdslopProj == popSrcRoot->Edslop() && popSrcRoot->FDistinct() &&
		1 == popSrcRoot->UlChildren() &&
		EdslopInput == (*popSrcRoot)[0]->Edslop();
	const BOOL fVirtualDqaTarget =
		EdslopInput == popTgtRoot->Edslop() ||
		(EdslopProj == popTgtRoot->Edslop() && !popTgtRoot->FDistinct() &&
		 1 == popTgtRoot->UlChildren() &&
		 EdslopInput == (*popTgtRoot)[0]->Edslop());
	CExpression *pexprTgt = nullptr;
	if (nullptr != pmodel->PexprDistinctAgg())
	{
		// A DQA GbAgg is the ORCA representation of the *outer* WeTune
		// Agg(Proj*) pair. Only a rule replacing that virtual source root can be
		// reconstructed without inventing or discarding the surrounding Agg.
		if (!fVirtualDqaSource || !fVirtualDqaTarget)
		{
			return nullptr;
		}
		pexprTgt = PexprWithoutDistinctAgg(
			m_mp, pmodel->PexprDistinctAgg());
	}
	else
	{
		pexprTgt = PexprBuild(popTgtRoot, pmodel);
	}

	const EDslOpKind edslopSrc = popSrcRoot->Edslop();
	const EDslOpKind edslopTgt = popTgtRoot->Edslop();

	// dedup drop: the source root was a redundant SELECT DISTINCT (pure-dedup
	// CLogicalGbAgg whose grouping cols form a key). PexprBuild produced the
	// resolved relational child (a bare Input target); wrap it in Select(child,
	// TRUE) to drop the GbAgg, exactly like ORCA's CXformSimplifyGbAgg::FDropGbAgg.
	// This keeps the memo group's output-column invariant: the trivial Select
	// outputs the child's columns (a superset of the GbAgg's grouping-only output),
	// which is the same substitution the native xform makes. The Select is a fresh
	// CExpression, so PexprFreshRoot returns it as-is (no remap needed).
	if (nullptr != pexprTgt && pmodel->FDedupDrop() &&
		EdslopProj == prule->PfragSrc()->PopRoot()->Edslop() &&
		prule->PfragSrc()->PopRoot()->FDistinct() &&
		!(EdslopProj == prule->PfragTgt()->PopRoot()->Edslop() &&
		  prule->PfragTgt()->PopRoot()->FDistinct()))
	{
		pexprTgt = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprTgt,
			CPredicateUtils::PexprConjunction(m_mp, nullptr));
	}
	// A memo group cannot be replaced directly by one of its child groups.
	// Preserve the relational identity with the same trivial Select shell used
	// by CXformCTEAnchor2TrivialSelect.
	if (nullptr != pexprTgt && EdslopCTEAnchor == edslopSrc &&
		EdslopInput == edslopTgt)
	{
		pexprTgt = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprTgt,
			CPredicateUtils::PexprConjunction(m_mp, nullptr));
	}
	CExpression *pexprResult =
		PexprFreshRoot(PexprFinalizeSharedInputs(pexprTgt));
	CollectTargetInputOrigins(pexprResult);
	return pexprResult;
}
