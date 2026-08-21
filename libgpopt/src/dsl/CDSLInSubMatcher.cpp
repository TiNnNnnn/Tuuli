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
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CScalarSubqueryAny.h"

using namespace gpopt;

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

	// Pre-Apply: Select(outer, ... AND outer_expr = ANY(inner) AND ...).
	if (COperator::EopLogicalSelect == pexpr->Pop()->Eopid())
	{
		if (2 != pexpr->Arity())
		{
			return false;
		}

		CExpressionArray *pdrgpexprConj =
			CPredicateUtils::PdrgpexprConjuncts(m_mp, (*pexpr)[1]);
		CExpression *pexprAny = nullptr;
		ULONG ulAny = 0;
		for (ULONG ul = 0; ul < pdrgpexprConj->Size(); ul++)
		{
			CExpression *pexprConj = (*pdrgpexprConj)[ul];
			if (COperator::EopScalarSubqueryAny ==
					pexprConj->Pop()->Eopid() &&
				2 == pexprConj->Arity())
			{
				CScalarSubqueryAny *popAny =
					CScalarSubqueryAny::PopConvert(pexprConj->Pop());
				if (IMDType::EcmptEq == CUtils::ParseCmpType(popAny->MdIdOp()))
				{
					pexprAny = pexprConj;
					ulAny++;
				}
			}
		}

		BOOL fMatched = false;
		BOOL fInnerMatched = false;
		if (1 == ulAny)
		{
			const CDSLOp *popInner = (*pop)[1];
			CExpression *pexprInner = (*pexprAny)[0];

			// PostgreSQL's ORCA translator removes a pass-through SELECT-list
			// projection from a scalar IN subquery and stores its selected column
			// directly on CScalarSubqueryAny. WeTune still exposes that SQL node as
			// Proj<a s>(child). Treat this one precise representation difference as
			// a transparent projection: the ANY comparison column is both the
			// referenced attr and output schema, then match the Proj's child against
			// the live inner relation. Computed/non-pass-through projects remain real
			// CLogicalProject nodes and go through CDSLProjMatcher.
			if (EdslopProj == popInner->Edslop() &&
				COperator::EopLogicalProject != pexprInner->Pop()->Eopid() &&
				1 == popInner->UlChildren() && nullptr != popInner->Pdrgpsym() &&
				2 == popInner->Pdrgpsym()->Size())
			{
				CColRefArray *pdrgpcr = GPOS_NEW(m_mp) CColRefArray(m_mp);
				pdrgpcr->Append(const_cast<CColRef *>(
					CScalarSubqueryAny::PopConvert(pexprAny->Pop())->Pcr()));
				fInnerMatched =
					pmodel->FBind((*popInner->Pdrgpsym())[0], pdrgpcr) &&
					pmodel->FBind((*popInner->Pdrgpsym())[1], pdrgpcr) &&
					m_pmatcher->FMatch((*popInner)[0], pexprInner, pmodel);
				pdrgpcr->Release();
			}
			else
			{
				fInnerMatched =
					m_pmatcher->FMatch(popInner, pexprInner, pmodel);
			}
		}
		if (1 == ulAny && fInnerMatched &&
			FBindOuterAttrs(pop, (*pexprAny)[1], pmodel) &&
			m_pmatcher->FMatch((*pop)[0], (*pexpr)[0], pmodel))
		{
			CScalarSubqueryAny *popAny =
				CScalarSubqueryAny::PopConvert(pexprAny->Pop());
			CExpression *pexprOuterScalar = (*pexprAny)[1];
			pexprOuterScalar->AddRef();
			IMDId *pmdidOp = popAny->MdIdOp();
			pmdidOp->AddRef();
			CExpression *pexprPred = CUtils::PexprScalarCmp(
				m_mp, pexprOuterScalar, popAny->Pcr(), *popAny->PstrOp(),
				pmdidOp);
			pmodel->SetInSubPred(pexprPred);

			CExpressionArray *pdrgpexprResidual =
				GPOS_NEW(m_mp) CExpressionArray(m_mp);
			for (ULONG ul = 0; ul < pdrgpexprConj->Size(); ul++)
			{
				CExpression *pexprConj = (*pdrgpexprConj)[ul];
				if (pexprConj != pexprAny)
				{
					pexprConj->AddRef();
					pdrgpexprResidual->Append(pexprConj);
				}
			}
			pmodel->SetInSubResidualConjuncts(pdrgpexprResidual);
			fMatched = true;
		}
		pdrgpexprConj->Release();
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
		if (COperator::EopScalarSubqueryAny != popApply->EopidOriginSubq())
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
			!m_pmatcher->FMatch((*pop)[1], (*pexpr)[1], pmodel))
		{
			return false;
		}
		(*pexpr)[2]->AddRef();
		pmodel->SetInSubPred((*pexpr)[2]);
		return true;
	}

	return false;
}

// EOF
