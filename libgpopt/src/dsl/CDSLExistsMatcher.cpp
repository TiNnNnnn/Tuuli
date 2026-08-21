//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLExistsMatcher.cpp
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLExistsMatcher.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarConst.h"
#include "gpopt/operators/CScalarSubqueryExists.h"
#include "naucrates/base/IDatumInt2.h"
#include "naucrates/base/IDatumInt4.h"
#include "naucrates/base/IDatumInt8.h"

using namespace gpopt;
using namespace gpnaucrates;

namespace
{
BOOL
FScalarConstIntOne(CExpression *pexpr)
{
	if (COperator::EopScalarConst != pexpr->Pop()->Eopid())
	{
		return false;
	}

	IDatum *pdatum = CScalarConst::PopConvert(pexpr->Pop())->GetDatum();
	if (pdatum->IsNull())
	{
		return false;
	}

	switch (pdatum->GetDatumType())
	{
		case IMDType::EtiInt2:
			return 1 == dynamic_cast<IDatumInt2 *>(pdatum)->Value();
		case IMDType::EtiInt4:
			return 1 == dynamic_cast<IDatumInt4 *>(pdatum)->Value();
		case IMDType::EtiInt8:
			return 1 == dynamic_cast<IDatumInt8 *>(pdatum)->Value();
		default:
			return false;
	}
}
}  // namespace

BOOL
CDSLExistsMatcher::FExistsLimitOne(CExpression *pexpr) const
{
	if (COperator::EopLogicalLimit != pexpr->Pop()->Eopid() ||
		3 != pexpr->Arity())
	{
		return false;
	}

	CLogicalLimit *popLimit = CLogicalLimit::PopConvert(pexpr->Pop());
	return popLimit->FGlobal() && popLimit->FHasCount() &&
		   CUtils::FHasZeroOffset(pexpr) && FScalarConstIntOne((*pexpr)[2]);
}

BOOL
CDSLExistsMatcher::FMatch(const CDSLOp *pop, CExpression *pexpr,
						  CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != pop);
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != pmodel);
	GPOS_ASSERT(EdslopExists == pop->Edslop());

	if (2 != pop->UlChildren() || nullptr == pop->Pdrgpsym() ||
		0 != pop->Pdrgpsym()->Size())
	{
		return false;
	}

	// Before native subquery unnesting, EXISTS is represented as
	// Select(outer, ScalarSubqueryExists(inner)). Matching this shape is what
	// lets the DSL rule replace CXformSelect2Apply instead of depending on it.
	if (COperator::EopLogicalSelect == pexpr->Pop()->Eopid())
	{
		if (2 != pexpr->Arity())
		{
			return false;
		}

		// EXISTS frequently arrives with translator-generated guards, e.g.
		// NOT(outer_agg IS NULL) AND EXISTS(...). Consume exactly one direct
		// existential conjunct and preserve all other conjuncts as a Select above
		// the generated Apply.
		CExpressionArray *pdrgpexprConj =
			CPredicateUtils::PdrgpexprConjuncts(m_mp, (*pexpr)[1]);
		CExpression *pexprExists = nullptr;
		ULONG ulExists = 0;
		for (ULONG ul = 0; ul < pdrgpexprConj->Size(); ul++)
		{
			CExpression *pexprConj = (*pdrgpexprConj)[ul];
			if (COperator::EopScalarSubqueryExists ==
					pexprConj->Pop()->Eopid() &&
				1 == pexprConj->Arity())
			{
				pexprExists = pexprConj;
				ulExists++;
			}
		}

		BOOL fMatched = false;
		if (1 == ulExists &&
			m_pmatcher->FMatch((*pop)[0], (*pexpr)[0], pmodel) &&
			m_pmatcher->FMatch((*pop)[1], (*pexprExists)[0], pmodel))
		{
			CExpressionArray *pdrgpexprResidual =
				GPOS_NEW(m_mp) CExpressionArray(m_mp);
			for (ULONG ul = 0; ul < pdrgpexprConj->Size(); ul++)
			{
				CExpression *pexprConj = (*pdrgpexprConj)[ul];
				if (pexprConj != pexprExists)
				{
					pexprConj->AddRef();
					pdrgpexprResidual->Append(pexprConj);
				}
			}
			pmodel->SetExistsResidualConjuncts(pdrgpexprResidual);
			fMatched = true;
		}
		pdrgpexprConj->Release();
		return fMatched;
	}

	// Keep accepting the post-unnesting representation as well. This allows
	// rules to participate regardless of whether another exploration path has
	// already introduced the Apply. LIMIT 1 is the native implementation detail
	// used for an uncorrelated EXISTS and is transparent to the DSL.
	if (COperator::EopLogicalLeftSemiApply == pexpr->Pop()->Eopid())
	{
		if (3 != pexpr->Arity() ||
			!CUtils::FScalarConstTrue((*pexpr)[2]))
		{
			return false;
		}

		CLogicalApply *popApply = dynamic_cast<CLogicalApply *>(pexpr->Pop());
		if (nullptr == popApply || COperator::EopScalarSubqueryExists !=
								 popApply->EopidOriginSubq())
		{
			return false;
		}

		CExpression *pexprInner = (*pexpr)[1];
		if (FExistsLimitOne(pexprInner))
		{
			pexprInner = (*pexprInner)[0];
		}

		return m_pmatcher->FMatch((*pop)[0], (*pexpr)[0], pmodel) &&
			   m_pmatcher->FMatch((*pop)[1], pexprInner, pmodel);
	}

	return false;
}

// EOF
