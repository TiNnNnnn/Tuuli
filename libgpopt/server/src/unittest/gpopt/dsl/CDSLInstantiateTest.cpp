//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLInstantiateTest.cpp
//
//	@doc:
//		Implementation of the end-to-end instantiation tests (see header). Each
//		test builds a live Select, runs the matcher to populate a model, then the
//		instantiator to build the target, and asserts the target's shape / output
//		columns / preserved conjuncts.
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLInstantiateTest.h"

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

static CDSLRule *
PdslruleParseLocal(CMemoryPool *mp, const CHAR *sz_dsl)
{
	CWStringDynamic strErr(mp);
	return CDSLRuleParser::PdslruleParse(mp, sz_dsl, "EQ" /*verdict*/, &strErr);
}

// build Select(Get t0[ulCols], AND(IsNull(c0..c(ulAtoms-1)))). See CDSLFilterSplitTest.
static void
BuildSelectOverAtoms(CDSLTestFixture &fix, ULONG ulCols, ULONG ulAtoms,
					 CExpression **ppGet, CExpression **ppSelect,
					 CColRefArray **ppdrgpcrOut)
{
	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", ulCols, &pdrgpcrOut);

	CColRef *rgpcr[8];
	GPOS_ASSERT(ulAtoms <= 8 && ulAtoms <= ulCols);
	for (ULONG ul = 0; ul < ulAtoms; ul++)
	{
		rgpcr[ul] = (*pdrgpcrOut)[ul];
	}
	CExpression *pexprPred = fix.PexprConjunctionOfAtoms(rgpcr, ulAtoms);
	CExpression *pexprSelect = fix.PexprLogicalSelect(pexprGet, pexprPred);
	pexprPred->Release();

	*ppGet = pexprGet;
	*ppSelect = pexprSelect;
	*ppdrgpcrOut = pdrgpcrOut;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiateTest::EresUnittest
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLInstantiateTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(
			CDSLInstantiateTest::EresUnittest_FilterIdentityPreservesOutput),
		GPOS_UNITTEST_FUNC(
			CDSLInstantiateTest::EresUnittest_ResidualConjunctsPreserved),
		GPOS_UNITTEST_FUNC(
			CDSLInstantiateTest::EresUnittest_TargetFilterChainFlattened),
		GPOS_UNITTEST_FUNC(
			CDSLInstantiateTest::EresUnittest_PushedFilterPredicateRemapped),
		GPOS_UNITTEST_FUNC(CDSLInstantiateTest::EresUnittest_BaseSubtreeReused),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiateTest::EresUnittest_PushedFilterPredicateRemapped
//
//	@doc:
//		The live ORCA tree has already pushed a right-key Filter below an inner
//		join. Match it through the pre-pushdown view, peel the Select from the
//		bound right Input, and instantiate the target predicate over the left key.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLInstantiateTest::EresUnittest_PushedFilterPredicateRemapped()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp,
		"Filter<p0 a2>(InnerJoin<a0 a1>(Input<t0>,Input<t1>))|"
		"Filter<p1 a5>(InnerJoin<a3 a4>(Input<t2>,Input<t3>))|"
		"AttrsEq(a1,a2);AttrsSub(a0,t0);AttrsSub(a1,t1);"
		"AttrsSub(a2,t1);AttrsSub(a5,t0);"
		"TableEq(t2,t0);TableEq(t3,t1);"
		"AttrsEq(a3,a0);AttrsEq(a4,a1);AttrsEq(a5,a0);"
		"PredicateEq(p1,p0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrLeft = nullptr;
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprLeft =
		fix.PexprLogicalGet("left_t", 1, &pdrgpcrLeft);
	CExpression *pexprRight =
		fix.PexprLogicalGet("right_t", 1, &pdrgpcrRight);
	CExpression *pexprRightPred =
		fix.PexprPredAtom((*pdrgpcrRight)[0]);
	CExpression *pexprRightSelect =
		fix.PexprLogicalSelect(pexprRight, pexprRightPred);
	CExpression *pexprJoinPred =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprJoin = fix.PexprLogicalInnerJoin(
		pexprLeft, pexprRightSelect, pexprJoinPred);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CExpression *pexprTgt = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprJoin, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		const CDSLOp *popSourceJoin =
			(*prule->PfragSrc()->PopRoot())[0];
		const CDSLSymbol *psymRightTable =
			(*(*popSourceJoin)[1]->Pdrgpsym())[0];
		if (pmodel->PexprTable(psymRightTable) != pexprRight)
		{
			eres = GPOS_FAILED;
		}

		CDSLConstraintChecker checker(mp);
		if (!checker.FCheck(prule, pmodel))
		{
			eres = GPOS_FAILED;
		}
		CDSLInstantiator inst(mp);
		pexprTgt = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt ||
			COperator::EopLogicalSelect != pexprTgt->Pop()->Eopid() ||
			COperator::EopLogicalInnerJoin != (*pexprTgt)[0]->Pop()->Eopid() ||
			COperator::EopLogicalGet != (*(*pexprTgt)[0])[1]->Pop()->Eopid() ||
			!(*pexprTgt)[1]->DeriveUsedColumns()->FMember(
				(*pdrgpcrLeft)[0]) ||
			(*pexprTgt)[1]->DeriveUsedColumns()->FMember(
				(*pdrgpcrRight)[0]))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprJoin->Release();
	pexprJoinPred->Release();
	pexprRightSelect->Release();
	pexprRightPred->Release();
	pexprRight->Release();
	pexprLeft->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiateTest::EresUnittest_TargetFilterChainFlattened
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLInstantiateTest::EresUnittest_TargetFilterChainFlattened()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp,
		"Filter<p0 a0>(Input<t0>)|"
		"Filter<p2 a2>(Filter<p1 a1>(Input<t1>))|"
		"TableEq(t1,t0);AttrsEq(a1,a0);AttrsEq(a2,a0);"
		"PredicateEq(p1,p0);PredicateEq(p2,p0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprSelect = nullptr;
	CColRefArray *pdrgpcrOut = nullptr;
	BuildSelectOverAtoms(fix, 3, 3, &pexprGet, &pexprSelect, &pdrgpcrOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CExpression *pexprTgt = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSelect, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTgt = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt ||
			COperator::EopLogicalSelect != pexprTgt->Pop()->Eopid() ||
			COperator::EopLogicalGet != (*pexprTgt)[0]->Pop()->Eopid())
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
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprGet->Release();
	pexprSelect->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiateTest::EresUnittest_FilterIdentityPreservesOutput
