//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLMatcher.cpp
//
//	@doc:
//		Implementation of the generic recursive matcher (see CDSLMatcher.h).
//		Mirrors WeTune Match.matchOne's dispatch skeleton.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLMatcher.h"

#include "gpos/base.h"

#include "gpopt/base/CDistributionSpecHashed.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLAggMatcher.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/dsl/CDSLExistsMatcher.h"
#include "gpopt/dsl/CDSLExpressionDefinitions.h"
#include "gpopt/dsl/CDSLFilterMatcher.h"
#include "gpopt/dsl/CDSLInSubMatcher.h"
#include "gpopt/dsl/CDSLJoinMatcher.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/dsl/CDSLProjMatcher.h"
#include "gpopt/dsl/CDSLQuantifiedMatcher.h"
#include "gpopt/dsl/CDSLUnionMatcher.h"
#include "gpopt/operators/CLogicalCTEAnchor.h"
#include "gpopt/operators/CLogicalCTEConsumer.h"
#include "gpopt/operators/CLogicalConstTableGet.h"
#include "gpopt/operators/CLogicalSequenceProject.h"
#include "gpopt/operators/CScalarBooleanTest.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarSubqueryExists.h"
#include "gpopt/operators/CScalarSubquery.h"
#include "gpopt/operators/CScalarSubqueryQuantified.h"
#include "gpopt/operators/CScalarWindowFunc.h"
#include "gpopt/optimizer/COptimizerConfig.h"
#include "gpopt/xforms/CXformUtils.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;

namespace
{
BOOL FMatchExpressionBinding(CMemoryPool *mp, const CDSLExpressionDefinitions *definitions,
	const CDSLSymbol *symbol, CExpression *expression, CDSLModel *model, ULONG depth = 0);

BOOL
FMatchValueArguments(CMemoryPool *mp, const CDSLExpressionDefinitions *definitions,
	const CDSLSymbol *symbol, CExpressionArray *arguments, CDSLModel *model, ULONG depth)
{
	GPOS_CHECK_STACK_SIZE;
	if (depth > definitions->UlDefinitions())
		return false;
	const auto *existing = static_cast<CExpressionArray *>(model->PvalLookup(symbol));
	if (nullptr != existing)
	{
		if (existing->Size() != arguments->Size()) return false;
		for (ULONG i = 0; i < existing->Size(); ++i)
			if (!CDSLMatchView::FSameCapturedExpression((*existing)[i], (*arguments)[i])) return false;
	}
	else if (!model->FBind(symbol, arguments)) return false;
	const auto *def = definitions->Pdef(symbol);
	if (nullptr == def)
	{
		for (ULONG i = 0; i < arguments->Size(); ++i)
			if ((*arguments)[i]->DeriveHasSubquery()) return false;
		return true;
	}
	if (CDSLExpressionDefinitions::EMatch != def->Binding() || EdslexprArgs != def->Edslexpr())
		return false;
	if (0 == def->Arity()) return 0 == arguments->Size();
	if (0 == arguments->Size()) return false;
	CExpressionArray *tail = GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG i = 1; i < arguments->Size(); ++i)
	{
		(*arguments)[i]->AddRef();
		tail->Append((*arguments)[i]);
	}
	const BOOL matched = FMatchExpressionBinding(mp, definitions, def->PsymOperand(0),
		(*arguments)[0], model, depth + 1) && FMatchValueArguments(mp, definitions,
		def->PsymOperand(1), tail, model, depth + 1);
	tail->Release();
	return matched;
}

