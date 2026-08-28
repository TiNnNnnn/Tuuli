//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLInSubMatcher.cpp
//--------------------------------------------------------------------------
#include "gpopt/dsl/CDSLInSubMatcher.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarSubqueryAny.h"
#include "gpopt/operators/CScalarSubqueryExists.h"

#include <vector>

using namespace gpopt;

namespace
{
BOOL
FPredicateRejectsNull(CMemoryPool *mp, CExpressionArray *pdrgpexpr,
					  CColRef *pcr)
{
	CColRefSet *pcrs = GPOS_NEW(mp) CColRefSet(mp);
	pcrs->Include(pcr);
	BOOL fRejects = false;
	for (ULONG ul = 0; ul < pdrgpexpr->Size() && !fRejects; ul++)
	{
		CExpression *pexprPred = (*pdrgpexpr)[ul];
		fRejects = pexprPred->DeriveUsedColumns()->FMember(pcr) &&
				   CPredicateUtils::FNullRejecting(mp, pexprPred, pcrs);
	}
	pcrs->Release();
	return fRejects;
}

// Build Select(rel, conjunction) and consume the conjunct array. A synthesized
// IS NOT NULL predicate is added only when the existing predicates do not
// already reject NULL for pcrRequiredNotNull.
CExpression *
PexprSelectWithNotNull(CMemoryPool *mp, CExpression *pexprRel,
					   CExpressionArray *pdrgpexprConj,
					   CColRef *pcrRequiredNotNull)
{
	if (!pexprRel->DeriveNotNullColumns()->FMember(pcrRequiredNotNull) &&
		!FPredicateRejectsNull(mp, pdrgpexprConj, pcrRequiredNotNull))
	{
		pdrgpexprConj->Append(CUtils::PexprIsNotNull(
			mp, CUtils::PexprScalarIdent(mp, pcrRequiredNotNull)));
	}
	if (0 == pdrgpexprConj->Size())
	{
		pdrgpexprConj->Release();
		pexprRel->AddRef();
		return pexprRel;
	}
	pexprRel->AddRef();
	return GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), pexprRel,
		CPredicateUtils::PexprConjunction(mp, pdrgpexprConj));
}
}  // namespace

BOOL
CDSLInSubMatcher::FBindOuterAttrs(const CDSLOp *pop,
								 CExpression *pexprScalar,
								 CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	if (nullptr == pdrgpsym || 1 != pdrgpsym->Size())
	{
		return false;
	}

	CColRefArray *pdrgpcr = pexprScalar->DeriveUsedColumns()->Pdrgpcr(m_mp);
	BOOL fBound = 0 < pdrgpcr->Size() &&
				  pmodel->FBind((*pdrgpsym)[0], pdrgpcr);
	pdrgpcr->Release();
	return fBound;
}

BOOL
CDSLInSubMatcher::FMatchInner(const CDSLOp *popInner,
							 CExpression *pexprInner,
							 const CColRef *pcrProjected,
							 CDSLModel *pmodel) const

{
	CColRefArray *pdrgpcrProjected = GPOS_NEW(m_mp) CColRefArray(m_mp);
	if (nullptr != pcrProjected)
	{
		pdrgpcrProjected->Append(const_cast<CColRef *>(pcrProjected));
	}
	BOOL fMatched =
		FMatchInner(popInner, pexprInner, pdrgpcrProjected, pmodel);
	pdrgpcrProjected->Release();
	return fMatched;
}

