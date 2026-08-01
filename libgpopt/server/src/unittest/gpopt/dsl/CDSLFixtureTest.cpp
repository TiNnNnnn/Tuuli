//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLFixtureTest.cpp
//
//	@doc:
//		Smoke tests for CDSLTestFixture (see header).
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLFixtureTest.h"

#include "gpos/base.h"
#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CDSLFixtureTest::EresUnittest
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLFixtureTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CDSLFixtureTest::EresUnittest_GetDerivesColumns),
		GPOS_UNITTEST_FUNC(CDSLFixtureTest::EresUnittest_SelectConjuncts),
		GPOS_UNITTEST_FUNC(CDSLFixtureTest::EresUnittest_JoinDerivesColumns),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLFixtureTest::EresUnittest_GetDerivesColumns
//
//	@doc:
//		A 3-column Get derives exactly 3 output columns.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLFixtureTest::EresUnittest_GetDerivesColumns()
{
	CAutoMemoryPool amp;
	CDSLTestFixture fix(amp.Pmp());

	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 3, &pdrgpcrOut);

	GPOS_RESULT eres = GPOS_OK;
	CColRefSet *pcrs = pexprGet->DeriveOutputColumns();
	if (nullptr == pcrs || 3 != pcrs->Size() || nullptr == pdrgpcrOut ||
		3 != pdrgpcrOut->Size())
	{
		eres = GPOS_FAILED;
	}

	pexprGet->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLFixtureTest::EresUnittest_SelectConjuncts
//
//	@doc:
//		Select(Get, IsNull(c0) AND IsNull(c1) AND IsNull(c2)) flattens to 3
//		conjuncts via CPredicateUtils; output columns == the Get's 3 columns.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLFixtureTest::EresUnittest_SelectConjuncts()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 3, &pdrgpcrOut);

	CColRef *rgpcr[3] = {(*pdrgpcrOut)[0], (*pdrgpcrOut)[1], (*pdrgpcrOut)[2]};
	CExpression *pexprPred = fix.PexprConjunctionOfAtoms(rgpcr, 3);
	CExpression *pexprSelect = fix.PexprLogicalSelect(pexprGet, pexprPred);

	GPOS_RESULT eres = GPOS_OK;

	// conjunct split: the AND predicate flattens to 3 atoms
	CExpression *pexprScalar = (*pexprSelect)[1];
	CExpressionArray *pdrgpexprConj =
		CPredicateUtils::PdrgpexprConjuncts(mp, pexprScalar);
	if (3 != pdrgpexprConj->Size())
	{
		eres = GPOS_FAILED;
	}
	pdrgpexprConj->Release();

	// output-column invariant: Select passes the Get's columns through
	CColRefSet *pcrs = pexprSelect->DeriveOutputColumns();
	if (nullptr == pcrs || 3 != pcrs->Size())
	{
		eres = GPOS_FAILED;
	}

	pexprPred->Release();
	pexprGet->Release();
	pexprSelect->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLFixtureTest::EresUnittest_JoinDerivesColumns
//
//	@doc:
//		InnerJoin(Get t0[2], Get t1[2], IsNull(t0.c0)) derives 4 output columns.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLFixtureTest::EresUnittest_JoinDerivesColumns()
{
	CAutoMemoryPool amp;
	CDSLTestFixture fix(amp.Pmp());

	CColRefArray *pdrgpcrLeft = nullptr;
	CExpression *pexprLeft = fix.PexprLogicalGet("t0", 2, &pdrgpcrLeft);
	CExpression *pexprRight = fix.PexprLogicalGet("t1", 2, nullptr);

	CColRef *rgpcr[1] = {(*pdrgpcrLeft)[0]};
	CExpression *pexprPred = fix.PexprConjunctionOfAtoms(rgpcr, 1);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprPred);

	GPOS_RESULT eres = GPOS_OK;
	CColRefSet *pcrs = pexprJoin->DeriveOutputColumns();
	if (nullptr == pcrs || 4 != pcrs->Size())
	{
		eres = GPOS_FAILED;
	}

	pexprPred->Release();
	pexprLeft->Release();
	pexprRight->Release();
	pexprJoin->Release();
	return eres;
}

// EOF
