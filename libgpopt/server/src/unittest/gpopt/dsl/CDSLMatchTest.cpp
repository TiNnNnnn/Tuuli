//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLMatchTest.cpp
//
//	@doc:
//		Implementation of the generic-matcher tests (see header). Migrates the
//		SKELETON slice of WeTune Match.java: the INPUT opaque-subtree binding, the
//		operator-identity dispatch, and relational-child recursion. Per-operator
//		SYMBOL binding (Filter conjuncts, join keys, Proj/Agg attrs) is asserted by
//		the dedicated components' tests (#25 / #27).
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLMatchTest.h"

#include "gpos/base.h"
#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

// local helper: parse a rule DSL string to IR, returning NULL on failure. The
// matcher tests only care about the SOURCE fragment, so target/constraints just
// need to parse.
static CDSLRule *
PdslruleParseLocal(CMemoryPool *mp, const CHAR *sz_dsl)
{
	CWStringDynamic strErr(mp);
	return CDSLRuleParser::PdslruleParse(mp, sz_dsl, "EQ" /*verdict*/, &strErr);
}

// walk to the first relational child of an op (its child[0] template)
static CDSLOp *
PopFirstChild(CDSLOp *pop)
{
	if (nullptr == pop || 0 == pop->UlChildren())
	{
		return nullptr;
	}
	return (*pop)[0];
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLMatchTest::EresUnittest
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLMatchTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CDSLMatchTest::EresUnittest_InputBindsAnySubtree),
		GPOS_UNITTEST_FUNC(
			CDSLMatchTest::EresUnittest_SelectRootMatchesAndRecurses),
		GPOS_UNITTEST_FUNC(
			CDSLMatchTest::EresUnittest_JoinRootMatchesBothChildren),
		GPOS_UNITTEST_FUNC(CDSLMatchTest::EresUnittest_IdentityGateRejects),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLMatchTest::EresUnittest_InputBindsAnySubtree
//
//	@doc:
//		WeTune: Match.matchOne INPUT branch — an Input placeholder binds to ANY
//		plan node with no type check. Here Input<t0> is matched against a bare
//		Get; t0 must bind to exactly that subtree.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLMatchTest::EresUnittest_InputBindsAnySubtree()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	// a Filter-rooted rule; we reach into its source for the Input template.
	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CDSLOp *popInput = PopFirstChild(prule->PfragSrc()->PopRoot());

	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 2, &pdrgpcrOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);

	GPOS_RESULT eres = GPOS_OK;
	if (nullptr == popInput || EdslopInput != popInput->Edslop() ||
		!matcher.FMatch(popInput, pexprGet, pmodel) || 1 != pmodel->Size())
	{
		eres = GPOS_FAILED;
	}
	else
	{
		// t0 must be bound to the very Get subtree we matched
		const CDSLSymbol *psymT0 = (*popInput->Pdrgpsym())[0];
		if (pexprGet != pmodel->PexprTable(psymT0))
		{
			eres = GPOS_FAILED;
		}
	}

	pmodel->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLMatchTest::EresUnittest_SelectRootMatchesAndRecurses
//
//	@doc:
//		Filter<p0 a0>(Input<t0>) — mapped to EopLogicalSelect — matches a live
//		Select(Get, pred): the identity gate passes, recursion descends into the
//		Select's relational child[0] (the Get) and binds t0 to it. The Select's
//		scalar predicate child[1] is beyond the template's relational arity and is
//		left for the filter matcher (#25).
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLMatchTest::EresUnittest_SelectRootMatchesAndRecurses()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 2, &pdrgpcrOut);
	CColRef *rgpcr[1] = {(*pdrgpcrOut)[0]};
	CExpression *pexprPred = fix.PexprConjunctionOfAtoms(rgpcr, 1);
	CExpression *pexprSelect = fix.PexprLogicalSelect(pexprGet, pexprPred);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLOp *popRoot = prule->PfragSrc()->PopRoot();

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(popRoot, pexprSelect, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		// the recursion must have bound t0 to the Get under the Select
		CDSLOp *popInput = PopFirstChild(popRoot);
		const CDSLSymbol *psymT0 = (*popInput->Pdrgpsym())[0];
		if (pexprGet != pmodel->PexprTable(psymT0))
		{
			eres = GPOS_FAILED;
		}
	}

	pmodel->Release();
	pexprPred->Release();
	pexprGet->Release();
	pexprSelect->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLMatchTest::EresUnittest_JoinRootMatchesBothChildren
//
//	@doc:
//		InnerJoin<a0 a1>(Input<t0>,Input<t1>) matches a live InnerJoin(Get,Get,
//		pred): both relational children recurse and bind. The scalar join
//		predicate (child[2]) is beyond the template's arity of 2 — join-key
//		binding is #27's job. WeTune: Match.matchOne join branch (identity +
//		child recursion; ORCA leaves join reordering to the memo, doc §6).
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLMatchTest::EresUnittest_JoinRootMatchesBothChildren()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp, "InnerJoin<a0 a1>(Input<t0>,Input<t1>)|Input<t2>|TableEq(t2,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrLeft = nullptr;
	CExpression *pexprLeft = fix.PexprLogicalGet("t0", 2, &pdrgpcrLeft);
	CExpression *pexprRight = fix.PexprLogicalGet("t1", 2, nullptr);
	CColRef *rgpcr[1] = {(*pdrgpcrLeft)[0]};
	CExpression *pexprPred = fix.PexprConjunctionOfAtoms(rgpcr, 1);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprPred);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLOp *popRoot = prule->PfragSrc()->PopRoot();

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(popRoot, pexprJoin, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		// both Input symbols must be bound to the respective Get subtrees
		CDSLOp *popIn0 = (*popRoot)[0];
		CDSLOp *popIn1 = (*popRoot)[1];
		const CDSLSymbol *psymT0 = (*popIn0->Pdrgpsym())[0];
		const CDSLSymbol *psymT1 = (*popIn1->Pdrgpsym())[0];
		if (pexprLeft != pmodel->PexprTable(psymT0) ||
			pexprRight != pmodel->PexprTable(psymT1) || 2 != pmodel->Size())
		{
			eres = GPOS_FAILED;
		}
	}

	pmodel->Release();
	pexprPred->Release();
	pexprLeft->Release();
	pexprRight->Release();
	pexprJoin->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLMatchTest::EresUnittest_IdentityGateRejects
//
//	@doc:
//		The operator-identity gate: a Filter-rooted template (EopLogicalSelect)
//		must NOT match a bare Get (EopLogicalGet). WeTune fails immediately when
//		OpKind differs; here FMatch returns false and binds nothing.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLMatchTest::EresUnittest_IdentityGateRejects()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = fix.PexprLogicalGet("t0", 2, nullptr);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLOp *popRoot = prule->PfragSrc()->PopRoot();

	GPOS_RESULT eres = GPOS_OK;
	// Filter template vs bare Get: identity mismatch => no match, no binding
	if (matcher.FMatch(popRoot, pexprGet, pmodel) || 0 != pmodel->Size())
	{
		eres = GPOS_FAILED;
	}

	pmodel->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

// EOF
