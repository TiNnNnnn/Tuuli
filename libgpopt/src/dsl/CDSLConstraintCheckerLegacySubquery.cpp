//---------------------------------------------------------------------------
// Compatibility for PredicateAny/All captures. Typed Any/All uses the matcher.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLMatchView.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/dsl/CDSLQuantifiedMatcher.h"
#include "gpopt/operators/CScalarSubqueryQuantified.h"

#include <initializer_list>

using namespace gpopt;
using namespace gpnaucrates;

namespace
{
BOOL
FSameSubqueryOutput(EDslSymbolKind kind, const CRefCount *left,
					const CRefCount *right)
{
	if (left == right) return true;
	switch (kind)
	{
		case EdslsymAttrs:
		case EdslsymSchema:
			return static_cast<const CColRefArray *>(left)->Equals(
				static_cast<const CColRefArray *>(right));
		case EdslsymTable:
		case EdslsymPred:
		case EdslsymExpr:
		case EdslsymWindow:
			return CDSLMatchView::FSameCapturedExpression(
				static_cast<const CExpression *>(left), static_cast<const CExpression *>(right));
		case EdslsymFunc:
		{
			const auto *l = static_cast<const CExpressionArray *>(left);
			const auto *r = static_cast<const CExpressionArray *>(right);
			if (l->Size() != r->Size()) return false;
			for (ULONG i = 0; i < l->Size(); ++i)
				if (!CDSLMatchView::FSameCapturedExpression((*l)[i], (*r)[i])) return false;
			return true;
		}
		default:
			return false;
	}
}

// Slot zero is the source. Validate every output (including repeated symbols)
// before publishing any, preserving existing captures and predicate provenance.
BOOL
FBindSubqueryOutputs(const CDSLSymbolArray *symbols, CDSLModel *model,
	std::initializer_list<CRefCount *> outputs, const CDSLSymbol *derived = nullptr)
{
	GPOS_ASSERT(symbols->Size() == outputs.size() + 1);
	for (ULONG i = 0; i < outputs.size(); ++i)
	{
		const auto *symbol = (*symbols)[i + 1];
		const auto *value = outputs.begin()[i];
		const auto *bound = model->PvalLookup(symbol);
		if (nullptr != bound && !FSameSubqueryOutput(symbol->Esymkind(), bound, value)) return false;
		for (ULONG j = 0; j < i; ++j)
			if (symbol == (*symbols)[j + 1] &&
				!FSameSubqueryOutput(symbol->Esymkind(), outputs.begin()[j], value)) return false;
	}
	for (ULONG i = 0; i < outputs.size(); ++i)
	{
		const auto *symbol = (*symbols)[i + 1];
		auto *bound = model->PvalLookup(symbol);
		auto *value = nullptr != bound ? bound : outputs.begin()[i];
		if (symbol == derived ? !model->FBindDerived(symbol, value)
			: (nullptr == bound && !model->FBind(symbol, value))) return false;
	}
	return true;
}
}  // namespace

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
	CExpression *pexprInner = (*pexprQuantified)[0];
	if (!pexprInner->Pop()->FLogical() ||
		!pexprInner->DeriveOutputColumns()->FMember(
			CScalarSubqueryQuantified::PopConvert(pexprQuantified->Pop())->Pcr()))
		return false;

	CExpression *pexprComparison =
		CDSLQuantifiedMatcher::PexprComparison(m_mp, pexprQuantified);
	CColRefArray *pdrgpcrOuter =
		(*pexprQuantified)[1]->DeriveUsedColumns()->Pdrgpcr(m_mp);
	const BOOL fMatches = FBindSubqueryOutputs(pdrgpsym, pmodel,
		{pexprComparison, pdrgpcrOuter, (*pexprQuantified)[0]});
	pexprComparison->Release();
	pdrgpcrOuter->Release();
	return fMatches;
}