BOOL
FMatchExpressionBinding(CMemoryPool *mp, const CDSLExpressionDefinitions *definitions,
					   const CDSLSymbol *symbol, CExpression *expression,
					   CDSLModel *model, ULONG depth)
{
	GPOS_CHECK_STACK_SIZE;
	if (depth > definitions->UlDefinitions())
	{
		return false;
	}
	CExpression *existing = static_cast<CExpression *>(model->PvalLookup(symbol));
	if (nullptr != existing ? !CDSLMatchView::FSameCapturedExpression(existing, expression)
							: !model->FBind(symbol, expression))
	{
		return false;
	}
	const auto *def = definitions->Pdef(symbol);
	if (nullptr == def)
	{
		// Subqueries must be exposed by a typed constructor, not smuggled
		// through an opaque scalar capture. Its TABLE child stays opaque.
		return EdslsymTable == symbol->Esymkind() || !expression->DeriveHasSubquery();
	}
	if (CDSLExpressionDefinitions::EMatch != def->Binding())
		return false;
	if (EdslexprNot == def->Edslexpr() &&
		COperator::EopScalarSubqueryNotExists == expression->Pop()->Eopid() &&
		1 == expression->Arity())
	{
		// Native NOT EXISTS is the compact representation of Not(Exists(q)).
		(*expression)[0]->AddRef();
		CExpression *exists = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CScalarSubqueryExists(mp), (*expression)[0]);
		const BOOL matched = FMatchExpressionBinding(mp, definitions,
			def->PsymOperand(0), exists, model, depth + 1);
		exists->Release();
		return matched;
	}
	if (EdslexprExists == def->Edslexpr())
	{
		// Capture the complete native query, not its base tables or a flattened
		// view. Correlated column references and demand-sensitive operators stay.
		return COperator::EopScalarSubqueryExists == expression->Pop()->Eopid() &&
			1 == expression->Arity() && (*expression)[0]->Pop()->FLogical() &&
			FMatchExpressionBinding(mp, definitions, def->PsymOperand(0),
				(*expression)[0], model, depth + 1);
	}
	if (EdslexprCall == def->Edslexpr() || EdslexprCompare == def->Edslexpr())
	{
		if (!CDSLMatchView::FScalarCall(expression) ||
			(EdslexprCompare == def->Edslexpr() &&
				(COperator::EopScalarCmp != expression->Pop()->Eopid() ||
				 2 != expression->Arity()))) return false;
		const CDSLSymbol *head = def->PsymOperand(0);
		const auto *bound = static_cast<CExpression *>(model->PvalLookup(head));
		if (nullptr != bound ? !CDSLMatchView::FSameCallHead(bound, expression)
			: !model->FBind(head, expression)) return false;
		CExpressionArray *arguments = GPOS_NEW(mp) CExpressionArray(mp);
		for (ULONG i = 0; i < expression->Arity(); ++i)
		{
			(*expression)[i]->AddRef();
			arguments->Append((*expression)[i]);
		}
		const BOOL matched = FMatchValueArguments(mp, definitions, def->PsymOperand(1),
			arguments, model, depth + 1);
		arguments->Release();
		return matched;
	}
	if (EdslexprColumn == def->Edslexpr())
	{
		if (COperator::EopScalarIdent != expression->Pop()->Eopid() || 0 != expression->Arity())
			return false;
		CColRefArray *columns = GPOS_NEW(mp) CColRefArray(mp);
		columns->Append(const_cast<CColRef *>(CScalarIdent::PopConvert(expression->Pop())->Pcr()));
		const auto *bound = model->PdrgpcrAttrs(def->PsymOperand(0));
		const BOOL matched = nullptr != bound ? bound->Equals(columns)
			: model->FBind(def->PsymOperand(0), columns);
		columns->Release();
		return matched;
	}
	if (EdslexprScalarSubquery == def->Edslexpr())
	{
		if (COperator::EopScalarSubquery != expression->Pop()->Eopid() || 1 != expression->Arity())
			return false;
		const auto *op = CScalarSubquery::PopConvert(expression->Pop());
		if (op->FGeneratedByExist() || op->FGeneratedByQuantified() ||
			!CDSLMatchView::FSelectedSubqueryInput((*expression)[0], op->Pcr())) return false;
		CColRefArray *outputs = GPOS_NEW(mp) CColRefArray(mp);
		outputs->Append(const_cast<CColRef *>(op->Pcr()));
		const auto *bound = model->PdrgpcrAttrs(def->PsymOperand(0));
		const BOOL selected = nullptr != bound ? bound->Equals(outputs)
			: model->FBind(def->PsymOperand(0), outputs);
		outputs->Release();
		return selected && FMatchExpressionBinding(mp, definitions, def->PsymOperand(1),
			(*expression)[0], model, depth + 1);
	}
	if (EdslexprAny == def->Edslexpr() || EdslexprAll == def->Edslexpr())
	{
		const auto expected = EdslexprAny == def->Edslexpr()
			? COperator::EopScalarSubqueryAny : COperator::EopScalarSubqueryAll;
		if (expected != expression->Pop()->Eopid() || 2 != expression->Arity()) return false;
		CExpressionArray *arguments = GPOS_NEW(mp) CExpressionArray(mp);
		(*expression)[1]->AddRef();
		arguments->Append((*expression)[1]);
		const CDSLSymbol *head = def->PsymOperand(0);
		const auto *bound = static_cast<CExpression *>(model->PvalLookup(head));
		const auto *comparison = CScalarSubqueryQuantified::PopConvert(expression->Pop());
		CColRefArray *outputs = GPOS_NEW(mp) CColRefArray(mp);
		outputs->Append(const_cast<CColRef *>(comparison->Pcr()));
		const auto *bound_outputs = model->PdrgpcrAttrs(def->PsymOperand(2));
		// The constructor selects ANY/ALL; c captures the comparison and
		// its type signature in the same native form as Compare(c, args).
		// The selected column and complete query are independent captures.
		CExpression *comparison_head = CDSLQuantifiedMatcher::PexprComparison(mp, expression);
		const BOOL matched = CDSLMatchView::FQuantifiedInputs(expression, (*expression)[0], arguments, comparison->Pcr()) &&
			(nullptr != bound ?
				CDSLMatchView::FSameCallHead(bound, comparison_head)
				: model->FBind(head, comparison_head)) &&
			(nullptr != bound_outputs ? bound_outputs->Equals(outputs) : model->FBind(def->PsymOperand(2), outputs)) &&
			FMatchValueArguments(mp, definitions, def->PsymOperand(1), arguments, model, depth + 1) &&
			FMatchExpressionBinding(mp, definitions, def->PsymOperand(3), (*expression)[0], model, depth + 1);
		comparison_head->Release();
		outputs->Release();
		arguments->Release();
		return matched;
	}
	if (EdslexprBoolValue == def->Edslexpr() || EdslexprValueBool == def->Edslexpr())
	{
		// PostgreSQL Boolean values already are nullable scalar expressions.
		// Do not turn UNKNOWN into the filter-only "not true" interpretation.
		return IMDType::EtiBool == COptCtxt::PoctxtFromTLS()->Pmda()->RetrieveType(
			CScalar::PopConvert(expression->Pop())->MdidType())->GetDatumType() &&
			FMatchExpressionBinding(mp, definitions, def->PsymOperand(0), expression, model, depth + 1);
	}
	if (EdslexprCase == def->Edslexpr())
	{
		if (COperator::EopScalarIf != expression->Pop()->Eopid() || 3 != expression->Arity())
			return false;
		const auto *result = CScalar::PopConvert(expression->Pop());
		for (ULONG i = 1; i < 3; ++i)
			if (!result->MdidType()->Equals(CScalar::PopConvert((*expression)[i]->Pop())->MdidType()))
				return false;
		if (IMDType::EtiBool != COptCtxt::PoctxtFromTLS()->Pmda()->RetrieveType(
			CScalar::PopConvert((*expression)[0]->Pop())->MdidType())->GetDatumType())
			return false;
		for (ULONG i = 0; i < 3; ++i)
			if (!FMatchExpressionBinding(mp, definitions, def->PsymOperand(i),
				(*expression)[i], model, depth + 1))
				return false;
		return true;
	}
	if (EdslexprItem == def->Edslexpr())
	{
		if (COperator::EopScalarProjectList != expression->Pop()->Eopid() ||
			0 == expression->Arity() ||
			COperator::EopScalarProjectElement != (*expression)[0]->Pop()->Eopid() ||
			1 != (*expression)[0]->Arity())
			return false;
		CExpression *head = (*expression)[0];
		CColRefArray *columns = GPOS_NEW(mp) CColRefArray(mp);
		columns->Append(CScalarProjectElement::PopConvert(head->Pop())->Pcr());
		CColRefArray *bound = model->PdrgpcrAttrs(def->PsymOperand(1));
		BOOL matched = nullptr != bound ? bound->Equals(columns)
			: model->FBind(def->PsymOperand(1), columns);
		columns->Release();
		CExpressionArray *items = GPOS_NEW(mp) CExpressionArray(mp);
		for (ULONG i = 1; i < expression->Arity(); ++i)
		{
			(*expression)[i]->AddRef();
			items->Append((*expression)[i]);
		}
		CExpression *tail = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp), items);
		matched = matched &&
			FMatchExpressionBinding(mp, definitions, def->PsymOperand(0), (*head)[0], model, depth + 1) &&
			FMatchExpressionBinding(mp, definitions, def->PsymOperand(2), tail, model, depth + 1);
		tail->Release();
		return matched;
	}
	if (EdslexprNullSafeEq == def->Edslexpr())
	{
		CColRefArray *left = nullptr;
		CColRefArray *right = nullptr;
		if (!CDSLMatchView::FNullSafeEqColumns(mp, expression, &left, &right))
			return false;
		BOOL matched = true;
		for (ULONG i = 0; matched && i < 2; ++i)
		{
			CColRefArray *columns = 0 == i ? left : right;
			CColRefArray *bound = model->PdrgpcrAttrs(def->PsymOperand(i));
			matched = nullptr != bound ? bound->Equals(columns)
				: model->FBind(def->PsymOperand(i), columns);
		}
		left->Release();
		right->Release();
		return matched;
	}
	if (def->Arity() != expression->Arity() ||
		!(EdslexprNotTrue == def->Edslexpr()
			? COperator::EopScalarBooleanTest == expression->Pop()->Eopid() &&
				CScalarBooleanTest::EbtIsNotTrue ==
					CScalarBooleanTest::PopConvert(expression->Pop())->Ebt()
			: CUtils::FScalarBoolOp(expression, EdslexprAnd == def->Edslexpr()
			? CScalarBoolOp::EboolopAnd : EdslexprOr == def->Edslexpr()
			? CScalarBoolOp::EboolopOr : CScalarBoolOp::EboolopNot)))
	{
		return false;
	}
	for (ULONG i = 0; i < def->Arity(); i++)
	{
		if (!FMatchExpressionBinding(mp, definitions, def->PsymOperand(i),
				(*expression)[i], model, depth + 1))
			return false;
	}
	return true;
}

}  // namespace

