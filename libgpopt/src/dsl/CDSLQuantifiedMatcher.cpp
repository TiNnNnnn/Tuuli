//---------------------------------------------------------------------------
// Generic matcher for quantified comparison filters (ANY / ALL).
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLQuantifiedMatcher.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarSubqueryQuantified.h"

using namespace gpopt;

BOOL
CDSLQuantifiedMatcher::FMatchInner(const CDSLOp *popInner,
								   CExpression *pexprInner,
								   CColRefArray *pdrgpcrProjected,
								   CDSLModel *pmodel) const
{
	// PostgreSQL folds a pass-through subquery projection into the quantified
	// operator's selected-column metadata. Re-expose only that transparent
	// projection; computed projections and Proj* remain real tree nodes.
	if (EdslopProj == popInner->Edslop() && !popInner->FDistinct() &&
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
			const BOOL fPassThrough =
				(*pexprRel)[0]->DeriveOutputColumns()->ContainsAll(pcrsProjected);
			pcrsProjected->Release();
			if (!fPassThrough)
			{
				break;
			}
			pexprRel = (*pexprRel)[0];
		}
		if (COperator::EopLogicalProject == pexprRel->Pop()->Eopid())
		{
			return m_pmatcher->FMatch(popInner, pexprInner, pmodel);
		}
		return pmodel->FBind((*popInner->Pdrgpsym())[0], pdrgpcrProjected) &&
			pmodel->FBind((*popInner->Pdrgpsym())[1], pdrgpcrProjected) &&
			m_pmatcher->FMatch((*popInner)[0], pexprRel, pmodel);
	}

	return m_pmatcher->FMatch(popInner, pexprInner, pmodel);
}

CExpression *
CDSLQuantifiedMatcher::PexprComparison(CExpression *pexprSubquery) const
{
	CScalarSubqueryQuantified *popQuantified =
		CScalarSubqueryQuantified::PopConvert(pexprSubquery->Pop());
	CExpression *pexprOuterScalar = (*pexprSubquery)[1];
	pexprOuterScalar->AddRef();
	IMDId *pmdidOp = popQuantified->MdIdOp();
	pmdidOp->AddRef();
	return CUtils::PexprScalarCmp(m_mp, pexprOuterScalar,
								 popQuantified->Pcr(),
								 *popQuantified->PstrOp(), pmdidOp);
}

