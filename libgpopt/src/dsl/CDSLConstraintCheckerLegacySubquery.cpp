//---------------------------------------------------------------------------
// MONSOON DSL rule engine — CDSLConstraintCheckerLegacySubquery.cpp
// Legacy constraint-driven subquery extraction and sequence lowering.
// These checks also produce bindings consumed by subsequent constraints and
// target builders; preserve FCheck's ordered check/materialize protocol.
// Remove only after the Predicate*/ExprList* consumers have migrated to typed
// captures/builds with equivalent correlation, NULL and cardinality behavior.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLMatchView.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLQuantifiedMatcher.h"
#include "gpopt/operators/CScalarIf.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarSubquery.h"
#include "gpopt/operators/CScalarSubqueryQuantified.h"
#include "naucrates/md/IMDTypeBool.h"

using namespace gpopt;
using namespace gpnaucrates;

BOOL
CDSLConstraintChecker::FCheckPredicateExists(
	const CDSLConstraint *pcon, CDSLModel *pmodel, BOOL fNegated) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (nullptr == pdrgpsym || 2 != pdrgpsym->Size() ||
		EdslsymPred != (*pdrgpsym)[0]->Esymkind() ||
		EdslsymTable != (*pdrgpsym)[1]->Esymkind())
	{
		return false;
	}

	CExpression *pexprPredicate = pmodel->PexprPred((*pdrgpsym)[0]);
	if (nullptr == pexprPredicate)
	{
		return false;
	}
	const COperator::EOperatorId eopid =
		fNegated ? COperator::EopScalarSubqueryNotExists
				 : COperator::EopScalarSubqueryExists;
	if (1 != pexprPredicate->Arity() ||
		eopid != pexprPredicate->Pop()->Eopid())
	{
		return false;
	}
	CExpression *pexprInput = (*pexprPredicate)[0];
	CExpression *pexprBound = pmodel->PexprTable((*pdrgpsym)[1]);
	return nullptr == pexprBound ? pmodel->FBind((*pdrgpsym)[1], pexprInput)
							 : CDSLMatchView::FSameCapturedExpression(pexprBound, pexprInput);
}

BOOL
CDSLConstraintChecker::FCheckPredicateQuantified(const CDSLConstraint *pcon,
											 CDSLModel *pmodel,
											 BOOL fAll) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (nullptr == pdrgpsym || 4 != pdrgpsym->Size() ||
		EdslsymPred != (*pdrgpsym)[0]->Esymkind() ||
		EdslsymPred != (*pdrgpsym)[1]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[2]->Esymkind() ||
		EdslsymTable != (*pdrgpsym)[3]->Esymkind())
	{
		return false;
	}

	CExpression *pexprQuantified = pmodel->PexprPred((*pdrgpsym)[0]);
	const COperator::EOperatorId eopid =
		fAll ? COperator::EopScalarSubqueryAll
			 : COperator::EopScalarSubqueryAny;
	if (nullptr == pexprQuantified || 2 != pexprQuantified->Arity() ||
		eopid != pexprQuantified->Pop()->Eopid() ||
		(*pexprQuantified)[1]->DeriveHasSubquery())
	{
		return false;
	}

	CExpression *pexprComparison =
		CDSLQuantifiedMatcher::PexprComparison(m_mp, pexprQuantified);
	CColRefArray *pdrgpcrOuter =
		(*pexprQuantified)[1]->DeriveUsedColumns()->Pdrgpcr(m_mp);
	const BOOL fMatches = pmodel->FBind((*pdrgpsym)[1], pexprComparison) &&
		pmodel->FBind((*pdrgpsym)[2], pdrgpcrOuter) &&
		pmodel->FBind((*pdrgpsym)[3], (*pexprQuantified)[0]);
	pexprComparison->Release();
	pdrgpcrOuter->Release();
	return fMatches;
}

