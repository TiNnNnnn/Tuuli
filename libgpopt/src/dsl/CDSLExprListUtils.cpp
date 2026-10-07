//---------------------------------------------------------------------------
//	MONSOON DSL expression-list algebra
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLExprListUtils.h"
#include "gpopt/dsl/CDSLMatchView.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CFunctionProp.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/translate/CTranslatorExprToDXLUtils.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"

using namespace gpopt;

BOOL
CDSLScalarContext::Matches(const CDSLScalarContext *other) const
{
	return nullptr != other && m_path == other->m_path &&
		CDSLMatchView::FSameCapturedExpression(m_root, other->m_root);
}

CExpression *
CDSLScalarContext::PexprPlug(CMemoryPool *mp, CExpression *replacement) const
{
	return CDSLExprListUtils::PexprReplaceAt(mp, m_root, m_path, replacement);
}

namespace
{
CExpression *
PexprPlugPath(CMemoryPool *mp, CExpression *root,
	const std::vector<ULONG> &path, ULONG offset, CExpression *replacement)
{
	GPOS_CHECK_STACK_SIZE;
	if (offset == path.size())
	{
		replacement->AddRef();
		return replacement;
	}
	CExpressionArray *children = GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG i = 0; i < root->Arity(); ++i)
	{
		if (i == path[offset])
			children->Append(PexprPlugPath(mp, (*root)[i], path, offset + 1, replacement));
		else
		{
			(*root)[i]->AddRef();
			children->Append((*root)[i]);
		}
	}
	root->Pop()->AddRef();
	return GPOS_NEW(mp) CExpression(mp, root->Pop(), children);
}

BOOL
FReorderableProjectList(CExpression *pexpr)
{
	// Composition can reorder evaluation and move scalar work across an SRF.
	// Column independence alone does not preserve volatile calls or their count.
	return CDSLExprListUtils::FProjectList(pexpr) &&
		IMDFunction::EfsVolatile > pexpr->DeriveScalarFunctionProperties()->Efs();
}
}  // namespace

CExpression *
CDSLExprListUtils::PexprAtScalarPath(CExpression *root,
	const std::vector<ULONG> &path)
{
	if (nullptr == root || !root->Pop()->FScalar()) return nullptr;
	for (ULONG index : path)
	{
		if (index >= root->Arity()) return nullptr;
		root = (*root)[index];
		if (!root->Pop()->FScalar()) return nullptr;
	}
	return root;
}

CExpression *
CDSLExprListUtils::PexprReplaceAt(CMemoryPool *mp, CExpression *root,
	const std::vector<ULONG> &path, CExpression *replacement)
{
	CExpression *selected = PexprAtScalarPath(root, path);
	if (nullptr == selected || nullptr == replacement ||
		!CDSLMatchView::FScalarValue(selected) ||
		!CDSLMatchView::FScalarValue(replacement)) return nullptr;
	const auto *before = CScalar::PopConvert(selected->Pop());
	const auto *after = CScalar::PopConvert(replacement->Pop());
	if (!before->MdidType()->Equals(after->MdidType()) ||
		CDSLMatchView::ScalarValueTypeModifier(selected) !=
			CDSLMatchView::ScalarValueTypeModifier(replacement))
		return nullptr;
	return PexprPlugPath(mp, root, path, 0, replacement);
}

BOOL
CDSLExprListUtils::FProjectList(const CExpression *pexpr)
{
	return nullptr != pexpr &&
		COperator::EopScalarProjectList == pexpr->Pop()->Eopid();
}

BOOL
CDSLExprListUtils::FTypedProjectElement(const CExpression *pexpr)
{
	if (nullptr == pexpr ||
		COperator::EopScalarProjectElement != pexpr->Pop()->Eopid() ||
		1 != pexpr->Arity() || !CDSLMatchView::FScalarValue((*pexpr)[0]))
		return false;
	const CScalar *value = CScalar::PopConvert((*pexpr)[0]->Pop());
	return CScalarProjectElement::PopConvert(pexpr->Pop())->Pcr()->RetrieveType()->MDId()->Equals(
		value->MdidType());
}

