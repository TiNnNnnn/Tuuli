//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLFilterSplitTest.cpp
//
//	@doc:
//		Implementation of the Filter-split tests (see header). Each test builds a
//		Select over a Get whose predicate is an AND of opaque single-column atoms
//		(the fixture's WeTune-style p0/p1 atoms), then matches a Filter-rooted DSL
//		rule against it and asserts the conjunct flatten / assign / residual
//		behaviour.
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLFilterSplitTest.h"

#include "gpos/base.h"
#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"

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

// build Select(Get t0[ulCols], AND(IsNull(c0..c(ulAtoms-1)))). Fills *ppGet /
// *ppSelect (caller releases both) and *ppdrgpcrOut with the Get's columns.
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
	pexprPred->Release();  // Select AddRef'd it

	*ppGet = pexprGet;
	*ppSelect = pexprSelect;
	*ppdrgpcrOut = pdrgpcrOut;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLFilterSplitTest::EresUnittest
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLFilterSplitTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(
			CDSLFilterSplitTest::
				EresUnittest_SingleFilterSplitsAndKeepsResidual),
		GPOS_UNITTEST_FUNC(CDSLFilterSplitTest::EresUnittest_FullCoverNoResidual),
		GPOS_UNITTEST_FUNC(
			CDSLFilterSplitTest::EresUnittest_SubsetMatchWithResidual),
		GPOS_UNITTEST_FUNC(
			CDSLFilterSplitTest::EresUnittest_ConstraintAwareBacktracking),
		GPOS_UNITTEST_FUNC(
			CDSLFilterSplitTest::EresUnittest_CorrelatedDependencyPartitions),
		GPOS_UNITTEST_FUNC(
			CDSLFilterSplitTest::
				EresUnittest_CorrelatedFilterBindsWholePredicate),
		GPOS_UNITTEST_FUNC(
			CDSLFilterSplitTest::
				EresUnittest_CorrelatedFilterCollectsSelectChain),
		GPOS_UNITTEST_FUNC(
			CDSLFilterSplitTest::
				EresUnittest_CorrelatedFilterKeepsSelectBoundary),
		GPOS_UNITTEST_FUNC(
			CDSLFilterSplitTest::
				EresUnittest_NormalizedDuplicateFilterMatchesOnce),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLFilterSplitTest::EresUnittest_CorrelatedFilterKeepsSelectBoundary()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0 o0>(Input<t0>)|Filter<p1 a1 o1>(Input<t1>)|"
			"TableEq(t1,t0);PredicateEq(p1,p0);AttrsEq(a1,a0);"
			"AttrsEq(o1,o0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrInner = nullptr;
	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprInner =
		fix.PexprLogicalGet("select_boundary_inner", 2, &pdrgpcrInner);
	CExpression *pexprOuter =
		fix.PexprLogicalGet("select_boundary_outer", 1, &pdrgpcrOuter);
	CExpression *pexprLocal =
		fix.PexprEqPred((*pdrgpcrInner)[0], (*pdrgpcrInner)[1]);
	CExpression *pexprInnerSelect =
		fix.PexprLogicalSelect(pexprInner, pexprLocal);
	pexprLocal->Release();
	CExpression *pexprCorrelated =
		fix.PexprEqPred((*pdrgpcrInner)[0], (*pdrgpcrOuter)[0]);
	CExpression *pexprOuterSelect =
		fix.PexprLogicalSelect(pexprInnerSelect, pexprCorrelated);
	pexprCorrelated->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLOp *popFilter = prule->PfragSrc()->PopRoot();
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(popFilter, pexprOuterSelect, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CExpression *pexprBound =
			pmodel->PexprPred((*popFilter->Pdrgpsym())[0]);
		CColRefArray *pdrgpcrLocal =
			pmodel->PdrgpcrAttrs((*popFilter->Pdrgpsym())[1]);
		CColRefArray *pdrgpcrOuterBound =
			pmodel->PdrgpcrAttrs((*popFilter->Pdrgpsym())[2]);
		if (nullptr == pexprBound ||
			!pexprBound->Matches((*pexprOuterSelect)[1]) ||
			nullptr == pdrgpcrLocal || 1 != pdrgpcrLocal->Size() ||
			nullptr == pdrgpcrOuterBound || 1 != pdrgpcrOuterBound->Size() ||
			pexprInnerSelect !=
				pmodel->PexprTable((*(*popFilter)[0]->Pdrgpsym())[0]))
		{
			eres = GPOS_FAILED;
		}
	}

	pmodel->Release();
	pexprOuterSelect->Release();
	pexprInnerSelect->Release();
	pexprOuter->Release();
	pexprInner->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLFilterSplitTest::EresUnittest_CorrelatedFilterCollectsSelectChain()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp,
		"SemiApply<p0 a0 a1 a2>(Input<t0>,Filter<p1 a3 a4>(Input<t1>))|"
		"Filter<p3 a7 a8>(Proj*<a10 s0>(InnerJoin<p2 a5 a6>(Input<t2>,"
		"Input<t3>)))|TableEq(t2,t0);TableEq(t3,t1);"
		"PredicateDomainSplit(p0,p1,p2,p3,a5,a6,a7,a8,t0,t1);"
		"CorrelationEquality(p3,a7,a8);OutputAttrs(a9,t0);Unique(t0,a9);"
		"AttrsUnion(a10,a9,a7);SchemaFromAttrs(s0,a10);ErrorFree(p0);"
		"Deterministic(p0);ErrorFree(p1);Deterministic(p1);"
		"ErrorFree(p2);Deterministic(p2);ErrorFree(p3);"
		"Deterministic(p3);ErrorFree(a10)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrInner = nullptr;
	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprInner =
		fix.PexprLogicalGet("select_chain_inner", 2, &pdrgpcrInner);
	CExpression *pexprOuter =
		fix.PexprLogicalGet("select_chain_outer", 1, &pdrgpcrOuter);
	CExpression *pexprLocal =
		fix.PexprEqPred((*pdrgpcrInner)[0], (*pdrgpcrInner)[1]);
	CExpression *pexprInnerSelect =
		fix.PexprLogicalSelect(pexprInner, pexprLocal);
	pexprLocal->Release();
	CExpression *pexprCorrelated =
		fix.PexprEqPred((*pdrgpcrInner)[0], (*pdrgpcrOuter)[0]);
	CExpression *pexprOuterSelect =
		fix.PexprLogicalSelect(pexprInnerSelect, pexprCorrelated);
	pexprCorrelated->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLOp *popFilter = (*prule->PfragSrc()->PopRoot())[1];
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(popFilter, pexprOuterSelect, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CExpression *pexprBound =
			pmodel->PexprPred((*popFilter->Pdrgpsym())[0]);
		CExpressionArray *pdrgpexprBound = nullptr;
		if (nullptr != pexprBound)
		{
			pdrgpexprBound =
				CPredicateUtils::PdrgpexprConjuncts(mp, pexprBound);
		}
		CExpressionArray *pdrgpexprResidual = pmodel->PdrgpexprResidual();
		CColRefArray *pdrgpcrLocal =
			pmodel->PdrgpcrAttrs((*popFilter->Pdrgpsym())[1]);
		CColRefArray *pdrgpcrOuterBound =
			pmodel->PdrgpcrAttrs((*popFilter->Pdrgpsym())[2]);
		if (nullptr == pdrgpexprBound || 2 != pdrgpexprBound->Size() ||
			nullptr == pdrgpexprResidual || 0 != pdrgpexprResidual->Size() ||
			nullptr == pdrgpcrLocal || 2 != pdrgpcrLocal->Size() ||
			nullptr == pdrgpcrOuterBound || 1 != pdrgpcrOuterBound->Size() ||
			pexprInner !=
				pmodel->PexprTable((*(*popFilter)[0]->Pdrgpsym())[0]))
		{
			eres = GPOS_FAILED;
		}
		CRefCount::SafeRelease(pdrgpexprBound);
	}

	pmodel->Release();
	pexprOuterSelect->Release();
	pexprInnerSelect->Release();
	pexprOuter->Release();
	pexprInner->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLFilterSplitTest::EresUnittest_CorrelatedFilterBindsWholePredicate()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0 o0>(Input<t0>)|Filter<p1 a1 o1>(Input<t1>)|"
			"TableEq(t1,t0);PredicateEq(p1,p0);AttrsEq(a1,a0);"
			"AttrsEq(o1,o0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrInner = nullptr;
	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprInner =
		fix.PexprLogicalGet("whole_pred_inner", 2, &pdrgpcrInner);
	CExpression *pexprOuter =
		fix.PexprLogicalGet("whole_pred_outer", 1, &pdrgpcrOuter);
	CExpressionArray *pdrgpexprConj = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexprConj->Append(
		fix.PexprEqPred((*pdrgpcrInner)[0], (*pdrgpcrInner)[1]));
	pdrgpexprConj->Append(
		fix.PexprEqPred((*pdrgpcrInner)[0], (*pdrgpcrOuter)[0]));
	CExpression *pexprPred =
		CPredicateUtils::PexprConjunction(mp, pdrgpexprConj);
	CExpression *pexprSelect = fix.PexprLogicalSelect(pexprInner, pexprPred);
	pexprPred->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLOp *popFilter = prule->PfragSrc()->PopRoot();
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(popFilter, pexprSelect, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CExpression *pexprBound =
			pmodel->PexprPred((*popFilter->Pdrgpsym())[0]);
		CExpressionArray *pdrgpexprResidual = pmodel->PdrgpexprResidual();
		CColRefArray *pdrgpcrLocal =
			pmodel->PdrgpcrAttrs((*popFilter->Pdrgpsym())[1]);
		CColRefArray *pdrgpcrOuterBound =
			pmodel->PdrgpcrAttrs((*popFilter->Pdrgpsym())[2]);
		if (nullptr == pexprBound || !pexprBound->Matches((*pexprSelect)[1]) ||
			nullptr == pdrgpexprResidual || 0 != pdrgpexprResidual->Size() ||
			nullptr == pdrgpcrLocal || 2 != pdrgpcrLocal->Size() ||
			nullptr == pdrgpcrOuterBound || 1 != pdrgpcrOuterBound->Size())
		{
			eres = GPOS_FAILED;
		}
	}

	pmodel->Release();
	pexprSelect->Release();
	pexprOuter->Release();
	pexprInner->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLFilterSplitTest::EresUnittest_CorrelatedDependencyPartitions
