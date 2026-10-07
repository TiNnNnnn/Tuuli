// Native expression properties shared by DSL matching, checks and construction.
#include "gpopt/dsl/CDSLExpressionProperties.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarAggFunc.h"
#include "gpopt/operators/CScalarCast.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CScalarConst.h"
#include "gpopt/operators/CScalarNullIf.h"
#include "gpopt/operators/CScalarSortGroupClause.h"
#include "gpopt/operators/CScalarSubquery.h"
#include "gpopt/operators/CScalarSubqueryQuantified.h"
#include "gpopt/operators/CScalarWindowFunc.h"
#include "naucrates/base/IDatumInt2.h"
#include "naucrates/base/IDatumInt4.h"
#include "naucrates/base/IDatumInt8.h"
#include "naucrates/dxl/gpdb_types.h"
#include "naucrates/md/CMDIdGPDB.h"
#include "naucrates/md/CMDTypeInt2GPDB.h"
#include "naucrates/md/CMDTypeInt4GPDB.h"
#include "naucrates/md/CMDTypeInt8GPDB.h"
#include "naucrates/md/IMDFunction.h"

using namespace gpopt;
using namespace gpnaucrates;

namespace gpopt
{
namespace dslproperties
{
namespace
{
BOOL
FScalarCastProvablyErrorFree(CExpression *pexpr)
{
	CScalarCast *popCast = CScalarCast::PopConvert(pexpr->Pop());
	if (popCast->IsBinaryCoercible())
	{
		return true;
	}
	if (1 != pexpr->Arity())
	{
		return false;
	}

	IMDId *pmdidSource = CScalar::PopConvert((*pexpr)[0]->Pop())->MdidType();
	IMDId *pmdidTarget = popCast->MdidType();
	IMDId *pmdidFunction = popCast->FuncMdId();
	if (IMDId::EmdidGeneral != pmdidSource->MdidType() ||
		IMDId::EmdidGeneral != pmdidTarget->MdidType() ||
		!IMDId::IsValid(pmdidFunction) ||
		IMDId::EmdidGeneral != pmdidFunction->MdidType())
	{
		return false;
	}
	const OID oidSource = CMDIdGPDB::CastMdid(pmdidSource)->Oid();
	const OID oidTarget = CMDIdGPDB::CastMdid(pmdidTarget)->Oid();
	const OID oidFunction = CMDIdGPDB::CastMdid(pmdidFunction)->Oid();

	// PostgreSQL's signed-integer widening casts are total over their source
	// domains, but an arbitrary function with that signature need not be.
	// Match implementation OIDs from pg_proc.dat as well as the direction.
	return (GPDB_INT2 == oidSource && GPDB_INT4 == oidTarget &&
			313 == oidFunction /*i2toi4*/) ||
		(GPDB_INT2 == oidSource && GPDB_INT8 == oidTarget &&
			754 == oidFunction /*int28*/) ||
		(GPDB_INT4 == oidSource && GPDB_INT8 == oidTarget &&
			481 == oidFunction /*int48*/);
}

BOOL
FAggFuncProvablyErrorFree(CScalarAggFunc *popAgg)
{
	// COUNT is not unconditionally total: PostgreSQL's int8inc transition
	// reports bigint overflow. MIN/MAX below only select an existing value.
	IMDId *pmdid = popAgg->MDId();
	if (IMDId::EmdidGeneral != pmdid->MdidType())
	{
		return false;
	}
	const OID oid = CMDIdGPDB::CastMdid(pmdid)->Oid();
	switch (oid)
	{
		case GPDB_INT2_AGG_MIN:
		case GPDB_INT2_AGG_MAX:
		case GPDB_INT4_AGG_MIN:
		case GPDB_INT4_AGG_MAX:
		case GPDB_INT8_AGG_MIN:
		case GPDB_INT8_AGG_MAX:
			return true;
		default:
			return false;
	}
}

BOOL
FWindowFuncProvablyErrorFree(CScalarWindowFunc *popWindow)
{
	if (!popWindow->FAgg() ||
		IMDId::EmdidGeneral != popWindow->FuncMdId()->MdidType())
	{
		return false;
	}
	const OID oid = CMDIdGPDB::CastMdid(popWindow->FuncMdId())->Oid();
	switch (oid)
	{
		case GPDB_INT2_AGG_MIN:
		case GPDB_INT2_AGG_MAX:
		case GPDB_INT4_AGG_MIN:
		case GPDB_INT4_AGG_MAX:
		case GPDB_INT8_AGG_MIN:
		case GPDB_INT8_AGG_MAX:
			return true;
		default:
			return false;
	}
}

BOOL
FExistenceInputProvablySafe(CExpression *expr, BOOL deterministic = false)
{
	// Fixed nonnegative slicing can change which rows survive, but not their
	// count. Only existential demand may ignore that row nondeterminism.
	while (COperator::EopLogicalLimit == expr->Pop()->Eopid())
	{
		CDSLMatchView::SOrderLimit view{};
		if (!CDSLMatchView::FOrderLimit(expr, &view) || !view.m_pos->IsEmpty() ||
			!FNonnegativeLimitBound(view.m_pexprOffset) ||
			(view.m_fHasLimit && !FNonnegativeLimitBound(view.m_pexprCount))) return false;
		expr = view.m_pexprChild;
	}
	return FRelationalTreeProvablyErrorFree(expr, deterministic);
}

BOOL
FSelectionChainRejectsNull(CMemoryPool *mp, CExpression *pexpr,
						   const CColRef *pcr)
{
	// Only inspect Selects above the bound relation. Descending through an
	// arbitrary relational child is unsafe: a lower Select may reject NULL, but
	// a parent outer join can null-extend that same column again.
	CExpression *pexprCurrent = pexpr;
	while (COperator::EopLogicalSelect == pexprCurrent->Pop()->Eopid() &&
		   2 == pexprCurrent->Arity())
	{
		CExpression *pexprPred = (*pexprCurrent)[1];
		if (pexprPred->DeriveUsedColumns()->FMember(pcr))
		{
			CColRefSet *pcrs = GPOS_NEW(mp) CColRefSet(mp);
			pcrs->Include(const_cast<CColRef *>(pcr));
			BOOL fRejects =
				CPredicateUtils::FNullRejecting(mp, pexprPred, pcrs);
			pcrs->Release();
			if (fRejects)
			{
				return true;
			}
		}
		pexprCurrent = (*pexprCurrent)[0];
	}
	return false;
}

}  // namespace

BOOL
FNonnegativeLimitBound(CExpression *expr)
{
	if (nullptr == expr) return false;
	while (COperator::EopScalarCast == expr->Pop()->Eopid())
	{
		if (1 != expr->Arity() || !FScalarCastProvablyErrorFree(expr)) return false;
		expr = (*expr)[0];
	}
	if (COperator::EopScalarConst != expr->Pop()->Eopid()) return false;
	IDatum *datum = CScalarConst::PopConvert(expr->Pop())->GetDatum();
	if (datum->IsNull()) return false;
	switch (datum->GetDatumType())
	{
		case IMDType::EtiInt2: return 0 <= dynamic_cast<IDatumInt2 *>(datum)->Value();
		case IMDType::EtiInt4: return 0 <= dynamic_cast<IDatumInt4 *>(datum)->Value();
		case IMDType::EtiInt8: return 0 <= dynamic_cast<IDatumInt8 *>(datum)->Value();
		default: return false;
	}
}

BOOL
FScalarTreeProvablyErrorFree(CExpression *pexpr,
							   const CMaxCard &aggregateInput)
{
	switch (pexpr->Pop()->Eopid())
	{
		case COperator::EopScalarIdent:
		case COperator::EopScalarConst:
			return true;
		case COperator::EopScalarSubquery:
		{
			const auto *subquery = CScalarSubquery::PopConvert(pexpr->Pop());
			// Empty input yields NULL; a singleton yields its selected value.
			// A wrapper's one-row bound alone does not establish totality:
			// recurse so an Assert on a multirow child still fails the audit.
			return 1 == pexpr->Arity() &&
				!subquery->FGeneratedByExist() && !subquery->FGeneratedByQuantified() &&
				CDSLMatchView::FSelectedSubqueryInput((*pexpr)[0], subquery->Pcr()) &&
				(*pexpr)[0]->DeriveMaxCard().Ull() <= 1 &&
				FRelationalTreeProvablyErrorFree((*pexpr)[0]);
		}
		case COperator::EopScalarSubqueryExists:
		case COperator::EopScalarSubqueryNotExists:
			return 1 == pexpr->Arity() && (*pexpr)[0]->Pop()->FLogical() &&
				FExistenceInputProvablySafe((*pexpr)[0]);
		case COperator::EopScalarSubqueryAny:
		case COperator::EopScalarSubqueryAll:
			// Quantifiers add no cardinality assertion. Audit both the query
			// and scalar argument, not just the comparison's function metadata.
			return 2 == pexpr->Arity() && (*pexpr)[0]->Pop()->FLogical() &&
				(*pexpr)[1]->Pop()->FScalar() &&
				CPredicateUtils::FBuiltInComparisonIsVeryStrict(
					CScalarSubqueryQuantified::PopConvert(pexpr->Pop())->MdIdOp()) &&
				FRelationalTreeProvablyErrorFree((*pexpr)[0]) &&
				FScalarTreeProvablyErrorFree((*pexpr)[1]);
		case COperator::EopScalarCmp:
		case COperator::EopScalarIsDistinctFrom:
			// Admit every comparison in ORCA's explicit built-in strict whitelist.
			// This includes the <>/< <=/> >= operators used by quantified ALL, while
			// still rejecting user-defined comparisons whose evaluation may throw.
			if (!CPredicateUtils::FBuiltInComparisonIsVeryStrict(
					static_cast<CScalarCmp *>(pexpr->Pop())->MdIdOp()))
			{
				return false;
			}
			break;
		case COperator::EopScalarNullIf:
			// NULL handling is total, but the underlying comparison need not
			// be. Reuse the same built-in whitelist and check both operands.
			if (!CPredicateUtils::FBuiltInComparisonIsVeryStrict(
					CScalarNullIf::PopConvert(pexpr->Pop())->MdIdOp()))
			{
				return false;
			}
			break;
		case COperator::EopScalarCast:
			// Binary coercions and explicitly whitelisted widening casts are total.
			// Parsing or narrowing casts can still fail and remain rejected.
			if (!FScalarCastProvablyErrorFree(pexpr))
			{
				return false;
			}
			break;
		case COperator::EopScalarAggFunc:
		{
			// A total transition alone does not establish safety of DISTINCT.
			// Audit its comparison metadata too; ordered/direct arguments and
			// split states remain outside this contract.
			CScalarAggFunc *popAgg =
				CScalarAggFunc::PopConvert(pexpr->Pop());
			// This bound belongs to this aggregate's actual relational input,
			// never to an arbitrary invocation of a captured function symbol.
			// For ordinary COUNT, every transition state is in [0, input rows].
			const BOOL boundedCount = (popAgg->FCountStar() || popAgg->FCountAny()) &&
				aggregateInput.Ull() != GPOPT_MAX_CARD &&
				aggregateInput.Ull() <= static_cast<ULLONG>(gpos::lint_max);
			const BOOL selectingInteger = FAggFuncProvablyErrorFree(popAgg);
			// DISTINCT cannot increase COUNT's transitions beyond its input
			// bound. The argument's equality/order callbacks still need auditing.
			const BOOL boundedCountAny = boundedCount && popAgg->FCountAny();
			if (!popAgg->FGlobal() || popAgg->FSplit() ||
				popAgg->AggKind() != EaggfunckindNormal ||
				(!boundedCount && !selectingInteger) ||
				(popAgg->IsDistinct() && !selectingInteger && !boundedCountAny) ||
				EaggfuncIndexSentinel != pexpr->Arity())
			{
				return false;
			}
			// Native MIN/MAX preprocessing clears IsDistinct but keeps this
			// slot. Validate that dormant metadata as well as an active clause.
			const BOOL hasDistinctClause =
				(*pexpr)[EaggfuncIndexDistinct]->Arity() != 0;
			if (hasDistinctClause && !selectingInteger && !boundedCountAny)
			{
				return false;
			}
			for (ULONG i = 0; i < pexpr->Arity(); ++i)
			{
				if (COperator::EopScalarValuesList != (*pexpr)[i]->Pop()->Eopid() ||
					(*pexpr)[i]->Arity() !=
						((i == EaggfuncIndexArgs && !popAgg->FCountStar()) ||
						 (i == EaggfuncIndexDistinct &&
						  (popAgg->IsDistinct() || hasDistinctClause)) ? 1U : 0U))
				{
					return false;
				}
			}
			if (hasDistinctClause)
			{
				CExpression *argument = (*(*pexpr)[EaggfuncIndexArgs])[0];
				CExpression *clause = (*(*pexpr)[EaggfuncIndexDistinct])[0];
				if (!argument->Pop()->FScalar() || clause->Arity() != 0 ||
					clause->Pop()->Eopid() != COperator::EopScalarSortGroupClause)
				{
					return false;
				}
				IMDId *typeId = CScalar::PopConvert(argument->Pop())->MdidType();
				// Only built-in integer DISTINCT callbacks have been audited here.
				if (!IMDId::IsValid(typeId) ||
					typeId->MdidType() != IMDId::EmdidGeneral)
				{
					return false;
				}
				const OID typeOid = CMDIdGPDB::CastMdid(typeId)->Oid();
				if (typeOid != GPDB_INT2 && typeOid != GPDB_INT4 && typeOid != GPDB_INT8)
				{
					return false;
				}
				const IMDType *type =
					COptCtxt::PoctxtFromTLS()->Pmda()->RetrieveType(typeId);
				if (selectingInteger
					? !popAgg->IsMinMax(type) || !popAgg->MdidType()->Equals(typeId)
					: !popAgg->MdidType()->Equals(&CMDIdGPDB::m_mdid_int8))
				{
					return false;
				}
				const auto *sort = CScalarSortGroupClause::PopConvert(clause->Pop());
				const auto *eq = type->GetMdidForCmpType(IMDType::EcmptEq);
				const auto *lt = type->GetMdidForCmpType(IMDType::EcmptL);
				const auto *gt = type->GetMdidForCmpType(IMDType::EcmptG);
				if (sort->Index() != 0 ||
					sort->EqOp() != static_cast<INT>(CMDIdGPDB::CastMdid(eq)->Oid()) ||
					(sort->SortOp() != static_cast<INT>(CMDIdGPDB::CastMdid(lt)->Oid()) &&
					 sort->SortOp() != static_cast<INT>(CMDIdGPDB::CastMdid(gt)->Oid())))
				{
					return false;
				}
			}
			return FScalarTreeProvablyErrorFree((*pexpr)[EaggfuncIndexArgs],
											   aggregateInput);
		}
		case COperator::EopScalarWindowFunc:
			if (!FWindowFuncProvablyErrorFree(
					CScalarWindowFunc::PopConvert(pexpr->Pop())))
			{
				return false;
			}
			break;
		case COperator::EopScalarNullTest:
		case COperator::EopScalarBooleanTest:
		case COperator::EopScalarIf:
			// Boolean tests and searched CASE add no errors of their own.
			// Check every child below, including both CASE branches: moving
			// this expression may expose rows its original Filter excluded.
		case COperator::EopScalarBoolOp:
		case COperator::EopScalarCoalesce:
		case COperator::EopScalarValuesList:
		case COperator::EopScalarProjectElement:
		case COperator::EopScalarProjectList:
			break;
		default:
			// Function/operator error behavior is not represented in the current
			// ORCA scalar metadata. Reject unknown shapes instead of assuming that
			// evaluation can be duplicated, removed, or reordered.
			return false;
	}
	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		if (!FScalarTreeProvablyErrorFree((*pexpr)[ul], aggregateInput))
		{
			return false;
		}
	}
	return true;
}

BOOL
FRelationalTreeProvablyErrorFree(CExpression *pexpr, BOOL deterministic)
{
	if (nullptr == pexpr) return false;
	if (pexpr->Pop()->FScalar())
		return FScalarTreeProvablyErrorFree(pexpr) &&
			(!deterministic || FScalarTreeProvablyDeterministic(pexpr));
	switch (pexpr->Pop()->Eopid())
	{
		case COperator::EopLogicalMaxOneRow:
			// The operator's own bound is always one, even when it can raise a
			// cardinality violation. Only a guaranteed child bound discharges it.
			return 1 == pexpr->Arity() && (*pexpr)[0]->Pop()->FLogical() &&
				(*pexpr)[0]->DeriveMaxCard().Ull() <= 1 &&
				FRelationalTreeProvablyErrorFree((*pexpr)[0], deterministic);
		case COperator::EopLogicalGbAgg:
		{
			// Audit every grouping key and recurse through all aggregate items
			// and the input. COUNT additionally needs a guaranteed input bound;
			// neither that bound nor MIN/MAX makes its arguments safe.
			const auto *agg = CLogicalGbAgg::PopConvert(pexpr->Pop());
			if (!agg->FGlobal() || 2 != pexpr->Arity() ||
				!(*pexpr)[0]->Pop()->FLogical() ||
				COperator::EopScalarProjectList != (*pexpr)[1]->Pop()->Eopid()) return false;
			const CColRefArray *keys = agg->Pdrgpcr();
			if (nullptr == keys) return false;
			for (ULONG i = 0; i < keys->Size(); ++i)
			{
				IMDId *equality = (*keys)[i]->RetrieveType()->GetMdidForCmpType(IMDType::EcmptEq);
				if (!IMDId::IsValid(equality) || IMDId::EmdidGeneral != equality->MdidType() ||
					!CPredicateUtils::FBuiltInComparisonIsVeryStrict(equality)) return false;
			}
			return FRelationalTreeProvablyErrorFree((*pexpr)[0], deterministic) &&
				FScalarTreeProvablyErrorFree((*pexpr)[1], (*pexpr)[0]->DeriveMaxCard()) &&
				(!deterministic || FScalarTreeProvablyDeterministic((*pexpr)[1]));
		}
		case COperator::EopLogicalGet:
		case COperator::EopLogicalConstTableGet:
		case COperator::EopLogicalSelect:
		case COperator::EopLogicalProject:
		case COperator::EopLogicalInnerJoin:
		case COperator::EopLogicalLeftOuterJoin:
		case COperator::EopLogicalFullOuterJoin:
		case COperator::EopLogicalLeftSemiJoin:
		case COperator::EopLogicalLeftAntiSemiJoin:
		case COperator::EopLogicalLeftAntiSemiJoinNotIn:
		case COperator::EopLogicalUnionAll:
			break;
		default:
			// Cardinality assertions, dynamic LIMITs, window frames,
			// and opaque/CTE inputs need their own totality contracts. A table
			// placeholder is not evidence that an arbitrary subtree cannot err.
			return false;
	}
	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		if (!FRelationalTreeProvablyErrorFree((*pexpr)[ul], deterministic)) return false;
	}
	return true;
}