BOOL
CDSLInSubMatcher::FMatchInner(const CDSLOp *popInner,
							 CExpression *pexprInner,
							 CColRefArray *pdrgpcrProjected,
							 CDSLModel *pmodel) const
{
	// PostgreSQL's ORCA translator removes a pass-through SELECT-list
	// projection from a scalar IN subquery and stores its selected column
	// on the subquery/Apply operator. WeTune still exposes that SQL node as
	// Proj<a s>(child). Treat this representation difference as a transparent
	// projection in both pre- and post-Apply matching. Computed projects remain
	// CLogicalProject nodes.
	if (EdslopProj == popInner->Edslop() &&
		1 == popInner->UlChildren() && nullptr != popInner->Pdrgpsym() &&
		2 == popInner->Pdrgpsym()->Size() &&
		nullptr != pdrgpcrProjected && 0 < pdrgpcrProjected->Size())
	{
		CExpression *pexprRel = pexprInner;
		while (COperator::EopLogicalProject == pexprRel->Pop()->Eopid() &&
			   2 == pexprRel->Arity())
		{
			CColRefSet *pcrsProjected = GPOS_NEW(m_mp) CColRefSet(m_mp);
			pcrsProjected->Include(pdrgpcrProjected);
			const BOOL fKeysFromChild =
				(*pexprRel)[0]->DeriveOutputColumns()->ContainsAll(pcrsProjected);
			pcrsProjected->Release();
			if (!fKeysFromChild)
			{
				break;
			}
			// EXISTS target lists and pass-through IN projections do not define
			// the comparison keys. They are absent from WeTune's canonical
			// Proj(key) view, so continue at the relational child. A genuinely
			// computed key is not available below and therefore remains a real
			// Project handled by the ordinary matcher.
			pexprRel = (*pexprRel)[0];
		}
		if (COperator::EopLogicalProject == pexprRel->Pop()->Eopid())
		{
			return m_pmatcher->FMatch(popInner, pexprInner, pmodel);
		}
		BOOL fMatched =
			pmodel->FBind((*popInner->Pdrgpsym())[0], pdrgpcrProjected) &&
			pmodel->FBind((*popInner->Pdrgpsym())[1], pdrgpcrProjected) &&
			m_pmatcher->FMatch((*popInner)[0], pexprRel, pmodel);
		return fMatched;
	}

	return m_pmatcher->FMatch(popInner, pexprInner, pmodel);
}

CExpression *
CDSLInSubMatcher::PexprComparison(CExpression *pexprAny) const
{
	CScalarSubqueryAny *popAny =
		CScalarSubqueryAny::PopConvert(pexprAny->Pop());
	CExpression *pexprOuterScalar = (*pexprAny)[1];
	pexprOuterScalar->AddRef();
	IMDId *pmdidOp = popAny->MdIdOp();
	pmdidOp->AddRef();
	return CUtils::PexprScalarCmp(m_mp, pexprOuterScalar, popAny->Pcr(),
								 *popAny->PstrOp(), pmdidOp);
}

