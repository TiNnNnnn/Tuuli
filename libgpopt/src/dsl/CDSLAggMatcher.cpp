//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLAggMatcher.cpp
//
//	@doc:
//		Implementation of the Proj* dedup and real aggregate symbol binders.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLAggMatcher.h"

#include "gpos/base.h"

#include "gpopt/base/CKeyCollection.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/operators/CLogicalGbAgg.h"
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
FCompensationChainEndsInLeftApply(const CDSLOp *pop)
{
	while (nullptr != pop &&
		   (EdslopProj == pop->Edslop() || EdslopAgg == pop->Edslop()) &&
		   1 == pop->UlChildren())
	{
		pop = (*pop)[0];
	}
	return nullptr != pop && EdslopLeftOuterApply == pop->Edslop();
}
}  // namespace

//---------------------------------------------------------------------------
//	@function:
//		CDSLAggMatcher::PdrgpcrGrouping
//
//	@doc:
//		Copy the GbAgg's grouping columns into a fresh ordered array. Caller owns
//		the returned ref.
//---------------------------------------------------------------------------
CColRefArray *
CDSLAggMatcher::PdrgpcrGrouping(CExpression *pexprAgg) const
{
	CLogicalGbAgg *popGbAgg = CLogicalGbAgg::PopConvert(pexprAgg->Pop());
	CColRefArray *pdrgpcrGrp = popGbAgg->Pdrgpcr();

	CColRefArray *pdrgpcr = GPOS_NEW(m_mp) CColRefArray(m_mp);
	const ULONG ulCols = (nullptr == pdrgpcrGrp) ? 0 : pdrgpcrGrp->Size();
	for (ULONG ul = 0; ul < ulCols; ul++)
	{
		pdrgpcr->Append((*pdrgpcrGrp)[ul]);
	}
	return pdrgpcr;
}

