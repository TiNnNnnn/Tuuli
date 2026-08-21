//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLAggMatcher.cpp
//
//	@doc:
//		Implementation of the Proj* dedup and real aggregate symbol binders.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLAggMatcher.h"

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CScalarAggFunc.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"

using namespace gpopt;

namespace
{
BOOL
FAggNameEquals(CMemoryPool *mp, const CWStringConst *pstrActual,
			   const CHAR *szExpected)
{
	CWStringConst strExpected(mp, szExpected);
	return pstrActual->Equals(&strExpected);
}

BOOL
FAggFuncMatches(CMemoryPool *mp, const CDSLOp *popAgg,
				const CScalarAggFunc *popFunc)
{
	if (popAgg->FDistinct() != popFunc->IsDistinct())
	{
		return false;
	}

	switch (popAgg->Edslaggfunc())
	{
		case EdslaggfuncUnknown:
			return true;
		case EdslaggfuncSentinel:
			return false;
		case EdslaggfuncAverage:
			return FAggNameEquals(mp, popFunc->PstrAggFunc(), "avg") ||
				   FAggNameEquals(mp, popFunc->PstrAggFunc(), "average");
		default:
			return FAggNameEquals(
				mp, popFunc->PstrAggFunc(),
				CDSLOpKindTable::SzAggFuncName(popAgg->Edslaggfunc()));
	}
}
}  // namespace

