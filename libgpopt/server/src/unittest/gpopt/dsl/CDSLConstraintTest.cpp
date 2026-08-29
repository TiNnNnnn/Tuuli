//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLConstraintTest.cpp
//
//	@doc:
//		Implementation of the constraint-checker tests (see header). Each test
//		parses a rule for its constraint + symbols, builds a Get, manually binds
//		the table/attrs symbols into a model, and asserts FCheck admits/rejects.
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLConstraintTest.h"

#include "gpos/base.h"
#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringConst.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

// parse a rule DSL string to IR (verdict EQ); NULL on failure.
static CDSLRule *
PdslruleParseLocal(CMemoryPool *mp, const CHAR *sz_dsl)
{
	CWStringDynamic strErr(mp);
	return CDSLRuleParser::PdslruleParse(mp, sz_dsl, "EQ" /*verdict*/, &strErr);
}

// find the first source-fragment symbol with the given name (e.g. "t0", "a0").
// Returns NULL if absent.
static const CDSLSymbol *
PsymByName(CDSLRule *prule, const CHAR *sz_name)
{
	CDSLSymbolArray *pdrgpsym = prule->PfragSrc()->Pdrgpsym();
	const ULONG ulSyms = pdrgpsym->Size();

	// narrow name length
	ULONG ulNameLen = 0;
	while (0 != sz_name[ulNameLen])
	{
		ulNameLen++;
	}

	for (ULONG ul = 0; ul < ulSyms; ul++)
	{
		const CDSLSymbol *psym = (*pdrgpsym)[ul];
		const CWStringConst *pstr = psym->PstrName();
		if (pstr->Length() != ulNameLen)
		{
			continue;
		}
		const WCHAR *wsz = pstr->GetBuffer();
		BOOL fEq = true;
		for (ULONG i = 0; i < ulNameLen; i++)
		{
			if (wsz[i] != (WCHAR) sz_name[i])
			{
				fEq = false;
				break;
			}
		}
		if (fEq)
		{
			return psym;
		}
	}
	return nullptr;
}

