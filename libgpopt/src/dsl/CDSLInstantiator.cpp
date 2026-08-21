//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLInstantiator.cpp
//
//	@doc:
//		Implementation of the target builder (see CDSLInstantiator.h). Migrates
//		the SEMANTICS of WeTune Instantiation for structural operators.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLInstantiator.h"

#include "gpos/base.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalJoin.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CLogicalLeftSemiApply.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarAggFunc.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"

using namespace gpopt;

namespace
{
BOOL
FAggNameEquals(CMemoryPool *mp, const CWStringConst *pstrActual,
			   const CHAR *szExpected)
{
	CWStringConst strExpected(mp, szExpected);
	return pstrActual->Equals(&strExpected);
}

BOOL
FAggFuncMatches(CMemoryPool *mp, const CDSLOp *popAgg,
				const CScalarAggFunc *popFunc)
{
	if (popAgg->FDistinct() != popFunc->IsDistinct())
	{
		return false;
	}
	switch (popAgg->Edslaggfunc())
	{
		case EdslaggfuncUnknown:
			return true;
		case EdslaggfuncSentinel:
			return false;
		case EdslaggfuncAverage:
			return FAggNameEquals(mp, popFunc->PstrAggFunc(), "avg") ||
				   FAggNameEquals(mp, popFunc->PstrAggFunc(), "average");
		default:
			return FAggNameEquals(
				mp, popFunc->PstrAggFunc(),
				CDSLOpKindTable::SzAggFuncName(popAgg->Edslaggfunc()));
	}
}

BOOL
FColArraysSameSet(CMemoryPool *mp, const CColRefArray *pdrgpcrFirst,
				  const CColRefArray *pdrgpcrSecond)
{
	CColRefSet *pcrsFirst = GPOS_NEW(mp) CColRefSet(mp);
	CColRefSet *pcrsSecond = GPOS_NEW(mp) CColRefSet(mp);
	pcrsFirst->Include(const_cast<CColRefArray *>(pdrgpcrFirst));
	pcrsSecond->Include(const_cast<CColRefArray *>(pdrgpcrSecond));
	BOOL fEqual = pcrsFirst->Equals(pcrsSecond);
	pcrsFirst->Release();
	pcrsSecond->Release();
	return fEqual;
}
}  // namespace

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::CDSLInstantiator
//---------------------------------------------------------------------------
CDSLInstantiator::CDSLInstantiator(CMemoryPool *mp) : m_mp(mp), m_phmAlias(nullptr)
{
	GPOS_ASSERT(nullptr != mp);
	m_phmAlias = GPOS_NEW(mp) CDSLSymbolAliasMap(mp);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::~CDSLInstantiator
//---------------------------------------------------------------------------
CDSLInstantiator::~CDSLInstantiator()
{
	m_phmAlias->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::BuildAliasMap
//
//	@doc:
//		From each equality constraint *Eq(x,y), link the target-side symbol to the
//		source-side symbol whose binding it reuses. Both symbols share a kind
//		(the DSL guarantees *Eq relates same-kind symbols), so we only need the
//		side to orient the alias.
//---------------------------------------------------------------------------
void
CDSLInstantiator::BuildAliasMap(const CDSLRule *prule)
{
	CDSLConstraintArray *pdrgpcon = prule->Pdrgpcon();
	const ULONG ulCon = pdrgpcon->Size();
	for (ULONG ul = 0; ul < ulCon; ul++)
	{
		const CDSLConstraint *pcon = (*pdrgpcon)[ul];
		switch (pcon->Edslcon())
		{
			case EdslconTableEq:
			case EdslconAttrsEq:
			case EdslconPredicateEq:
			case EdslconSchemaEq:
			case EdslconFuncEq:
			case EdslconScalarEq:
				break;	// an aliasing equality
			default:
				continue;  // structural constraint: not an alias
		}

		CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
		if (2 != pdrgpsym->Size())
		{
			continue;
		}
		CDSLSymbol *psym0 = (*pdrgpsym)[0];
		CDSLSymbol *psym1 = (*pdrgpsym)[1];

		// orient: target-side symbol aliases the source-side symbol.
		CDSLSymbol *psymTgt = nullptr;
		CDSLSymbol *psymSrc = nullptr;
		if (EdslsideTarget == psym0->Eside() &&
			EdslsideSource == psym1->Eside())
		{
			psymTgt = psym0;
			psymSrc = psym1;
		}
		else if (EdslsideSource == psym0->Eside() &&
				 EdslsideTarget == psym1->Eside())
		{
			psymTgt = psym1;
			psymSrc = psym0;
		}
		else
		{
			// both same side (e.g. two source symbols in one class): no target
			// alias to record here.
			continue;
		}

		// first alias wins (a target symbol may appear in several *Eq; any source
		// representative of its class is fine since they are all co-bound).
		if (nullptr == m_phmAlias->Find(psymTgt))
		{
			BOOL fOk = m_phmAlias->Insert(psymTgt, psymSrc);
			GPOS_ASSERT(fOk);
			(void) fOk;
		}
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PsymResolve
//---------------------------------------------------------------------------
const CDSLSymbol *
CDSLInstantiator::PsymResolve(const CDSLSymbol *psym) const
{
	CDSLSymbol *psymSrc = m_phmAlias->Find(psym);
	return (nullptr != psymSrc) ? psymSrc : psym;
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
	const CDSLSymbol *psymTable = PsymResolve((*pdrgpsym)[0]);
	CExpression *pexpr = pmodel->PexprTable(psymTable);
	if (nullptr == pexpr)
	{
		return nullptr;
	}
	pexpr->AddRef();
	return pexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildFilter
//
//	@doc:
//		Filter<p a>: Select(child, predicate). The predicate is the conjunct
//		bound to p (resolved through the alias map) CONJOINED with every residual
//		conjunct the matcher preserved, so no predicate is dropped.
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildFilter(const CDSLOp *pop,
								   const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	if (nullptr == pdrgpsym || 2 != pdrgpsym->Size() || 1 != pop->UlChildren())
	{
		return nullptr;
	}
	const CDSLSymbol *psymPred = PsymResolve((*pdrgpsym)[0]);

	CExpression *pexprChild = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprChild)
	{
		return nullptr;
	}

	CExpression *pexprPredBound = pmodel->PexprPred(psymPred);
	if (nullptr == pexprPredBound)
	{
		pexprChild->Release();
		return nullptr;
	}

	// collect the target predicate's conjuncts: the bound conjunct + residuals.
	CExpressionArray *pdrgpexpr = GPOS_NEW(m_mp) CExpressionArray(m_mp);
	pexprPredBound->AddRef();
	pdrgpexpr->Append(pexprPredBound);

	CExpressionArray *pdrgpexprResidual = pmodel->PdrgpexprResidual();
	if (nullptr != pdrgpexprResidual)
	{
		const ULONG ulResidual = pdrgpexprResidual->Size();
		for (ULONG ul = 0; ul < ulResidual; ul++)
		{
			CExpression *pexprR = (*pdrgpexprResidual)[ul];
			pexprR->AddRef();
			pdrgpexpr->Append(pexprR);
		}
	}

	// build one conjunctive predicate (single conjunct -> itself; many -> And).
	CExpression *pexprPred = CPredicateUtils::PexprConjunction(m_mp, pdrgpexpr);

	return GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprChild, pexprPred);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildJoin
//
//	@doc:
//		InnerJoin/LeftJoin<a a>: rebuild both relational children and graft the
//		SOURCE-matched join predicate (recorded on the model by CDSLJoinMatcher),
//		building the join operator the TARGET op names. Reusing the exact predicate
//		subtree the matcher saw preserves the equi + non-equi conjuncts (and the
//		precise comparison ops) — so no predicate is dropped and output columns are
//		preserved without per-key remapping (rebuilding NEW keys is future work).
//		Returns NULL if the model carries no join predicate (source was not a join),
//		in which case the rule simply does not fire.
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildJoin(const CDSLOp *pop,
								 const CDSLModel *pmodel) const
{
	if (2 != pop->UlChildren())
	{
		return nullptr;
	}

	CExpression *pexprJoinPred = pmodel->PexprJoinPred();
	if (nullptr == pexprJoinPred)
	{
		return nullptr;
	}

	CExpression *pexprLeft = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprLeft)
	{
		return nullptr;
	}
	CExpression *pexprRight = PexprBuild((*pop)[1], pmodel);
	if (nullptr == pexprRight)
	{
		pexprLeft->Release();
		return nullptr;
	}

	// build the join operator the TARGET names (Inner or LeftOuter).
	CLogicalJoin *popJoin = nullptr;
	switch (pop->Edslop())
	{
		case EdslopInnerJoin:
			popJoin = GPOS_NEW(m_mp) CLogicalInnerJoin(m_mp);
			break;
		case EdslopLeftJoin:
			popJoin = GPOS_NEW(m_mp) CLogicalLeftOuterJoin(m_mp);
			break;
		default:
			pexprLeft->Release();
			pexprRight->Release();
			return nullptr;
	}

	// graft the matched predicate (AddRef — the model keeps its own ref).
	pexprJoinPred->AddRef();
	return GPOS_NEW(m_mp)
		CExpression(m_mp, popJoin, pexprLeft, pexprRight, pexprJoinPred);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildProj
//
//	@doc:
//		Proj<a s>: CLogicalProject(child, project-list). M1 rebuilds the relational
//		child and grafts the SOURCE-matched project-list subtree (recorded on the
//		model by CDSLProjMatcher). Reusing the very list the matcher saw preserves
//		the projected/computed columns — and hence the output-column invariant —
//		exactly, without per-column remapping (that is only needed once a target
//		introduces NEW columns; future work). Returns NULL if the model carries no
//		project list (i.e. the source was not a Proj) — the rule then does not fire.
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildProj(const CDSLOp *pop,
								 const CDSLModel *pmodel) const
{
	if (1 != pop->UlChildren())
	{
		return nullptr;
	}

	CExpression *pexprProjList = pmodel->PexprProjList();
	if (nullptr == pexprProjList)
	{
		return nullptr;
	}

	CExpression *pexprChild = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprChild)
	{
		return nullptr;
	}

	// graft the matched project list (AddRef — the model keeps its own ref).
	pexprProjList->AddRef();
	return GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CLogicalProject(m_mp), pexprChild, pexprProjList);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildAgg
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildAgg(const CDSLOp *pop,
								const CDSLModel *pmodel) const
{
	if (1 != pop->UlChildren())
	{
		return nullptr;
	}
	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	if (nullptr == pdrgpsym ||
		(5 != pdrgpsym->Size() && 6 != pdrgpsym->Size()))
	{
		return nullptr;
	}
	const BOOL fLegacy = 5 == pdrgpsym->Size();
	const ULONG ulFunc = fLegacy ? 2 : 3;
	const ULONG ulSchema = fLegacy ? 3 : 4;
	const ULONG ulHaving = fLegacy ? 4 : 5;

	const CDSLSymbol *psymGroup = PsymResolve((*pdrgpsym)[0]);
	const CDSLSymbol *psymAggInputs = PsymResolve((*pdrgpsym)[1]);
	const CDSLSymbol *psymFuncs = PsymResolve((*pdrgpsym)[ulFunc]);
	const CDSLSymbol *psymSchema = PsymResolve((*pdrgpsym)[ulSchema]);
	const CDSLSymbol *psymHaving = PsymResolve((*pdrgpsym)[ulHaving]);

	CColRefArray *pdrgpcrGroup = pmodel->PdrgpcrAttrs(psymGroup);
	CColRefArray *pdrgpcrAggInputs = pmodel->PdrgpcrAttrs(psymAggInputs);
	CExpressionArray *pdrgpexprFuncs = pmodel->PdrgpexprFunc(psymFuncs);
	CColRefArray *pdrgpcrSchema = pmodel->PdrgpcrSchema(psymSchema);
	CExpression *pexprHaving = pmodel->PexprPred(psymHaving);
	if (nullptr == pdrgpcrGroup || nullptr == pdrgpcrAggInputs ||
		nullptr == pdrgpexprFuncs || nullptr == pdrgpcrSchema ||
		nullptr == pexprHaving)
	{
		return nullptr;
	}

	// The repository's established Agg<a a f s p> format has no explicit
	// aggregate-output symbol. In a GbAgg schema, grouping columns are passed
	// through and every remaining schema column is defined by one aggregate
	// project element, so recover the output array as schema - groupByAttrs.
	CColRefArray *pdrgpcrAggOutputs = nullptr;
	BOOL fOwnAggOutputs = false;
	if (fLegacy)
	{
		fOwnAggOutputs = true;
		pdrgpcrAggOutputs = GPOS_NEW(m_mp) CColRefArray(m_mp);
		CColRefSet *pcrsGroup = GPOS_NEW(m_mp) CColRefSet(m_mp);
		pcrsGroup->Include(pdrgpcrGroup);
		for (ULONG ul = 0; ul < pdrgpcrSchema->Size(); ul++)
		{
			CColRef *pcr = (*pdrgpcrSchema)[ul];
			if (!pcrsGroup->FMember(pcr))
			{
				pdrgpcrAggOutputs->Append(pcr);
			}
		}
		pcrsGroup->Release();
	}
	else
	{
		const CDSLSymbol *psymAggOutputs = PsymResolve((*pdrgpsym)[2]);
		pdrgpcrAggOutputs = pmodel->PdrgpcrAttrs(psymAggOutputs);
	}
	if (nullptr == pdrgpcrAggOutputs ||
		pdrgpcrAggOutputs->Size() != pdrgpexprFuncs->Size())
	{
		if (fOwnAggOutputs)
		{
			pdrgpcrAggOutputs->Release();
		}
		return nullptr;
	}

	CExpression *pexprChild = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprChild)
	{
		if (fOwnAggOutputs)
		{
			pdrgpcrAggOutputs->Release();
		}
		return nullptr;
	}

	CColRefSet *pcrsChild = pexprChild->DeriveOutputColumns();
	CColRefSet *pcrsGroup = GPOS_NEW(m_mp) CColRefSet(m_mp);
	CColRefSet *pcrsFuncInputs = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsGroup->Include(pdrgpcrGroup);
	for (ULONG ul = 0; ul < pdrgpexprFuncs->Size(); ul++)
	{
		CExpression *pexprFunc = (*pdrgpexprFuncs)[ul];
		if (COperator::EopScalarAggFunc != pexprFunc->Pop()->Eopid() ||
			!FAggFuncMatches(
				m_mp, pop,
				CScalarAggFunc::PopConvert(pexprFunc->Pop())))
		{
			pcrsGroup->Release();
			pcrsFuncInputs->Release();
			pexprChild->Release();
			if (fOwnAggOutputs)
			{
				pdrgpcrAggOutputs->Release();
			}
			return nullptr;
		}
		pcrsFuncInputs->Include(pexprFunc->DeriveUsedColumns());
	}

	CColRefArray *pdrgpcrActualInputs = pcrsFuncInputs->Pdrgpcr(m_mp);
	BOOL fInputsValid = FColArraysSameSet(
		m_mp, pdrgpcrAggInputs, pdrgpcrActualInputs);
	pdrgpcrActualInputs->Release();

	CColRefSet *pcrsExpectedSchema = GPOS_NEW(m_mp) CColRefSet(m_mp);
	CColRefSet *pcrsSchema = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsExpectedSchema->Include(pdrgpcrGroup);
	pcrsExpectedSchema->Include(pdrgpcrAggOutputs);
	pcrsSchema->Include(pdrgpcrSchema);
	BOOL fSchemaValid = pcrsExpectedSchema->Equals(pcrsSchema);

	BOOL fColumnsValid = fInputsValid && fSchemaValid &&
					 pcrsChild->ContainsAll(pcrsGroup) &&
					 pcrsChild->ContainsAll(pcrsFuncInputs);
	pcrsGroup->Release();
	pcrsFuncInputs->Release();
	pcrsExpectedSchema->Release();
	pcrsSchema->Release();
	if (!fColumnsValid)
	{
		pexprChild->Release();
		if (fOwnAggOutputs)
		{
			pdrgpcrAggOutputs->Release();
		}
		return nullptr;
	}

	CExpressionArray *pdrgpexprPrEl =
		GPOS_NEW(m_mp) CExpressionArray(m_mp);
	for (ULONG ul = 0; ul < pdrgpexprFuncs->Size(); ul++)
	{
		CExpression *pexprFunc = (*pdrgpexprFuncs)[ul];
		pexprFunc->AddRef();
		pdrgpexprPrEl->Append(GPOS_NEW(m_mp) CExpression(
			m_mp,
			GPOS_NEW(m_mp) CScalarProjectElement(
				m_mp, (*pdrgpcrAggOutputs)[ul]),
			pexprFunc));
	}
	CExpression *pexprAggList = GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CScalarProjectList(m_mp), pdrgpexprPrEl);

	pdrgpcrGroup->AddRef();
	CExpression *pexprResult = GPOS_NEW(m_mp) CExpression(
		m_mp,
		GPOS_NEW(m_mp) CLogicalGbAgg(m_mp, pdrgpcrGroup,
									 COperator::EgbaggtypeGlobal),
		pexprChild, pexprAggList);

	if (!CUtils::FScalarConstTrue(pexprHaving))
	{
		pexprHaving->AddRef();
		pexprResult = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprResult,
			pexprHaving);
	}
	if (fOwnAggOutputs)
	{
		pdrgpcrAggOutputs->Release();
	}
	return pexprResult;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildExists
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildExists(const CDSLOp *pop,
								   const CDSLModel *pmodel) const
{
	if (2 != pop->UlChildren() || nullptr == pop->Pdrgpsym() ||
		0 != pop->Pdrgpsym()->Size())
	{
		return nullptr;
	}

	CExpression *pexprOuter = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprOuter)
	{
		return nullptr;
	}
	CExpression *pexprInner = PexprBuild((*pop)[1], pmodel);
	if (nullptr == pexprInner)
	{
		pexprOuter->Release();
		return nullptr;
	}

	CColRefSet *pcrsInnerOutput = pexprInner->DeriveOutputColumns();
	if (0 == pcrsInnerOutput->Size())
	{
		pexprOuter->Release();
		pexprInner->Release();
		return nullptr;
	}
	CColRef *pcrInner = pcrsInnerOutput->PcrFirst();

	// Mirror subquery removal: LIMIT 1 is valid and avoids unnecessary work only
	// for an uncorrelated EXISTS input.
	if (0 == pexprInner->DeriveOuterReferences()->Size() &&
		1 < pexprInner->DeriveMaxCard().Ull())
	{
		pexprInner = CUtils::PexprLimit(m_mp, pexprInner, 0, 1);
	}

	CExpression *pexprResult =
		CUtils::PexprLogicalApply<CLogicalLeftSemiApply>(
		m_mp, pexprOuter, pexprInner, pcrInner,
		COperator::EopScalarSubqueryExists);

	CExpressionArray *pdrgpexprResidual =
		pmodel->PdrgpexprExistsResidual();
	if (nullptr != pdrgpexprResidual && 0 < pdrgpexprResidual->Size())
	{
		CExpressionArray *pdrgpexprCopy =
			GPOS_NEW(m_mp) CExpressionArray(m_mp);
		for (ULONG ul = 0; ul < pdrgpexprResidual->Size(); ul++)
		{
			CExpression *pexprConj = (*pdrgpexprResidual)[ul];
			pexprConj->AddRef();
			pdrgpexprCopy->Append(pexprConj);
		}
		CExpression *pexprPred =
			CPredicateUtils::PexprConjunction(m_mp, pdrgpexprCopy);
		pexprResult = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprResult,
			pexprPred);
	}
	return pexprResult;
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
		case EdslopFilter:
			return PexprBuildFilter(pop, pmodel);
		case EdslopProj:
			return PexprBuildProj(pop, pmodel);
		case EdslopAgg:
			return PexprBuildAgg(pop, pmodel);
		case EdslopExists:
			return PexprBuildExists(pop, pmodel);
		case EdslopInnerJoin:
		case EdslopLeftJoin:
			return PexprBuildJoin(pop, pmodel);
		default:
			// Union / Sort / Limit: not yet instantiable (future work).
			// The rule does not fire.
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
//		re-root it. PexprCopyWithRemappedColumns with an EMPTY mapping is the
//		standard ORCA primitive that does exactly this: every node (root included)
//		is GPOS_NEW'd afresh, and with must_exist=false unmapped CColRefs pass
//		through unchanged, so DeriveOutputColumns is preserved (output-col
//		invariant holds). Fresh-rooted targets (Filter/Join) are returned as-is.
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprFreshRoot(CExpression *pexpr) const
{
	if (nullptr == pexpr || nullptr == pexpr->Pgexpr())
	{
		// already a freshly-built root (or NULL) — nothing to do.
		return pexpr;
	}

	// re-root via an identity remap (empty mapping => colrefs pass through).
	UlongToColRefMap *colref_mapping = GPOS_NEW(m_mp) UlongToColRefMap(m_mp);
	CExpression *pexprFresh = pexpr->PexprCopyWithRemappedColumns(
		m_mp, colref_mapping, false /*must_exist*/);
	colref_mapping->Release();
	pexpr->Release();
	return pexprFresh;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprInstantiate
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprInstantiate(const CDSLRule *prule,
								   const CDSLModel *pmodel)
{
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(nullptr != pmodel);

	BuildAliasMap(prule);
	CExpression *pexprTgt = PexprBuild(prule->PfragTgt()->PopRoot(), pmodel);

	// dedup drop: the source root was a redundant SELECT DISTINCT (pure-dedup
	// CLogicalGbAgg whose grouping cols form a key). PexprBuild produced the
	// resolved relational child (a bare Input target); wrap it in Select(child,
	// TRUE) to drop the GbAgg, exactly like ORCA's CXformSimplifyGbAgg::FDropGbAgg.
	// This keeps the memo group's output-column invariant: the trivial Select
	// outputs the child's columns (a superset of the GbAgg's grouping-only output),
	// which is the same substitution the native xform makes. The Select is a fresh
	// CExpression, so PexprFreshRoot returns it as-is (no remap needed).
	if (nullptr != pexprTgt && pmodel->FDedupDrop())
	{
		pexprTgt = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprTgt,
			CPredicateUtils::PexprConjunction(m_mp, nullptr));
	}
	return PexprFreshRoot(pexprTgt);
}

// EOF