BOOL
CDSLInSubMatcher::FMatchCorrelatedExists(const CDSLOp *pop,
							  CExpression *pexpr,
							  CDSLModel *pmodel) const
{
	CExpressionArray *pdrgpexprOuterConj =
		CPredicateUtils::PdrgpexprConjuncts(m_mp, (*pexpr)[1]);
	CExpression *pexprExists = nullptr;
	ULONG ulExists = 0;
	for (ULONG ul = 0; ul < pdrgpexprOuterConj->Size(); ul++)
	{
		if (CDSLMatchView::FDirectExists((*pdrgpexprOuterConj)[ul]))
		{
			pexprExists = (*pdrgpexprOuterConj)[ul];
			ulExists++;
		}
	}
	if (1 != ulExists)
	{
		pdrgpexprOuterConj->Release();
		return false;
	}

	// EXISTS ignores its SELECT list. ORCA may retain that list as one or more
	// LogicalProject shells (for example SELECT 1) or fold a pass-through list
	// away entirely. WeTune's EXISTS-to-InSub canonicalization projects the
	// correlation key instead, so peel these semantically irrelevant shells and
	// recover the correlated Select below them.
	CExpression *pexprInnerSelect = (*pexprExists)[0];
	while (COperator::EopLogicalProject ==
			   pexprInnerSelect->Pop()->Eopid() &&
		   2 == pexprInnerSelect->Arity())
	{
		pexprInnerSelect = (*pexprInnerSelect)[0];
	}
	if (COperator::EopLogicalSelect !=
			pexprInnerSelect->Pop()->Eopid() ||
		2 != pexprInnerSelect->Arity())
	{
		pdrgpexprOuterConj->Release();
		return false;
	}

	CExpression *pexprOuterRel = (*pexpr)[0];
	CExpression *pexprInnerRel = (*pexprInnerSelect)[0];
	CColRefSet *pcrsOuter = pexprOuterRel->DeriveOutputColumns();
	CColRefSet *pcrsInner = pexprInnerRel->DeriveOutputColumns();
	CExpressionArray *pdrgpexprInnerConj =
		CPredicateUtils::PdrgpexprConjuncts(m_mp, (*pexprInnerSelect)[1]);
	CExpression *pexprCorrelation = nullptr;
	CColRef *pcrOuter = nullptr;
	CColRef *pcrInner = nullptr;
	ULONG ulCorrelation = 0;
	for (ULONG ul = 0; ul < pdrgpexprInnerConj->Size(); ul++)
	{
		CExpression *pexprConj = (*pdrgpexprInnerConj)[ul];
		if (2 != pexprConj->Arity() ||
			!CPredicateUtils::FPlainEquality(pexprConj) ||
			COperator::EopScalarIdent != (*pexprConj)[0]->Pop()->Eopid() ||
			COperator::EopScalarIdent != (*pexprConj)[1]->Pop()->Eopid())
		{
			continue;
		}
		CColRef *pcr0 = const_cast<CColRef *>(
			CScalarIdent::PopConvert((*pexprConj)[0]->Pop())->Pcr());
		CColRef *pcr1 = const_cast<CColRef *>(
			CScalarIdent::PopConvert((*pexprConj)[1]->Pop())->Pcr());
		if (pcrsOuter->FMember(pcr0) && pcrsInner->FMember(pcr1))
		{
			pexprCorrelation = pexprConj;
			pcrOuter = pcr0;
			pcrInner = pcr1;
			ulCorrelation++;
		}
		else if (pcrsOuter->FMember(pcr1) && pcrsInner->FMember(pcr0))
		{
			pexprCorrelation = pexprConj;
			pcrOuter = pcr1;
			pcrInner = pcr0;
			ulCorrelation++;
		}
	}
	if (1 != ulCorrelation)
	{
		pdrgpexprOuterConj->Release();
		pdrgpexprInnerConj->Release();
		return false;
	}

	// Canonical InSub inputs exclude the correlation equality. Preserve every
	// other predicate and make the equality's NULL rejection explicit on both
	// sides. These filters become part of the Input bindings, so any target that
	// removes InSub still retains the necessary SQL semantics.
	CExpressionArray *pdrgpexprOuterResidual =
		GPOS_NEW(m_mp) CExpressionArray(m_mp);
	for (ULONG ul = 0; ul < pdrgpexprOuterConj->Size(); ul++)
	{
		CExpression *pexprConj = (*pdrgpexprOuterConj)[ul];
		if (pexprConj != pexprExists)
		{
			pexprConj->AddRef();
			pdrgpexprOuterResidual->Append(pexprConj);
		}
	}
	CExpressionArray *pdrgpexprInnerResidual =
		GPOS_NEW(m_mp) CExpressionArray(m_mp);
	for (ULONG ul = 0; ul < pdrgpexprInnerConj->Size(); ul++)
	{
		CExpression *pexprConj = (*pdrgpexprInnerConj)[ul];
		if (pexprConj != pexprCorrelation)
		{
			pexprConj->AddRef();
			pdrgpexprInnerResidual->Append(pexprConj);
		}
	}
	CExpression *pexprOuterInput = PexprSelectWithNotNull(
		m_mp, pexprOuterRel, pdrgpexprOuterResidual, pcrOuter);
	CExpression *pexprInnerInput = PexprSelectWithNotNull(
		m_mp, pexprInnerRel, pdrgpexprInnerResidual, pcrInner);

	CColRefArray *pdrgpcrOuter = GPOS_NEW(m_mp) CColRefArray(m_mp);
	pdrgpcrOuter->Append(pcrOuter);
	BOOL fMatched = pmodel->FBind((*pop->Pdrgpsym())[0], pdrgpcrOuter) &&
		m_pmatcher->FMatch((*pop)[0], pexprOuterInput, pmodel) &&
		FMatchInner((*pop)[1], pexprInnerInput, pcrInner, pmodel);
	pdrgpcrOuter->Release();
	if (fMatched)
	{
		pexprCorrelation->AddRef();
		fMatched = pmodel->FSetInSubPred((*pop->Pdrgpsym())[0],
									 pexprCorrelation);
	}

	pexprOuterInput->Release();
	pexprInnerInput->Release();
	pdrgpexprOuterConj->Release();
	pdrgpexprInnerConj->Release();
	return fMatched;
}