//
//	@doc:
//		Both predicates use the same inner column but different outer columns.
//		AttrsEq(local0,local1) must therefore match, while each outer vector stays
//		distinct. Comparing the predicates' complete used-column sets would reject
//		this valid assignment.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLFilterSplitTest::EresUnittest_CorrelatedDependencyPartitions()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp,
		"Filter<p1 a1 o1>(Filter<p0 a0 o0>(Input<t0>))|Input<t1>|"
		"AttrsEq(a0,a1);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrInner = nullptr;
	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprInner =
		fix.PexprLogicalGet("partition_inner", 1, &pdrgpcrInner);
	CExpression *pexprOuter =
		fix.PexprLogicalGet("partition_outer", 2, &pdrgpcrOuter);
	CExpressionArray *pdrgpexprConj = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexprConj->Append(
		fix.PexprEqPred((*pdrgpcrInner)[0], (*pdrgpcrOuter)[0]));
	pdrgpexprConj->Append(
		fix.PexprEqPred((*pdrgpcrInner)[0], (*pdrgpcrOuter)[1]));
	CExpression *pexprPred =
		CPredicateUtils::PexprConjunction(mp, pdrgpexprConj);
	CExpression *pexprSelect = fix.PexprLogicalSelect(pexprInner, pexprPred);
	pexprPred->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLOp *popOuterFilter = prule->PfragSrc()->PopRoot();
	CDSLOp *popInnerFilter = (*popOuterFilter)[0];
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(popOuterFilter, pexprSelect, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CColRefArray *pdrgpcrLocal0 =
			pmodel->PdrgpcrAttrs((*popInnerFilter->Pdrgpsym())[1]);
		CColRefArray *pdrgpcrOuter0 =
			pmodel->PdrgpcrAttrs((*popInnerFilter->Pdrgpsym())[2]);
		CColRefArray *pdrgpcrLocal1 =
			pmodel->PdrgpcrAttrs((*popOuterFilter->Pdrgpsym())[1]);
		CColRefArray *pdrgpcrOuter1 =
			pmodel->PdrgpcrAttrs((*popOuterFilter->Pdrgpsym())[2]);
		if (nullptr == pdrgpcrLocal0 || nullptr == pdrgpcrOuter0 ||
			nullptr == pdrgpcrLocal1 || nullptr == pdrgpcrOuter1 ||
			1 != pdrgpcrLocal0->Size() || 1 != pdrgpcrLocal1->Size() ||
			1 != pdrgpcrOuter0->Size() || 1 != pdrgpcrOuter1->Size() ||
			(*pdrgpcrLocal0)[0] != (*pdrgpcrLocal1)[0] ||
			(*pdrgpcrOuter0)[0] == (*pdrgpcrOuter1)[0])
		{
			eres = GPOS_FAILED;
		}
	}

	pmodel->Release();
	pexprOuter->Release();
	pexprInner->Release();
	pexprSelect->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLFilterSplitTest::EresUnittest_ConstraintAwareBacktracking