BOOL
CDSLExprListUtils::FTypedProjectList(const CExpression *pexpr)
{
	if (!FProjectList(pexpr))
		return false;
	for (ULONG i = 0; i < pexpr->Arity(); ++i)
		if (!FTypedProjectElement((*pexpr)[i]))
			return false;
	// Output identities belong to the complete SELECT list, not individual
	// Items. DISTINCT removes duplicate rows, never duplicate definitions.
	return const_cast<CExpression *>(pexpr)->DeriveDefinedColumns()->Size() == pexpr->Arity();
}

BOOL
CDSLExprListUtils::FProjectListColumns(CMemoryPool *mp, CExpression *list,
	const CColRefArray *attrs, const CColRefArray *schema)
{
	if (!FProjectList(list) || nullptr == attrs || nullptr == schema ||
		list->Arity() != schema->Size())
		return false;
	for (ULONG i = 0; i < list->Arity(); ++i)
	{
		const CExpression *item = (*list)[i];
		if (COperator::EopScalarProjectElement != item->Pop()->Eopid() ||
			CScalarProjectElement::PopConvert(item->Pop())->Pcr() != (*schema)[i])
			return false;
	}
	CColRefSet *dependencies = GPOS_NEW(mp) CColRefSet(mp);
	dependencies->Include(attrs);
	const BOOL matches = dependencies->Equals(list->DeriveUsedColumns());
	dependencies->Release();
	return matches;
}

BOOL
CDSLExprListUtils::FRowScalar(CExpression *pexpr)
{
	GPOS_CHECK_STACK_SIZE;
	if (nullptr == pexpr || !pexpr->Pop()->FScalar() ||
		COperator::EopScalarAggFunc == pexpr->Pop()->Eopid() ||
		COperator::EopScalarWindowFunc == pexpr->Pop()->Eopid() ||
		pexpr->DeriveHasNonScalarFunction())
		return false;
	for (ULONG i = 0; i < pexpr->Arity(); ++i)
		if ((*pexpr)[i]->Pop()->FScalar() && !FRowScalar((*pexpr)[i]))
			return false;
	return true;
}

CExpressionArray *
CDSLExprListUtils::PdrgpexprFunctions(CMemoryPool *mp, CExpression *list)
{
	if (!FTypedProjectList(list)) return nullptr;
	CExpressionArray *functions = GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG i = 0; i < list->Arity(); ++i)
	{
		CExpression *function = (*(*list)[i])[0];
		BOOL valid = COperator::EopScalarAggFunc == function->Pop()->Eopid();
		// Replugging must not introduce a nested aggregate/window/SRF in an
		// argument. A scalar subquery remains a separate relational scope.
		for (ULONG j = 0; valid && j < function->Arity(); ++j)
			valid = FRowScalar((*function)[j]);
		if (!valid)
		{
			functions->Release();
			return nullptr;
		}
		function->AddRef();
		functions->Append(function);
	}
	return functions;
}

BOOL
CDSLExprListUtils::FComputeList(CExpression *pexpr)
{
	return FTypedProjectList(pexpr) && FRowScalar(pexpr) &&
		pexpr->DeriveDefinedColumns()->IsDisjoint(pexpr->DeriveUsedColumns());
}

BOOL
CDSLExprListUtils::FConcatSafe(CExpression *pexprUpper,
							   CExpression *pexprLower)
{
	return FReorderableProjectList(pexprUpper) && FReorderableProjectList(pexprLower) &&
		!(pexprUpper->DeriveHasNonScalarFunction() &&
		  pexprLower->DeriveHasNonScalarFunction());
}

CExpression *
CDSLExprListUtils::PexprNulls(CMemoryPool *mp, CColRefArray *columns)
{
	IDatumArray *datums = CTranslatorExprToDXLUtils::PdrgpdatumNulls(mp, columns);
	CExpression *list = CUtils::PexprScalarProjListConst(mp, columns, datums, nullptr);
	datums->Release();
	return list;
}

CColRefArray *
CDSLExprListUtils::PdrgpcrOutput(CMemoryPool *mp, CExpression *list)
{
	if (!FTypedProjectList(list)) return nullptr;
	CColRefArray *columns = GPOS_NEW(mp) CColRefArray(mp);
	for (ULONG i = 0; i < list->Arity(); ++i)
		columns->Append(CScalarProjectElement::PopConvert((*list)[i]->Pop())->Pcr());
	return columns;
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
	if (!FReorderableProjectList(pexprUpper) || !FReorderableProjectList(pexprLower))
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