BOOL
CDSLInSubMatcher::FMatchSemiJoin(const CDSLOp *pop, CExpression *pexpr,
								 CDSLModel *pmodel) const
{
	if (COperator::EopLogicalLeftSemiJoin != pexpr->Pop()->Eopid() ||
		3 != pexpr->Arity())
	{
		return false;
	}
	// InSubFilter denotes an uncorrelated relational membership test. A live
	// SemiJoin with outer references has LATERAL/correlation dependencies that
	// this DSL operator does not bind, so treating it as the same view would let
	// a target move those dependencies across a dedup or join boundary.
	if (0 != pexpr->DeriveOuterReferences()->Size())
	{
		return false;
	}

	CColRefSet *pcrsOuter = (*pexpr)[0]->DeriveOutputColumns();
	CColRefSet *pcrsInner = (*pexpr)[1]->DeriveOutputColumns();
	CExpressionArray *pdrgpexprConj =
		CPredicateUtils::PdrgpexprConjuncts(m_mp, (*pexpr)[2]);
	CColRefArray *pdrgpcrOuter = GPOS_NEW(m_mp) CColRefArray(m_mp);
	CColRefArray *pdrgpcrInner = GPOS_NEW(m_mp) CColRefArray(m_mp);
	BOOL fSimpleCrossEqualities = 0 < pdrgpexprConj->Size();
	for (ULONG ul = 0; ul < pdrgpexprConj->Size() && fSimpleCrossEqualities;
		 ul++)
	{
		CExpression *pexprCmp = (*pdrgpexprConj)[ul];
		if (2 == pexprCmp->Arity() &&
			CPredicateUtils::FPlainEquality(pexprCmp) &&
			COperator::EopScalarIdent == (*pexprCmp)[0]->Pop()->Eopid() &&
			COperator::EopScalarIdent == (*pexprCmp)[1]->Pop()->Eopid())
		{
			CColRef *pcr0 = const_cast<CColRef *>(
				CScalarIdent::PopConvert((*pexprCmp)[0]->Pop())->Pcr());
			CColRef *pcr1 = const_cast<CColRef *>(
				CScalarIdent::PopConvert((*pexprCmp)[1]->Pop())->Pcr());
			if (pcrsOuter->FMember(pcr0) && pcrsInner->FMember(pcr1))
			{
				pdrgpcrOuter->Append(pcr0);
				pdrgpcrInner->Append(pcr1);
			}
			else if (pcrsOuter->FMember(pcr1) && pcrsInner->FMember(pcr0))
			{
				pdrgpcrOuter->Append(pcr1);
				pdrgpcrInner->Append(pcr0);
			}
			else
			{
				fSimpleCrossEqualities = false;
			}
		}
		else
		{
			fSimpleCrossEqualities = false;
		}
	}
	pdrgpexprConj->Release();
	if (!fSimpleCrossEqualities)
	{
		pdrgpcrOuter->Release();
		pdrgpcrInner->Release();
		return false;
	}

	BOOL fMatched =
		pmodel->FBind((*pop->Pdrgpsym())[0], pdrgpcrOuter) &&
		m_pmatcher->FMatch((*pop)[0], (*pexpr)[0], pmodel) &&
		FMatchInner((*pop)[1], (*pexpr)[1], pdrgpcrInner, pmodel);
	pdrgpcrOuter->Release();
	pdrgpcrInner->Release();
	if (!fMatched)
	{
		return false;
	}

	const CDSLSymbol *psymAttrs = (*pop->Pdrgpsym())[0];
	(*pexpr)[2]->AddRef();
	BOOL fStored = pmodel->FSetInSubPred(psymAttrs, (*pexpr)[2]);
	if (fStored)
	{
		pexpr->AddRef();
		fStored = pmodel->FSetInSubCarrier(psymAttrs, pexpr);
	}
	return fStored;
}