//
//	@doc:
//		WeTune: InstantiationTest (output-column invariant). An identity-shaped
//		rule Filter<p0 a0>(Input<t0>) -> Filter<p1 a1>(Input<t1>) with
//		TableEq/AttrsEq/PredicateEq re-binds the target to the source's artifacts.
//		Over a single-conjunct Select the instantiated target is a Select whose
//		output columns equal the source's.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLInstantiateTest::EresUnittest_FilterIdentityPreservesOutput()
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

	CExpression *pexprGet = nullptr;
	CExpression *pexprSelect = nullptr;
	CColRefArray *pdrgpcrOut = nullptr;
	BuildSelectOverAtoms(fix, 3 /*cols*/, 1 /*atoms*/, &pexprGet, &pexprSelect,
						 &pdrgpcrOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CExpression *pexprTgt = nullptr;

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSelect, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTgt = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt ||
			COperator::EopLogicalSelect != pexprTgt->Pop()->Eopid())
		{
			eres = GPOS_FAILED;
		}
		else
		{
			// output-column invariant: target output == source output
			CColRefSet *pcrsSrc = pexprSelect->DeriveOutputColumns();
			CColRefSet *pcrsTgt = pexprTgt->DeriveOutputColumns();
			if (!pcrsSrc->Equals(pcrsTgt))
			{
				eres = GPOS_FAILED;
			}
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprGet->Release();
	pexprSelect->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiateTest::EresUnittest_ResidualConjunctsPreserved
//
//	@doc:
//		A single DSL Filter matches ONE conjunct of a 3-conjunct Select; the other
//		two are residual. The instantiated target Select must re-conjoin all three
//		(bound + residuals) — dropping a predicate would be a wrong plan. We assert
//		the target predicate flattens back to 3 conjuncts.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLInstantiateTest::EresUnittest_ResidualConjunctsPreserved()
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

	CExpression *pexprGet = nullptr;
	CExpression *pexprSelect = nullptr;
	CColRefArray *pdrgpcrOut = nullptr;
	BuildSelectOverAtoms(fix, 3 /*cols*/, 3 /*atoms*/, &pexprGet, &pexprSelect,
						 &pdrgpcrOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CExpression *pexprTgt = nullptr;

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSelect, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTgt = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt ||
			COperator::EopLogicalSelect != pexprTgt->Pop()->Eopid())
		{
			eres = GPOS_FAILED;
		}
		else
		{
			// target predicate must carry all 3 conjuncts (1 bound + 2 residual)
			CExpression *pexprPred = (*pexprTgt)[1];
			CExpressionArray *pdrgpexprConj =
				CPredicateUtils::PdrgpexprConjuncts(mp, pexprPred);
			if (3 != pdrgpexprConj->Size())
			{
				eres = GPOS_FAILED;
			}
			pdrgpexprConj->Release();

			// and output columns still match the source
			if (GPOS_OK == eres &&
				!pexprSelect->DeriveOutputColumns()->Equals(
					pexprTgt->DeriveOutputColumns()))
			{
				eres = GPOS_FAILED;
			}
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprGet->Release();
	pexprSelect->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiateTest::EresUnittest_BaseSubtreeReused
//
//	@doc:
//		The target's relational child is the SAME bound subtree the source matched
//		(Input<t1> resolves via TableEq to t0's binding = the Get). Confirms
//		AddRef-graft reuse rather than a rebuild.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLInstantiateTest::EresUnittest_BaseSubtreeReused()
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

	CExpression *pexprGet = nullptr;
	CExpression *pexprSelect = nullptr;
	CColRefArray *pdrgpcrOut = nullptr;
	BuildSelectOverAtoms(fix, 2 /*cols*/, 1 /*atoms*/, &pexprGet, &pexprSelect,
						 &pdrgpcrOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CExpression *pexprTgt = nullptr;

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSelect, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTgt = inst.PexprInstantiate(prule, pmodel);
		// target Select's relational child[0] must be the very Get subtree the
		// source Select was built over (pointer identity — grafted, not rebuilt).
		if (nullptr == pexprTgt || (*pexprTgt)[0] != pexprGet)
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprGet->Release();
	pexprSelect->Release();
	prule->Release();
	return eres;
}

// EOF