BOOL
CDSLMatcher::FMatchPredicate(const CDSLSymbol *symbol, CExpression *expression,
							CDSLModel *model) const
{
	return FMatchExpression(symbol, expression, model);
}

BOOL
CDSLMatcher::FMatchExpression(const CDSLSymbol *symbol, CExpression *expression,
							 CDSLModel *model) const
{
	if (nullptr != m_prule && m_prule->Pexprdefs()->FHasBindings())
	{
		return FMatchExpressionBinding(m_mp, m_prule->Pexprdefs(), symbol, expression, model);
	}
	return model->FBind(symbol, expression);
}

BOOL
CDSLMatcher::FMatchSortView(const CDSLOp *popSort,
							CExpression *pexprChild, const COrderSpec *pos,
							CDSLModel *pmodel) const
{
	GPOS_ASSERT(EdslopSort == popSort->Edslop());
	if (1 != popSort->UlChildren() || nullptr == popSort->Pdrgpsym() ||
		1 != popSort->Pdrgpsym()->Size())
	{
		return false;
	}
	if (EdslsortSpec == popSort->Edslsort())
	{
		if (EdslsymOrder != (*popSort->Pdrgpsym())[0]->Esymkind())
			return false;
		COrderSpecArray *orders = GPOS_NEW(m_mp) COrderSpecArray(m_mp);
		const_cast<COrderSpec *>(pos)->AddRef();
		orders->Append(const_cast<COrderSpec *>(pos));
		const BOOL bound = pmodel->FBind((*popSort->Pdrgpsym())[0], orders);
		orders->Release();
		return bound && FMatch((*popSort)[0], pexprChild, pmodel);
	}
	if (popSort->Edslsort() != CDSLMatchView::EdslsortDefault(pos) ||
		EdslsymAttrs != (*popSort->Pdrgpsym())[0]->Esymkind())
		return false;

	CColRefArray *pdrgpcr = GPOS_NEW(m_mp) CColRefArray(m_mp);
	for (ULONG ul = 0; ul < pos->UlSortColumns(); ul++)
	{
		pdrgpcr->Append(const_cast<CColRef *>(pos->Pcr(ul)));
	}
	BOOL fBound = pmodel->FBind((*popSort->Pdrgpsym())[0], pdrgpcr);
	pdrgpcr->Release();
	return fBound && FMatch((*popSort)[0], pexprChild, pmodel);
}

