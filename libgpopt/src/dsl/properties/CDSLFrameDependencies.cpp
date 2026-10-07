// Native footprint for FormalSQL's retained-scope transport boundary.
#include "gpopt/dsl/CDSLExpressionProperties.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/dsl/CDSLExprListUtils.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarSubquery.h"

using namespace gpopt;

namespace
{
BOOL
FCollectFrameReads(CExpression *expr, CColRefSet *reads)
{
	GPOS_CHECK_STACK_SIZE;
	GPOS_CHECK_ABORT;
	if (nullptr == expr) return false;
	switch (expr->Pop()->Eopid())
	{
		case COperator::EopLogicalGet:
		case COperator::EopLogicalConstTableGet:
			return 0 == expr->Arity(); // Closed relations, not reads of their outputs.
		case COperator::EopScalarConst:
			return 0 == expr->Arity();
		case COperator::EopScalarIdent:
			if (0 != expr->Arity()) return false;
			reads->Include(CScalarIdent::PopConvert(expr->Pop())->Pcr());
			return true;
		case COperator::EopLogicalGbAgg:
		{
			const auto *agg = CLogicalGbAgg::PopConvert(expr->Pop());
			if (!agg->FGlobal() || 2 != expr->Arity() ||
				!(*expr)[0]->Pop()->FLogical() || !CDSLExprListUtils::FProjectList((*expr)[1]))
				return false;
			reads->Include(agg->Pdrgpcr());
			break;
		}
		case COperator::EopLogicalProject:
			if (2 != expr->Arity() || !(*expr)[0]->Pop()->FLogical() ||
				!CDSLExprListUtils::FProjectList((*expr)[1])) return false;
			// Compute lowers to identity SELECT items followed by new items.
			reads->Include((*expr)[0]->DeriveOutputColumns());
			break;
		case COperator::EopLogicalInnerJoin:
		case COperator::EopLogicalLeftOuterJoin:
		case COperator::EopLogicalFullOuterJoin:
		case COperator::EopLogicalInnerApply:
		case COperator::EopLogicalLeftOuterApply:
			if (3 != expr->Arity() || !(*expr)[0]->Pop()->FLogical() ||
				!(*expr)[1]->Pop()->FLogical() || !(*expr)[2]->Pop()->FScalar()) return false;
			// Identity SELECTs of joined rows also read these column identities.
			reads->Include((*expr)[0]->DeriveOutputColumns());
			reads->Include((*expr)[1]->DeriveOutputColumns());
			break;
		case COperator::EopLogicalSelect:
			if (2 != expr->Arity() || !(*expr)[0]->Pop()->FLogical() ||
				!(*expr)[1]->Pop()->FScalar()) return false;
			break;
		case COperator::EopLogicalUnionAll:
			if (expr->Arity() < 2) return false;
			for (ULONG i = 0; i < expr->Arity(); ++i)
				if (!(*expr)[i]->Pop()->FLogical()) return false;
			break; // Branch remapping observes rows, not the surrounding frame.
		case COperator::EopLogicalLimit:
		{
			CDSLMatchView::SOrderLimit view{};
			if (!CDSLMatchView::FOrderLimit(expr, &view) ||
				!view.m_pexprChild->Pop()->FLogical() ||
				!dslproperties::FNonnegativeLimitBound(view.m_pexprOffset) ||
				(CLogicalLimit::PopConvert(expr->Pop())->FHasCount() &&
				 !dslproperties::FNonnegativeLimitBound(view.m_pexprCount))) return false;
			for (ULONG i = 0; i < view.m_pos->UlSortColumns(); ++i)
				if (!view.m_pexprChild->DeriveOutputColumns()->FMember(view.m_pos->Pcr(i)))
					return false;
			// Fixed OFFSET/FETCH and row-local ORDER BY add no environment reads.
			return FCollectFrameReads(view.m_pexprChild, reads);
		}
		case COperator::EopScalarSubquery:
			if (1 != expr->Arity() || !(*expr)[0]->Pop()->FLogical()) return false;
			reads->Include(CScalarSubquery::PopConvert(expr->Pop())->Pcr());
			break;
		case COperator::EopScalarSubqueryExists:
		case COperator::EopScalarSubqueryNotExists:
			if (1 != expr->Arity() || !(*expr)[0]->Pop()->FLogical()) return false;
			break;
		case COperator::EopScalarSubqueryAny:
		case COperator::EopScalarSubqueryAll:
			if (2 != expr->Arity() || !(*expr)[0]->Pop()->FLogical() ||
				!(*expr)[1]->Pop()->FScalar()) return false;
			break;
		case COperator::EopScalarProjectList:
		case COperator::EopScalarProjectElement:
		case COperator::EopScalarValuesList:
		case COperator::EopScalarAggFunc:
		case COperator::EopScalarBoolOp:
		case COperator::EopScalarNullTest:
		case COperator::EopScalarBooleanTest:
		case COperator::EopScalarIf:
			break;
		default:
			// Unknown implicit evaluation scopes cannot borrow a free-ref set.
			// ponytail: no window/CTE lowering here; add after its frame contract is checked.
			if (!CDSLMatchView::FScalarCall(expr)) return false;
	}
	for (ULONG i = 0; i < expr->Arity(); ++i)
		if (!FCollectFrameReads((*expr)[i], reads)) return false;
	return true;
}
} // namespace

CColRefSet *
gpopt::dslproperties::PcrsFrameReads(CMemoryPool *mp, CExpression *expr)
{
	CColRefSet *reads = GPOS_NEW(mp) CColRefSet(mp);
	if (FCollectFrameReads(expr, reads)) return reads;
	reads->Release();
	return nullptr;
}