BOOL
FScalarTreeProvablyDeterministic(CExpression *pexpr)
{
	if (!pexpr->Pop()->FScalar()) return false;
	if (COperator::EopScalarSubquery == pexpr->Pop()->Eopid())
		return FScalarTreeProvablyErrorFree(pexpr) &&
			FRelationalTreeProvablyErrorFree((*pexpr)[0], true);
	if (COperator::EopScalarSubqueryAny == pexpr->Pop()->Eopid() ||
		COperator::EopScalarSubqueryAll == pexpr->Pop()->Eopid())
	{
		return FScalarTreeProvablyErrorFree(pexpr) &&
			FRelationalTreeProvablyErrorFree((*pexpr)[0], true) &&
			FScalarTreeProvablyDeterministic((*pexpr)[1]);
	}
	if (COperator::EopScalarSubqueryExists == pexpr->Pop()->Eopid() ||
		COperator::EopScalarSubqueryNotExists == pexpr->Pop()->Eopid())
	{
		// Repeatability follows from the audited existential observation,
		// not from immutable function metadata alone.
		return 1 == pexpr->Arity() && (*pexpr)[0]->Pop()->FLogical() &&
			FExistenceInputProvablySafe((*pexpr)[0], true);
	}
	if (pexpr->DeriveHasSubquery())
		for (ULONG i = 0; i < pexpr->Arity(); i++)
			if (!FScalarTreeProvablyDeterministic((*pexpr)[i])) return false;
	return !pexpr->DeriveHasNonScalarFunction() &&
		IMDFunction::EfsImmutable ==
			pexpr->DeriveScalarFunctionProperties()->Efs();
}