BOOL
CDSLQuantifiedMatcher::FMatch(const CDSLOp *pop, CExpression *pexpr,
							   CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != pop);
	GPOS_ASSERT(EdslopAny == pop->Edslop() || EdslopAll == pop->Edslop());
	if (2 != pop->UlChildren() || nullptr == pop->Pdrgpsym() ||
		2 != pop->Pdrgpsym()->Size())
	{
		return false;
	}

	const BOOL fAll = EdslopAll == pop->Edslop();
	const COperator::EOperatorId eopidSubquery =
		fAll ? COperator::EopScalarSubqueryAll
			 : COperator::EopScalarSubqueryAny;

	// Pre-unnest representation: Select(outer, outer_expr OP ANY/ALL(inner)).
	if (COperator::EopLogicalSelect == pexpr->Pop()->Eopid() &&
		2 == pexpr->Arity())
	{
		CExpressionArray *pdrgpexprConj =
			CPredicateUtils::PdrgpexprConjuncts(m_mp, (*pexpr)[1]);
		CExpression *pexprQuantified = nullptr;
		ULONG ulQuantified = 0;
		for (ULONG ul = 0; ul < pdrgpexprConj->Size(); ul++)
		{
			if (eopidSubquery == (*pdrgpexprConj)[ul]->Pop()->Eopid())
			{
				pexprQuantified = (*pdrgpexprConj)[ul];
				ulQuantified++;
			}
		}
		if (1 != ulQuantified)
		{
			pdrgpexprConj->Release();
			return false;
		}

		CScalarSubqueryQuantified *popQuantified =
			CScalarSubqueryQuantified::PopConvert(pexprQuantified->Pop());
		CColRefArray *pdrgpcrInner = GPOS_NEW(m_mp) CColRefArray(m_mp);
		pdrgpcrInner->Append(
			const_cast<CColRef *>(popQuantified->Pcr()));
		CColRefArray *pdrgpcrOuter =
			(*pexprQuantified)[1]->DeriveUsedColumns()->Pdrgpcr(m_mp);
		CExpression *pexprCmp = PexprComparison(pexprQuantified);
		BOOL fMatched = pmodel->FBind((*pop->Pdrgpsym())[0], pexprCmp) &&
			pmodel->FBind((*pop->Pdrgpsym())[1], pdrgpcrOuter) &&
			m_pmatcher->FMatch((*pop)[0], (*pexpr)[0], pmodel) &&
			FMatchInner((*pop)[1], (*pexprQuantified)[0], pdrgpcrInner,
						pmodel);
		pexprCmp->Release();
		pdrgpcrOuter->Release();
		pdrgpcrInner->Release();
		if (fMatched)
		{
			CExpressionArray *pdrgpexprResidual =
				GPOS_NEW(m_mp) CExpressionArray(m_mp);
			for (ULONG ul = 0; ul < pdrgpexprConj->Size(); ul++)
			{
				CExpression *pexprConj = (*pdrgpexprConj)[ul];
				if (pexprConj != pexprQuantified)
				{
					pexprConj->AddRef();
					pdrgpexprResidual->Append(pexprConj);
				}
			}
			pmodel->SetInSubResidualConjuncts(pdrgpexprResidual);
		}
		pdrgpexprConj->Release();
		return fMatched;
	}

	// Post-unnest ANY and ALL both retain the quantified comparison. The NotIn
	// anti-apply implements ALL/NULL semantics; its scalar child is not a
	// separately negated or inverted witness predicate.
	const COperator::EOperatorId eopidApply =
		fAll ? COperator::EopLogicalLeftAntiSemiApplyNotIn
			 : COperator::EopLogicalLeftSemiApplyIn;
	const COperator::EOperatorId eopidCorrelatedApply =
		fAll ? COperator::EopLogicalLeftAntiSemiCorrelatedApplyNotIn
			 : COperator::EopLogicalLeftSemiCorrelatedApplyIn;
	if ((eopidApply != pexpr->Pop()->Eopid() &&
		 eopidCorrelatedApply != pexpr->Pop()->Eopid()) ||
		3 != pexpr->Arity() ||
		COperator::EopScalarCmp != (*pexpr)[2]->Pop()->Eopid())
	{
		return false;
	}
	CLogicalApply *popApply = CLogicalApply::PopConvert(pexpr->Pop());
	CColRefArray *pdrgpcrInner = popApply->PdrgPcrInner();
	if (eopidSubquery != popApply->EopidOriginSubq() ||
		nullptr == pdrgpcrInner || 1 != pdrgpcrInner->Size())
	{
		return false;
	}

	CExpression *pexprCmp = (*pexpr)[2];
	pexprCmp->AddRef();
	CColRefSet *pcrsOuterUsed =
		GPOS_NEW(m_mp) CColRefSet(m_mp, *pexprCmp->DeriveUsedColumns());
	pcrsOuterUsed->Exclude((*pexpr)[1]->DeriveOutputColumns());
	CColRefArray *pdrgpcrOuter = pcrsOuterUsed->Pdrgpcr(m_mp);
	pcrsOuterUsed->Release();
	BOOL fMatched = pmodel->FBind((*pop->Pdrgpsym())[0], pexprCmp) &&
		pmodel->FBind((*pop->Pdrgpsym())[1], pdrgpcrOuter) &&
		m_pmatcher->FMatch((*pop)[0], (*pexpr)[0], pmodel) &&
		FMatchInner((*pop)[1], (*pexpr)[1], pdrgpcrInner, pmodel);
	if (fMatched)
	{
		pexpr->AddRef();
		fMatched = pmodel->FSetInSubCarrier((*pop->Pdrgpsym())[1], pexpr);
	}
	pexprCmp->Release();
	pdrgpcrOuter->Release();
	return fMatched;
}

// EOF