// bind table symbol -> Get subtree, attrs symbol -> the given single column.
static void
BindTableAndAttr(CDSLModel *pmodel, const CDSLSymbol *psymTable,
				 CExpression *pexprGet, const CDSLSymbol *psymAttrs,
				 CColRef *pcr, CMemoryPool *mp)
{
	pmodel->FBind(psymTable, pexprGet);
	CColRefArray *pdrgpcr = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcr->Append(pcr);
	pmodel->FBind(psymAttrs, pdrgpcr);
	pdrgpcr->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintTest::EresUnittest
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLConstraintTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CDSLConstraintTest::EresUnittest_AttrsSubAdmit),
		GPOS_UNITTEST_FUNC(CDSLConstraintTest::EresUnittest_AttrsSubReject),
		GPOS_UNITTEST_FUNC(
			CDSLConstraintTest::EresUnittest_AttrsSubAttrsAdmit),
		GPOS_UNITTEST_FUNC(
			CDSLConstraintTest::EresUnittest_AttrsSubAttrsReject),
		GPOS_UNITTEST_FUNC(CDSLConstraintTest::EresUnittest_UniqueAdmit),
		GPOS_UNITTEST_FUNC(
			CDSLConstraintTest::EresUnittest_UniqueAdmitOnFixedKey),
		GPOS_UNITTEST_FUNC(
			CDSLConstraintTest::EresUnittest_UniqueAdmitThroughJoin),
		GPOS_UNITTEST_FUNC(CDSLConstraintTest::EresUnittest_UniqueReject),
		GPOS_UNITTEST_FUNC(CDSLConstraintTest::EresUnittest_NotNullAdmit),
		GPOS_UNITTEST_FUNC(
			CDSLConstraintTest::EresUnittest_NotNullThroughLeftJoin),
		GPOS_UNITTEST_FUNC(CDSLConstraintTest::EresUnittest_NotNullReject),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_UniqueAdmitThroughJoin()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp, "Proj*<a0 s0>(Input<t0>)|Input<t1>|Unique(t0,a0);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrLeft = nullptr;
	CExpression *pexprLeft =
		fix.PexprLogicalGet("left_t", 2, &pdrgpcrLeft, 0 /*key*/);
	CTableDescriptor *ptabdescRight = fix.PtabdescCreate("right_t", 2);
	CBitSet *pbsRightKey = GPOS_NEW(mp) CBitSet(mp);
	(void) pbsRightKey->ExchangeSet(0);
	(void) pbsRightKey->ExchangeSet(1);
	(void) ptabdescRight->FAddKeySet(pbsRightKey);
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprRightGet =
		fix.PexprLogicalGet(ptabdescRight, "right_t", &pdrgpcrRight);
	CExpression *pexprFixed = fix.PexprEqConst((*pdrgpcrRight)[1], 7);
	CExpression *pexprRight =
		fix.PexprLogicalSelect(pexprRightGet, pexprFixed);
	CExpression *pexprJoinPred =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprJoinPred);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	BindTableAndAttr(pmodel, PsymByName(prule, "t0"), pexprJoin,
					 PsymByName(prule, "a0"), (*pdrgpcrLeft)[0], mp);
	CDSLConstraintChecker checker(mp);
	GPOS_RESULT eres = checker.FCheck(prule, pmodel) ? GPOS_OK : GPOS_FAILED;

	// Adding a column from the other input does not invalidate the anchor-key
	// proof. The left key still identifies at most one left row and the filtered
	// composite right key permits at most one match for its join column.
	CDSLModel *pmodelCombined = GPOS_NEW(mp) CDSLModel(mp);
	pmodelCombined->FBind(PsymByName(prule, "t0"), pexprJoin);
	CColRefArray *pdrgpcrCombined = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrCombined->Append((*pdrgpcrLeft)[0]);
	pdrgpcrCombined->Append((*pdrgpcrRight)[0]);
	pmodelCombined->FBind(PsymByName(prule, "a0"), pdrgpcrCombined);
	pdrgpcrCombined->Release();
	if (!checker.FCheck(prule, pmodelCombined))
	{
		eres = GPOS_FAILED;
	}

	pmodelCombined->Release();
	pmodel->Release();
	pexprJoin->Release();
	pexprJoinPred->Release();
	pexprRight->Release();
	pexprFixed->Release();
	pexprRightGet->Release();
	pexprLeft->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_UniqueAdmitOnFixedKey()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|Unique(t0,a0);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CTableDescriptor *ptabdesc = fix.PtabdescCreate("t0", 3);
	CBitSet *pbsKey = GPOS_NEW(mp) CBitSet(mp);
	(void) pbsKey->ExchangeSet(0);
	(void) pbsKey->ExchangeSet(1);
	(void) ptabdesc->FAddKeySet(pbsKey);
	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet(ptabdesc, "t0", &pdrgpcrOut);
	CExpression *pexprPred = fix.PexprEqConst((*pdrgpcrOut)[0], 10);
	CExpression *pexprSelect = fix.PexprLogicalSelect(pexprGet, pexprPred);
	pexprPred->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	// c1 is not a key by itself, but together with fixed c0 it covers the
	// composite key (c0,c1), so c1 is unique within the selected rows.
	BindTableAndAttr(pmodel, PsymByName(prule, "t0"), pexprSelect,
					 PsymByName(prule, "a0"), (*pdrgpcrOut)[1], mp);
	CDSLConstraintChecker checker(mp);
	GPOS_RESULT eres = checker.FCheck(prule, pmodel) ? GPOS_OK : GPOS_FAILED;

	pmodel->Release();
	pexprSelect->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	AttrsSub(a0,t0)
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLConstraintTest::EresUnittest_AttrsSubAdmit()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|AttrsSub(a0,t0);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 3, &pdrgpcrOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	// a0 bound to c0 (which IS in t0's output) => subset holds
	BindTableAndAttr(pmodel, PsymByName(prule, "t0"), pexprGet,
					 PsymByName(prule, "a0"), (*pdrgpcrOut)[0], mp);

	CDSLConstraintChecker checker(mp);
	GPOS_RESULT eres = checker.FCheck(prule, pmodel) ? GPOS_OK : GPOS_FAILED;

	pmodel->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_AttrsSubReject()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|AttrsSub(a0,t0);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	// t0 is a 2-column Get; a0 bound to a column from a DIFFERENT table => not a
	// subset of t0's output.
	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 2, &pdrgpcrOut);
	CColRefArray *pdrgpcrOther = nullptr;
	CExpression *pexprOther = fix.PexprLogicalGet("tX", 2, &pdrgpcrOther);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	BindTableAndAttr(pmodel, PsymByName(prule, "t0"), pexprGet,
					 PsymByName(prule, "a0"), (*pdrgpcrOther)[0], mp);

	CDSLConstraintChecker checker(mp);
	// must REJECT: foreign column is not in t0's output
	GPOS_RESULT eres = checker.FCheck(prule, pmodel) ? GPOS_FAILED : GPOS_OK;

	pmodel->Release();
	pexprGet->Release();
	pexprOther->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	AttrsSub(a0,a1)
