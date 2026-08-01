//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLProjTest.cpp
//
//	@doc:
//		Implementation of the Proj three-stage tests (see header). Builds a live
//		CLogicalProject via the fixture, drives match -> instantiate, and asserts
//		binding / output-column invariant / operator-identity gating.
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLProjTest.h"

#include "gpos/base.h"
#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

// identity Proj rule: target reuses source's table / attrs / schema bindings.
#define GPOPT_DSL_PROJ_IDENTITY_RULE                            \
	"Proj<a0 s0>(Input<t0>)|Proj<a1 s1>(Input<t1>)|"            \
	"TableEq(t1,t0);AttrsEq(a1,a0);SchemaEq(s1,s0)"

static CDSLRule *
PdslruleParseLocal(CMemoryPool *mp, const CHAR *sz_dsl)
{
	CWStringDynamic strErr(mp);
	return CDSLRuleParser::PdslruleParse(mp, sz_dsl, "EQ" /*verdict*/, &strErr);
}

// build Project(Get t0[ulCols], projlist over the first ulProj output columns).
static void
BuildProjectOverGet(CDSLTestFixture &fix, ULONG ulCols, ULONG ulProj,
					CExpression **ppGet, CExpression **ppProject,
					CColRefArray **ppdrgpcrOut)
{
	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", ulCols, &pdrgpcrOut);

	GPOS_ASSERT(ulProj <= ulCols);
	CColRefArray *pdrgpcrProj = GPOS_NEW(fix.Pmp()) CColRefArray(fix.Pmp());
	for (ULONG ul = 0; ul < ulProj; ul++)
	{
		pdrgpcrProj->Append((*pdrgpcrOut)[ul]);
	}
	CExpression *pexprProject = fix.PexprLogicalProject(pexprGet, pdrgpcrProj);
	pdrgpcrProj->Release();

	*ppGet = pexprGet;
	*ppProject = pexprProject;
	*ppdrgpcrOut = pdrgpcrOut;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLProjTest::EresUnittest
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLProjTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(
			CDSLProjTest::EresUnittest_MatchBindsProjectedColumns),
		GPOS_UNITTEST_FUNC(CDSLProjTest::EresUnittest_InstantiatePreservesOutput),
		GPOS_UNITTEST_FUNC(CDSLProjTest::EresUnittest_NoFireOnWrongRoot),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLProjTest::EresUnittest_MatchBindsProjectedColumns
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLProjTest::EresUnittest_MatchBindsProjectedColumns()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_PROJ_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprProject = nullptr;
	CColRefArray *pdrgpcrOut = nullptr;
	BuildProjectOverGet(fix, 3 /*cols*/, 2 /*proj*/, &pexprGet, &pexprProject,
						&pdrgpcrOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprProject, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		// <a0> must be bound to the 2 projected columns; <t0> to the Get subtree;
		// the project list must have been recorded on the model.
		const CDSLSymbol *psymAttrs =
			prule->PfragSrc()->PopRoot()->Pdrgpsym()->operator[](0);
		CColRefArray *pdrgpcrBound = pmodel->PdrgpcrAttrs(psymAttrs);
		if (nullptr == pdrgpcrBound || 2 != pdrgpcrBound->Size() ||
			nullptr == pmodel->PexprProjList())
		{
			eres = GPOS_FAILED;
		}
	}

	pmodel->Release();
	pexprGet->Release();
	pexprProject->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLProjTest::EresUnittest_InstantiatePreservesOutput
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLProjTest::EresUnittest_InstantiatePreservesOutput()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_PROJ_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprProject = nullptr;
	CColRefArray *pdrgpcrOut = nullptr;
	BuildProjectOverGet(fix, 3 /*cols*/, 2 /*proj*/, &pexprGet, &pexprProject,
						&pdrgpcrOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CExpression *pexprTgt = nullptr;

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprProject, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTgt = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt ||
			COperator::EopLogicalProject != pexprTgt->Pop()->Eopid())
		{
			eres = GPOS_FAILED;
		}
		else if (!pexprProject->DeriveOutputColumns()->Equals(
					 pexprTgt->DeriveOutputColumns()))
		{
			// output-column invariant
			eres = GPOS_FAILED;
		}
		else if ((*pexprTgt)[0] != pexprGet)
		{
			// relational child is the grafted Get subtree (pointer identity)
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprGet->Release();
	pexprProject->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLProjTest::EresUnittest_NoFireOnWrongRoot
//
//	@doc:
//		A Proj-rooted rule must NOT match a Select (operator-identity gate). Build
//		a Select and confirm the Proj source template fails to match.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLProjTest::EresUnittest_NoFireOnWrongRoot()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_PROJ_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 2 /*cols*/, &pdrgpcrOut);
	CExpression *pexprPred = fix.PexprPredAtom((*pdrgpcrOut)[0]);
	CExpression *pexprSelect = fix.PexprLogicalSelect(pexprGet, pexprPred);
	pexprPred->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);

	// must NOT match a Select
	GPOS_RESULT eres =
		matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSelect, pmodel)
			? GPOS_FAILED
			: GPOS_OK;

	pmodel->Release();
	pexprGet->Release();
	pexprSelect->Release();
	prule->Release();
	return eres;
}

// EOF
