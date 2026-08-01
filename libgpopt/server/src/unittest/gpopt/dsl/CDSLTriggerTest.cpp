//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLTriggerTest.cpp
//
//	@doc:
//		Implementation of the end-to-end rule-triggering tests (see header).
//
//		The single source of truth for "did the rule fire?" is FFire(), which
//		replicates — line for line — the decision CXformDSLRule_Select::Transform
//		makes per rule: match, then check, then instantiate; a non-NULL target is
//		a fire. Every test builds an input expression, calls FFire, and asserts
//		fired / not-fired (and, where relevant, the shape of the fired target).
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLTriggerTest.h"

#include "gpos/base.h"
#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

// parse a rule DSL string to IR (verdict EQ); NULL on failure.
static CDSLRule *
PdslruleParseLocal(CMemoryPool *mp, const CHAR *sz_dsl)
{
	CWStringDynamic strErr(mp);
	return CDSLRuleParser::PdslruleParse(mp, sz_dsl, "EQ" /*verdict*/, &strErr);
}

//---------------------------------------------------------------------------
//	@function:
//		FFire
//
//	@doc:
//		Replicate CXformDSLRule_Select::Transform's per-rule decision: run the
//		three stages and report whether the rule fired. When it fires and ppTgt
//		is non-NULL, hand back the instantiated target (caller owns the ref); on
//		no-fire *ppTgt is left NULL. Any transient model is released here.
//---------------------------------------------------------------------------
static BOOL
FFire(CMemoryPool *mp, const CDSLRule *prule, CExpression *pexpr,
	  CExpression **ppTgt = nullptr)
{
	if (nullptr != ppTgt)
	{
		*ppTgt = nullptr;
	}

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);

	BOOL fFired = false;
	if (matcher.FMatch(prule->PfragSrc()->PopRoot(), pexpr, pmodel))
	{
		CDSLConstraintChecker checker(mp);
		if (checker.FCheck(prule, pmodel))
		{
			CDSLInstantiator inst(mp);
			CExpression *pexprTgt = inst.PexprInstantiate(prule, pmodel);
			if (nullptr != pexprTgt)
			{
				fFired = true;
				if (nullptr != ppTgt)
				{
					*ppTgt = pexprTgt;	// transfer ownership to caller
				}
				else
				{
					pexprTgt->Release();
				}
			}
		}
	}

	pmodel->Release();
	return fFired;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLTriggerTest::EresUnittest
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLTriggerTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CDSLTriggerTest::EresUnittest_FiresOnMatchingSelect),
		GPOS_UNITTEST_FUNC(CDSLTriggerTest::EresUnittest_NoFireOnWrongRoot),
		GPOS_UNITTEST_FUNC(
			CDSLTriggerTest::EresUnittest_ConstraintGates_KeyPresent),
		GPOS_UNITTEST_FUNC(
			CDSLTriggerTest::EresUnittest_ConstraintGates_KeyAbsent),
		GPOS_UNITTEST_FUNC(
			CDSLTriggerTest::EresUnittest_FiredTargetPreservesResiduals),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLTriggerTest::EresUnittest_FiresOnMatchingSelect
//
//	@doc:
//		WeTune: OptimizerTest (anyMatch — the result set contains the rewrite).
//		An identity-shaped Filter rule fires on a Select(Get, atom): the three
//		stages all succeed, so the shell would Add() a rewrite.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLTriggerTest::EresUnittest_FiresOnMatchingSelect()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp,
		"Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
		"TableEq(t1,t0);AttrsEq(a1,a0);PredicateEq(p1,p0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 3, &pdrgpcrOut);
	CExpression *pexprPred = fix.PexprPredAtom((*pdrgpcrOut)[0]);
	CExpression *pexprSelect = fix.PexprLogicalSelect(pexprGet, pexprPred);
	pexprPred->Release();

	GPOS_RESULT eres = FFire(mp, prule, pexprSelect) ? GPOS_OK : GPOS_FAILED;

	pexprGet->Release();
	pexprSelect->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLTriggerTest::EresUnittest_NoFireOnWrongRoot
//
//	@doc:
//		The same Select-rooted rule does NOT fire on a bare Get: the matcher's
//		identity gate (template root -> EopLogicalSelect) rejects EopLogicalGet
//		before any binding, so no rewrite is produced. (In the live optimizer the
//		shell's pattern would also never route a Get here; this asserts the engine
//		itself is safe even if handed the wrong shape.)
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLTriggerTest::EresUnittest_NoFireOnWrongRoot()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp,
		"Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
		"TableEq(t1,t0);AttrsEq(a1,a0);PredicateEq(p1,p0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	// a bare Get — no Select root for the Filter template to match.
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 3);

	// must NOT fire.
	GPOS_RESULT eres = FFire(mp, prule, pexprGet) ? GPOS_FAILED : GPOS_OK;

	pexprGet->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLTriggerTest::EresUnittest_ConstraintGates_KeyPresent
