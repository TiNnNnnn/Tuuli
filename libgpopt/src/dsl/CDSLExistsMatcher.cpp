//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLExistsMatcher.cpp
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLExistsMatcher.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CLogicalLeftAntiSemiApply.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarConst.h"
#include "gpopt/operators/CScalarSubqueryExists.h"
#include "gpopt/operators/CScalarSubqueryNotExists.h"
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

BOOL
FDirectExistential(CExpression *pexpr, BOOL fNegated)
{
	const COperator::EOperatorId eopidExpected =
		fNegated ? COperator::EopScalarSubqueryNotExists
				 : COperator::EopScalarSubqueryExists;
	return eopidExpected == pexpr->Pop()->Eopid() && 1 == pexpr->Arity();
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
	GPOS_ASSERT(EdslopExists == pop->Edslop() ||
				EdslopNotExists == pop->Edslop());
	const BOOL fNegated = EdslopNotExists == pop->Edslop();

	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	const ULONG ulSymbols = nullptr == pdrgpsym ? 0 : pdrgpsym->Size();
	if (2 != pop->UlChildren() || nullptr == pdrgpsym ||
		(0 != ulSymbols && 3 != ulSymbols))
	{
		return false;
	}

	// The predicate-bearing form is the common view of a decorrelated
	// semi-join whose condition has no extractable equality key. Equality-plus-
	// residual conditions use InSubFilter<a a p a a>; keeping the shapes
	// disjoint prevents two different DSL operators from claiming the same
	// expression. The complete predicate and its dependencies remain ordinary
	// symbols, so rules can move them without understanding ORCA scalar nodes.
	if (3 == ulSymbols)
	{
		if (fNegated ||
			COperator::EopLogicalLeftSemiJoin != pexpr->Pop()->Eopid() ||
			3 != pexpr->Arity() ||
			0 != pexpr->DeriveOuterReferences()->Size())
		{
			return false;
		}

		CColRefArray *pdrgpcrLeftKeys = GPOS_NEW(m_mp) CColRefArray(m_mp);
		CColRefArray *pdrgpcrRightKeys = GPOS_NEW(m_mp) CColRefArray(m_mp);
		CExpressionArray *pdrgpexprPred =
			GPOS_NEW(m_mp) CExpressionArray(m_mp);
		CDSLMatchView::FSplitJoinPredicate(
			m_mp, (*pexpr)[2], (*pexpr)[0], pdrgpcrLeftKeys,
			pdrgpcrRightKeys, pdrgpexprPred);
		const BOOL fPredicateOnly = 0 == pdrgpcrLeftKeys->Size() &&
			0 == pdrgpcrRightKeys->Size() && 0 < pdrgpexprPred->Size();
		pdrgpcrLeftKeys->Release();
		pdrgpcrRightKeys->Release();
		if (!fPredicateOnly)
		{
			pdrgpexprPred->Release();
			return false;
		}

		CExpression *pexprPred =
			CPredicateUtils::PexprConjunction(m_mp, pdrgpexprPred);
		CColRefSet *pcrsUsed = pexprPred->DeriveUsedColumns();
		CColRefSet *pcrsLeftDeps =
			GPOS_NEW(m_mp) CColRefSet(m_mp, *pcrsUsed);
		pcrsLeftDeps->Intersection((*pexpr)[0]->DeriveOutputColumns());
		CColRefSet *pcrsRightDeps =
			GPOS_NEW(m_mp) CColRefSet(m_mp, *pcrsUsed);
		pcrsRightDeps->Intersection((*pexpr)[1]->DeriveOutputColumns());
		CColRefSet *pcrsDeclared =
			GPOS_NEW(m_mp) CColRefSet(m_mp, *pcrsLeftDeps);
		pcrsDeclared->Union(pcrsRightDeps);
		const BOOL fDependenciesExact = pcrsDeclared->Equals(pcrsUsed);
		CColRefArray *pdrgpcrLeftDeps = pcrsLeftDeps->Pdrgpcr(m_mp);
		CColRefArray *pdrgpcrRightDeps = pcrsRightDeps->Pdrgpcr(m_mp);
		pcrsDeclared->Release();
		pcrsLeftDeps->Release();
		pcrsRightDeps->Release();

		BOOL fMatched = fDependenciesExact &&
			m_pmatcher->FMatch((*pop)[0], (*pexpr)[0], pmodel) &&
			m_pmatcher->FMatch((*pop)[1], (*pexpr)[1], pmodel) &&
			pmodel->FBind((*pdrgpsym)[0], pexprPred) &&
			pmodel->FBind((*pdrgpsym)[1], pdrgpcrLeftDeps) &&
			pmodel->FBind((*pdrgpsym)[2], pdrgpcrRightDeps);
		pexprPred->Release();
		pdrgpcrLeftDeps->Release();
		pdrgpcrRightDeps->Release();
		return fMatched;
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
		for (ULONG ul = 0; ul < pdrgpexprConj->Size(); ul++)
		{
			CExpression *pexprConj = (*pdrgpexprConj)[ul];
			if (nullptr == pexprExists &&
				FDirectExistential(pexprConj, fNegated))
			{
				pexprExists = pexprConj;
			}
		}

		BOOL fMatched = false;
		if (nullptr != pexprExists &&
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
	const COperator::EOperatorId eopidApply =
		fNegated ? COperator::EopLogicalLeftAntiSemiApply
				 : COperator::EopLogicalLeftSemiApply;
	const COperator::EOperatorId eopidOrigin =
		fNegated ? COperator::EopScalarSubqueryNotExists
				 : COperator::EopScalarSubqueryExists;
	if (eopidApply == pexpr->Pop()->Eopid())
	{
		if (3 != pexpr->Arity() ||
			!CUtils::FScalarConstTrue((*pexpr)[2]))
		{
			return false;
		}

		CLogicalApply *popApply = dynamic_cast<CLogicalApply *>(pexpr->Pop());
		if (nullptr == popApply || eopidOrigin != popApply->EopidOriginSubq())
		{
			return false;
		}

		CExpression *pexprInner = (*pexpr)[1];
		if (!fNegated && FExistsLimitOne(pexprInner))
		{
			pexprInner = (*pexprInner)[0];
		}

		return m_pmatcher->FMatch((*pop)[0], (*pexpr)[0], pmodel) &&
			   m_pmatcher->FMatch((*pop)[1], pexprInner, pmodel);
	}

	return false;
}

// EOF