namespace
{
CExpression *
PexprOnlySubquery(CExpression *pexpr, COperator::EOperatorId eopid,
				  ULONG *pulCount)
{
	if (eopid == pexpr->Pop()->Eopid())
	{
		(*pulCount)++;
		return pexpr;
	}
	if (!pexpr->Pop()->FScalar())
	{
		return nullptr;
	}
	CExpression *pexprFound = nullptr;
	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		CExpression *pexprChild =
			PexprOnlySubquery((*pexpr)[ul], eopid, pulCount);
		if (nullptr != pexprChild && nullptr == pexprFound)
		{
			pexprFound = pexprChild;
		}
	}
	return pexprFound;
}

CExpression *
PexprReplaceNode(CMemoryPool *mp, CExpression *pexpr,
				 CExpression *pexprNeedle, CExpression *pexprReplacement)
{
	if (pexpr == pexprNeedle)
	{
		pexprReplacement->AddRef();
		return pexprReplacement;
	}
	CExpressionArray *pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		pdrgpexpr->Append(PexprReplaceNode(
			mp, (*pexpr)[ul], pexprNeedle, pexprReplacement));
	}
	pexpr->Pop()->AddRef();
	return GPOS_NEW(mp) CExpression(mp, pexpr->Pop(), pdrgpexpr);
}

ULONG
UlSubqueryPriority(COperator::EOperatorId eopid)
{
	switch (eopid)
	{
		case COperator::EopScalarSubquery:
			return 0;
		case COperator::EopScalarSubqueryExists:
			return 1;
		case COperator::EopScalarSubqueryNotExists:
			return 2;
		case COperator::EopScalarSubqueryAny:
			return 3;
		case COperator::EopScalarSubqueryAll:
			return 4;
		default:
			return gpos::ulong_max;
	}
}

void
FindNextSubquery(CExpression *pexpr, ULONG ulDepth,
				 CExpression **ppexprBest, ULONG *pulBestDepth,
				 ULONG *pulBestPriority)
{
	if (!pexpr->Pop()->FScalar())
	{
		return;
	}
	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		FindNextSubquery((*pexpr)[ul], ulDepth + 1, ppexprBest,
						 pulBestDepth, pulBestPriority);
	}

	const ULONG ulPriority = UlSubqueryPriority(pexpr->Pop()->Eopid());
	if (gpos::ulong_max == ulPriority)
	{
		return;
	}
	if (nullptr == *ppexprBest || ulDepth > *pulBestDepth ||
		(ulDepth == *pulBestDepth && ulPriority < *pulBestPriority))
	{
		*ppexprBest = pexpr;
		*pulBestDepth = ulDepth;
		*pulBestPriority = ulPriority;
	}
}

CExpression *
PexprNextSubqueryInSequence(CDSLModel *pmodel, const CDSLSymbol *psym)
{
	CExpression *pexprBest = nullptr;
	ULONG ulBestDepth = 0;
	ULONG ulBestPriority = gpos::ulong_max;
	if (EdslsymExpr == psym->Esymkind() ||
		EdslsymPred == psym->Esymkind() ||
		EdslsymWindow == psym->Esymkind())
	{
		CExpression *pexpr = EdslsymExpr == psym->Esymkind()
			? pmodel->PexprExpr(psym)
			: (EdslsymPred == psym->Esymkind()
				   ? pmodel->PexprPred(psym)
				   : pmodel->PexprWindow(psym));
		if (nullptr != pexpr)
			FindNextSubquery(pexpr, 0, &pexprBest, &ulBestDepth,
							 &ulBestPriority);
		return pexprBest;
	}
	CExpressionArray *pdrgpexpr = pmodel->PdrgpexprFunc(psym);
	for (ULONG ul = 0; nullptr != pdrgpexpr && ul < pdrgpexpr->Size(); ul++)
		FindNextSubquery((*pdrgpexpr)[ul], 0, &pexprBest, &ulBestDepth,
						 &ulBestPriority);
	return pexprBest;
}

