//---------------------------------------------------------------------------
// MONSOON DSL rule engine — Compute construction and shared SELECT scopes.
// These checks validate native construction, not FormalSQL transport evidence.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLExprListUtils.h"
#include "gpopt/dsl/CDSLExpressionDefinitions.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "../CDSLInstantiatorUtils.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CScalarProjectElement.h"

using namespace gpopt;
using namespace gpopt::dslinstantiator;

namespace
{
// Recover a SELECT-list owner only from exact dependencies and ordered output
// identities, or an unchanged captured Item tail. Rebuilt values cannot borrow
// a tail's scope merely because their dependency sets happen to agree.
BOOL
FFindProjectListCarrier(const CDSLOp *source, const CDSLModel *model,
					const CColRefArray *outputs, const CColRefSet *used,
					CExpression **carrier, const CExpression *captured = nullptr)
{
	GPOS_CHECK_STACK_SIZE;
	const BOOL compute = EdslopCompute == source->Edslop();
	if (compute || (EdslopProj == source->Edslop() &&
		nullptr != source->Pdrgpsym() && 3 == source->Pdrgpsym()->Size()))
	{
		CExpression *candidate = model->PexprProjectListCarrier(
			(*source->Pdrgpsym())[compute ? 0 : 2]);
		BOOL matches = nullptr != candidate &&
			CColRef::Equals(outputs, model->PdrgpcrSchema((*source->Pdrgpsym())[compute ? 2 : 1])) &&
			(*candidate)[1]->DeriveUsedColumns()->Equals(used);
		if (!matches && nullptr != candidate && nullptr != captured &&
			CDSLExprListUtils::FProjectList(captured) &&
			0 < captured->Arity() && captured->Arity() < (*candidate)[1]->Arity())
		{
			const ULONG offset = (*candidate)[1]->Arity() - captured->Arity();
			matches = true;
			for (ULONG i = 0; matches && i < captured->Arity(); ++i)
				matches = CDSLMatchView::FSameCapturedExpression(
					(*captured)[i], (*(*candidate)[1])[offset + i]);
		}
		if (matches)
		{
			if (nullptr != *carrier && !(*carrier)->Matches(candidate)) return false;
			*carrier = candidate;
		}
	}
	for (ULONG i = 0; i < source->UlChildren(); ++i)
		if (!FFindProjectListCarrier((*source)[i], model, outputs, used, carrier, captured)) return false;
	return true;
}
}  // namespace

