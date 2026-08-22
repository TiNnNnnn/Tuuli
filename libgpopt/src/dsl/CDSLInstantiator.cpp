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
#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalJoin.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CLogicalLeftSemiApply.h"
#include "gpopt/operators/CLogicalLeftSemiApplyIn.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CLogicalSetOp.h"
#include "gpopt/operators/CLogicalUnion.h"
#include "gpopt/operators/CLogicalUnionAll.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarAggFunc.h"
#include "gpopt/operators/CScalarConst.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarValuesList.h"

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

BOOL
FColSetContainsArray(const CColRefSet *pcrs,
					 const CColRefArray *pdrgpcr)
{
	for (ULONG ul = 0; ul < pdrgpcr->Size(); ul++)
	{
		if (!pcrs->FMember((*pdrgpcr)[ul]))
		{
			return false;
		}
	}
	return true;
}

BOOL
FContainsGbAgg(const CExpression *pexpr)
{
	if (COperator::EopLogicalGbAgg == pexpr->Pop()->Eopid())
	{
		return true;
	}
	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		if (FContainsGbAgg((*pexpr)[ul]))
		{
			return true;
		}
	}
	return false;
}

BOOL
FNullScalarConst(const CExpression *pexpr)
{
	return nullptr != pexpr && COperator::EopScalarConst == pexpr->Pop()->Eopid() &&
		CScalarConst::PopConvert(pexpr->Pop())->GetDatum()->IsNull();
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

// A set-op's output identities are anchored to its first input. Reordering
// branches therefore cannot merely reorder the operator's input-column arrays:
// later ORCA property derivation assumes every non-first input maps to distinct
// output CColRefs. Copy the moved child and remap its set-op columns into the
// identities required at the target position. Consumes pexpr.
CExpression *
PexprRemapSetOpChild(CMemoryPool *mp, CExpression *pexpr,
					 const CColRefArray *pdrgpcrFrom,
					 const CColRefArray *pdrgpcrTo)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != pdrgpcrFrom);
	GPOS_ASSERT(nullptr != pdrgpcrTo);
	GPOS_ASSERT(pdrgpcrFrom->Size() == pdrgpcrTo->Size());

	UlongToColRefMap *colref_mapping = GPOS_NEW(mp) UlongToColRefMap(mp);
	BOOL fNeedsRemap = false;
	for (ULONG ul = 0; ul < pdrgpcrFrom->Size(); ul++)
	{
		CColRef *pcrFrom = (*pdrgpcrFrom)[ul];
		CColRef *pcrTo = (*pdrgpcrTo)[ul];
		if (pcrFrom == pcrTo)
		{
			continue;
		}
		BOOL fInserted GPOS_ASSERTS_ONLY = colref_mapping->Insert(
			GPOS_NEW(mp) ULONG(pcrFrom->Id()), pcrTo);
		GPOS_ASSERT(fInserted);
		fNeedsRemap = true;
	}

	if (!fNeedsRemap)
	{
		colref_mapping->Release();
		return pexpr;
	}

	CExpression *pexprRemapped = pexpr->PexprCopyWithRemappedColumns(
		mp, colref_mapping, false /*must_exist*/);
	colref_mapping->Release();
	pexpr->Release();
	return pexprRemapped;
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

	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	if (nullptr == pdrgpsym || 2 != pdrgpsym->Size())
	{
		return nullptr;
	}
	const CDSLSymbol *psymLeft = PsymResolve((*pdrgpsym)[0]);
	const CDSLSymbol *psymRight = PsymResolve((*pdrgpsym)[1]);
	CExpression *pexprJoinPred =
		pmodel->PexprJoinPred(psymLeft, psymRight);
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
	if (1 != pop->UlChildren() || nullptr == pop->Pdrgpsym() ||
		2 != pop->Pdrgpsym()->Size())
	{
		return nullptr;
	}

	CExpression *pexprChild = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprChild)
	{
		return nullptr;
	}

	const CDSLSymbol *psymAttrs = PsymResolve((*pop->Pdrgpsym())[0]);
	const CDSLSymbol *psymSchema = PsymResolve((*pop->Pdrgpsym())[1]);

	// Proj* is ORCA's pure-dedup Global GbAgg. Unlike the special root-level
	// elimination rule, a Proj* nested in Union/Join must be rebuilt, not dropped.
	if (pop->FDistinct())
	{
		CColRefArray *pdrgpcrAttrs = pmodel->PdrgpcrAttrs(psymAttrs);
		CColRefArray *pdrgpcrSchema = pmodel->PdrgpcrSchema(psymSchema);
		if (nullptr == pdrgpcrAttrs || nullptr == pdrgpcrSchema ||
			0 == pdrgpcrSchema->Size() ||
			!FColArraysSameSet(m_mp, pdrgpcrAttrs, pdrgpcrSchema))
		{
			pexprChild->Release();
			return nullptr;
		}

		CColRefSet *pcrsChild = pexprChild->DeriveOutputColumns();
		CColRefSet *pcrsGrouping = GPOS_NEW(m_mp) CColRefSet(m_mp);
		pcrsGrouping->Include(pdrgpcrSchema);
		BOOL fValid = pcrsChild->ContainsAll(pcrsGrouping);
		pcrsGrouping->Release();
		if (!fValid)
		{
			pexprChild->Release();
			return nullptr;
		}

		pdrgpcrSchema->AddRef();
		CExpression *pexprEmptyList = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CScalarProjectList(m_mp),
			GPOS_NEW(m_mp) CExpressionArray(m_mp));
		return GPOS_NEW(m_mp) CExpression(
			m_mp,
			GPOS_NEW(m_mp) CLogicalGbAgg(
				m_mp, pdrgpcrSchema, COperator::EgbaggtypeGlobal),
			pexprChild, pexprEmptyList);
	}

	CExpression *pexprProjList = pmodel->PexprProjList(psymSchema);
	if (nullptr == pexprProjList)
	{
		// A Proj* source is represented by GbAgg and therefore has no matched
		// CScalarProjectList to reuse.  In the common corpus rule
		// Proj*(Input) -> Proj(Input), the target Proj is a pass-through view of
		// the same bound attrs/schema. ORCA has no column-pruning logical Project,
		// so return the child here; PexprInstantiate will apply the memo-safe
		// Select(child, TRUE) dedup-drop shell.
		CColRefArray *pdrgpcrAttrs = pmodel->PdrgpcrAttrs(psymAttrs);
		CColRefArray *pdrgpcrSchema = pmodel->PdrgpcrSchema(psymSchema);
		if (!pmodel->FDedupDrop() || nullptr == pdrgpcrAttrs ||
			nullptr == pdrgpcrSchema || 0 == pdrgpcrSchema->Size() ||
			!FColArraysSameSet(m_mp, pdrgpcrAttrs, pdrgpcrSchema))
		{
			pexprChild->Release();
			return nullptr;
		}

		CColRefSet *pcrsSchema = GPOS_NEW(m_mp) CColRefSet(m_mp);
		pcrsSchema->Include(pdrgpcrSchema);
		const BOOL fContains =
			pexprChild->DeriveOutputColumns()->ContainsAll(pcrsSchema);
		pcrsSchema->Release();
		if (!fContains)
		{
			pexprChild->Release();
			return nullptr;
		}
		return pexprChild;
	}

	if (!pexprChild->DeriveOutputColumns()->ContainsAll(
			pexprProjList->DeriveUsedColumns()))
	{
		pexprChild->Release();
		return nullptr;
	}

	// graft the matched project list (AddRef — the model keeps its own ref).
	pexprProjList->AddRef();
	return GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CLogicalProject(m_mp), pexprChild, pexprProjList);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildUnion
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildUnion(const CDSLOp *pop,
								  const CDSLModel *pmodel) const
{
	if (2 != pop->UlChildren() || nullptr == pop->Pdrgpsym() ||
		0 != pop->Pdrgpsym()->Size())
	{
		return nullptr;
	}

	CExpression *pexprLeft = PexprBuild((*pop)[0], pmodel);
	CExpression *pexprRight = PexprBuild((*pop)[1], pmodel);
	if (nullptr == pexprLeft || nullptr == pexprRight)
	{
		CRefCount::SafeRelease(pexprLeft);
		CRefCount::SafeRelease(pexprRight);
		return nullptr;
	}

	CExpression *rgpexprTarget[2] = {pexprLeft, pexprRight};
	CExpressionArray *pdrgpexprBindings = pmodel->PdrgpexprUnionBindings();
	CColRefArray *pdrgpcrOutput = nullptr;
	CColRefArray *rgpdrgpcrInput[2] = {nullptr, nullptr};
	ULONG rgulSourceForTarget[2] = {0, 1};

	for (ULONG ulBinding = 0;
		 nullptr != pdrgpexprBindings && ulBinding < pdrgpexprBindings->Size();
		 ulBinding++)
	{
		CExpression *pexprSource = (*pdrgpexprBindings)[ulBinding];
		if (2 != pexprSource->Arity())
		{
			continue;
		}
		CLogicalSetOp *popSource =
			CLogicalSetOp::PopConvert(pexprSource->Pop());
		CColRefArray *pdrgpcrCandidateOutput = popSource->PdrgpcrOutput();
		CColRef2dArray *pdrgpdrgpcrCandidateInput =
			popSource->PdrgpdrgpcrInput();
		if (2 != pdrgpdrgpcrCandidateInput->Size() ||
			0 == pdrgpcrCandidateOutput->Size())
		{
			continue;
		}

		BOOL rgfFits[2][2];
		for (ULONG ulTarget = 0; ulTarget < 2; ulTarget++)
		{
			CColRefSet *pcrsTarget =
				rgpexprTarget[ulTarget]->DeriveOutputColumns();
			for (ULONG ulSource = 0; ulSource < 2; ulSource++)
			{
				CColRefArray *pdrgpcrInput =
					(*pdrgpdrgpcrCandidateInput)[ulSource];
				rgfFits[ulTarget][ulSource] =
					pdrgpcrInput->Size() == pdrgpcrCandidateOutput->Size() &&
					FColSetContainsArray(pcrsTarget, pdrgpcrInput);
			}
		}

		if (rgfFits[0][0] && rgfFits[1][1])
		{
			pdrgpcrOutput = pdrgpcrCandidateOutput;
			rgpdrgpcrInput[0] = (*pdrgpdrgpcrCandidateInput)[0];
			rgpdrgpcrInput[1] = (*pdrgpdrgpcrCandidateInput)[1];
			rgulSourceForTarget[0] = 0;
			rgulSourceForTarget[1] = 1;
			break;
		}
		if (rgfFits[0][1] && rgfFits[1][0])
		{
			pdrgpcrOutput = pdrgpcrCandidateOutput;
			// Keep the set-op position maps stable and move the child semantics
			// into those identities below. Swapping these arrays directly can put
			// an output CColRef in a non-first input and violates ORCA invariants.
			rgpdrgpcrInput[0] = (*pdrgpdrgpcrCandidateInput)[0];
			rgpdrgpcrInput[1] = (*pdrgpdrgpcrCandidateInput)[1];
			rgulSourceForTarget[0] = 1;
			rgulSourceForTarget[1] = 0;
			break;
		}
	}

	if (nullptr == pdrgpcrOutput)
	{
		pexprLeft->Release();
		pexprRight->Release();
		return nullptr;
	}

	// Moving a memo-derived aggregate by deep column remapping recreates its
	// Local/Global split as fresh groups, losing the native xform provenance that
	// prevents incompatible aggregate xforms from running again. Identity-shaped
	// Union rules (including the real corpus Proj* rules) need no copy and remain
	// supported; conservatively reject only a branch move across a GbAgg subtree.
	for (ULONG ulTarget = 0; ulTarget < 2; ulTarget++)
	{
		if (rgulSourceForTarget[ulTarget] != ulTarget &&
			FContainsGbAgg(rgpexprTarget[ulTarget]))
		{
			pexprLeft->Release();
			pexprRight->Release();
			return nullptr;
		}
	}

	for (ULONG ulTarget = 0; ulTarget < 2; ulTarget++)
	{
		ULONG ulSource = rgulSourceForTarget[ulTarget];
		rgpexprTarget[ulTarget] = PexprRemapSetOpChild(
			m_mp, rgpexprTarget[ulTarget], rgpdrgpcrInput[ulSource],
			rgpdrgpcrInput[ulTarget]);
	}
	pexprLeft = rgpexprTarget[0];
	pexprRight = rgpexprTarget[1];

	pdrgpcrOutput->AddRef();
	CColRef2dArray *pdrgpdrgpcrInput =
		GPOS_NEW(m_mp) CColRef2dArray(m_mp, 2);
	for (ULONG ul = 0; ul < 2; ul++)
	{
		rgpdrgpcrInput[ul]->AddRef();
		pdrgpdrgpcrInput->Append(rgpdrgpcrInput[ul]);
	}
	CExpressionArray *pdrgpexprChildren =
		GPOS_NEW(m_mp) CExpressionArray(m_mp, 2);
	pdrgpexprChildren->Append(pexprLeft);
	pdrgpexprChildren->Append(pexprRight);

	COperator *popSet = pop->FDistinct()
		? static_cast<COperator *>(GPOS_NEW(m_mp) CLogicalUnion(
			  m_mp, pdrgpcrOutput, pdrgpdrgpcrInput))
		: static_cast<COperator *>(GPOS_NEW(m_mp) CLogicalUnionAll(
			  m_mp, pdrgpcrOutput, pdrgpdrgpcrInput));
	return GPOS_NEW(m_mp) CExpression(m_mp, popSet, pdrgpexprChildren);
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
//		CDSLInstantiator::PexprBuildInSub
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildInSub(const CDSLOp *pop,
								  const CDSLModel *pmodel) const
{
	if (2 != pop->UlChildren() || nullptr == pop->Pdrgpsym() ||
		1 != pop->Pdrgpsym()->Size())
	{
		return nullptr;
	}

	CExpression *pexprOuter = PexprBuild((*pop)[0], pmodel);
	CExpression *pexprInner = PexprBuild((*pop)[1], pmodel);
	const CDSLSymbol *psymAttrs = PsymResolve((*pop->Pdrgpsym())[0]);
	CExpression *pexprPred = pmodel->PexprInSubPred(psymAttrs);
	if (nullptr == pexprOuter || nullptr == pexprInner || nullptr == pexprPred)
	{
		CRefCount::SafeRelease(pexprOuter);
		CRefCount::SafeRelease(pexprInner);
		return nullptr;
	}

	// The saved comparison must reference exactly one column produced by the
	// rebuilt inner. This is the plain single-column IN shape WeTune models.
	CColRefSet *pcrsInnerUsed =
		GPOS_NEW(m_mp) CColRefSet(m_mp, *pexprPred->DeriveUsedColumns());
	pcrsInnerUsed->Intersection(pexprInner->DeriveOutputColumns());
	if (1 != pcrsInnerUsed->Size())
	{
		pcrsInnerUsed->Release();
		pexprOuter->Release();
		pexprInner->Release();
		return nullptr;
	}
	CColRef *pcrInner = pcrsInnerUsed->PcrFirst();
	pcrsInnerUsed->Release();

	pexprPred->AddRef();
	CExpression *pexprResult =
		CUtils::PexprLogicalApply<CLogicalLeftSemiApplyIn>(
			m_mp, pexprOuter, pexprInner, pcrInner,
			COperator::EopScalarSubqueryAny, pexprPred);

	CExpressionArray *pdrgpexprResidual =
		pmodel->PdrgpexprInSubResidual();
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
		CExpression *pexprResidual =
			CPredicateUtils::PexprConjunction(m_mp, pdrgpexprCopy);
		pexprResult = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprResult,
			pexprResidual);
	}
	return pexprResult;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PosBuildSort
