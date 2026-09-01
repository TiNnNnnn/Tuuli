//---------------------------------------------------------------------------
//	MONSOON DSL expression-list algebra
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLExprListUtils.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CScalarProjectList.h"

using namespace gpopt;

BOOL
CDSLExprListUtils::FProjectList(const CExpression *pexpr)
{
	return nullptr != pexpr &&
		COperator::EopScalarProjectList == pexpr->Pop()->Eopid();
}

BOOL
CDSLExprListUtils::FConcatSafe(CExpression *pexprUpper,
							   CExpression *pexprLower)
{
	return FProjectList(pexprUpper) && FProjectList(pexprLower) &&
		!(pexprUpper->DeriveHasNonScalarFunction() &&
		  pexprLower->DeriveHasNonScalarFunction());
}

BOOL
CDSLExprListUtils::FDepsDisjoint(CMemoryPool *mp, CExpression *pexprList,
								 CColRefArray *pdrgpcrSchema)
{
	if (!FProjectList(pexprList) || nullptr == pdrgpcrSchema)
	{
		return false;
	}
	CColRefSet *pcrsSchema = GPOS_NEW(mp) CColRefSet(mp);
	pcrsSchema->Include(pdrgpcrSchema);
	pcrsSchema->Intersection(pexprList->DeriveUsedColumns());
	const BOOL fDisjoint = 0 == pcrsSchema->Size();
	pcrsSchema->Release();
	return fDisjoint;
}

CExpression *
CDSLExprListUtils::PexprConcat(CMemoryPool *mp, CExpression *pexprUpper,
								   CExpression *pexprLower)
{
	if (!FConcatSafe(pexprUpper, pexprLower))
	{
		return nullptr;
	}
	CExpressionArray *pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	// Keep an upper SRF cohort together and after ordinary upper elements. This
	// is the canonical order used by CUtils::PexprCollapseProjects.
	for (ULONG ul = 0; ul < pexprUpper->Arity(); ul++)
	{
		CExpression *pexprElem = (*pexprUpper)[ul];
		if (pexprElem->DeriveHasNonScalarFunction())
		{
			continue;
		}
		pexprElem->AddRef();
		pdrgpexpr->Append(pexprElem);
	}
	for (ULONG ul = 0; ul < pexprUpper->Arity(); ul++)
	{
		CExpression *pexprElem = (*pexprUpper)[ul];
		if (!pexprElem->DeriveHasNonScalarFunction())
		{
			continue;
		}
		pexprElem->AddRef();
		pdrgpexpr->Append(pexprElem);
	}
	for (ULONG ul = 0; ul < pexprLower->Arity(); ul++)
	{
		CExpression *pexprElem = (*pexprLower)[ul];
		pexprElem->AddRef();
		pdrgpexpr->Append(pexprElem);
	}
	return GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexpr);
}

BOOL
CDSLExprListUtils::FSplit(CMemoryPool *mp, CExpression *pexprUpper,
						  CExpression *pexprLower,
						  CExpression **ppexprMerged,
						  CExpression **ppexprResidual)
{
	GPOS_ASSERT(nullptr != ppexprMerged && nullptr != ppexprResidual);
	*ppexprMerged = nullptr;
	*ppexprResidual = nullptr;
	if (!FProjectList(pexprUpper) || !FProjectList(pexprLower))
	{
		return false;
	}

	CColRefSet *pcrsLowerDefined = GPOS_NEW(mp)
		CColRefSet(mp, *pexprLower->DeriveDefinedColumns());
	ULONG ulUpperSrf = 0;
	ULONG ulIndependentSrf = 0;
	for (ULONG ul = 0; ul < pexprUpper->Arity(); ul++)
	{
		CExpression *pexprElem = (*pexprUpper)[ul];
		if (!pexprElem->DeriveHasNonScalarFunction())
		{
			continue;
		}
		ulUpperSrf++;
		CColRefSet *pcrsUsed = GPOS_NEW(mp)
			CColRefSet(mp, *pexprElem->DeriveUsedColumns());
		pcrsUsed->Intersection(pcrsLowerDefined);
		if (0 == pcrsUsed->Size())
		{
			ulIndependentSrf++;
		}
		pcrsUsed->Release();
	}
	const BOOL fMoveSrfCohort =
		!pexprLower->DeriveHasNonScalarFunction() &&
		ulUpperSrf == ulIndependentSrf;

	CExpressionArray *pdrgpexprMoved = GPOS_NEW(mp) CExpressionArray(mp);
	CExpressionArray *pdrgpexprResidual = GPOS_NEW(mp) CExpressionArray(mp);
	CExpressionArray *pdrgpexprSrf = GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < pexprUpper->Arity(); ul++)
	{
		CExpression *pexprElem = (*pexprUpper)[ul];
		if (pexprElem->DeriveHasNonScalarFunction())
		{
			pexprElem->AddRef();
			pdrgpexprSrf->Append(pexprElem);
			continue;
		}
		CColRefSet *pcrsUsed = GPOS_NEW(mp)
			CColRefSet(mp, *pexprElem->DeriveUsedColumns());
		pcrsUsed->Intersection(pcrsLowerDefined);
		const BOOL fIndependent = 0 == pcrsUsed->Size();
		pcrsUsed->Release();
		pexprElem->AddRef();
		(fIndependent ? pdrgpexprMoved : pdrgpexprResidual)->Append(pexprElem);
	}
	pcrsLowerDefined->Release();
	CExpressionArray *pdrgpexprSrfDest =
		fMoveSrfCohort ? pdrgpexprMoved : pdrgpexprResidual;
	for (ULONG ul = 0; ul < pdrgpexprSrf->Size(); ul++)
	{
		CExpression *pexprElem = (*pdrgpexprSrf)[ul];
		pexprElem->AddRef();
		pdrgpexprSrfDest->Append(pexprElem);
	}
	pdrgpexprSrf->Release();

	// A split is meaningful only when it changes the lower layer and leaves a
	// real residual upper layer. The all-movable case is represented by the
	// simpler DepsDisjoint + ExprConcat rule.
	if (0 == pdrgpexprMoved->Size() || 0 == pdrgpexprResidual->Size())
	{
		pdrgpexprMoved->Release();
		pdrgpexprResidual->Release();
		return false;
	}
	for (ULONG ul = 0; ul < pexprLower->Arity(); ul++)
	{
		CExpression *pexprElem = (*pexprLower)[ul];
		pexprElem->AddRef();
		pdrgpexprMoved->Append(pexprElem);
	}
	*ppexprMerged = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprMoved);
	*ppexprResidual = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprResidual);
	return true;
}