BOOL
CDSLMatcher::FMatchOrderLimit(const CDSLOp *pop, CExpression *pexpr,
							  CDSLModel *pmodel) const
{
	CDSLMatchView::SOrderLimit view;
	if (!CDSLMatchView::FOrderLimit(pexpr, &view))
	{
		return false;
	}

	if (EdslopSort == pop->Edslop())
	{
		return !view.m_fHasLimit &&
			FMatchSortView(pop, view.m_pexprChild, view.m_pos, pmodel);
	}

	GPOS_ASSERT(EdslopLimit == pop->Edslop());
	if (!view.m_fHasLimit || 1 != pop->UlChildren() ||
		nullptr == pop->Pdrgpsym() || 2 != pop->Pdrgpsym()->Size())
	{
		return false;
	}

	// DSL positional order is Limit<count offset>; ORCA child order is
	// relational, offset, count.
	if (!pmodel->FBind((*pop->Pdrgpsym())[0], view.m_pexprCount) ||
		!pmodel->FBind((*pop->Pdrgpsym())[1], view.m_pexprOffset))
	{
		return false;
	}

	const CDSLOp *popChild = (*pop)[0];
	if (EdslopSort == popChild->Edslop())
	{
		// One fused ORCA node is the canonical view Limit(Sort(child)).
		return FMatchSortView(popChild, view.m_pexprChild, view.m_pos, pmodel);
	}

	// A plain DSL Limit carries no ordering. Do not silently consume an order
	// property which the target side would be unable to reconstruct.
	return view.m_pos->IsEmpty() &&
		   FMatch(popChild, view.m_pexprChild, pmodel);
}

BOOL
CDSLMatcher::FMatchWindow(const CDSLOp *pop, CExpression *pexpr,
					  CDSLModel *pmodel) const
{
	const BOOL fFrame = EdslopWindowFrame == pop->Edslop();
	if ((!fFrame && EdslopWindowRows != pop->Edslop()) ||
		COperator::EopLogicalSequenceProject != pexpr->Pop()->Eopid() ||
		2 != pexpr->Arity() || 1 != pop->UlChildren() ||
		nullptr == pop->Pdrgpsym() ||
		(fFrame ? 4 : 3) != pop->Pdrgpsym()->Size())
	{
		return false;
	}

	CLogicalSequenceProject *popWindow =
		CLogicalSequenceProject::PopConvert(pexpr->Pop());
	if (fFrame != popWindow->FHasFrameSpecs())
	{
		return false;
	}

	CColRefArray *pdrgpcrPartition = GPOS_NEW(m_mp) CColRefArray(m_mp);
	CDistributionSpec *pds = popWindow->Pds();
	if (CDistributionSpec::EdtHashed == pds->Edt())
	{
		CExpressionArray *pdrgpexpr =
			CDistributionSpecHashed::PdsConvert(pds)->Pdrgpexpr();
		for (ULONG ul = 0; ul < pdrgpexpr->Size(); ul++)
		{
			CExpression *pexprPart = (*pdrgpexpr)[ul];
			if (COperator::EopScalarIdent != pexprPart->Pop()->Eopid())
			{
				pdrgpcrPartition->Release();
				return false;
			}
			pdrgpcrPartition->Append(const_cast<CColRef *>(
				CScalarIdent::PopConvert(pexprPart->Pop())->Pcr()));
		}
	}
	else if (CDistributionSpec::EdtSingleton != pds->Edt())
	{
		pdrgpcrPartition->Release();
		return false;
	}

	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	const ULONG ulWindow = fFrame ? 3 : 2;
	BOOL fBound = pmodel->FBind((*pdrgpsym)[0], pdrgpcrPartition) &&
		pmodel->FBind((*pdrgpsym)[1], popWindow->Pdrgpos());
	pdrgpcrPartition->Release();
	if (fFrame)
	{
		fBound = fBound &&
			pmodel->FBind((*pdrgpsym)[2], popWindow->Pdrgpwf());
	}
	fBound = fBound && pmodel->FBind((*pdrgpsym)[ulWindow], (*pexpr)[1]);
	if (!fBound)
	{
		return false;
	}
	pexpr->AddRef();
	if (!pmodel->FSetWindowCarrier((*pdrgpsym)[ulWindow], pexpr))
	{
		return false;
	}
	return FMatch((*pop)[0], (*pexpr)[0], pmodel);
}

