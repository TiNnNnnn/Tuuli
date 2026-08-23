//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLJoinSpineRouter.cpp
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLJoinSpineRouter.h"

#include "gpos/base.h"

#include "gpopt/operators/CLogicalNAryJoin.h"

using namespace gpopt;

CDSLJoinSpineRouter::SRouteArray *
CDSLJoinSpineRouter::Pdrgproute(CMemoryPool *mp, CExpression *pexpr,
								COperator::EOperatorId eopidCarrier,
								ULONG ulDepth)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	SRouteArray *pdrgproute = GPOS_NEW(mp) SRouteArray(mp);
	if (64 <= ulDepth)
	{
		return pdrgproute;
	}

	if (eopidCarrier == pexpr->Pop()->Eopid())
	{
		// Select and LeftSemiApplyIn both carry their filtered outer relation in
		// child zero. The caller validates the carrier's remaining shape.
		if (0 < pexpr->Arity())
		{
			(*pexpr)[0]->AddRef();
			pexpr->AddRef();
			pdrgproute->Append(
				GPOS_NEW(mp) SRoute((*pexpr)[0], pexpr));
		}
		// Nested Select predicates commute. Keep looking below this Select so a
		// caller can reject the outer conjunct and still expose an eligible inner
		// one while preserving the outer Select in the rebuilt spine.
		if (COperator::EopLogicalSelect != eopidCarrier ||
			2 != pexpr->Arity())
		{
			return pdrgproute;
		}
	}

	ULONG ulSides = 0;
	switch (pexpr->Pop()->Eopid())
	{
		case COperator::EopLogicalInnerJoin:
			// A single-side filter or semi-apply commutes through either input of
			// an inner join when its outer columns originate below that input.
			ulSides = 2;
			break;
		case COperator::EopLogicalLeftOuterJoin:
			// Pulling a filter from the null-supplying side would reject unmatched
			// rows. The preserved left side is the only safe route.
			ulSides = 1;
			break;
		case COperator::EopLogicalNAryJoin:
			// The final child is scalar. For an NAryJoin containing LOJs, only
			// children marked inner/preserved are safe routing paths.
			if (2 > pexpr->Arity())
			{
				return pdrgproute;
			}
			ulSides = pexpr->Arity() - 1;
			break;
		case COperator::EopLogicalSelect:
			// Reached only while searching below a carrier Select. Pulling another
			// selection through it is equivalent to reordering conjuncts.
			ulSides = 1;
			break;
		default:
			return pdrgproute;
	}
	if ((COperator::EopLogicalInnerJoin == pexpr->Pop()->Eopid() ||
		 COperator::EopLogicalLeftOuterJoin == pexpr->Pop()->Eopid()) &&
		3 != pexpr->Arity())
	{
		return pdrgproute;
	}

	for (ULONG ulSide = 0; ulSide < ulSides; ulSide++)
	{
		if (COperator::EopLogicalNAryJoin == pexpr->Pop()->Eopid() &&
			!CLogicalNAryJoin::PopConvert(pexpr->Pop())->IsInnerJoinChild(
				ulSide))
		{
			continue;
		}
		SRouteArray *pdrgprouteChild = Pdrgproute(
			mp, (*pexpr)[ulSide], eopidCarrier, ulDepth + 1);
		for (ULONG ulRoute = 0; ulRoute < pdrgprouteChild->Size(); ulRoute++)
		{
			SRoute *prouteChild = (*pdrgprouteChild)[ulRoute];
			CExpressionArray *pdrgpexpr =
				GPOS_NEW(mp) CExpressionArray(mp);
			for (ULONG ulChild = 0; ulChild < pexpr->Arity(); ulChild++)
			{
				CExpression *pexprChild =
					(ulChild == ulSide) ? prouteChild->m_pexprRel
										 : (*pexpr)[ulChild];
				pexprChild->AddRef();
				pdrgpexpr->Append(pexprChild);
			}
			pexpr->Pop()->AddRef();
			CExpression *pexprRouted = GPOS_NEW(mp) CExpression(
				mp, pexpr->Pop(), pdrgpexpr);
			prouteChild->m_pexprCarrier->AddRef();
			pdrgproute->Append(GPOS_NEW(mp) SRoute(
				pexprRouted, prouteChild->m_pexprCarrier));
		}
		pdrgprouteChild->Release();
	}
	return pdrgproute;
}

// EOF
