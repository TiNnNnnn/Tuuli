#include "gpopt/dsl/CDSLColumnDomain.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/base/CKeyCollection.h"
#include "gpopt/dsl/CDSLExprListUtils.h"
#include "gpopt/dsl/CDSLExpressionProperties.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarProjectList.h"

using namespace gpopt;

BOOL
CDSLColumnDomain::FObservable(const CColRef *column)
{
	return !column->IsSystemCol() || CColRef::EUsed == column->GetUsage(true, true);
}

CColRefArray *
CDSLColumnDomain::PdrgpcrOutput(CMemoryPool *mp, const CColRefSet *outputs)
{
	CColRefArray *columns = GPOS_NEW(mp) CColRefArray(mp);
	CColRefSetIter iter(*outputs);
	while (iter.Advance())
	{
		if (FObservable(iter.Pcr())) columns->Append(iter.Pcr());
	}
	return columns;
}

// ORCA Project passes child columns through. An eliminated inner join's key
// remains a Memo output even when the DSL projection only reads the other side.
CExpression *
CDSLColumnDomain::PexprRestoreJoinKeys(CMemoryPool *mp, CExpression *source,
									  CExpression *target)
{
	if (COperator::EopLogicalProject != source->Pop()->Eopid() ||
		COperator::EopLogicalProject != target->Pop()->Eopid() ||
		2 != source->Arity() || 2 != target->Arity() || nullptr != target->Pgexpr())
		return target;

	CExpression *join = (*source)[0];
	CExpression *input = (*target)[0];
	if (COperator::EopLogicalInnerJoin != join->Pop()->Eopid() || 3 != join->Arity() ||
		(input != (*join)[0] && input != (*join)[1]))
		return target;

	CExpression *list = (*target)[1];
	if (!list->Matches((*source)[1]) || list->DeriveDefinedColumns()->Size() != list->Arity() ||
		!CDSLExprListUtils::FRowScalar(list) || !dslproperties::FScalarTreeProvablyDeterministic(list))
		return target;
	CExpression *removed = (*join)[input == (*join)[0] ? 1 : 0];
	const CKeyCollection *keys = removed->DeriveKeyCollection();
	if (nullptr == keys)
		return target;
	auto *restored = GPOS_NEW(mp) CColRefSet(mp);
	const CColRefSet *available = input->DeriveOutputColumns();
	CExpressionArray *conjuncts = CPredicateUtils::PdrgpexprConjuncts(mp, (*join)[2]);
	CExpressionArray *items = nullptr;
	for (ULONG i = 0; i < conjuncts->Size(); i++)
	{
		CExpression *eq = (*conjuncts)[i];
		if (COperator::EopScalarCmp != eq->Pop()->Eopid() || 2 != eq->Arity() ||
			!CUtils::FScalarIdent((*eq)[0]) || !CUtils::FScalarIdent((*eq)[1]))
			continue;

		const CColRef *from = CScalarIdent::PopConvert((*eq)[0]->Pop())->Pcr();
		const CColRef *to = CScalarIdent::PopConvert((*eq)[1]->Pop())->Pcr();
		if (!available->FMember(from))
			std::swap(from, to);
		const IMDType *type = from->RetrieveType();
		// Built-in integers, OIDs and booleans have one value per equality
		// class. Numeric scale and collatable strings need a stronger witness.
		const auto kind = type->GetDatumType();
		if (kind != IMDType::EtiInt2 && kind != IMDType::EtiInt4 && kind != IMDType::EtiInt8 &&
			kind != IMDType::EtiOid && kind != IMDType::EtiBool)
			continue;
		if (!available->FMember(from) ||
			!removed->DeriveOutputColumns()->FMember(to) || !FObservable(to) ||
			!source->DeriveOutputColumns()->FMember(to) ||
			target->DeriveOutputColumns()->FMember(to) || restored->FMember(to) ||
			type != to->RetrieveType() ||
			!CScalarCmp::PopConvert(eq->Pop())->MdIdOp()->Equals(
				type->GetMdidForCmpType(IMDType::EcmptEq)))
			continue;

		if (nullptr == items)
		{
			items = GPOS_NEW(mp) CExpressionArray(mp);
			if (0 < list->Arity())
				CUtils::AddRefAppend(items, list->PdrgPexpr());
		}
		items->Append(CUtils::PexprScalarProjectElement(mp, const_cast<CColRef *>(to),
			CUtils::PexprScalarIdent(mp, from)));
		restored->Include(to);
	}
	conjuncts->Release();
	// The checked rewrite preserves the projected bag. With a unique partner,
	// no retained row can multiply to compensate for a different missing row.
	const BOOL unique = nullptr != items && keys->FKey(restored, false);
	restored->Release();
	if (!unique)
	{
		CRefCount::SafeRelease(items);
		return target;
	}

	input->AddRef();
	target->Release();
	return CUtils::PexprLogicalProject(mp, input,
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp), items), false);
}