CRefCount *
PvalReplaceNodeInSequence(CMemoryPool *mp, CDSLModel *pmodel,
						  const CDSLSymbol *psym, CExpression *pexprNeedle,
						  CExpression *pexprReplacement)
{
	if (EdslsymExpr == psym->Esymkind() ||
		EdslsymPred == psym->Esymkind() ||
		EdslsymWindow == psym->Esymkind())
	{
		CExpression *pexprSource = EdslsymExpr == psym->Esymkind()
			? pmodel->PexprExpr(psym)
			: (EdslsymPred == psym->Esymkind()
				   ? pmodel->PexprPred(psym)
				   : pmodel->PexprWindow(psym));
		CExpression *pexprLowered = PexprReplaceNode(
			mp, pexprSource, pexprNeedle, pexprReplacement);
		return pexprLowered;
	}
	CExpressionArray *pdrgpexpr = pmodel->PdrgpexprFunc(psym);
	CExpressionArray *pdrgpexprLowered =
		GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < pdrgpexpr->Size(); ul++)
	{
		CExpression *pexprLowered = PexprReplaceNode(
			mp, (*pdrgpexpr)[ul], pexprNeedle, pexprReplacement);
		pdrgpexprLowered->Append(pexprLowered);
	}
	return pdrgpexprLowered;
}
}  // namespace

BOOL
CDSLConstraintChecker::FCheckPredicateScalarSubquery(
	const CDSLConstraint *pcon, CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (nullptr == pdrgpsym || 6 != pdrgpsym->Size() ||
		EdslsymPred != (*pdrgpsym)[0]->Esymkind() ||
		EdslsymPred != (*pdrgpsym)[1]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[2]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[3]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[4]->Esymkind() ||
		EdslsymTable != (*pdrgpsym)[5]->Esymkind())
	{
		return false;
	}
	CExpressionArray *pdrgpexprResidual = pmodel->PdrgpexprResidual();
	for (ULONG ul = 0;
		 nullptr != pdrgpexprResidual && ul < pdrgpexprResidual->Size(); ul++)
	{
		if ((*pdrgpexprResidual)[ul]->DeriveHasSubquery())
		{
			return false;
		}
	}

	CExpression *pexprPredicate = pmodel->PexprPred((*pdrgpsym)[0]);
	ULONG ulSubqueries = 0;
	CExpression *pexprSubquery = nullptr == pexprPredicate
		? nullptr
		: PexprOnlySubquery(pexprPredicate,
						COperator::EopScalarSubquery, &ulSubqueries);
	if (1 != ulSubqueries || nullptr == pexprSubquery ||
		1 != pexprSubquery->Arity())
	{
		return false;
	}

	CScalarSubquery *popSubquery =
		CScalarSubquery::PopConvert(pexprSubquery->Pop());
	if (popSubquery->FGeneratedByQuantified())
	{
		return false;
	}
	if (!(*pexprSubquery)[0]->DeriveOutputColumns()->FMember(
			popSubquery->Pcr()))
	{
		return false;
	}
	CColRef *pcrInner = const_cast<CColRef *>(popSubquery->Pcr());
	CExpression *pexprIdent = CUtils::PexprScalarIdent(m_mp, pcrInner);
	CExpression *pexprLowered = PexprReplaceNode(
		m_mp, pexprPredicate, pexprSubquery, pexprIdent);
	pexprIdent->Release();

	CColRefSet *pcrsLeft =
		GPOS_NEW(m_mp) CColRefSet(m_mp, *pexprLowered->DeriveUsedColumns());
	pcrsLeft->Exclude(pcrInner);
	CColRefArray *pdrgpcrLeft = pcrsLeft->Pdrgpcr(m_mp);
	pcrsLeft->Release();
	CColRefArray *pdrgpcrRight = GPOS_NEW(m_mp) CColRefArray(m_mp);
	pdrgpcrRight->Append(pcrInner);
	CExpression *pexprInner = (*pexprSubquery)[0];
	CColRefArray *pdrgpcrCorrelation =
		pexprInner->DeriveOuterReferences()->Pdrgpcr(m_mp);

	const BOOL fMatches =
		pmodel->FBind((*pdrgpsym)[1], pexprLowered) &&
		pmodel->FBind((*pdrgpsym)[2], pdrgpcrLeft) &&
		pmodel->FBind((*pdrgpsym)[3], pdrgpcrRight) &&
		pmodel->FBind((*pdrgpsym)[4], pdrgpcrCorrelation) &&
		pmodel->FBind((*pdrgpsym)[5], pexprInner);
	pexprLowered->Release();
	pdrgpcrLeft->Release();
	pdrgpcrRight->Release();
	pdrgpcrCorrelation->Release();
	return fMatches;
}

