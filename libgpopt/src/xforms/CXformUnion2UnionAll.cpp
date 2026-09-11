//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformUnion2UnionAll.cpp
//
//	@doc:
//		Implementation of the transformation that takes a logical union and
//		coverts it into an aggregate over a logical union all
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformUnion2UnionAll.h"

#include "gpos/base.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CLogicalUnion.h"
#include "gpopt/operators/CLogicalUnionAll.h"
#include "gpopt/operators/CPatternMultiLeaf.h"
#include "gpopt/operators/CScalarProjectList.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CXformUnion2UnionAll::CXformUnion2UnionAll
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformUnion2UnionAll::CXformUnion2UnionAll(CMemoryPool *mp)
	:  // pattern
	  CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalUnion(mp),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternMultiLeaf(mp))))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformUnion2UnionAll::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformUnion2UnionAll::Transform(CXformContext *pxfctxt, CXformResult *pxfres,
								CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();
	// extract components
	CLogicalUnion *popUnion = CLogicalUnion::PopConvert(pexpr->Pop());
	CColRefArray *pdrgpcrOutput = popUnion->PdrgpcrOutput();
	CColRef2dArray *pdrgpdrgpcrInput = popUnion->PdrgpdrgpcrInput();

	CExpressionArray *pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	const ULONG arity = pexpr->Arity();

	for (ULONG ul = 0; ul < arity; ul++)
	{
		CExpression *pexprChild = (*pexpr)[ul];
		pexprChild->AddRef();
		pdrgpexpr->Append(pexprChild);
	}

	pdrgpcrOutput->AddRef();
	pdrgpdrgpcrInput->AddRef();

	// assemble new logical operator
	CExpression *pexprUnionAll = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalUnionAll(mp, pdrgpcrOutput, pdrgpdrgpcrInput),
		pdrgpexpr);

	// Deduplication of zero-column tuples is existence, not a scalar aggregate:
	// empty input must stay empty rather than producing one aggregate row.
	if (0 == pdrgpcrOutput->Size())
	{
		pxfres->Add(GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CLogicalLimit(mp, GPOS_NEW(mp) COrderSpec(mp),
										  true, true, false),
			pexprUnionAll, CUtils::PexprScalarConstInt8(mp, 0),
			CUtils::PexprScalarConstInt8(mp, 1)));
		return;
	}

	pdrgpcrOutput->AddRef();

	CExpression *pexprProjList =
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
								 GPOS_NEW(mp) CExpressionArray(mp));

	CExpression *pexprAgg = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CLogicalGbAgg(mp, pdrgpcrOutput,
								   COperator::EgbaggtypeGlobal /*egbaggtype*/),
		pexprUnionAll, pexprProjList);

	pxfres->Add(pexprAgg);
}

// EOF