//---------------------------------------------------------------------------
static GPOS_RESULT
EresAttrsSubAttrs(BOOL fReverse)
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp,
		"Filter<p1 a1>(Filter<p0 a0>(Input<t0>))|Input<t1>|"
		"AttrsSub(a0,a1);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 3, &pdrgpcrOut);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	pmodel->FBind(PsymByName(prule, "t0"), pexprGet);

	CColRefArray *pdrgpcrNarrow = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrNarrow->Append((*pdrgpcrOut)[0]);
	CColRefArray *pdrgpcrWide = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrWide->Append((*pdrgpcrOut)[0]);
	pdrgpcrWide->Append((*pdrgpcrOut)[1]);
	pmodel->FBind(PsymByName(prule, "a0"),
				  fReverse ? pdrgpcrWide : pdrgpcrNarrow);
	pmodel->FBind(PsymByName(prule, "a1"),
				  fReverse ? pdrgpcrNarrow : pdrgpcrWide);
	pdrgpcrWide->Release();
	pdrgpcrNarrow->Release();

	CDSLConstraintChecker checker(mp);
	const BOOL fHolds = checker.FCheck(prule, pmodel);
	const GPOS_RESULT eres = fReverse == fHolds ? GPOS_FAILED : GPOS_OK;

	pmodel->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_AttrsSubAttrsAdmit()
{
	return EresAttrsSubAttrs(false /*fReverse*/);
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_AttrsSubAttrsReject()
{
	return EresAttrsSubAttrs(true /*fReverse*/);
}

//---------------------------------------------------------------------------
//	Unique(t0,a0)
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLConstraintTest::EresUnittest_UniqueAdmit()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|Unique(t0,a0);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	// t0 with column 0 registered as a unique key
	CTableDescriptor *ptabdesc = fix.PtabdescCreate("t0", 3, 0 /*ulKeyCol*/);
	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet(ptabdesc, "t0", &pdrgpcrOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	// a0 bound to the key column
	BindTableAndAttr(pmodel, PsymByName(prule, "t0"), pexprGet,
					 PsymByName(prule, "a0"), (*pdrgpcrOut)[0], mp);

	CDSLConstraintChecker checker(mp);
	GPOS_RESULT eres = checker.FCheck(prule, pmodel) ? GPOS_OK : GPOS_FAILED;

	pmodel->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_UniqueReject()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|Unique(t0,a0);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	// t0 with NO key registered
	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 3, &pdrgpcrOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	BindTableAndAttr(pmodel, PsymByName(prule, "t0"), pexprGet,
					 PsymByName(prule, "a0"), (*pdrgpcrOut)[0], mp);

	CDSLConstraintChecker checker(mp);
	// must REJECT: no key collection => uniqueness cannot be confirmed
	GPOS_RESULT eres = checker.FCheck(prule, pmodel) ? GPOS_FAILED : GPOS_OK;

	pmodel->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	NotNull(t0,a0)
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLConstraintTest::EresUnittest_NotNullAdmit()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|NotNull(t0,a0);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	// default fixture columns are non-nullable
	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 3, &pdrgpcrOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	BindTableAndAttr(pmodel, PsymByName(prule, "t0"), pexprGet,
					 PsymByName(prule, "a0"), (*pdrgpcrOut)[0], mp);

	CDSLConstraintChecker checker(mp);
	GPOS_RESULT eres = checker.FCheck(prule, pmodel) ? GPOS_OK : GPOS_FAILED;

	pmodel->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_NotNullThroughLeftJoin()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|NotNull(t0,a0);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrLeft = nullptr;
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprLeft =
		fix.PexprLogicalGet("left_t", 2, &pdrgpcrLeft);
	CExpression *pexprRight =
		fix.PexprLogicalGet("right_t", 2, &pdrgpcrRight);
	CExpression *pexprPred =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprJoin =
		fix.PexprLogicalLeftOuterJoin(pexprLeft, pexprRight, pexprPred);

	CDSLConstraintChecker checker(mp);
	CDSLModel *pmodelLeft = GPOS_NEW(mp) CDSLModel(mp);
	BindTableAndAttr(pmodelLeft, PsymByName(prule, "t0"), pexprJoin,
					 PsymByName(prule, "a0"), (*pdrgpcrLeft)[0], mp);
	BOOL fLeftAdmitted = checker.FCheck(prule, pmodelLeft);
	pmodelLeft->Release();

	CDSLModel *pmodelRight = GPOS_NEW(mp) CDSLModel(mp);
	BindTableAndAttr(pmodelRight, PsymByName(prule, "t0"), pexprJoin,
					 PsymByName(prule, "a0"), (*pdrgpcrRight)[0], mp);
	BOOL fRightRejected = !checker.FCheck(prule, pmodelRight);
	pmodelRight->Release();

	pexprJoin->Release();
	pexprPred->Release();
	pexprRight->Release();
	pexprLeft->Release();
	prule->Release();
	return fLeftAdmitted && fRightRejected ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_NotNullReject()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|NotNull(t0,a0);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	// t0 with NULLABLE columns
	CTableDescriptor *ptabdesc =
		fix.PtabdescCreate("t0", 3, gpos::ulong_max /*no key*/, true /*nullable*/);
	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet(ptabdesc, "t0", &pdrgpcrOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	BindTableAndAttr(pmodel, PsymByName(prule, "t0"), pexprGet,
					 PsymByName(prule, "a0"), (*pdrgpcrOut)[0], mp);

	CDSLConstraintChecker checker(mp);
	// must REJECT: bound column is nullable
	GPOS_RESULT eres = checker.FCheck(prule, pmodel) ? GPOS_FAILED : GPOS_OK;

	pmodel->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

// EOF