BOOL
CDSLConstraintChecker::FCheckExprListScalarSubquery(
	const CDSLConstraint *pcon, CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (nullptr == pdrgpsym || 8 != pdrgpsym->Size() ||
		(EdslsymExpr != (*pdrgpsym)[0]->Esymkind() &&
		 EdslsymFunc != (*pdrgpsym)[0]->Esymkind() &&
		 EdslsymPred != (*pdrgpsym)[0]->Esymkind() &&
		 EdslsymWindow != (*pdrgpsym)[0]->Esymkind()) ||
		(*pdrgpsym)[0]->Esymkind() != (*pdrgpsym)[1]->Esymkind() ||
		EdslsymPred != (*pdrgpsym)[2]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[3]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[4]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[5]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[6]->Esymkind() ||
		EdslsymTable != (*pdrgpsym)[7]->Esymkind())
	{
		return false;
	}

	CExpression *pexprSubquery =
		PexprNextSubqueryInSequence(pmodel, (*pdrgpsym)[0]);
	if (nullptr == pexprSubquery ||
		COperator::EopScalarSubquery != pexprSubquery->Pop()->Eopid() ||
		1 != pexprSubquery->Arity())
	{
		return false;
	}

	CScalarSubquery *popSubquery =
		CScalarSubquery::PopConvert(pexprSubquery->Pop());
	if (popSubquery->FGeneratedByQuantified())
	{
		return false;
	}
	if (!(*pexprSubquery)[0]->DeriveOutputColumns()->FMember(
			popSubquery->Pcr()))
	{
		return false;
	}
	CColRef *pcrInner = const_cast<CColRef *>(popSubquery->Pcr());
	CExpression *pexprIdent = CUtils::PexprScalarIdent(m_mp, pcrInner);
	CRefCount *pvalLowered = PvalReplaceNodeInSequence(
		m_mp, pmodel, (*pdrgpsym)[0], pexprSubquery, pexprIdent);
	pexprIdent->Release();

	CExpression *pexprTrue = CUtils::PexprScalarConstBool(m_mp, true);
	CColRefArray *pdrgpcrLeft = GPOS_NEW(m_mp) CColRefArray(m_mp);
	CColRefArray *pdrgpcrRight = GPOS_NEW(m_mp) CColRefArray(m_mp);
	CColRefArray *pdrgpcrInner = GPOS_NEW(m_mp) CColRefArray(m_mp);
	pdrgpcrInner->Append(pcrInner);
	CExpression *pexprInner = (*pexprSubquery)[0];
	CColRefArray *pdrgpcrCorrelation =
		pexprInner->DeriveOuterReferences()->Pdrgpcr(m_mp);

	const BOOL fLowered = EdslsymPred == (*pdrgpsym)[1]->Esymkind()
		? pmodel->FBindDerived((*pdrgpsym)[1], pvalLowered)
		: pmodel->FBind((*pdrgpsym)[1], pvalLowered);
	const BOOL fMatches =
		fLowered &&
		pmodel->FBind((*pdrgpsym)[2], pexprTrue) &&
		pmodel->FBind((*pdrgpsym)[3], pdrgpcrLeft) &&
		pmodel->FBind((*pdrgpsym)[4], pdrgpcrRight) &&
		pmodel->FBind((*pdrgpsym)[5], pdrgpcrCorrelation) &&
		pmodel->FBind((*pdrgpsym)[6], pdrgpcrInner) &&
		pmodel->FBind((*pdrgpsym)[7], pexprInner);
	pvalLowered->Release();
	pexprTrue->Release();
	pdrgpcrLeft->Release();
	pdrgpcrRight->Release();
	pdrgpcrCorrelation->Release();
	pdrgpcrInner->Release();
	return fMatches;
}