BOOL
CDSLInSubMatcher::FMatchRoutedCarrier(const CDSLOp *pop,
									 CExpression *pexprCarrier,
									 CExpression *pexprRel,
									 CDSLModel *pmodel) const
{
	CExpression *pexprInSub = nullptr;
	if (COperator::EopLogicalSelect == pexprCarrier->Pop()->Eopid())
	{
		if (2 != pexprCarrier->Arity())
		{
			return false;
		}
		CExpressionArray *pdrgpexprConj =
			CPredicateUtils::PdrgpexprConjuncts(m_mp, (*pexprCarrier)[1]);
		BOOL fHasAny = false;
		for (ULONG ul = 0; ul < pdrgpexprConj->Size() && !fHasAny; ul++)
		{
			fHasAny = CDSLMatchView::FPlainEqAny((*pdrgpexprConj)[ul]);
		}
		pdrgpexprConj->Release();
		if (!fHasAny)
		{
			return false;
		}
	}
	else if (COperator::EopLogicalLeftSemiApplyIn ==
			 pexprCarrier->Pop()->Eopid())
	{
		if (3 != pexprCarrier->Arity())
		{
			return false;
		}
	}
	else if (COperator::EopLogicalLeftSemiJoin ==
			 pexprCarrier->Pop()->Eopid())
	{
		if (3 != pexprCarrier->Arity())
		{
			return false;
		}
	}
	else
	{
		return false;
	}
	pexprInSub =
		CDSLMatchView::PexprRebaseInSubCarrier(m_mp, pexprCarrier, pexprRel);
	GPOS_ASSERT(nullptr != pexprInSub);

	CDSLModel *pmodelProbe = GPOS_NEW(m_mp) CDSLModel(m_mp);
	BOOL fMatched = FMatch(pop, pexprInSub, pmodelProbe);
	const CDSLRule *prule = m_pmatcher->Prule();
	if (fMatched && nullptr != prule)
	{
		CDSLConstraintChecker checker(m_mp);
		fMatched = checker.FCheck(prule, pmodelProbe);
	}
	pmodelProbe->Release();
	if (fMatched)
	{
		fMatched = FMatch(pop, pexprInSub, pmodel);
	}
	pexprInSub->Release();
	return fMatched;
}

BOOL
CDSLInSubMatcher::FMatchPushedDownInnerJoin(const CDSLOp *pop,
									 CExpression *pexpr,
									 CDSLModel *pmodel) const
{
	if (EdslopInnerJoin != (*pop)[0]->Edslop() ||
		COperator::EopLogicalInnerJoin != pexpr->Pop()->Eopid() ||
		3 != pexpr->Arity())
	{
		return false;
	}

	BOOL fMatched = false;
	const COperator::EOperatorId rgeopidCarrier[] = {
		COperator::EopLogicalLeftSemiApplyIn,
		COperator::EopLogicalLeftSemiJoin,
		COperator::EopLogicalSelect};
	for (ULONG ulCarrier = 0;
		 ulCarrier < GPOS_ARRAY_SIZE(rgeopidCarrier) && !fMatched;
		 ulCarrier++)
	{
		CDSLMatchView::SJoinSpineRouteArray *pdrgproute =
			CDSLMatchView::PdrgprouteJoinSpine(
				m_mp, pexpr, rgeopidCarrier[ulCarrier]);
		for (ULONG ulRoute = 0;
			 ulRoute < pdrgproute->Size() && !fMatched; ulRoute++)
		{
			CDSLMatchView::SJoinSpineRoute *proute = (*pdrgproute)[ulRoute];
			fMatched = FMatchRoutedCarrier(
				pop, proute->m_pexprCarrier, proute->m_pexprRel, pmodel);
		}
		pdrgproute->Release();
	}
	return fMatched;
}