//
//	@doc:
//		The first two conjuncts use different columns while the first and third
//		use the same column. AttrsEq(a0,a1) therefore requires the matcher to
//		backtrack from the tempting first pair and commit the equal-column pair.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLFilterSplitTest::EresUnittest_ConstraintAwareBacktracking()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp,
		"Filter<p1 a1>(Filter<p0 a0>(Input<t0>))|Input<t1>|"
		"AttrsEq(a0,a1);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 2, &pdrgpcrOut);
	CColRef *rgpcr[] = {(*pdrgpcrOut)[0], (*pdrgpcrOut)[1],
						(*pdrgpcrOut)[0]};
	CExpression *pexprPred = fix.PexprConjunctionOfAtoms(rgpcr, 3);
	CExpression *pexprSelect = fix.PexprLogicalSelect(pexprGet, pexprPred);
	pexprPred->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLOp *popRoot = prule->PfragSrc()->PopRoot();
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(popRoot, pexprSelect, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLOp *popInner = (*popRoot)[0];
		CColRefArray *pdrgpcrA0 =
			pmodel->PdrgpcrAttrs((*popInner->Pdrgpsym())[1]);
		CColRefArray *pdrgpcrA1 =
			pmodel->PdrgpcrAttrs((*popRoot->Pdrgpsym())[1]);
		CExpressionArray *pdrgpexprResidual = pmodel->PdrgpexprResidual();
		if (nullptr == pdrgpcrA0 || nullptr == pdrgpcrA1 ||
			1 != pdrgpcrA0->Size() || 1 != pdrgpcrA1->Size() ||
			(*pdrgpcrA0)[0] != (*pdrgpcrA1)[0] ||
			nullptr == pdrgpexprResidual ||
			1 != pdrgpexprResidual->Size() ||
			!(*pdrgpexprResidual)[0]->DeriveUsedColumns()->FMember(
				(*pdrgpcrOut)[1]))
		{
			eres = GPOS_FAILED;
		}
	}

	pmodel->Release();
	pexprGet->Release();
	pexprSelect->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLFilterSplitTest::EresUnittest_SingleFilterSplitsAndKeepsResidual