BOOL
CDSLMatcher::FMatchRowNumber(const CDSLOp *pop, CExpression *pexpr,
							  CDSLModel *pmodel) const
{
	if (EdslopRowNumber != pop->Edslop() || 1 != pop->UlChildren() ||
		nullptr == pop->Pdrgpsym() || 3 != pop->Pdrgpsym()->Size() ||
		COperator::EopLogicalSequenceProject != pexpr->Pop()->Eopid() ||
		2 != pexpr->Arity())
	{
		return false;
	}
	CLogicalSequenceProject *popWindow =
		CLogicalSequenceProject::PopConvert(pexpr->Pop());
	CExpression *pexprList = (*pexpr)[1];
	if (popWindow->FHasFrameSpecs() ||
		COperator::EopScalarProjectList != pexprList->Pop()->Eopid() ||
		1 != pexprList->Arity() ||
		COperator::EopScalarProjectElement != (*pexprList)[0]->Pop()->Eopid() ||
		1 != (*pexprList)[0]->Arity() ||
		COperator::EopScalarWindowFunc != (*(*pexprList)[0])[0]->Pop()->Eopid() ||
		0 != (*(*pexprList)[0])[0]->Arity())
	{
		return false;
	}
	CScalarWindowFunc *popFunc = CScalarWindowFunc::PopConvert(
		(*(*pexprList)[0])[0]->Pop());
	if (!IMDId::MDIdCompare(
			popFunc->FuncMdId(), COptCtxt::PoctxtFromTLS()
									->GetOptimizerConfig()
									->GetWindowOids()
									->MDIdRowNumber()))
	{
		return false;
	}

	CColRefArray *pdrgpcrPartition = GPOS_NEW(m_mp) CColRefArray(m_mp);
	CDistributionSpec *pds = popWindow->Pds();
	if (CDistributionSpec::EdtHashed == pds->Edt())
	{
		CExpressionArray *pdrgpexpr =
			CDistributionSpecHashed::PdsConvert(pds)->Pdrgpexpr();
		for (ULONG ul = 0; ul < pdrgpexpr->Size(); ul++)
		{
			CExpression *pexprPart = (*pdrgpexpr)[ul];
			if (COperator::EopScalarIdent != pexprPart->Pop()->Eopid())
			{
				pdrgpcrPartition->Release();
				return false;
			}
			pdrgpcrPartition->Append(const_cast<CColRef *>(
				CScalarIdent::PopConvert(pexprPart->Pop())->Pcr()));
		}
	}
	else if (CDistributionSpec::EdtSingleton != pds->Edt())
	{
		pdrgpcrPartition->Release();
		return false;
	}

	CColRefArray *pdrgpcrRank = GPOS_NEW(m_mp) CColRefArray(m_mp);
	pdrgpcrRank->Append(
		CScalarProjectElement::PopConvert((*pexprList)[0]->Pop())->Pcr());
	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	const BOOL fBound = pmodel->FBind((*pdrgpsym)[0], pdrgpcrPartition) &&
		pmodel->FBind((*pdrgpsym)[1], popWindow->Pdrgpos()) &&
		pmodel->FBind((*pdrgpsym)[2], pdrgpcrRank);
	pdrgpcrPartition->Release();
	pdrgpcrRank->Release();
	return fBound && FMatch((*pop)[0], (*pexpr)[0], pmodel);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLMatcher::FMatchInput
//
//	@doc:
//		Input<t> is an opaque table placeholder. WeTune's INPUT branch binds the
//		table symbol to whatever plan node sits there WITHOUT checking its type
//		— any relational subtree qualifies. So we bind the single <t> symbol to
//		the whole pexpr subtree (FBind AddRefs it). If <t> is already bound (same
//		symbol appears again under an equality class), FBind enforces it points at
//		the SAME subtree.
//---------------------------------------------------------------------------
BOOL
CDSLMatcher::FMatchInput(const CDSLOp *pop, CExpression *pexpr,
						 CDSLModel *pmodel) const
{
	GPOS_ASSERT(EdslopInput == pop->Edslop());

	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	// Input declares exactly one table symbol <t> (validated at parse time).
	if (nullptr == pdrgpsym || 1 != pdrgpsym->Size())
	{
		return false;
	}

	const CDSLSymbol *psymTable = (*pdrgpsym)[0];
	return pmodel->FBind(psymTable, pexpr);
}

BOOL
CDSLMatcher::FMatchEmpty(const CDSLOp *pop, CExpression *pexpr,
					 CDSLModel *pmodel) const
{
	GPOS_ASSERT(EdslopEmpty == pop->Edslop());
	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	if (nullptr == pdrgpsym || 1 != pdrgpsym->Size() ||
		COperator::EopLogicalConstTableGet != pexpr->Pop()->Eopid() ||
		0 != pexpr->Arity())
	{
		return false;
	}
	CLogicalConstTableGet *popConst =
		CLogicalConstTableGet::PopConvert(pexpr->Pop());
	return 0 == popConst->Pdrgpdrgpdatum()->Size() &&
		pmodel->FBind((*pdrgpsym)[0], pexpr);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLMatcher::FBindOpSymbols
//
//	@doc:
//		Bind the positional symbols of a non-Input operator against pexpr.
//
//		The generic skeleton knows how to bind NOTHING structural on its own:
//		  * symbol-free operators (Union / Exists) bind nothing here — success.
//		  * InnerJoin / LeftJoin / Proj / Agg carry symbols whose binding needs
//		    operator-specific structural knowledge (join-key extraction,
//		    project-list / group-by columns). Those are delegated to dedicated
//		    binding in a later component (#27):
//		        Join    <a a>            -> join-key binding
//		        Proj    <a s>            -> attrs/schema binding
//		        Agg     <a a f s p>      -> agg symbol binding
//		  * Filter <p a> is NOT handled here at all — it is intercepted earlier in
//		    FMatch and routed to CDSLFilterMatcher (#25), because a DSL Filter
//		    chain maps to a single ORCA Select, not a per-node match.
//
//		Until #27 lands this is a NO-OP seam: an operator with symbols still
//		matches structurally (identity + children), it simply leaves those
//		symbols unbound. That is deliberately safe for the skeleton's own tests
//		(Input, Union, bare identity) and is filled in by the later component
//		without touching the recursion here.
//---------------------------------------------------------------------------
BOOL
CDSLMatcher::FBindOpSymbols(const CDSLOp *pop,
							CExpression *,	// pexpr
							CDSLModel *		// pmodel
) const
{
	const ULONG ulSyms =
		(nullptr == pop->Pdrgpsym()) ? 0 : pop->Pdrgpsym()->Size();
	if (0 == ulSyms)
	{
		// Union / Exists and friends: purely structural, nothing to bind.
		return true;
	}

	// Operator carries symbols but no collaborator has claimed it yet. The
	// skeleton leaves them unbound (see doc). This is intentionally permissive;
	// #25/#27 replace this with real binding.
	return true;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLMatcher::FMatchChildren
//
//	@doc:
//		Match the DSL op's relational children positionally. Every ORCA logical
//		operator we map to lays out its relational inputs FIRST (indices
//		[0, UlChildren)) with scalar children (predicates, project lists) after;
//		the DSL op's UlChildren() is exactly that relational arity (WeTune
//		OpKind.numPredecessors). Scalar children are consumed by symbol binding,
//		not walked here.
//---------------------------------------------------------------------------
BOOL
CDSLMatcher::FMatchChildren(const CDSLOp *pop, CExpression *pexpr,
							CDSLModel *pmodel) const
{
	const ULONG ulChildren = pop->UlChildren();

	// the live expression must have at least as many children as the template
	// has relational children (it may have more: trailing scalar children).
	if (pexpr->Arity() < ulChildren)
	{
		return false;
	}

	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		if (!FMatch((*pop)[ul], (*pexpr)[ul], pmodel))
		{
			return false;
		}
	}
	return true;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLMatcher::FMatch
//
//	@doc:
//		Match one DSL op subtree against one live expression. Dispatch mirrors
//		WeTune Match.matchOne: Input is the opaque leaf; everything else is an
//		operator-identity gate followed by symbol binding + child recursion.
//---------------------------------------------------------------------------
BOOL
CDSLMatcher::FMatch(const CDSLOp *pop, CExpression *pexpr,
					CDSLModel *pmodel) const
{
	const ULONG ulDepth = m_ulMatchDepth++;
	const BOOL fMatched = FMatchInternal(pop, pexpr, pmodel);
	--m_ulMatchDepth;
	if (!fMatched && (!m_fHasFailure || ulDepth > m_ulFailureDepth))
	{
		m_fHasFailure = true;
		m_ulFailureDepth = ulDepth;
		m_edslopFailureExpected = pop->Edslop();
		m_szFailureActual = pexpr->Pop()->SzId();
	}
	return fMatched;
}

BOOL
CDSLMatcher::FMatchInternal(const CDSLOp *pop, CExpression *pexpr,
							CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != pop);
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != pmodel);

	// Input<t>: opaque subtree placeholder — bind and stop (no identity check,
	// no child recursion).
	if (EdslopInput == pop->Edslop())
	{
		return FMatchInput(pop, pexpr, pmodel);
	}
	if (EdslopEmpty == pop->Edslop())
	{
		return FMatchEmpty(pop, pexpr, pmodel);
	}
	if (EdslopCTEConsumer == pop->Edslop())
	{
		CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
		if (nullptr == pdrgpsym || 1 != pdrgpsym->Size() ||
			COperator::EopLogicalCTEConsumer != pexpr->Pop()->Eopid() ||
			0 != pexpr->Arity())
		{
			return false;
		}
		CLogicalCTEConsumer *popConsumer =
			CLogicalCTEConsumer::PopConvert(pexpr->Pop());
		const ULONG ulCTEId = popConsumer->UlCTEId();
		CCTEInfo *pcteinfo = COptCtxt::PoctxtFromTLS()->Pcteinfo();
		return (pcteinfo->FEnableInlining() ||
				1 == pcteinfo->UlConsumers(ulCTEId) ||
				pcteinfo->HasOuterReferences(ulCTEId)) &&
			CXformUtils::FInlinableCTE(ulCTEId) &&
			pmodel->FBind((*pdrgpsym)[0], popConsumer->PexprInlined());
	}
	if (EdslopCTEAnchor == pop->Edslop())
	{
		if (COperator::EopLogicalCTEAnchor != pexpr->Pop()->Eopid() ||
			1 != pexpr->Arity())
		{
			return false;
		}
		CLogicalCTEAnchor *popAnchor =
			CLogicalCTEAnchor::PopConvert(pexpr->Pop());
		CCTEInfo *pcteinfo = COptCtxt::PoctxtFromTLS()->Pcteinfo();
		const ULONG ulConsumers = pcteinfo->UlConsumers(popAnchor->Id());
		CGroupExpression *pgexprOrigin = pexpr->Pgexpr();
		if (0 == ulConsumers ||
			(!pcteinfo->FEnableInlining() && 1 != ulConsumers &&
			 0 == pexpr->DeriveOuterReferences()->Size()) ||
			!CXformUtils::FInlinableCTE(popAnchor->Id()) ||
			(!GPOS_FTRACE(EopttraceExpandFullJoin) &&
			 nullptr != pgexprOrigin &&
			 CXform::ExfExpandFullOuterJoin == pgexprOrigin->ExfidOrigin()))
		{
			return false;
		}
	}

	// Filter chain: a DSL single-predicate Filter (chain) matches ONE ORCA
	// CLogicalSelect whose conjunctive predicate is split into a conjunct set.
	// Delegated to the filter matcher, which recurses the chain's base op back
	// into this matcher (see CDSLFilterMatcher). This is the hardest mismatch
	// (doc §2) and is why Filter does not go through the generic child recursion.
	if (EdslopFilter == pop->Edslop())
	{
		if (nullptr != m_prule && m_prule->Pexprdefs()->FHasBindings())
		{
			// Oriented scalar patterns match the actual tree. In particular,
			// do not split/reorder predicates through the legacy filter views.
			CDSLSymbolArray *symbols = pop->Pdrgpsym();
			if (COperator::EopLogicalSelect != pexpr->Pop()->Eopid() ||
				2 != pexpr->Arity() ||
				(2 != symbols->Size() && 3 != symbols->Size()) ||
				(2 == symbols->Size() &&
				 !(*pexpr)[0]->DeriveOutputColumns()->ContainsAll(
					 (*pexpr)[1]->DeriveUsedColumns())))
			{
				return false;
			}
			for (ULONG i = 1; i < symbols->Size(); i++)
			{
				CColRefArray *columns = CDSLFilterMatcher::PdrgpcrDependencies(
					m_mp, pop, (*pexpr)[1], (*pexpr)[0], i);
				const BOOL bound = pmodel->FBind((*symbols)[i], columns);
				columns->Release();
				if (!bound)
					return false;
			}
			return FMatchPredicate((*symbols)[0], (*pexpr)[1], pmodel) &&
				FMatch((*pop)[0], (*pexpr)[0], pmodel);
		}
		CDSLFilterMatcher fm(m_mp, this, m_prule);
		return fm.FMatch(pop, pexpr, pmodel);
	}

	// Proj<a s>: bind the projected-column symbols against the live
	// CLogicalProject's project list, then recurse the relational child. Like
	// Filter, Proj carries scalar structure (the project list) that only
	// operator-specific code reads, so it does not go through generic child
	// recursion (see CDSLProjMatcher, doc M1).
	//
	// Proj* (deduplicated projection) has NO CLogicalProject counterpart in ORCA
	// — SELECT DISTINCT becomes a CLogicalGbAgg (empty agg list). So a DISTINCT
	// Proj uses the Agg compatibility matcher for legacy rules; new expression
	// bindings use the exact DISTINCT projection adapter instead.
	if (EdslopProj == pop->Edslop())
	{
		if (pop->FDistinct())
		{
			if (nullptr != m_prule && m_prule->Pexprdefs()->FHasBindings())
			{
				CDSLProjMatcher pm(m_mp, this);
				return pm.FMatchDistinct(pop, pexpr, pmodel);
			}
			CDSLAggMatcher am(m_mp, this);
			return am.FMatch(pop, pexpr, pmodel);
		}
		CDSLProjMatcher pm(m_mp, this);
		return pm.FMatch(pop, pexpr, pmodel);
	}

	// Compute<e a s> names ORCA's actual ComputeScalar/LET node. It shares the
	// Project shell with Proj, but deliberately bypasses every Proj compatibility
	// view and captures the complete scalar project list under <e>.
	if (EdslopCompute == pop->Edslop())
	{
		CDSLProjMatcher pm(m_mp, this);
		return pm.FMatchCompute(pop, pexpr, pmodel);
	}

	// Corpus Agg<a a f s p> and the six-symbol extension route to the Agg
	// matcher, including ORCA's Select-over-GbAgg representation of HAVING.
	if (EdslopAgg == pop->Edslop())
	{
		CDSLAggMatcher am(m_mp, this);
		return am.FMatch(pop, pexpr, pmodel);
	}

	// EXISTS in a filter context is normalized by ORCA into a LeftSemiApply.
	// Its uncorrelated inner LIMIT 1 is an implementation detail hidden from the
	// two-child DSL operator.
	if (EdslopExists == pop->Edslop() ||
		EdslopNotExists == pop->Edslop())
	{
		CDSLExistsMatcher em(m_mp, this);
		return em.FMatch(pop, pexpr, pmodel);
	}

	if (EdslopInSubFilter == pop->Edslop())
	{
		CDSLInSubMatcher ism(m_mp, this);
		return ism.FMatch(pop, pexpr, pmodel);
	}

	if (EdslopAny == pop->Edslop() || EdslopAll == pop->Edslop())
	{
		CDSLQuantifiedMatcher qm(m_mp, this);
		return qm.FMatch(pop, pexpr, pmodel);
	}

	if (EdslopUnion == pop->Edslop() || EdslopIntersect == pop->Edslop() ||
		EdslopExcept == pop->Edslop())
	{
		CDSLUnionMatcher um(m_mp, this);
		return um.FMatch(pop, pexpr, pmodel);
	}

	if (EdslopSort == pop->Edslop() || EdslopLimit == pop->Edslop())
	{
		return FMatchOrderLimit(pop, pexpr, pmodel);
	}

	if (EdslopWindowRows == pop->Edslop() ||
		EdslopWindowFrame == pop->Edslop())
	{
		return FMatchWindow(pop, pexpr, pmodel);
	}
	if (EdslopRowNumber == pop->Edslop())
	{
		return FMatchRowNumber(pop, pexpr, pmodel);
	}

	// AssertMaxOneRow is a target-only semantic macro. Its live ORCA shape is
	// intentionally not reverse-matched as a generic Assert because the scalar
	// predicate and row-number shell must remain an indivisible implementation.
	if (EdslopAssertMaxOneRow == pop->Edslop())
	{
		return false;
	}

	// InnerJoin/LeftJoin<a a>: bind the equi-join key columns to the two <a>
	// symbols, keep non-equi conjuncts as residual, and recurse both relational
	// children. Like Filter/Proj, the join predicate (child[2]) is scalar structure
	// only operator-specific code reads, so join does not go through generic child
	// recursion (see CDSLJoinMatcher, doc M2).
	if (EdslopInnerJoin == pop->Edslop() || EdslopLeftJoin == pop->Edslop() ||
		EdslopFullJoin == pop->Edslop() ||
		EdslopSemiJoin == pop->Edslop() ||
		EdslopSemiApply == pop->Edslop() ||
		EdslopAntiJoin == pop->Edslop() ||
		EdslopAntiApply == pop->Edslop() ||
		EdslopAntiJoinNotIn == pop->Edslop() ||
		EdslopAntiApplyNotIn == pop->Edslop() ||
		EdslopInnerApply == pop->Edslop() ||
		EdslopLeftOuterApply == pop->Edslop())
	{
		CDSLJoinMatcher jm(m_mp, this, m_prule);
		return jm.FMatch(pop, pexpr, pmodel);
	}

	// operator-identity gate for directly represented logical operators.
	const COperator::EOperatorId eopidTemplate = pop->Eopid();
	if (COperator::EopSentinel == eopidTemplate ||
		eopidTemplate != pexpr->Pop()->Eopid())
	{
		return false;
	}

	// bind this node's own symbols (delegated per operator), then recurse into
	// its relational children.
	if (!FBindOpSymbols(pop, pexpr, pmodel))
	{
		return false;
	}
	return FMatchChildren(pop, pexpr, pmodel);
}

// EOF
