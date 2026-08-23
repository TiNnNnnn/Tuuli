//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLJoinSpineRouter.h
//
//	@doc:
//		Build transient pre-pushdown views for relational filters which ORCA has
//		moved below a join spine. Binary and n-ary inner-join paths are
//		transparent. A left outer join is transparent only through its preserved
//		(left) side, including the equivalent CLogicalNAryJoin child marking.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLJoinSpineRouter_H
#define GPOPT_CDSLJoinSpineRouter_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"

#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

class CDSLJoinSpineRouter
{
public:
	struct SRoute
	{
		CExpression *m_pexprRel;      // owned, carrier peeled from the spine
		CExpression *m_pexprCarrier;  // owned ref to Select/Apply being pulled

		SRoute(CExpression *pexprRel, CExpression *pexprCarrier)
			: m_pexprRel(pexprRel), m_pexprCarrier(pexprCarrier)
		{
		}

		~SRoute()
		{
			m_pexprCarrier->Release();
			m_pexprRel->Release();
		}
	};

	using SRouteArray = CDynamicPtrArray<SRoute, CleanupDelete>;

private:
	static SRouteArray *Pdrgproute(CMemoryPool *mp, CExpression *pexpr,
								 COperator::EOperatorId eopidCarrier,
								 ULONG ulDepth);

public:
	CDSLJoinSpineRouter() = delete;

	// Return every safe view obtained by peeling one carrier from a join spine.
	// Each result retains the exact intervening join operators and predicates.
	static SRouteArray *Pdrgproute(CMemoryPool *mp, CExpression *pexpr,
								 COperator::EOperatorId eopidCarrier)
	{
		return Pdrgproute(mp, pexpr, eopidCarrier, 0);
	}
};
}  // namespace gpopt

#endif  // !GPOPT_CDSLJoinSpineRouter_H