BOOL
CDSLAggMatcher::FMatchDistinctAggDedup(const CDSLOp *popAgg,
									 CExpression *pexprAgg,
									 CDSLModel *pmodel) const
{
	CLogicalGbAgg *popGbAgg = CLogicalGbAgg::PopConvert(pexprAgg->Pop());
	CExpression *pexprAggList = (*pexprAgg)[1];
	if (COperator::EgbaggtypeGlobal != popGbAgg->Egbaggtype() ||
		nullptr != popGbAgg->PdrgpcrMinimal() ||
		COperator::EopScalarProjectList != pexprAggList->Pop()->Eopid() ||
		0 == pexprAggList->Arity() || 1 != popAgg->UlChildren())
	{
		return false;
	}

	// WeTune inserts one deduplicated projection below Agg when the query has a
	// DISTINCT aggregate. One Proj* cannot describe independent DQA domains, so
	// multiple DQAs are accepted only when they reference the same column set.
	CColRefSet *pcrsDistinctArgs = nullptr;
	BOOL fValid = true;
	for (ULONG ul = 0; ul < pexprAggList->Arity() && fValid; ul++)
	{
		CExpression *pexprPrEl = (*pexprAggList)[ul];
		if (COperator::EopScalarProjectElement != pexprPrEl->Pop()->Eopid() ||
			1 != pexprPrEl->Arity() ||
			COperator::EopScalarAggFunc != (*pexprPrEl)[0]->Pop()->Eopid())
		{
			fValid = false;
			break;
		}
		CExpression *pexprFunc = (*pexprPrEl)[0];
		CScalarAggFunc *popFunc =
			CScalarAggFunc::PopConvert(pexprFunc->Pop());
		if (!popFunc->IsDistinct())
		{
			continue;
		}

		CColRefSet *pcrsUsed = pexprFunc->DeriveUsedColumns();
		if (nullptr == pcrsDistinctArgs)
		{
			pcrsDistinctArgs = GPOS_NEW(m_mp) CColRefSet(m_mp);
			pcrsDistinctArgs->Include(pcrsUsed);
		}
		else if (!pcrsDistinctArgs->Equals(pcrsUsed))
		{
			fValid = false;
		}
	}
	if (!fValid || nullptr == pcrsDistinctArgs)
	{
		CRefCount::SafeRelease(pcrsDistinctArgs);
		return false;
	}

	// The virtual Proj* key is (grouping columns, DISTINCT arguments), exactly
	// the tuple domain deduplicated within each group. The rule's ordinary
	// Unique(t,a) constraint remains responsible for proving redundancy.
	CColRefArray *pdrgpcrAttrs = PdrgpcrGrouping(pexprAgg);
	CColRefSet *pcrsAttrs = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsAttrs->Include(pdrgpcrAttrs);
	CColRefSetIter crsi(*pcrsDistinctArgs);
	while (crsi.Advance())
	{
		CColRef *pcr = crsi.Pcr();
		if (!pcrsAttrs->FMember(pcr))
		{
			pdrgpcrAttrs->Append(pcr);
			pcrsAttrs->Include(pcr);
		}
	}
	pcrsAttrs->Release();
	pcrsDistinctArgs->Release();

	CColRefSet *pcrsVirtual = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsVirtual->Include(pdrgpcrAttrs);
	BOOL fAttrsValid = 0 < pdrgpcrAttrs->Size() &&
					   (*pexprAgg)[0]->DeriveOutputColumns()->ContainsAll(
						   pcrsVirtual);
	pcrsVirtual->Release();
	CDSLSymbolArray *pdrgpsym = popAgg->Pdrgpsym();
	if (!fAttrsValid || nullptr == pdrgpsym || 2 != pdrgpsym->Size())
	{
		pdrgpcrAttrs->Release();
		return false;
	}

	BOOL fBound = pmodel->FBind((*pdrgpsym)[0], pdrgpcrAttrs) &&
				  pmodel->FBind((*pdrgpsym)[1], pdrgpcrAttrs);
	pdrgpcrAttrs->Release();
	return fBound &&
		   m_pmatcher->FMatch((*popAgg)[0], (*pexprAgg)[0], pmodel) &&
		   pmodel->FSetDistinctAgg(pexprAgg);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLAggMatcher::FMatch
//---------------------------------------------------------------------------
BOOL
CDSLAggMatcher::FMatchDedup(const CDSLOp *popAgg, CExpression *pexprAgg,
						CDSLModel *pmodel) const
{
	// pure-dedup gate: reject any GbAgg that computes aggregate functions (a
	// non-empty project list). Mirrors CXformSimplifyGbAgg::FDropGbAgg's
	// `if (0 < pexprProjectList->Arity()) return false;`; real Agg templates are
	// handled separately by FMatchAggregate.
	if (0 != (*pexprAgg)[1]->Arity())
	{
		return FMatchDistinctAggDedup(popAgg, pexprAgg, pmodel);
	}

	// A source-root Proj* is an operator-eliminating rule. Keep that case on the
	// ORIGINAL user-level global dedup, exactly like native
	// CXformSimplifyGbAgg::Exfp: a split/DSL-generated aggregate carries
	// PdrgpcrMinimal and dropping it at its own memo group is invalid.
	//
	// A nested Proj*, however, is consumed as part of a larger source such as
	// Proj(Proj*). The larger xform replaces the outer group and reconstructs the
	// target dedup, so a complete Global dedup remains a faithful Proj* view even
	// when it carries minimal-grouping provenance. Allowing it here is what lets
	// one DSL alternative feed a later structural rule without weakening the
	// dangerous root-level drop gate. Local/intermediate stages remain rejected.
	CLogicalGbAgg *popGbAgg = CLogicalGbAgg::PopConvert(pexprAgg->Pop());
	const CDSLRule *prule = m_pmatcher->Prule();
	const BOOL fSourceRoot =
		nullptr == prule || popAgg == prule->PfragSrc()->PopRoot();
	if (COperator::EgbaggtypeGlobal != popGbAgg->Egbaggtype() ||
		(fSourceRoot && nullptr != popGbAgg->PdrgpcrMinimal()) ||
		nullptr == popGbAgg->Pdrgpcr() || 0 == popGbAgg->Pdrgpcr()->Size())
	{
		return false;
	}

	// Proj*<a s>: both attrs and schema are the grouping columns.
	CDSLSymbolArray *pdrgpsym = popAgg->Pdrgpsym();
	if (nullptr == pdrgpsym || 2 != pdrgpsym->Size())
	{
		return false;
	}

	// bind <a> and <s> to the grouping columns — for a pure dedup the referenced,
	// output and grouping columns coincide. FBind AddRefs; release the local copy.
	CColRefArray *pdrgpcrGrp = PdrgpcrGrouping(pexprAgg);
	BOOL fBound = pmodel->FBind((*pdrgpsym)[0], pdrgpcrGrp) &&
				  pmodel->FBind((*pdrgpsym)[1], pdrgpcrGrp);
	pdrgpcrGrp->Release();
	if (!fBound)
	{
		return false;
	}

	// the relational child recurses through the generic matcher (GbAgg has exactly
	// one relational child; the template's UlChildren() is 1).
	if (1 != popAgg->UlChildren())
	{
		return false;
	}
	if (!m_pmatcher->FMatch((*popAgg)[0], (*pexprAgg)[0], pmodel))
	{
		return false;
	}

	// flag the source root as a redundant dedup GbAgg: the instantiator drops it
	// by wrapping the resolved child in Select(child, TRUE), mirroring
	// CXformSimplifyGbAgg::FDropGbAgg (the memo-safe operator-drop idiom).
	pmodel->SetDedupDrop();
	return true;
}

BOOL
CDSLAggMatcher::FMatchDroppedDedup(const CDSLOp *popAgg,
								 CExpression *pexprMarker,
								 CDSLModel *pmodel) const
{
	const CDSLRule *prule = m_pmatcher->Prule();
	if (nullptr == prule || popAgg == prule->PfragSrc()->PopRoot() ||
		1 != popAgg->UlChildren())
	{
		return false;
	}

	CExpression *pexprChild = nullptr;
	if (!CDSLMatchView::FDroppedDedupIdentity(pexprMarker, &pexprChild))
	{
		return false;
	}

	CDSLSymbolArray *pdrgpsym = popAgg->Pdrgpsym();
	if (nullptr == pdrgpsym || 2 != pdrgpsym->Size())
	{
		return false;
	}

	CColRefArray *pdrgpcrOutput =
		pexprChild->DeriveOutputColumns()->Pdrgpcr(m_mp);
	BOOL fBound = pmodel->FBind((*pdrgpsym)[0], pdrgpcrOutput) &&
		pmodel->FBind((*pdrgpsym)[1], pdrgpcrOutput);
	pdrgpcrOutput->Release();
	return fBound &&
		m_pmatcher->FMatch((*popAgg)[0], pexprChild, pmodel);
}

BOOL
CDSLAggMatcher::FMatchKeyedIdentity(const CDSLOp *popAgg,
							 CExpression *pexpr,
							 CDSLModel *pmodel) const
{
	const CDSLRule *prule = m_pmatcher->Prule();
	if (nullptr == prule || popAgg != prule->PfragSrc()->PopRoot() ||
		1 != popAgg->UlChildren() ||
		COperator::EopLogicalLeftSemiJoin != pexpr->Pop()->Eopid())
	{
		return false;
	}

	CColRefSet *pcrsOutput = pexpr->DeriveOutputColumns();
	CKeyCollection *pkc = pexpr->DeriveKeyCollection();
	if (nullptr == pkc || 0 == pcrsOutput->Size() ||
		!pkc->FKey(pcrsOutput, false /*fExactMatch*/))
	{
		return false;
	}

	CDSLSymbolArray *pdrgpsym = popAgg->Pdrgpsym();
	if (nullptr == pdrgpsym || 2 != pdrgpsym->Size())
	{
		return false;
	}
	CColRefArray *pdrgpcrOutput = pcrsOutput->Pdrgpcr(m_mp);
	BOOL fMatched = pmodel->FBind((*pdrgpsym)[0], pdrgpcrOutput) &&
		pmodel->FBind((*pdrgpsym)[1], pdrgpcrOutput) &&
		m_pmatcher->FMatch((*popAgg)[0], pexpr, pmodel);
	pdrgpcrOutput->Release();
	if (!fMatched)
	{
		return false;
	}

	// There is no concrete GbAgg/project list to restore on the target side.
	// Mark the schema as an identity projection carrier, just like the ordinary
	// projection matcher does for ORCA's projection-free memo representation.
	pexpr->AddRef();
	return pmodel->FSetVirtualIdentityProj((*pdrgpsym)[1], pexpr);
}

BOOL
CDSLAggMatcher::FMatchAggregate(const CDSLOp *popAgg,
								 CExpression *pexprAgg,
								 CExpression *pexprHaving,
								 CDSLModel *pmodel) const
{
	CLogicalGbAgg *popGbAgg = CLogicalGbAgg::PopConvert(pexprAgg->Pop());
	CExpression *pexprAggList = (*pexprAgg)[1];
	if (COperator::EgbaggtypeGlobal != popGbAgg->Egbaggtype() ||
		COperator::EopScalarProjectList != pexprAggList->Pop()->Eopid() ||
		0 == pexprAggList->Arity())
	{
		return false;
	}
	// PdrgpcrMinimal is optimizer metadata derived for this particular child,
	// not part of the logical aggregate represented by the DSL.  It must not
	// prevent a real Agg from matching.  Instantiation preserves the full
	// grouping columns and intentionally does not copy child-dependent minimal
	// grouping metadata across a rewrite.

	CDSLSymbolArray *pdrgpsym = popAgg->Pdrgpsym();
	if (nullptr == pdrgpsym ||
		(5 != pdrgpsym->Size() && 6 != pdrgpsym->Size()) ||
		1 != popAgg->UlChildren())
	{
		return false;
	}
	const BOOL fLegacy = 5 == pdrgpsym->Size();
	const ULONG ulFunc = fLegacy ? 2 : 3;
	const ULONG ulSchema = fLegacy ? 3 : 4;
	const ULONG ulHaving = fLegacy ? 4 : 5;

	CExpressionArray *pdrgpexprFuncs =
		GPOS_NEW(m_mp) CExpressionArray(m_mp);
	CColRefArray *pdrgpcrAggOut = GPOS_NEW(m_mp) CColRefArray(m_mp);
	CColRefSet *pcrsAggInputs = GPOS_NEW(m_mp) CColRefSet(m_mp);
	BOOL fValid = true;

	for (ULONG ul = 0; ul < pexprAggList->Arity() && fValid; ul++)
	{
		CExpression *pexprPrEl = (*pexprAggList)[ul];
		if (COperator::EopScalarProjectElement != pexprPrEl->Pop()->Eopid() ||
			1 != pexprPrEl->Arity())
		{
			fValid = false;
			break;
		}

		CExpression *pexprFunc = (*pexprPrEl)[0];
		if (COperator::EopScalarAggFunc != pexprFunc->Pop()->Eopid() ||
			!FAggFuncMatches(
				m_mp, popAgg,
				CScalarAggFunc::PopConvert(pexprFunc->Pop())))
		{
			fValid = false;
			break;
		}

		pexprFunc->AddRef();
		pdrgpexprFuncs->Append(pexprFunc);
		pdrgpcrAggOut->Append(
			CScalarProjectElement::PopConvert(pexprPrEl->Pop())->Pcr());
		pcrsAggInputs->Include(pexprFunc->DeriveUsedColumns());
	}

	if (!fValid)
	{
		pdrgpexprFuncs->Release();
		pdrgpcrAggOut->Release();
		pcrsAggInputs->Release();
		return false;
	}

	CColRefArray *pdrgpcrGroup = PdrgpcrGrouping(pexprAgg);
	CColRefArray *pdrgpcrAggInputs = pcrsAggInputs->Pdrgpcr(m_mp);
	CColRefArray *pdrgpcrSchema = GPOS_NEW(m_mp) CColRefArray(m_mp);
	for (ULONG ul = 0; ul < pdrgpcrGroup->Size(); ul++)
	{
		pdrgpcrSchema->Append((*pdrgpcrGroup)[ul]);
	}
	for (ULONG ul = 0; ul < pdrgpcrAggOut->Size(); ul++)
	{
		pdrgpcrSchema->Append((*pdrgpcrAggOut)[ul]);
	}

	// ORCA represents HAVING as Select(GbAgg, predicate). A predicate may use
	// grouping columns and/or aggregate outputs, but no column outside the Agg
	// schema. Missing HAVING is the WeTune concrete predicate TRUE.
	BOOL fOwnHaving = false;
	if (nullptr == pexprHaving)
	{
		fOwnHaving = true;
		pexprHaving = CUtils::PexprScalarConstBool(m_mp, true);
	}
	CColRefSet *pcrsSchema = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsSchema->Include(pdrgpcrSchema);
	BOOL fHavingValid =
		pcrsSchema->ContainsAll(pexprHaving->DeriveUsedColumns());
	pcrsSchema->Release();
	BOOL fBound = pmodel->FBind((*pdrgpsym)[0], pdrgpcrGroup) &&
				  pmodel->FBind((*pdrgpsym)[1], pdrgpcrAggInputs);
	if (fBound && !fLegacy)
	{
		fBound = pmodel->FBind((*pdrgpsym)[2], pdrgpcrAggOut);
	}
	fBound = fHavingValid && fBound &&
			 pmodel->FBind((*pdrgpsym)[ulFunc], pdrgpexprFuncs) &&
			 pmodel->FBind((*pdrgpsym)[ulSchema], pdrgpcrSchema) &&
			 pmodel->FBind((*pdrgpsym)[ulHaving], pexprHaving);
	if (fBound)
	{
		pexprAgg->AddRef();
		fBound = pmodel->FSetAggBinding(
			(*pdrgpsym)[ulSchema], pexprAgg);
	}

	pdrgpcrGroup->Release();
	pdrgpcrAggInputs->Release();
	pdrgpcrAggOut->Release();
	pdrgpexprFuncs->Release();
	pdrgpcrSchema->Release();
	pcrsAggInputs->Release();
	if (fOwnHaving)
	{
		pexprHaving->Release();
	}

	return fBound &&
		   m_pmatcher->FMatch((*popAgg)[0], (*pexprAgg)[0], pmodel);
}

BOOL
CDSLAggMatcher::FMatch(const CDSLOp *popAgg, CExpression *pexprAgg,
					   CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != popAgg);
	GPOS_ASSERT(EdslopProj == popAgg->Edslop() ||
				EdslopAgg == popAgg->Edslop());
	GPOS_ASSERT(nullptr != pexprAgg);
	if (EdslopAgg == popAgg->Edslop() &&
		COperator::EopLogicalGbAgg == pexprAgg->Pop()->Eopid() &&
		2 == pexprAgg->Arity() && (*pexprAgg)[1]->DeriveHasSubquery() &&
		1 == popAgg->UlChildren() &&
		FCompensationChainEndsInLeftApply((*popAgg)[0]))
	{
		const BOOL fSourceNamesDirectApply =
			EdslopLeftOuterApply == (*popAgg)[0]->Edslop();
		CExpression *pexprLowered = CDSLMatchView::PexprLowerSubqueries(
			m_mp, pexprAgg, false /*fEnforceCorrelatedApply*/,
			false /*fScalarOnly*/);
		const COperator::EOperatorId eopidLoweredChild =
			(nullptr != pexprLowered &&
			 COperator::EopLogicalGbAgg == pexprLowered->Pop()->Eopid() &&
			 2 == pexprLowered->Arity())
				? (*pexprLowered)[0]->Pop()->Eopid()
				: COperator::EopSentinel;
		const BOOL fDirectLeftApply =
			COperator::EopLogicalLeftOuterApply == eopidLoweredChild ||
			COperator::EopLogicalLeftOuterCorrelatedApply ==
				eopidLoweredChild;
		if (nullptr != pexprLowered && fSourceNamesDirectApply &&
			!fDirectLeftApply)
		{
			// Count-zero and quantified value semantics may add a compensating
			// Project in the regular alternative. A direct-Apply source explicitly
			// selects the production handler's correlated alternative; a
			// Proj...(LeftApply) source retains and matches the regular alternative.
			pexprLowered->Release();
			pexprLowered = CDSLMatchView::PexprLowerSubqueries(
				m_mp, pexprAgg, true /*fEnforceCorrelatedApply*/,
				false /*fScalarOnly*/);
		}
		if (nullptr != pexprLowered)
		{
			const BOOL fMatched = FMatch(popAgg, pexprLowered, pmodel);
			pexprLowered->Release();
			return fMatched;
		}
	}
	if (EdslopProj == popAgg->Edslop() && popAgg->FDistinct() &&
		FMatchDroppedDedup(popAgg, pexprAgg, pmodel))
	{
		return true;
	}
	if (EdslopProj == popAgg->Edslop() && popAgg->FDistinct() &&
		FMatchKeyedIdentity(popAgg, pexprAgg, pmodel))
	{
		return true;
	}

	CDSLMatchView::SAggregate view;
	if (!CDSLMatchView::FAggregate(
			pexprAgg, EdslopAgg == popAgg->Edslop(), &view))
	{
		return false;
	}

	if (EdslopProj == popAgg->Edslop())
	{
		return popAgg->FDistinct() &&
			   FMatchDedup(popAgg, view.m_pexprAgg, pmodel);
	}
	return FMatchAggregate(popAgg, view.m_pexprAgg, view.m_pexprHaving,
						   pmodel);
}

// EOF