BOOL
CDSLConstraintChecker::FCheckExprListExistential(
	const CDSLConstraint *pcon, CDSLModel *pmodel, BOOL fNegated) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (nullptr == pdrgpsym || 11 != pdrgpsym->Size() ||
		(EdslsymExpr != (*pdrgpsym)[0]->Esymkind() &&
		 EdslsymFunc != (*pdrgpsym)[0]->Esymkind() &&
		 EdslsymPred != (*pdrgpsym)[0]->Esymkind() &&
		 EdslsymWindow != (*pdrgpsym)[0]->Esymkind()) ||
		(*pdrgpsym)[0]->Esymkind() != (*pdrgpsym)[1]->Esymkind() ||
		EdslsymExpr != (*pdrgpsym)[2]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[3]->Esymkind() ||
		EdslsymSchema != (*pdrgpsym)[4]->Esymkind() ||
		EdslsymPred != (*pdrgpsym)[5]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[6]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[7]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[8]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[9]->Esymkind() ||
		EdslsymTable != (*pdrgpsym)[10]->Esymkind())
	{
		return false;
	}

	CExpression *pexprSubquery =
		PexprNextSubqueryInSequence(pmodel, (*pdrgpsym)[0]);
	const COperator::EOperatorId eopid =
		fNegated ? COperator::EopScalarSubqueryNotExists
				 : COperator::EopScalarSubqueryExists;
	if (nullptr == pexprSubquery || eopid != pexprSubquery->Pop()->Eopid() ||
		1 != pexprSubquery->Arity())
	{
		return false;
	}

	CExpression *pexprInner = (*pexprSubquery)[0];
	CColRefSet *pcrsInnerOutput = pexprInner->DeriveOutputColumns();
	if (0 == pcrsInnerOutput->Size())
	{
		return false;
	}

	const IMDTypeBool *pmdtypebool =
		COptCtxt::PoctxtFromTLS()->Pmda()->PtMDType<IMDTypeBool>();
	CColRef *pcrMarker = COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
		pmdtypebool, default_type_modifier);
	CExpressionArray *pdrgpexprMarker = GPOS_NEW(m_mp) CExpressionArray(m_mp);
	pdrgpexprMarker->Append(CUtils::PexprScalarProjectElement(
		m_mp, pcrMarker, CUtils::PexprScalarConstBool(m_mp, true)));
	CExpression *pexprMarkerList = GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CScalarProjectList(m_mp), pdrgpexprMarker);

	IMDId *pmdidBool = pmdtypebool->MDId();
	pmdidBool->AddRef();
	CExpression *pexprExistsValue = GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CScalarIf(m_mp, pmdidBool),
		CUtils::PexprIsNotNull(
			m_mp, CUtils::PexprScalarIdent(m_mp, pcrMarker)),
		CUtils::PexprScalarConstBool(m_mp, !fNegated),
		CUtils::PexprScalarConstBool(m_mp, fNegated));
	CRefCount *pvalLowered = PvalReplaceNodeInSequence(
		m_mp, pmodel, (*pdrgpsym)[0], pexprSubquery, pexprExistsValue);
	pexprExistsValue->Release();

	CColRefArray *pdrgpcrMarkerAttrs =
		pexprMarkerList->DeriveUsedColumns()->Pdrgpcr(m_mp);
	CColRefArray *pdrgpcrMarkerSchema = GPOS_NEW(m_mp) CColRefArray(m_mp);
	pdrgpcrMarkerSchema->Append(pcrMarker);
	CExpression *pexprTrue = CUtils::PexprScalarConstBool(m_mp, true);
	CColRefArray *pdrgpcrLeft = GPOS_NEW(m_mp) CColRefArray(m_mp);
	CColRefArray *pdrgpcrRight = GPOS_NEW(m_mp) CColRefArray(m_mp);
	CColRefArray *pdrgpcrCorrelation =
		pexprInner->DeriveOuterReferences()->Pdrgpcr(m_mp);
	CColRefArray *pdrgpcrRequiredInner = GPOS_NEW(m_mp) CColRefArray(m_mp);
	pdrgpcrRequiredInner->Append(pcrMarker);
	pdrgpcrRequiredInner->Append(pcrsInnerOutput->PcrFirst());

	const BOOL fLowered = EdslsymPred == (*pdrgpsym)[1]->Esymkind()
		? pmodel->FBindDerived((*pdrgpsym)[1], pvalLowered)
		: pmodel->FBind((*pdrgpsym)[1], pvalLowered);
	const BOOL fMatches =
		fLowered &&
		pmodel->FBind((*pdrgpsym)[2], pexprMarkerList) &&
		pmodel->FBind((*pdrgpsym)[3], pdrgpcrMarkerAttrs) &&
		pmodel->FBind((*pdrgpsym)[4], pdrgpcrMarkerSchema) &&
		pmodel->FBind((*pdrgpsym)[5], pexprTrue) &&
		pmodel->FBind((*pdrgpsym)[6], pdrgpcrLeft) &&
		pmodel->FBind((*pdrgpsym)[7], pdrgpcrRight) &&
		pmodel->FBind((*pdrgpsym)[8], pdrgpcrCorrelation) &&
		pmodel->FBind((*pdrgpsym)[9], pdrgpcrRequiredInner) &&
		pmodel->FBind((*pdrgpsym)[10], pexprInner);
	pvalLowered->Release();
	pexprMarkerList->Release();
	pdrgpcrMarkerAttrs->Release();
	pdrgpcrMarkerSchema->Release();
	pexprTrue->Release();
	pdrgpcrLeft->Release();
	pdrgpcrRight->Release();
	pdrgpcrCorrelation->Release();
	pdrgpcrRequiredInner->Release();
	return fMatches;
}

