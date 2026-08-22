//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLAggTest.cpp
//
//	@doc:
//		Implementation of dedup elimination and corpus-format real Agg three-stage
//		tests. The latter uses bare Agg<a a f s p>, matching MONSOON/dataset/rules.
//
//		As with join elimination the source and target output-column SETS are not
//		equal (the GbAgg outputs only its grouping cols; the Select outputs the
//		child's full columns — a superset), so the invariant asserted is
//		ContainsAll, not Equals.
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLAggTest.h"

#include "gpos/base.h"
#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CScalarAggFunc.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

// Semantic source: WeTune dedup identity — a deduplicated projection over a
// relation equals a plain projection when the deduplicated columns are unique
// (SELECT DISTINCT k = SELECT k when k is a key). Proj* (dedup) routes to the
// GbAgg bucket; the target is the bare Input (drop the dedup). This is the DSL
// analogue of ORCA's CXformSimplifyGbAgg::FDropGbAgg.
#define GPOPT_DSL_DISTINCT_ELIM_RULE                                        \
	"Proj*<a0 s0>(Input<t0>)|"                                              \
	"Input<t2>|"                                                            \
	"AttrsSub(a0,t0);Unique(t0,a0);TableEq(t2,t0)"

#define GPOPT_DSL_DISTINCT_TO_PROJ_RULE                                     \
	"Proj*<a0 s0>(Input<t0>)|"                                              \
	"Proj<a1 s1>(Input<t1>)|"                                               \
	"AttrsSub(a0,t0);Unique(t0,a0);TableEq(t1,t0);"                         \
	"AttrsEq(a1,a0);SchemaEq(s1,s0)"

#define GPOPT_DSL_AGG_IDENTITY_RULE                                         \
	"Agg<a0 a1 f0 s0 p0>(Input<t0>)|"                                      \
	"Agg<a2 a3 f1 s1 p1>(Input<t1>)|"                                      \
	"AttrsSub(a0,t0);AttrsSub(a1,t0);"                                     \
	"TableEq(t1,t0);AttrsEq(a2,a0);AttrsEq(a3,a1);"                        \
	"FuncEq(f1,f0);SchemaEq(s1,s0);PredicateEq(p1,p0)"

static CDSLRule *
PdslruleParseLocal(CMemoryPool *mp, const CHAR *sz_dsl)
{
	CWStringDynamic strErr(mp);
	return CDSLRuleParser::PdslruleParse(mp, sz_dsl, "EQ" /*verdict*/, &strErr);
}

//---------------------------------------------------------------------------
//	@function:
//		PexprDedupGbAgg
//
//	@doc:
//		Build GbAgg(grouping=[t0.c0], Get t0[2]) with an EMPTY agg list — ORCA's
//		SELECT DISTINCT c0. t0's c0 is a unique key iff fUniqueKey. Hands back the
//		Get (for pointer-identity checks) and the GbAgg root. Caller owns both refs.
//---------------------------------------------------------------------------
static void
BuildDedupGbAgg(CDSLTestFixture &fix, BOOL fUniqueKey, CExpression **ppGet,
				CExpression **ppGbAgg)
{
	CMemoryPool *mp = fix.Pmp();

	CColRefArray *pdrgpcrT0 = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet(
		"t0", 2, &pdrgpcrT0, fUniqueKey ? 0 /*ulKeyCol*/ : gpos::ulong_max);

	// group by the (unique) key column c0.
	CColRefArray *pdrgpcrGrp = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGrp->Append((*pdrgpcrT0)[0]);
	CExpression *pexprGbAgg = fix.PexprLogicalGbAgg(pexprGet, pdrgpcrGrp);
	pdrgpcrGrp->Release();

	*ppGet = pexprGet;
	*ppGbAgg = pexprGbAgg;
}