//---------------------------------------------------------------------------
//	@function:
//		CDSLAggMatcher::PdrgpcrGrouping
//
//	@doc:
//		Copy the GbAgg's grouping columns into a fresh ordered array. Caller owns
//		the returned ref.
//---------------------------------------------------------------------------
CColRefArray *
CDSLAggMatcher::PdrgpcrGrouping(CExpression *pexprAgg) const
{
	CLogicalGbAgg *popGbAgg = CLogicalGbAgg::PopConvert(pexprAgg->Pop());
	CColRefArray *pdrgpcrGrp = popGbAgg->Pdrgpcr();

	CColRefArray *pdrgpcr = GPOS_NEW(m_mp) CColRefArray(m_mp);
	const ULONG ulCols = (nullptr == pdrgpcrGrp) ? 0 : pdrgpcrGrp->Size();
	for (ULONG ul = 0; ul < ulCols; ul++)
	{
		pdrgpcr->Append((*pdrgpcrGrp)[ul]);
	}
	return pdrgpcr;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLAggMatcher::FMatch
//---------------------------------------------------------------------------
BOOL
CDSLAggMatcher::FMatchDedup(const CDSLOp *popAgg, CExpression *pexprAgg,
						CDSLModel *pmodel) const
{
	// pure-dedup gate: reject any GbAgg that computes aggregate functions (a
	// non-empty project list). Mirrors CXformSimplifyGbAgg::FDropGbAgg's
	// `if (0 < pexprProjectList->Arity()) return false;`; real Agg templates are
	// handled separately by FMatchAggregate.
	if (0 != (*pexprAgg)[1]->Arity())
	{
		return false;
	}

	// fire only on the ORIGINAL user-level global dedup, exactly like native
	// CXformSimplifyGbAgg::Exfp: skip scalar aggs (no grouping), skip the
	// FD-annotated / split-generated aggs (PdrgpcrMinimal set — Local and
	// intermediate stages produced by CXformSplitGbAgg), and require Global.
	// Firing on the split-generated stages would insert dedup-drop alternatives
	// into groups where they are invalid and break memo plan extraction
	// (CMemo.cpp PocLookupBest returns null).
	CLogicalGbAgg *popGbAgg = CLogicalGbAgg::PopConvert(pexprAgg->Pop());
	if (COperator::EgbaggtypeGlobal != popGbAgg->Egbaggtype() ||
		nullptr != popGbAgg->PdrgpcrMinimal() ||
		nullptr == popGbAgg->Pdrgpcr() || 0 == popGbAgg->Pdrgpcr()->Size())
	{
		return false;
	}

	// Proj*<a s>: both attrs and schema are the grouping columns.
	CDSLSymbolArray *pdrgpsym = popAgg->Pdrgpsym();
	if (nullptr == pdrgpsym || 2 != pdrgpsym->Size())
	{
		return false;
	}

	// bind <a> and <s> to the grouping columns — for a pure dedup the referenced,
	// output and grouping columns coincide. FBind AddRefs; release the local copy.
	CColRefArray *pdrgpcrGrp = PdrgpcrGrouping(pexprAgg);
	BOOL fBound = pmodel->FBind((*pdrgpsym)[0], pdrgpcrGrp) &&
				  pmodel->FBind((*pdrgpsym)[1], pdrgpcrGrp);
	pdrgpcrGrp->Release();
	if (!fBound)
	{
		return false;
	}

	// the relational child recurses through the generic matcher (GbAgg has exactly
	// one relational child; the template's UlChildren() is 1).
	if (1 != popAgg->UlChildren())
	{
		return false;
	}
	if (!m_pmatcher->FMatch((*popAgg)[0], (*pexprAgg)[0], pmodel))
	{
		return false;
	}

	// flag the source root as a redundant dedup GbAgg: the instantiator drops it
	// by wrapping the resolved child in Select(child, TRUE), mirroring
	// CXformSimplifyGbAgg::FDropGbAgg (the memo-safe operator-drop idiom).
	pmodel->SetDedupDrop();
	return true;
}

BOOL
CDSLAggMatcher::FMatchAggregate(const CDSLOp *popAgg,
								 CExpression *pexprAgg,
								 CDSLModel *pmodel) const
{
	CLogicalGbAgg *popGbAgg = CLogicalGbAgg::PopConvert(pexprAgg->Pop());
	CExpression *pexprAggList = (*pexprAgg)[1];
	if (COperator::EgbaggtypeGlobal != popGbAgg->Egbaggtype() ||
		nullptr != popGbAgg->PdrgpcrMinimal() ||
		COperator::EopScalarProjectList != pexprAggList->Pop()->Eopid() ||
		0 == pexprAggList->Arity())
	{
		return false;
	}

	CDSLSymbolArray *pdrgpsym = popAgg->Pdrgpsym();
	if (nullptr == pdrgpsym ||
		(5 != pdrgpsym->Size() && 6 != pdrgpsym->Size()) ||
		1 != popAgg->UlChildren())
	{
		return false;
	}
	const BOOL fLegacy = 5 == pdrgpsym->Size();
	const ULONG ulFunc = fLegacy ? 2 : 3;
	const ULONG ulSchema = fLegacy ? 3 : 4;
	const ULONG ulHaving = fLegacy ? 4 : 5;

	CExpressionArray *pdrgpexprFuncs =
		GPOS_NEW(m_mp) CExpressionArray(m_mp);
	CColRefArray *pdrgpcrAggOut = GPOS_NEW(m_mp) CColRefArray(m_mp);
	CColRefSet *pcrsAggInputs = GPOS_NEW(m_mp) CColRefSet(m_mp);
	BOOL fValid = true;

	for (ULONG ul = 0; ul < pexprAggList->Arity() && fValid; ul++)
	{
		CExpression *pexprPrEl = (*pexprAggList)[ul];
		if (COperator::EopScalarProjectElement != pexprPrEl->Pop()->Eopid() ||
			1 != pexprPrEl->Arity())
		{
			fValid = false;
			break;
		}

		CExpression *pexprFunc = (*pexprPrEl)[0];
		if (COperator::EopScalarAggFunc != pexprFunc->Pop()->Eopid() ||
			!FAggFuncMatches(
				m_mp, popAgg,
				CScalarAggFunc::PopConvert(pexprFunc->Pop())))
		{
			fValid = false;
			break;
		}

		pexprFunc->AddRef();
		pdrgpexprFuncs->Append(pexprFunc);
		pdrgpcrAggOut->Append(
			CScalarProjectElement::PopConvert(pexprPrEl->Pop())->Pcr());
		pcrsAggInputs->Include(pexprFunc->DeriveUsedColumns());
	}

	if (!fValid)
	{
		pdrgpexprFuncs->Release();
		pdrgpcrAggOut->Release();
		pcrsAggInputs->Release();
		return false;
	}

	CColRefArray *pdrgpcrGroup = PdrgpcrGrouping(pexprAgg);
	CColRefArray *pdrgpcrAggInputs = pcrsAggInputs->Pdrgpcr(m_mp);
	CColRefArray *pdrgpcrSchema = GPOS_NEW(m_mp) CColRefArray(m_mp);
	for (ULONG ul = 0; ul < pdrgpcrGroup->Size(); ul++)
	{
		pdrgpcrSchema->Append((*pdrgpcrGroup)[ul]);
	}
	for (ULONG ul = 0; ul < pdrgpcrAggOut->Size(); ul++)
	{
		pdrgpcrSchema->Append((*pdrgpcrAggOut)[ul]);
	}

	// ORCA represents a missing HAVING clause by the absence of a Select above
	// the GbAgg. Bind the DSL predicate symbol to TRUE, matching WeTune's
	// concrete-predicate translation.
	CExpression *pexprHaving = CUtils::PexprScalarConstBool(m_mp, true);
	BOOL fBound = pmodel->FBind((*pdrgpsym)[0], pdrgpcrGroup) &&
				  pmodel->FBind((*pdrgpsym)[1], pdrgpcrAggInputs);
	if (fBound && !fLegacy)
	{
		fBound = pmodel->FBind((*pdrgpsym)[2], pdrgpcrAggOut);
	}
	fBound = fBound &&
			 pmodel->FBind((*pdrgpsym)[ulFunc], pdrgpexprFuncs) &&
			 pmodel->FBind((*pdrgpsym)[ulSchema], pdrgpcrSchema) &&
			 pmodel->FBind((*pdrgpsym)[ulHaving], pexprHaving);

	pdrgpcrGroup->Release();
	pdrgpcrAggInputs->Release();
	pdrgpcrAggOut->Release();
	pdrgpexprFuncs->Release();
	pdrgpcrSchema->Release();
	pcrsAggInputs->Release();
	pexprHaving->Release();

	return fBound &&
		   m_pmatcher->FMatch((*popAgg)[0], (*pexprAgg)[0], pmodel);
}

BOOL
CDSLAggMatcher::FMatch(const CDSLOp *popAgg, CExpression *pexprAgg,
					   CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != popAgg);
	GPOS_ASSERT(EdslopProj == popAgg->Edslop() ||
				EdslopAgg == popAgg->Edslop());
	GPOS_ASSERT(nullptr != pexprAgg);

	if (COperator::EopLogicalGbAgg != pexprAgg->Pop()->Eopid() ||
		2 != pexprAgg->Arity())
	{
		return false;
	}

	if (EdslopProj == popAgg->Edslop())
	{
		return popAgg->FDistinct() &&
			   FMatchDedup(popAgg, pexprAgg, pmodel);
	}
	return FMatchAggregate(popAgg, pexprAgg, pmodel);
}

// EOF