BOOL
CDSLConstraintChecker::FCheckExprListQuantified(
	const CDSLConstraint *pcon, CDSLModel *pmodel, BOOL fAll) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (nullptr == pdrgpsym || 11 != pdrgpsym->Size() ||
		(EdslsymExpr != (*pdrgpsym)[0]->Esymkind() &&
		 EdslsymFunc != (*pdrgpsym)[0]->Esymkind() &&
		 EdslsymPred != (*pdrgpsym)[0]->Esymkind() &&
		 EdslsymWindow != (*pdrgpsym)[0]->Esymkind()) ||
		(*pdrgpsym)[0]->Esymkind() != (*pdrgpsym)[1]->Esymkind() ||
		EdslsymExpr != (*pdrgpsym)[2]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[3]->Esymkind() ||
		EdslsymSchema != (*pdrgpsym)[4]->Esymkind() ||
		EdslsymPred != (*pdrgpsym)[5]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[6]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[7]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[8]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[9]->Esymkind() ||
		EdslsymTable != (*pdrgpsym)[10]->Esymkind())
	{
		return false;
	}

	CExpression *pexprSubquery =
		PexprNextSubqueryInSequence(pmodel, (*pdrgpsym)[0]);
	const COperator::EOperatorId eopid =
		fAll ? COperator::EopScalarSubqueryAll
			 : COperator::EopScalarSubqueryAny;
	if (nullptr == pexprSubquery || eopid != pexprSubquery->Pop()->Eopid() ||
		2 != pexprSubquery->Arity())
	{
		return false;
	}

	CScalarSubqueryQuantified *popQuantified =
		CScalarSubqueryQuantified::PopConvert(pexprSubquery->Pop());
	CColRef *pcrInner = const_cast<CColRef *>(popQuantified->Pcr());
	CExpression *pexprInner = (*pexprSubquery)[0];

	const IMDTypeBool *pmdtypebool =
		COptCtxt::PoctxtFromTLS()->Pmda()->PtMDType<IMDTypeBool>();
	CColRef *pcrMarker = COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
		pmdtypebool, default_type_modifier);
	CExpressionArray *pdrgpexprMarker = GPOS_NEW(m_mp) CExpressionArray(m_mp);
	pdrgpexprMarker->Append(CUtils::PexprScalarProjectElement(
		m_mp, pcrMarker, CUtils::PexprScalarConstBool(m_mp, true)));
	CExpression *pexprMarkerList = GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CScalarProjectList(m_mp), pdrgpexprMarker);

	CExpression *pexprMarker = CUtils::PexprScalarIdent(m_mp, pcrMarker);
	CRefCount *pvalLowered = PvalReplaceNodeInSequence(
		m_mp, pmodel, (*pdrgpsym)[0], pexprSubquery, pexprMarker);
	pexprMarker->Release();

	CExpression *pexprComparison =
		CDSLQuantifiedMatcher::PexprComparison(m_mp, pexprSubquery);
	CColRefSet *pcrsLeft = GPOS_NEW(m_mp)
		CColRefSet(m_mp, *pexprComparison->DeriveUsedColumns());
	pcrsLeft->Exclude(pcrInner);
	CColRefArray *pdrgpcrLeft = pcrsLeft->Pdrgpcr(m_mp);
	pcrsLeft->Release();
	CColRefArray *pdrgpcrRight = GPOS_NEW(m_mp) CColRefArray(m_mp);
	pdrgpcrRight->Append(pcrInner);
	CColRefArray *pdrgpcrCorrelation =
		pexprInner->DeriveOuterReferences()->Pdrgpcr(m_mp);
	CColRefArray *pdrgpcrRequiredInner = GPOS_NEW(m_mp) CColRefArray(m_mp);
	pdrgpcrRequiredInner->Append(pcrMarker);
	pdrgpcrRequiredInner->Append(pcrInner);
	CColRefArray *pdrgpcrMarkerAttrs =
		pexprMarkerList->DeriveUsedColumns()->Pdrgpcr(m_mp);
	CColRefArray *pdrgpcrMarkerSchema = GPOS_NEW(m_mp) CColRefArray(m_mp);
	pdrgpcrMarkerSchema->Append(pcrMarker);

	const BOOL fLowered = EdslsymPred == (*pdrgpsym)[1]->Esymkind()
		? pmodel->FBindDerived((*pdrgpsym)[1], pvalLowered)
		: pmodel->FBind((*pdrgpsym)[1], pvalLowered);
	const BOOL fMatches =
		fLowered &&
		pmodel->FBind((*pdrgpsym)[2], pexprMarkerList) &&
		pmodel->FBind((*pdrgpsym)[3], pdrgpcrMarkerAttrs) &&
		pmodel->FBind((*pdrgpsym)[4], pdrgpcrMarkerSchema) &&
		pmodel->FBind((*pdrgpsym)[5], pexprComparison) &&
		pmodel->FBind((*pdrgpsym)[6], pdrgpcrLeft) &&
		pmodel->FBind((*pdrgpsym)[7], pdrgpcrRight) &&
		pmodel->FBind((*pdrgpsym)[8], pdrgpcrCorrelation) &&
		pmodel->FBind((*pdrgpsym)[9], pdrgpcrRequiredInner) &&
		pmodel->FBind((*pdrgpsym)[10], pexprInner);
	pvalLowered->Release();
	pexprMarkerList->Release();
	pdrgpcrMarkerAttrs->Release();
	pdrgpcrMarkerSchema->Release();
	pexprComparison->Release();
	pdrgpcrLeft->Release();
	pdrgpcrRight->Release();
	pdrgpcrCorrelation->Release();
	pdrgpcrRequiredInner->Release();
	return fMatches;
}
