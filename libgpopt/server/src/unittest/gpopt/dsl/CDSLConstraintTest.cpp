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
		GPOS_UNITTEST_FUNC(CDSLConstraintTest::EresUnittest_UniqueAdmit),
		GPOS_UNITTEST_FUNC(CDSLConstraintTest::EresUnittest_UniqueReject),
		GPOS_UNITTEST_FUNC(CDSLConstraintTest::EresUnittest_NotNullAdmit),
		GPOS_UNITTEST_FUNC(CDSLConstraintTest::EresUnittest_NotNullReject),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
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