//
//	@doc:
//		WeTune: FilterChainTest — a Select with a 3-conjunct predicate flattens to
//		3; one DSL Filter binds one conjunct, the other two survive as residual.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLFilterSplitTest::EresUnittest_SingleFilterSplitsAndKeepsResidual()
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

	CExpression *pexprGet = nullptr;
	CExpression *pexprSelect = nullptr;
	CColRefArray *pdrgpcrOut = nullptr;
	BuildSelectOverAtoms(fix, 3 /*cols*/, 3 /*atoms*/, &pexprGet, &pexprSelect,
						 &pdrgpcrOut);

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
		// p0 bound; residual holds the other 2 conjuncts.
		const CDSLSymbol *psymPred = (*popRoot->Pdrgpsym())[0];
		CExpression *pexprBound = pmodel->PexprPred(psymPred);
		CExpressionArray *pdrgpexprResidual = pmodel->PdrgpexprResidual();
		if (nullptr == pexprBound || nullptr == pdrgpexprResidual ||
			2 != pdrgpexprResidual->Size())
		{
			eres = GPOS_FAILED;
		}
	}

	pmodel->Release();
	pexprGet->Release();
	pexprSelect->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLFilterSplitTest::EresUnittest_FullCoverNoResidual
//
//	@doc:
//		WeTune: FilterAssignmentTest.testSimple — 3 DSL Filters over exactly 3
//		conjuncts: full 1:1 cover, no residual, three distinct pred bindings.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLFilterSplitTest::EresUnittest_FullCoverNoResidual()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	// Filter<p2 a2>(Filter<p1 a1>(Filter<p0 a0>(Input<t0>)))
	CDSLRule *prule = PdslruleParseLocal(
		mp,
		"Filter<p2 a2>(Filter<p1 a1>(Filter<p0 a0>(Input<t0>)))|Input<t1>|"
		"TableEq(t1,t0)");
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
	CDSLOp *popRoot = prule->PfragSrc()->PopRoot();

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(popRoot, pexprSelect, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		// exactly 3 conjuncts consumed => residual empty (but recorded)
		CExpressionArray *pdrgpexprResidual = pmodel->PdrgpexprResidual();
		if (nullptr == pdrgpexprResidual || 0 != pdrgpexprResidual->Size())
		{
			eres = GPOS_FAILED;
		}
		// 3 pred + 3 attrs + 1 table = 7 bindings
		if (GPOS_OK == eres && 7 != pmodel->Size())
		{
			eres = GPOS_FAILED;
		}
	}

	pmodel->Release();
	pexprGet->Release();
	pexprSelect->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLFilterSplitTest::EresUnittest_SubsetMatchWithResidual