BOOL
CDSLInstantiator::FProjectListScope(const CDSLSymbol *psymExpr,
	const CDSLModel *pmodel, CExpression *pexprList, CExpression *pexprChild,
	const CColRefArray *pdrgpcrAttrs, const CColRefArray *pdrgpcrSchema) const
{
	CColRefSet *available = GPOS_NEW(m_mp) CColRefSet(m_mp);
	available->Include(pexprChild->DeriveOutputColumns());
	CExpression *carrier = m_prule->Pexprdefs()->FHasBindings()
		? pmodel->PexprProjectListCarrier(psymExpr) : nullptr;
	BOOL scope_valid = true;
	// Closed programs need no column scope. Several empty source lists may
	// have identical metadata without a unique owner.
	if (nullptr == carrier && m_prule->Pexprdefs()->FHasBindings() &&
		0 != pexprList->DeriveUsedColumns()->Size())
	{
		// Item construction need not alias the old list or metadata symbols.
		// Recover its captured scope from exact dependencies and ordered output
		// identities, including metadata derived from the rebuilt list. Reject
		// ambiguous owners; metadata alone never authorizes an external column.
		scope_valid = FFindProjectListCarrier(m_prule->PfragSrc()->PopRoot(), pmodel,
			pdrgpcrSchema, pexprList->DeriveUsedColumns(), &carrier);
	}
	if (nullptr != carrier)
	{
		// Only captured outer references may remain external; a dropped local
		// column must not silently turn into a correlation. Conversely, a new
		// child must not shadow a captured external column.
		CColRefSet *outer = GPOS_NEW(m_mp) CColRefSet(m_mp);
		outer->Include((*carrier)[1]->DeriveUsedColumns());
		outer->Exclude((*carrier)[0]->DeriveOutputColumns());
		scope_valid = scope_valid && outer->IsDisjoint(pexprChild->DeriveOutputColumns());
		available->Include(outer);
		outer->Release();
	}
	else if (m_prule->Pexprdefs()->FHasBindings())
	{
		// A composed list has several source scopes, not one arbitrarily chosen
		// carrier. Validate each captured list before combining its outer refs.
		const auto addScopes = [&](const auto &self, const CDSLSymbol *symbol) -> BOOL {
			GPOS_CHECK_STACK_SIZE;
			symbol = PsymResolve(symbol);
			CExpression *source = pmodel->PexprProjectListCarrier(symbol);
			if (nullptr == source)
			{
				CExpression *segment = PexprResolveExpr(symbol, pmodel);
				CColRefArray *outputs = CDSLExprListUtils::PdrgpcrOutput(m_mp, segment);
				const BOOL found = nullptr == outputs ||
					0 == segment->DeriveUsedColumns()->Size() ||
					FFindProjectListCarrier(m_prule->PfragSrc()->PopRoot(), pmodel,
						outputs, segment->DeriveUsedColumns(), &source,
						EdslsideSource == symbol->Eside()
							? pmodel->PexprExpr(symbol) : nullptr);
				CRefCount::SafeRelease(outputs);
				CRefCount::SafeRelease(segment);
				if (!found) return false;
			}
			if (nullptr != source)
			{
				CColRefSet *outer = GPOS_NEW(m_mp) CColRefSet(m_mp);
				outer->Include((*source)[1]->DeriveUsedColumns());
				outer->Exclude((*source)[0]->DeriveOutputColumns());
				// A removed Context occurrence no longer reads its old outer
				// column. Retained uses still face the same shadowing check.
				outer->Intersection(pexprList->DeriveUsedColumns());
				CColRefSet *local = GPOS_NEW(m_mp) CColRefSet(m_mp);
				local->Include((*source)[1]->DeriveUsedColumns());
				// A replaced occurrence may no longer use its original column.
				local->Intersection(pexprList->DeriveUsedColumns());
				local->Exclude(outer);
				local->Exclude(pexprChild->DeriveOutputColumns());
				const BOOL valid = 0 == local->Size() &&
					outer->IsDisjoint(pexprChild->DeriveOutputColumns());
				if (valid) available->Include(outer);
				local->Release();
				outer->Release();
				return valid;
			}
			const auto *definition = m_prule->Pexprdefs()->Pdef(symbol);
			// Replugging changes an occurrence, not the captured list's scope.
			if (nullptr != definition && EdslexprContext == definition->Edslexpr() &&
				CDSLExpressionDefinitions::EBuild == definition->Binding())
				return self(self, definition->PsymOperand(0));
			if (nullptr != definition && EdslexprItem == definition->Edslexpr() &&
				3 == definition->Arity() &&
				CDSLExpressionDefinitions::EBuild == definition->Binding())
				return self(self, definition->PsymOperand(2));
			if (nullptr == definition || EdslexprConcat != definition->Edslexpr())
				return true; // Uncaptured columns still face the final scope check.
			return self(self, definition->PsymOperand(0)) &&
				self(self, definition->PsymOperand(1));
		};
		scope_valid = scope_valid && addScopes(addScopes, psymExpr);
	}
	scope_valid = scope_valid && FColSetContainsArray(available, pdrgpcrAttrs);
	available->Release();
	return scope_valid;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildCompute
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildCompute(const CDSLOp *pop,
								 const CDSLModel *pmodel) const
{
	if (1 != pop->UlChildren() || nullptr == pop->Pdrgpsym() ||
		3 != pop->Pdrgpsym()->Size())
	{
		return nullptr;
	}

	const CDSLSymbol *psymExpr = PsymResolve((*pop->Pdrgpsym())[0]);
	const CDSLSymbol *psymAttrs = PsymResolve((*pop->Pdrgpsym())[1]);
	const CDSLSymbol *psymSchema = PsymResolve((*pop->Pdrgpsym())[2]);
	CExpression *pexprList = PexprResolveExpr(psymExpr, pmodel);
	CColRefArray *pdrgpcrAttrs =
		PdrgpcrResolveCols(psymAttrs, pmodel);
	CColRefArray *pdrgpcrSchema =
		PdrgpcrResolveCols(psymSchema, pmodel);
	BOOL fOwnAttrs = false;
	BOOL fOwnSchema = false;
	if (nullptr != pexprList && nullptr == pdrgpcrAttrs)
	{
		pdrgpcrAttrs = pexprList->DeriveUsedColumns()->Pdrgpcr(m_mp);
		fOwnAttrs = true;
	}
	if (nullptr != pexprList && nullptr == pdrgpcrSchema &&
		COperator::EopScalarProjectList == pexprList->Pop()->Eopid())
	{
		pdrgpcrSchema = GPOS_NEW(m_mp) CColRefArray(m_mp);
		fOwnSchema = true;
		for (ULONG ul = 0; ul < pexprList->Arity(); ul++)
		{
			CExpression *pexprElem = (*pexprList)[ul];
			if (COperator::EopScalarProjectElement !=
				pexprElem->Pop()->Eopid())
			{
				pdrgpcrSchema->Release();
				pdrgpcrSchema = nullptr;
				fOwnSchema = false;
				break;
			}
			pdrgpcrSchema->Append(
				CScalarProjectElement::PopConvert(pexprElem->Pop())->Pcr());
		}
	}
	if (!CDSLExprListUtils::FProjectListColumns(
		m_mp, pexprList, pdrgpcrAttrs, pdrgpcrSchema))
	{
		if (fOwnAttrs)
		{
			pdrgpcrAttrs->Release();
		}
		if (fOwnSchema)
		{
			pdrgpcrSchema->Release();
		}
		CRefCount::SafeRelease(pexprList);
		return nullptr;
	}

	CExpression *pexprChild = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprChild)
	{
		if (fOwnAttrs)
		{
			pdrgpcrAttrs->Release();
		}
		if (fOwnSchema)
		{
			pdrgpcrSchema->Release();
		}
		pexprList->Release();
		return nullptr;
	}
	const BOOL scope_valid = FProjectListScope(psymExpr, pmodel, pexprList,
		pexprChild, pdrgpcrAttrs, pdrgpcrSchema);
	if (!scope_valid ||
		(m_prule->Pexprdefs()->FHasBindings() &&
		 (!CDSLExprListUtils::FComputeList(pexprList) ||
		  !pexprChild->DeriveOutputColumns()->IsDisjoint(pexprList->DeriveDefinedColumns()))))
	{
		pexprChild->Release();
		if (fOwnAttrs)
		{
			pdrgpcrAttrs->Release();
		}
		if (fOwnSchema)
		{
			pdrgpcrSchema->Release();
		}
		pexprList->Release();
		return nullptr;
	}

	if (fOwnAttrs)
	{
		pdrgpcrAttrs->Release();
	}
	if (fOwnSchema)
	{
		pdrgpcrSchema->Release();
	}
	return PexprProjectWithoutSelfAliases(m_mp, pexprChild, pexprList);
}