BOOL
CDSLInSubMatcher::FMatch(const CDSLOp *pop, CExpression *pexpr,
						 CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != pop);
	GPOS_ASSERT(EdslopInSubFilter == pop->Edslop());
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != pmodel);

	if (2 != pop->UlChildren() || nullptr == pop->Pdrgpsym() ||
		1 != pop->Pdrgpsym()->Size())
	{
		return false;
	}

	if (COperator::EopLogicalInnerJoin == pexpr->Pop()->Eopid())
	{
		return FMatchPushedDownInnerJoin(pop, pexpr, pmodel);
	}
	if (COperator::EopLogicalLeftSemiJoin == pexpr->Pop()->Eopid())
	{
		return FMatchSemiJoin(pop, pexpr, pmodel);
	}

	// Pre-Apply: Select(outer, ... AND outer_expr = ANY(inner) AND ...).
	if (COperator::EopLogicalSelect == pexpr->Pop()->Eopid())
	{
		if (2 != pexpr->Arity())
		{
			return false;
		}

		CExpressionArray *pdrgpexprConj =
			CPredicateUtils::PdrgpexprConjuncts(m_mp, (*pexpr)[1]);
		BOOL fHasDirectExists = false;
		for (ULONG ul = 0; ul < pdrgpexprConj->Size(); ul++)
		{
			fHasDirectExists = fHasDirectExists ||
				CDSLMatchView::FDirectExists((*pdrgpexprConj)[ul]);
		}
		pdrgpexprConj->Release();
		if (fHasDirectExists)
		{
			return FMatchCorrelatedExists(pop, pexpr, pmodel);
		}
		pdrgpexprConj =
			CPredicateUtils::PdrgpexprConjuncts(m_mp, (*pexpr)[1]);
		// WeTune represents chained WHERE ... IN (...) AND ... IN (...) as nested
		// InSubFilter nodes, while PostgreSQL flattens them into sibling ANY
		// conjuncts under one Select. Flatten the DSL's left spine, pair each node
		// with one live ANY, then match their common relational base once.
		std::vector<const CDSLOp *> rgpopChain;
		const CDSLOp *popBase = pop;
		while (EdslopInSubFilter == popBase->Edslop())
		{
			if (2 != popBase->UlChildren() || nullptr == popBase->Pdrgpsym() ||
				1 != popBase->Pdrgpsym()->Size())
			{
				pdrgpexprConj->Release();
				return false;
			}
			rgpopChain.push_back(popBase);
			popBase = (*popBase)[0];
		}

		std::vector<CExpression *> rgpexprChosen(rgpopChain.size(), nullptr);
		std::vector<BOOL> rgfUsed(pdrgpexprConj->Size(), false);
		BOOL fMatched = true;
		for (ULONG ulNode = 0; ulNode < rgpopChain.size() && fMatched;
			 ulNode++)
		{
			const CDSLOp *popNode = rgpopChain[ulNode];
			fMatched = false;
			for (ULONG ulConj = 0; ulConj < pdrgpexprConj->Size(); ulConj++)
			{
				CExpression *pexprConj = (*pdrgpexprConj)[ulConj];
				if (rgfUsed[ulConj] ||
					!CDSLMatchView::FPlainEqAny(pexprConj))
				{
					continue;
				}
				// Probe structural compatibility in a disposable model so rejected
				// candidates cannot leave partial bindings in the real model.
				CDSLModel *pmodelProbe = GPOS_NEW(m_mp) CDSLModel(m_mp);
				CScalarSubqueryAny *popAny =
					CScalarSubqueryAny::PopConvert(pexprConj->Pop());
				BOOL fFits = FMatchInner((*popNode)[1], (*pexprConj)[0],
									 popAny->Pcr(), pmodelProbe);
				pmodelProbe->Release();
				if (fFits)
				{
					rgpexprChosen[ulNode] = pexprConj;
					rgfUsed[ulConj] = true;
					fMatched = true;
					break;
				}
			}
		}

		if (fMatched)
		{
			fMatched = m_pmatcher->FMatch(popBase, (*pexpr)[0], pmodel);
		}
		for (ULONG ulNode = 0; ulNode < rgpopChain.size() && fMatched;
			 ulNode++)
		{
			const CDSLOp *popNode = rgpopChain[ulNode];
			CExpression *pexprAny = rgpexprChosen[ulNode];
			const CDSLSymbol *psymAttrs = (*popNode->Pdrgpsym())[0];
			CScalarSubqueryAny *popAny =
				CScalarSubqueryAny::PopConvert(pexprAny->Pop());
			fMatched =
				FMatchInner((*popNode)[1], (*pexprAny)[0], popAny->Pcr(),
							pmodel) &&
				FBindOuterAttrs(popNode, (*pexprAny)[1], pmodel) &&
				pmodel->FSetInSubPred(psymAttrs, PexprComparison(pexprAny));
		}

		if (fMatched)
		{
			CExpressionArray *pdrgpexprResidual =
				GPOS_NEW(m_mp) CExpressionArray(m_mp);
			for (ULONG ul = 0; ul < pdrgpexprConj->Size(); ul++)
			{
				if (!rgfUsed[ul])
				{
					CExpression *pexprConj = (*pdrgpexprConj)[ul];
					pexprConj->AddRef();
					pdrgpexprResidual->Append(pexprConj);
				}
			}
			pmodel->SetInSubResidualConjuncts(pdrgpexprResidual);
		}
		pdrgpexprConj->Release();
		return fMatched;
	}

	// Post-Apply: LeftSemiApplyIn(outer, inner, equality-predicate).
	// A correlated EXISTS equality reaches the sibling LeftSemiApply shape after
	// native unnesting. Recreate its pre-unnest view transiently and feed it to
	// the same representation adapter, keeping one canonical implementation of
	// predicate extraction, NULL guards and symbol binding.
	if (COperator::EopLogicalLeftSemiApply == pexpr->Pop()->Eopid())
	{
		if (3 != pexpr->Arity() || !CUtils::FScalarConstTrue((*pexpr)[2]))
		{
			return false;
		}
		CLogicalApply *popApply = CLogicalApply::PopConvert(pexpr->Pop());
		if (COperator::EopScalarSubqueryExists != popApply->EopidOriginSubq())
		{
			return false;
		}

		(*pexpr)[1]->AddRef();
		CExpression *pexprExists = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CScalarSubqueryExists(m_mp), (*pexpr)[1]);
		(*pexpr)[0]->AddRef();
		CExpression *pexprSelect = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), (*pexpr)[0],
			pexprExists);
		BOOL fMatched = FMatchCorrelatedExists(pop, pexprSelect, pmodel);
		pexprSelect->Release();
		return fMatched;
	}

	// Post-Apply: LeftSemiApplyIn(outer, inner, equality-predicate).
	if (COperator::EopLogicalLeftSemiApplyIn == pexpr->Pop()->Eopid())
	{
		if (3 != pexpr->Arity() ||
			COperator::EopScalarCmp != (*pexpr)[2]->Pop()->Eopid() ||
			IMDType::EcmptEq !=
				CScalarCmp::PopConvert((*pexpr)[2]->Pop())->ParseCmpType())
		{
			return false;
		}
		CLogicalApply *popApply = CLogicalApply::PopConvert(pexpr->Pop());
		CColRefArray *pdrgpcrInner = popApply->PdrgPcrInner();
		if (COperator::EopScalarSubqueryAny != popApply->EopidOriginSubq() ||
			nullptr == pdrgpcrInner || 1 != pdrgpcrInner->Size())
		{
			return false;
		}

		CColRefSet *pcrsOuterUsed =
			GPOS_NEW(m_mp) CColRefSet(m_mp, *(*pexpr)[2]->DeriveUsedColumns());
		pcrsOuterUsed->Exclude((*pexpr)[1]->DeriveOutputColumns());
		CColRefArray *pdrgpcrOuter = pcrsOuterUsed->Pdrgpcr(m_mp);
		pcrsOuterUsed->Release();
		BOOL fBound = 0 < pdrgpcrOuter->Size() &&
			pmodel->FBind((*pop->Pdrgpsym())[0], pdrgpcrOuter);
		pdrgpcrOuter->Release();
		if (!fBound ||
			!m_pmatcher->FMatch((*pop)[0], (*pexpr)[0], pmodel) ||
			!FMatchInner((*pop)[1], (*pexpr)[1], (*pdrgpcrInner)[0],
						pmodel))
		{
			return false;
		}
		const CDSLSymbol *psymAttrs = (*pop->Pdrgpsym())[0];
		(*pexpr)[2]->AddRef();
		BOOL fStored = pmodel->FSetInSubPred(psymAttrs, (*pexpr)[2]);
		if (fStored)
		{
			pexpr->AddRef();
			fStored = pmodel->FSetInSubCarrier(psymAttrs, pexpr);
		}
		return fStored;
	}

	return false;
}

// EOF