//
//	@doc:
//		WeTune: FilterMatchTest.test0 — 2 DSL Filters over 3 conjuncts: a subset
//		match (reorder allowed), one conjunct left as residual, attrs symbols
//		bound to their conjunct's columns.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLFilterSplitTest::EresUnittest_SubsetMatchWithResidual()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp,
		"Filter<p1 a1>(Filter<p0 a0>(Input<t0>))|Input<t1>|TableEq(t1,t0)");
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
	CDSLOp *popRoot = prule->PfragSrc()->PopRoot();

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(popRoot, pexprSelect, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		// 1 residual conjunct; both attrs symbols bound to 1-column arrays.
		CExpressionArray *pdrgpexprResidual = pmodel->PdrgpexprResidual();
		CDSLOp *popInner = (*popRoot)[0];  // Filter<p0 a0>
		const CDSLSymbol *psymA0 = (*popInner->Pdrgpsym())[1];
		const CDSLSymbol *psymA1 = (*popRoot->Pdrgpsym())[1];
		CColRefArray *pdrgpcrA0 = pmodel->PdrgpcrAttrs(psymA0);
		CColRefArray *pdrgpcrA1 = pmodel->PdrgpcrAttrs(psymA1);
		if (nullptr == pdrgpexprResidual || 1 != pdrgpexprResidual->Size() ||
			nullptr == pdrgpcrA0 || 1 != pdrgpcrA0->Size() ||
			nullptr == pdrgpcrA1 || 1 != pdrgpcrA1->Size())
		{
			eres = GPOS_FAILED;
		}
	}

	pmodel->Release();
	pexprGet->Release();
	pexprSelect->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLFilterSplitTest::EresUnittest_NormalizedDuplicateFilterMatchesOnce
//
//	@doc:
//		ORCA removes duplicate AND children before DSL xforms. Two source Filter
//		variables may therefore bind the same remaining conjunct; it is consumed
//		once and produces no residual.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLFilterSplitTest::EresUnittest_NormalizedDuplicateFilterMatchesOnce()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp,
		"Filter<p1 a1>(Filter<p0 a0>(Input<t0>))|Input<t1>|TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprSelect = nullptr;
	CColRefArray *pdrgpcrOut = nullptr;
	// only ONE conjunct
	BuildSelectOverAtoms(fix, 3 /*cols*/, 1 /*atoms*/, &pexprGet, &pexprSelect,
						 &pdrgpcrOut);

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
		CDSLOp *popInner = (*popRoot)[0];
		CExpression *pexprP0 =
			pmodel->PexprPred((*popInner->Pdrgpsym())[0]);
		CExpression *pexprP1 =
			pmodel->PexprPred((*popRoot->Pdrgpsym())[0]);
		CExpressionArray *pdrgpexprResidual = pmodel->PdrgpexprResidual();
		if (nullptr == pexprP0 || pexprP0 != pexprP1 ||
			nullptr == pdrgpexprResidual || 0 != pdrgpexprResidual->Size())
		{
			eres = GPOS_FAILED;
		}
	}

	pmodel->Release();
	pexprGet->Release();
	pexprSelect->Release();
	prule->Release();
	return eres;
}

// EOF