//
//	@doc:
//		WeTune: OptimizerTest test4 (precondition holds -> fires). A rule guarded
//		by Unique(t0,a0) fires when a0's matched column is the table's unique key.
//		Filter<p0 a0>(Input<t0>) -> Input<t1> is a filter-elimination shape whose
//		target reuses t0's subtree via TableEq.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLTriggerTest::EresUnittest_ConstraintGates_KeyPresent()
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

	// t0 with column 0 registered as the unique key; predicate atom over c0 so
	// the filter matcher binds a0 -> {c0}.
	CTableDescriptor *ptabdesc = fix.PtabdescCreate("t0", 3, 0 /*ulKeyCol*/);
	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet(ptabdesc, "t0", &pdrgpcrOut);
	CExpression *pexprPred = fix.PexprPredAtom((*pdrgpcrOut)[0]);
	CExpression *pexprSelect = fix.PexprLogicalSelect(pexprGet, pexprPred);
	pexprPred->Release();

	// must FIRE: a0 == key column, Unique holds.
	GPOS_RESULT eres = FFire(mp, prule, pexprSelect) ? GPOS_OK : GPOS_FAILED;

	pexprGet->Release();
	pexprSelect->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLTriggerTest::EresUnittest_ConstraintGates_KeyAbsent
//
//	@doc:
//		WeTune: OptimizerTest test3 (precondition fails -> withheld). The SAME
//		rule and SAME input shape as KeyPresent, differing only in that t0 has no
//		unique key: the check stage rejects, so the rule does not fire even though
//		match succeeded. This is the trigger/no-trigger pair that proves the check
//		stage actually gates triggering.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLTriggerTest::EresUnittest_ConstraintGates_KeyAbsent()
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

	// t0 with NO key registered — identical shape, precondition now false.
	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 3, &pdrgpcrOut);
	CExpression *pexprPred = fix.PexprPredAtom((*pdrgpcrOut)[0]);
	CExpression *pexprSelect = fix.PexprLogicalSelect(pexprGet, pexprPred);
	pexprPred->Release();

	// must NOT fire: Unique cannot be confirmed => check rejects.
	GPOS_RESULT eres = FFire(mp, prule, pexprSelect) ? GPOS_FAILED : GPOS_OK;

	pexprGet->Release();
	pexprSelect->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLTriggerTest::EresUnittest_FiredTargetPreservesResiduals
//
//	@doc:
//		When a rule fires over a multi-conjunct Select, the target the shell would
//		Add() must be a correct plan: a single DSL Filter matches one of three
//		conjuncts, and the fired target re-conjoins all three (no dropped
//		predicate) while keeping the source's output columns. This is the
//		triggering-level guarantee behind the instantiate-level residual test.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLTriggerTest::EresUnittest_FiredTargetPreservesResiduals()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp,
		"Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
		"TableEq(t1,t0);AttrsEq(a1,a0);PredicateEq(p1,p0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	// Select over a 3-conjunct predicate (c0 AND c1 AND c2).
	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 3, &pdrgpcrOut);
	CColRef *rgpcr[3] = {(*pdrgpcrOut)[0], (*pdrgpcrOut)[1], (*pdrgpcrOut)[2]};
	CExpression *pexprPred = fix.PexprConjunctionOfAtoms(rgpcr, 3);
	CExpression *pexprSelect = fix.PexprLogicalSelect(pexprGet, pexprPred);
	pexprPred->Release();

	CExpression *pexprTgt = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!FFire(mp, prule, pexprSelect, &pexprTgt))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		// fired target is a Select whose predicate carries all 3 conjuncts and
		// whose output columns equal the source's.
		if (nullptr == pexprTgt ||
			COperator::EopLogicalSelect != pexprTgt->Pop()->Eopid())
		{
			eres = GPOS_FAILED;
		}
		else
		{
			CExpressionArray *pdrgpexprConj =
				CPredicateUtils::PdrgpexprConjuncts(mp, (*pexprTgt)[1]);
			if (3 != pdrgpexprConj->Size())
			{
				eres = GPOS_FAILED;
			}
			pdrgpexprConj->Release();

			if (GPOS_OK == eres &&
				!pexprSelect->DeriveOutputColumns()->Equals(
					pexprTgt->DeriveOutputColumns()))
			{
				eres = GPOS_FAILED;
			}
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pexprGet->Release();
	pexprSelect->Release();
	prule->Release();
	return eres;
}

// EOF
