// Read-only scalar recognition and capture identity. No plan rebuilding.
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CScalarFunc.h"
#include "gpopt/operators/CScalarOp.h"
#include "gpopt/operators/CScalarSubquery.h"
#include "naucrates/md/IMDFunction.h"
#include "naucrates/md/IMDScalarOp.h"
#include "naucrates/md/IMDType.h"

using namespace gpopt;

BOOL
CDSLMatchView::FScalarValue(const CExpression *expression)
{
	if (nullptr == expression || !expression->Pop()->FScalar())
		return false;
	switch (expression->Pop()->Eopid())
	{
		case COperator::EopScalarProjectList:
		case COperator::EopScalarProjectElement:
		case COperator::EopScalarValuesList:
		case COperator::EopScalarSwitchCase:
		case COperator::EopScalarArrayRefIndexList:
		case COperator::EopScalarSortGroupClause:
		case COperator::EopScalarAssertConstraint:
		case COperator::EopScalarAssertConstraintList:
		case COperator::EopScalarBitmapIndexProbe:
		case COperator::EopScalarBitmapBoolOp:
			return false;
		default:
			return true;
	}
}

INT
CDSLMatchView::ScalarValueTypeModifier(const CExpression *expression)
{
	GPOS_ASSERT(FScalarValue(expression));
	if (COperator::EopScalarSubquery == expression->Pop()->Eopid())
		return CScalarSubquery::PopConvert(expression->Pop())->Pcr()->TypeModifier();
	return CScalar::PopConvert(expression->Pop())->TypeModifier();
}

BOOL
CDSLMatchView::FBooleanValue(const CExpression *expression)
{
	return FScalarValue(expression) && IMDType::EtiBool == COptCtxt::PoctxtFromTLS()->Pmda()->RetrieveType(
		CScalar::PopConvert(expression->Pop())->MdidType())->GetDatumType();
}

namespace
{
BOOL
FImmutableCallTree(const CExpression *expression)
{
	GPOS_CHECK_STACK_SIZE;
	const auto id = expression->Pop()->Eopid();
	IMDId *function = nullptr;
	CMDAccessor *mda = COptCtxt::PoctxtFromTLS()->Pmda();
	if (COperator::EopScalarFunc == id)
		function = CScalarFunc::PopConvert(expression->Pop())->FuncMdId();
	else if (COperator::EopScalarOp == id || COperator::EopScalarCmp == id)
		function = mda->RetrieveScOp(COperator::EopScalarOp == id
			? CScalarOp::PopConvert(expression->Pop())->MdIdOp()
			: CScalarCmp::PopConvert(expression->Pop())->MdIdOp())->FuncMdId();
	if (nullptr != function)
	{
		const IMDFunction *metadata = mda->RetrieveFunc(function);
		if (metadata->ReturnsSet() || IMDFunction::EfsImmutable != metadata->GetFuncStability())
			return false;
	}
	// ScalarOp/Cmp stability cannot be inferred solely from child properties.
	// Check catalog-backed operators even below CASE and Boolean wrappers.
	for (ULONG i = 0; i < expression->Arity(); ++i)
		if (!FImmutableCallTree((*expression)[i])) return false;
	return true;
}
}  // namespace

BOOL
CDSLMatchView::FScalarCall(const CExpression *expression)
{
	if (nullptr == expression) return false;
	const auto id = expression->Pop()->Eopid();
	if (COperator::EopScalarFunc != id && COperator::EopScalarOp != id &&
		COperator::EopScalarCmp != id) return false;
	for (ULONG i = 0; i < expression->Arity(); ++i)
		if (!FScalarValue((*expression)[i])) return false;
	// Property derivation only populates CExpression's existing property cache.
	CExpression *derived = const_cast<CExpression *>(expression);
	// Captured arguments retain their subqueries. Matching a call does not
	// certify argument error freedom or authorize changing their scope.
	return !derived->DeriveHasNonScalarFunction() &&
		IMDFunction::EfsImmutable == derived->DeriveScalarFunctionProperties()->Efs() &&
		FImmutableCallTree(expression);
}

BOOL
CDSLMatchView::FCallArgumentTypes(const CExpression *source,
	const CExpressionArray *arguments)
{
	if (nullptr == source || nullptr == arguments || source->Arity() != arguments->Size())
		return false;
	for (ULONG i = 0; i < source->Arity(); ++i)
	{
		if (!FScalarValue((*source)[i]) || !FScalarValue((*arguments)[i]))
			return false;
		const auto *before = CScalar::PopConvert((*source)[i]->Pop());
		const auto *after = CScalar::PopConvert((*arguments)[i]->Pop());
		if (!before->MdidType()->Equals(after->MdidType()) ||
			ScalarValueTypeModifier((*source)[i]) != ScalarValueTypeModifier((*arguments)[i]))
			return false;
	}
	return true;
}
BOOL
CDSLMatchView::FSameCallHead(const CExpression *left, const CExpression *right)
{
	if (!left->Pop()->Matches(right->Pop()) ||
		!CScalar::PopConvert(left->Pop())->MdidType()->Equals(CScalar::PopConvert(right->Pop())->MdidType()) ||
		CScalar::PopConvert(left->Pop())->TypeModifier() != CScalar::PopConvert(right->Pop())->TypeModifier() ||
		left->Arity() != right->Arity())
		return false;
	if (COperator::EopScalarFunc == left->Pop()->Eopid())
	{
		const auto *l = CScalarFunc::PopConvert(left->Pop());
		const auto *r = CScalarFunc::PopConvert(right->Pop());
		if (l->FuncFormat() != r->FuncFormat() || l->IsFuncVariadic() != r->IsFuncVariadic())
			return false;
	}
	return 0 == right->Arity() || FCallArgumentTypes(left, right->PdrgPexpr());
}

BOOL
CDSLMatchView::FSelectedSubqueryInput(CExpression *query, const CColRef *output)
{
	return nullptr != query && nullptr != output && query->Pop()->FLogical() &&
		query->DeriveOutputColumns()->FMember(output);
}

BOOL
CDSLMatchView::FSameCapturedExpression(const CExpression *left, const CExpression *right)
{
	GPOS_CHECK_STACK_SIZE;
	if (left == right) return true;
	if (!left->Pop()->Matches(right->Pop()) || left->Arity() != right->Arity())
		return false;
	// Native Matches omits some function metadata, also inside captured trees.
	const auto id = left->Pop()->Eopid();
	if ((COperator::EopScalarFunc == id || COperator::EopScalarOp == id ||
		 COperator::EopScalarCmp == id) && !FSameCallHead(left, right))
		return false;
	for (ULONG i = 0; i < left->Arity(); ++i)
		if (!FSameCapturedExpression((*left)[i], (*right)[i])) return false;
	return true;
}