static void
BuildRealGbAgg(CDSLTestFixture &fix, CExpression **ppGet,
			   CExpression **ppGbAgg, CColRefArray **ppdrgpcrInput,
			   CColRef **ppcrAggOut)
{
	CMemoryPool *mp = fix.Pmp();
	CColRefArray *pdrgpcrInput = nullptr;
	CExpression *pexprGet =
		fix.PexprLogicalGet("t0", 2, &pdrgpcrInput);
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrInput)[0]);
	CColRef *pcrAggOut = fix.PcrCreateInt4("max_c1");
	CExpression *pexprGbAgg = fix.PexprLogicalGbAgg(
		pexprGet, pdrgpcrGroup, pcrAggOut, (*pdrgpcrInput)[1]);
	pdrgpcrGroup->Release();

	*ppGet = pexprGet;
	*ppGbAgg = pexprGbAgg;
	*ppdrgpcrInput = pdrgpcrInput;
	*ppcrAggOut = pcrAggOut;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLAggTest::EresUnittest
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLAggTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_MatchBindsDedupGbAgg),
		GPOS_UNITTEST_FUNC(
			CDSLAggTest::EresUnittest_InstantiateProducesSelectOverChild),
		GPOS_UNITTEST_FUNC(
			CDSLAggTest::EresUnittest_InstantiateDedupToPlainProj),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_RejectsWithoutUnique),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_RejectsNonEmptyAggList),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_MatchBindsRealAgg),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_InstantiateRealAgg),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_HavingRoundTrip),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_RejectsWrongAggFunction),
		GPOS_UNITTEST_FUNC(CDSLAggTest::EresUnittest_NoFireOnWrongRoot),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLAggTest::EresUnittest_InstantiateDedupToPlainProj()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_DISTINCT_TO_PROJ_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprGbAgg = nullptr;
	BuildDedupGbAgg(fix, true /*fUniqueKey*/, &pexprGet, &pexprGbAgg);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTgt = nullptr;
	GPOS_RESULT eres = GPOS_OK;

	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGbAgg, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTgt = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt ||
			COperator::EopLogicalSelect != pexprTgt->Pop()->Eopid() ||
			(*pexprTgt)[0] != pexprGet)
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprGet->Release();
	pexprGbAgg->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_MatchBindsRealAgg()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_AGG_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprGbAgg = nullptr;
	CColRefArray *pdrgpcrInput = nullptr;
	CColRef *pcrAggOut = nullptr;
	BuildRealGbAgg(fix, &pexprGet, &pexprGbAgg, &pdrgpcrInput, &pcrAggOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLOp *popSrc = prule->PfragSrc()->PopRoot();
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(popSrc, pexprGbAgg, pmodel) || 6 != pmodel->Size())
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLSymbolArray *syms = popSrc->Pdrgpsym();
		CColRefArray *group = pmodel->PdrgpcrAttrs((*syms)[0]);
		CColRefArray *inputs = pmodel->PdrgpcrAttrs((*syms)[1]);
		CExpressionArray *funcs = pmodel->PdrgpexprFunc((*syms)[2]);
		CColRefArray *schema = pmodel->PdrgpcrSchema((*syms)[3]);
		CExpression *having = pmodel->PexprPred((*syms)[4]);
		if (1 != group->Size() || (*group)[0] != (*pdrgpcrInput)[0] ||
			1 != inputs->Size() || (*inputs)[0] != (*pdrgpcrInput)[1] ||
			1 != funcs->Size() ||
			COperator::EopScalarAggFunc != (*funcs)[0]->Pop()->Eopid() ||
			2 != schema->Size() || (*schema)[1] != pcrAggOut ||
			!CUtils::FScalarConstTrue(having))
		{
			eres = GPOS_FAILED;
		}
	}

	pmodel->Release();
	pexprGet->Release();
	pexprGbAgg->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_InstantiateRealAgg()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_AGG_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprGbAgg = nullptr;
	CColRefArray *pdrgpcrInput = nullptr;
	CColRef *pcrAggOut = nullptr;
	BuildRealGbAgg(fix, &pexprGet, &pexprGbAgg, &pdrgpcrInput, &pcrAggOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTgt = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGbAgg, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTgt = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt ||
			COperator::EopLogicalGbAgg != pexprTgt->Pop()->Eopid() ||
			(*pexprTgt)[0] != pexprGet || 1 != (*pexprTgt)[1]->Arity() ||
			!pexprTgt->DeriveOutputColumns()->Equals(
				pexprGbAgg->DeriveOutputColumns()))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprGet->Release();
	pexprGbAgg->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_HavingRoundTrip()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_AGG_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprGbAgg = nullptr;
	CColRefArray *pdrgpcrInput = nullptr;
	CColRef *pcrAggOut = nullptr;
	BuildRealGbAgg(fix, &pexprGet, &pexprGbAgg, &pdrgpcrInput, &pcrAggOut);
	CExpression *pexprHaving = fix.PexprPredAtom(pcrAggOut);
	CExpression *pexprSelect =
		fix.PexprLogicalSelect(pexprGbAgg, pexprHaving);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTgt = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	CDSLOp *popSrc = prule->PfragSrc()->PopRoot();
	if (!matcher.FMatch(popSrc, pexprSelect, pmodel) ||
		pmodel->PexprPred((*popSrc->Pdrgpsym())[4]) != pexprHaving ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTgt = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt ||
			COperator::EopLogicalSelect != pexprTgt->Pop()->Eopid() ||
			COperator::EopLogicalGbAgg != (*pexprTgt)[0]->Pop()->Eopid() ||
			(*(*pexprTgt)[0])[0] != pexprGet || (*pexprTgt)[1] != pexprHaving ||
			!pexprTgt->DeriveOutputColumns()->Equals(
				pexprSelect->DeriveOutputColumns()))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprHaving->Release();
	pexprSelect->Release();
	pexprGet->Release();
	pexprGbAgg->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLAggTest::EresUnittest_RejectsWrongAggFunction()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp,
		"Agg_min<a0 a1 a2 f0 s0 p0>(Input<t0>)|Input<t1>|TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprGbAgg = nullptr;
	CColRefArray *pdrgpcrInput = nullptr;
	CColRef *pcrAggOut = nullptr;
	BuildRealGbAgg(fix, &pexprGet, &pexprGbAgg, &pdrgpcrInput, &pcrAggOut);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_RESULT eres = matcher.FMatch(
		prule->PfragSrc()->PopRoot(), pexprGbAgg, pmodel)
		? GPOS_FAILED
		: GPOS_OK;

	pmodel->Release();
	pexprGet->Release();
	pexprGbAgg->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLAggTest::EresUnittest_MatchBindsDedupGbAgg
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLAggTest::EresUnittest_MatchBindsDedupGbAgg()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_DISTINCT_ELIM_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprGbAgg = nullptr;
	BuildDedupGbAgg(fix, true /*fUniqueKey*/, &pexprGet, &pexprGbAgg);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGbAgg, pmodel))
	{
		// the dedup GbAgg must match the Proj* source root
		eres = GPOS_FAILED;
	}
	else if (!pmodel->FDedupDrop())
	{
		// and the matcher must have flagged a dedup drop
		eres = GPOS_FAILED;
	}

	pmodel->Release();
	pexprGet->Release();
	pexprGbAgg->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLAggTest::EresUnittest_InstantiateProducesSelectOverChild
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLAggTest::EresUnittest_InstantiateProducesSelectOverChild()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_DISTINCT_ELIM_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprGbAgg = nullptr;
	BuildDedupGbAgg(fix, true /*fUniqueKey*/, &pexprGet, &pexprGbAgg);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTgt = nullptr;

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGbAgg, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else if (!checker.FCheck(prule, pmodel))
	{
		// Unique(t0,a0) holds (c0 is t0's key) so the check must pass
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTgt = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt ||
			COperator::EopLogicalSelect != pexprTgt->Pop()->Eopid())
		{
			// dedup drop => target root is Select(child, TRUE) (FDropGbAgg idiom)
			eres = GPOS_FAILED;
		}
		else if ((*pexprTgt)[0] != pexprGet)
		{
			// THE elimination proof: the Select's relational child is the reused
			// t0 Get (pointer identity) — the GbAgg is gone.
			eres = GPOS_FAILED;
		}
		else if (COperator::EopLogicalGet != (*pexprTgt)[0]->Pop()->Eopid())
		{
			// and it is a plain Get, not a GbAgg
			eres = GPOS_FAILED;
		}
		else
		{
			// the deduplicated columns survive in the target output (Select over
			// the Get outputs a SUPERSET of the GbAgg's grouping-only output).
			CColRefSet *pcrsOut = pexprTgt->DeriveOutputColumns();
			CColRefArray *pdrgpcrGrp = pmodel->PdrgpcrAttrs(
				prule->PfragSrc()->PopRoot()->Pdrgpsym()->operator[](0));
			if (nullptr == pdrgpcrGrp || 0 == pdrgpcrGrp->Size())
			{
				eres = GPOS_FAILED;
			}
			else
			{
				CColRefSet *pcrsGrp = GPOS_NEW(mp) CColRefSet(mp);
				pcrsGrp->Include(pdrgpcrGrp);
				if (!pcrsOut->ContainsAll(pcrsGrp))
				{
					eres = GPOS_FAILED;
				}
				pcrsGrp->Release();
			}
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprGet->Release();
	pexprGbAgg->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLAggTest::EresUnittest_RejectsWithoutUnique
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLAggTest::EresUnittest_RejectsWithoutUnique()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_DISTINCT_ELIM_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprGbAgg = nullptr;
	// grouping column is NOT a key: the dedup is not redundant.
	BuildDedupGbAgg(fix, false /*fUniqueKey*/, &pexprGet, &pexprGbAgg);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGbAgg, pmodel))
	{
		// structural match still succeeds (empty agg list is a pure dedup)
		eres = GPOS_FAILED;
	}
	else if (checker.FCheck(prule, pmodel))
	{
		// but Unique(t0,a0) must gate the fire (c0 is not a key)
		eres = GPOS_FAILED;
	}

	pmodel->Release();
	pexprGet->Release();
	pexprGbAgg->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLAggTest::EresUnittest_RejectsNonEmptyAggList
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLAggTest::EresUnittest_RejectsNonEmptyAggList()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_DISTINCT_ELIM_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrT0 = nullptr;
	CExpression *pexprGet =
		fix.PexprLogicalGet("t0", 2, &pdrgpcrT0, 0 /*ulKeyCol*/);

	CColRefArray *pdrgpcrGrp = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGrp->Append((*pdrgpcrT0)[0]);
	// GbAgg WITH a (dummy) aggregate function => non-empty agg list.
	CColRef *pcrAgg = fix.PcrCreateInt4("agg0");
	CExpression *pexprGbAgg = fix.PexprLogicalGbAgg(pexprGet, pdrgpcrGrp, pcrAgg);
	pdrgpcrGrp->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);

	GPOS_RESULT eres = GPOS_OK;
	if (matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGbAgg, pmodel))
	{
		// a GbAgg that computes an aggregate is not a pure dedup => must not match
		eres = GPOS_FAILED;
	}

	pmodel->Release();
	pexprGet->Release();
	pexprGbAgg->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLAggTest::EresUnittest_NoFireOnWrongRoot
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLAggTest::EresUnittest_NoFireOnWrongRoot()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_DISTINCT_ELIM_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	// a bare Get (not a GbAgg).
	CColRefArray *pdrgpcrT0 = nullptr;
	CExpression *pexprGet =
		fix.PexprLogicalGet("t0", 2, &pdrgpcrT0, 0 /*ulKeyCol*/);

	// a plain Project (CLogicalProject, not a dedup GbAgg).
	CColRefArray *pdrgpcrProj = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrProj->Append((*pdrgpcrT0)[0]);
	CExpression *pexprProject = fix.PexprLogicalProject(pexprGet, pdrgpcrProj);
	pdrgpcrProj->Release();

	CDSLMatcher matcher(mp);

	GPOS_RESULT eres = GPOS_OK;

	CDSLModel *pmodel1 = GPOS_NEW(mp) CDSLModel(mp);
	if (matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGet, pmodel1))
	{
		// Proj* source root must not match a bare Get
		eres = GPOS_FAILED;
	}
	pmodel1->Release();

	CDSLModel *pmodel2 = GPOS_NEW(mp) CDSLModel(mp);
	if (matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprProject, pmodel2))
	{
		// nor a plain CLogicalProject (that is the non-distinct Proj shell's job)
		eres = GPOS_FAILED;
	}
	pmodel2->Release();

	pexprProject->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

// EOF