//---------------------------------------------------------------------------
COrderSpec *
CDSLInstantiator::PosBuildSort(const CDSLOp *pop,
							   const CDSLModel *pmodel,
							   CExpression *pexprChild) const
{
	GPOS_ASSERT(nullptr != pop);
	GPOS_ASSERT(EdslopSort == pop->Edslop());
	if (nullptr == pop->Pdrgpsym() || 1 != pop->Pdrgpsym()->Size() ||
		(EdslsortAsc != pop->Edslsort() &&
		 EdslsortDesc != pop->Edslsort()))
	{
		return nullptr;
	}

	const CDSLSymbol *psymAttrs = PsymResolve((*pop->Pdrgpsym())[0]);
	CColRefArray *pdrgpcr = pmodel->PdrgpcrAttrs(psymAttrs);
	if (nullptr == pdrgpcr || 0 == pdrgpcr->Size())
	{
		return nullptr;
	}

	CColRefSet *pcrsOutput = pexprChild->DeriveOutputColumns();
	for (ULONG ul = 0; ul < pdrgpcr->Size(); ul++)
	{
		if (!pcrsOutput->FMember((*pdrgpcr)[ul]))
		{
			return nullptr;
		}
	}

	const IMDType::ECmpType ecmpt =
		(EdslsortAsc == pop->Edslsort()) ? IMDType::EcmptL
										 : IMDType::EcmptG;
	const COrderSpec::ENullTreatment ent =
		(EdslsortAsc == pop->Edslsort()) ? COrderSpec::EntLast
										 : COrderSpec::EntFirst;
	COrderSpec *pos = GPOS_NEW(m_mp) COrderSpec(m_mp);
	for (ULONG ul = 0; ul < pdrgpcr->Size(); ul++)
	{
		CColRef *pcr = (*pdrgpcr)[ul];
		IMDId *pmdid = pcr->RetrieveType()->GetMdidForCmpType(ecmpt);
		if (!IMDId::IsValid(pmdid))
		{
			pos->Release();
			return nullptr;
		}
		pmdid->AddRef();
		pos->Append(pmdid, pcr, ent);
	}
	return pos;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildSort
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildSort(const CDSLOp *pop,
								 const CDSLModel *pmodel) const
{
	if (1 != pop->UlChildren())
	{
		return nullptr;
	}
	CExpression *pexprChild = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprChild)
	{
		return nullptr;
	}
	COrderSpec *pos = PosBuildSort(pop, pmodel, pexprChild);
	if (nullptr == pos)
	{
		pexprChild->Release();
		return nullptr;
	}

	return GPOS_NEW(m_mp) CExpression(
		m_mp,
		GPOS_NEW(m_mp) CLogicalLimit(m_mp, pos, true /*global*/,
									 false /*has count*/, false /*top DML*/),
		pexprChild, CUtils::PexprScalarConstInt8(m_mp, 0),
		CUtils::PexprScalarConstInt8(m_mp, 0, true /*is null*/));
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildLimit
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildLimit(const CDSLOp *pop,
								  const CDSLModel *pmodel) const
{
	if (1 != pop->UlChildren() || nullptr == pop->Pdrgpsym() ||
		2 != pop->Pdrgpsym()->Size())
	{
		return nullptr;
	}

	CExpression *pexprCount =
		pmodel->PexprScalar(PsymResolve((*pop->Pdrgpsym())[0]));
	CExpression *pexprOffset =
		pmodel->PexprScalar(PsymResolve((*pop->Pdrgpsym())[1]));
	if (nullptr == pexprCount || nullptr == pexprOffset)
	{
		return nullptr;
	}

	const CDSLOp *popChild = (*pop)[0];
	const CDSLOp *popSort = nullptr;
	CExpression *pexprChild = nullptr;
	if (EdslopSort == popChild->Edslop())
	{
		popSort = popChild;
		if (1 != popSort->UlChildren())
		{
			return nullptr;
		}
		pexprChild = PexprBuild((*popSort)[0], pmodel);
	}
	else
	{
		pexprChild = PexprBuild(popChild, pmodel);
	}
	if (nullptr == pexprChild)
	{
		return nullptr;
	}

	COrderSpec *pos = nullptr;
	if (nullptr != popSort)
	{
		pos = PosBuildSort(popSort, pmodel, pexprChild);
	}
	else
	{
		pos = GPOS_NEW(m_mp) COrderSpec(m_mp);
	}
	if (nullptr == pos)
	{
		pexprChild->Release();
		return nullptr;
	}

	pexprOffset->AddRef();
	pexprCount->AddRef();
	return GPOS_NEW(m_mp) CExpression(
		m_mp,
		GPOS_NEW(m_mp) CLogicalLimit(m_mp, pos, true /*global*/,
									 !FNullScalarConst(pexprCount),
									 false /*top DML*/),
		pexprChild, pexprOffset, pexprCount);
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
		case EdslopInSubFilter:
			return PexprBuildInSub(pop, pmodel);
		case EdslopUnion:
			return PexprBuildUnion(pop, pmodel);
		case EdslopSort:
			return PexprBuildSort(pop, pmodel);
		case EdslopLimit:
			return PexprBuildLimit(pop, pmodel);
		case EdslopInnerJoin:
		case EdslopLeftJoin:
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

	// EXISTS/IN are represented before decorrelation as one conjunct of a
	// CLogicalSelect. Their matchers retain every sibling conjunct. When the
	// target keeps the same subquery operator, PexprBuildExists/InSub attaches
	// those residuals at the corresponding structural position. An eliminating
	// rule (for example InSubFilter(...) -> Input<...>) has no target-side
	// builder at which to do that, so restore the source Select shell here.
	const EDslOpKind edslopSrc = popSrcRoot->Edslop();
	const EDslOpKind edslopTgt = popTgtRoot->Edslop();
	CExpressionArray *pdrgpexprResidual = nullptr;
	if (EdslopExists == edslopSrc && EdslopExists != edslopTgt)
	{
		pdrgpexprResidual = pmodel->PdrgpexprExistsResidual();
	}
	else if (EdslopInSubFilter == edslopSrc &&
			 EdslopInSubFilter != edslopTgt)
	{
		pdrgpexprResidual = pmodel->PdrgpexprInSubResidual();
	}
	if (nullptr != pexprTgt && nullptr != pdrgpexprResidual &&
		0 < pdrgpexprResidual->Size())
	{
		CExpressionArray *pdrgpexprCopy =
			GPOS_NEW(m_mp) CExpressionArray(m_mp);
		for (ULONG ul = 0; ul < pdrgpexprResidual->Size(); ul++)
		{
			CExpression *pexprConj = (*pdrgpexprResidual)[ul];
			pexprConj->AddRef();
			pdrgpexprCopy->Append(pexprConj);
		}
		pexprTgt = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprTgt,
			CPredicateUtils::PexprConjunction(m_mp, pdrgpexprCopy));
	}

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
	return PexprFreshRoot(pexprTgt);
}

// EOF
