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

#include <vector>

using namespace gpopt;

namespace
{
BOOL
FPlainEqAny(CExpression *pexpr)
{
	return COperator::EopScalarSubqueryAny == pexpr->Pop()->Eopid() &&
		   2 == pexpr->Arity() &&
		   IMDType::EcmptEq == CUtils::ParseCmpType(
				CScalarSubqueryAny::PopConvert(pexpr->Pop())->MdIdOp());
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
	// PostgreSQL's ORCA translator removes a pass-through SELECT-list
	// projection from a scalar IN subquery and stores its selected column
	// on the subquery/Apply operator. WeTune still exposes that SQL node as
	// Proj<a s>(child). Treat this representation difference as a transparent
	// projection in both pre- and post-Apply matching. Computed projects remain
	// CLogicalProject nodes.
	if (EdslopProj == popInner->Edslop() &&
		COperator::EopLogicalProject != pexprInner->Pop()->Eopid() &&
		1 == popInner->UlChildren() && nullptr != popInner->Pdrgpsym() &&
		2 == popInner->Pdrgpsym()->Size() && nullptr != pcrProjected)
	{
		CColRefArray *pdrgpcr = GPOS_NEW(m_mp) CColRefArray(m_mp);
		pdrgpcr->Append(const_cast<CColRef *>(pcrProjected));
		BOOL fMatched =
			pmodel->FBind((*popInner->Pdrgpsym())[0], pdrgpcr) &&
			pmodel->FBind((*popInner->Pdrgpsym())[1], pdrgpcr) &&
			m_pmatcher->FMatch((*popInner)[0], pexprInner, pmodel);
		pdrgpcr->Release();
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
				if (rgfUsed[ulConj] || !FPlainEqAny(pexprConj))
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
		(*pexpr)[2]->AddRef();
		return pmodel->FSetInSubPred((*pop->Pdrgpsym())[0], (*pexpr)[2]);
	}

	return false;
}

// EOF