BOOL
FQueryDemandInsensitive(CExpression *pexpr)
{
	return FRelationalTreeProvablyErrorFree(pexpr, true);
}

BOOL
FExpressionProvesNotNull(CMemoryPool *mp, CExpression *pexpr,
						 const CColRef *pcr)
{
	if (nullptr == pexpr ||
		!pexpr->DeriveOutputColumns()->FMember(pcr))
	{
		return false;
	}
	if (pexpr->DeriveNotNullColumns()->FMember(pcr) ||
		FSelectionChainRejectsNull(mp, pexpr, pcr))
	{
		return true;
	}

	// ORCA's root not-null property is deliberately conservative for several
	// join shapes. Follow only children whose rows cannot be null-extended by
	// the current operator; the opposite side of an outer join remains rejected.
	switch (pexpr->Pop()->Eopid())
	{
		case COperator::EopLogicalSelect:
		case COperator::EopLogicalProject:
		case COperator::EopLogicalSequenceProject:
		case COperator::EopLogicalLimit:
			return 0 < pexpr->Arity() &&
				   FExpressionProvesNotNull(mp, (*pexpr)[0], pcr);

		case COperator::EopLogicalInnerJoin:
		case COperator::EopLogicalInnerApply:
			for (ULONG ul = 0; ul < 2 && ul < pexpr->Arity(); ul++)
			{
				if (FExpressionProvesNotNull(mp, (*pexpr)[ul], pcr))
				{
					return true;
				}
			}
			return false;

		case COperator::EopLogicalLeftOuterJoin:
		case COperator::EopLogicalLeftOuterApply:
		case COperator::EopLogicalLeftSemiJoin:
		case COperator::EopLogicalLeftAntiSemiJoin:
		case COperator::EopLogicalLeftAntiSemiJoinNotIn:
			return 0 < pexpr->Arity() &&
				   FExpressionProvesNotNull(mp, (*pexpr)[0], pcr);

		case COperator::EopLogicalRightOuterJoin:
			return 1 < pexpr->Arity() &&
				   FExpressionProvesNotNull(mp, (*pexpr)[1], pcr);

		default:
			return false;
	}
}

}  // namespace dslproperties
}  // namespace gpopt
