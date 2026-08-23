//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLJoinElimTest.cpp
//
//	@doc:
//		Implementation of the join-elimination three-stage tests (see header).
//		Builds a live Proj-over-Join via the fixture and drives the full
//		match -> check -> instantiate pipeline, asserting that the join (and the
//		dropped table) really disappear from the instantiated target.
//
//		Unlike the identity Join/Proj tests, the source and target output-column
//		SETS are NOT equal here: eliminating the join removes the dropped table's
//		columns from the output. The invariant for an eliminating rule is instead:
//		the target root is a Project whose relational child is the SURVIVING table
//		(pointer-identical to the reused Get), and the projected columns are
//		preserved.
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLJoinElimTest.h"

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
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

// WeTune: sqlsolver_data/prepared/rules.txt line 205.
// LeftJoin elimination: Proj over (t0 LEFT JOIN t1) collapses to Proj over t0
// when t1's join key is unique (left rows never drop, never duplicate) and the
// projection reads only t0 columns.
#define GPOPT_DSL_LEFTJOIN_ELIM_RULE                                        \
	"Proj<a2 s0>(LeftJoin<a0 a1>(Input<t0>,Input<t1>))|"                    \
	"Proj<a3 s1>(Input<t2>)|"                                               \
	"AttrsSub(a0,t0);AttrsSub(a1,t1);AttrsSub(a2,t0);"                      \
	"Unique(t1,a1);"                                                        \
	"TableEq(t2,t0);AttrsEq(a3,a2);SchemaEq(s1,s0)"

// WeTune: sqlsolver_data/prepared/rules.txt line 180.
// InnerJoin elimination: Proj over (t0 INNER JOIN t1) collapses to Proj over t0
// when t0.a0 -> t1.a1 is a foreign key, t0.a0 is not null and t1.a1 is unique
// (every t0 row matches exactly one t1 row). FK metadata is live-relcache only,
// so this rule cannot fire on the programmatic fixture (base C verifies it).
#define GPOPT_DSL_INNERJOIN_ELIM_RULE                                       \
	"Proj<a2 s0>(InnerJoin<a0 a1>(Input<t0>,Input<t1>))|"                   \
	"Proj<a3 s1>(Input<t2>)|"                                               \
	"AttrsSub(a0,t0);AttrsSub(a1,t1);AttrsSub(a2,t0);"                      \
	"NotNull(t0,a0);Reference(t0,a0,t1,a1);Unique(t1,a1);"                  \
	"TableEq(t2,t0);AttrsEq(a3,a2);SchemaEq(s1,s0)"

static CDSLRule *
PdslruleParseLocal(CMemoryPool *mp, const CHAR *sz_dsl)
{
	CWStringDynamic strErr(mp);
	return CDSLRuleParser::PdslruleParse(mp, sz_dsl, "EQ" /*verdict*/, &strErr);
}

//---------------------------------------------------------------------------
//	@function:
//		BuildProjOverJoin
//
//	@doc:
//		Build Proj([t0.c1], <Inner|Left>Join(Get t0[2], Get t1[2], t0.c0=t1.c0)).
//		t1 is created with c0 as a unique key iff fUniqueT1. Hands back the two
//		Gets (for pointer-identity checks) and the Project root. Caller owns the
//		returned Get/Project refs.
//---------------------------------------------------------------------------
static void
BuildProjOverJoin(CDSLTestFixture &fix, BOOL fInner, BOOL fUniqueT1,
				  CExpression **ppGetT0, CExpression **ppGetT1,
				  CExpression **ppProject)
{
	CMemoryPool *mp = fix.Pmp();

	CColRefArray *pdrgpcrT0 = nullptr;
	CColRefArray *pdrgpcrT1 = nullptr;
	CExpression *pexprGetT0 = fix.PexprLogicalGet("t0", 2, &pdrgpcrT0);
	// t1: unique key on c0 (the join key) when requested.
	CExpression *pexprGetT1 = fix.PexprLogicalGet(
		"t1", 2, &pdrgpcrT1, fUniqueT1 ? 0 /*ulKeyCol*/ : gpos::ulong_max);

	// equi predicate t0.c0 = t1.c0 (a0 = {t0.c0}, a1 = {t1.c0}).
	CExpression *pexprPred =
		fix.PexprEqPred((*pdrgpcrT0)[0], (*pdrgpcrT1)[0]);
	CExpression *pexprJoin =
		fInner ? fix.PexprLogicalInnerJoin(pexprGetT0, pexprGetT1, pexprPred)
			   : fix.PexprLogicalLeftOuterJoin(pexprGetT0, pexprGetT1,
											   pexprPred);
	pexprPred->Release();

	// project only a t0 column (c1) so AttrsSub(a2,t0) holds and the surviving
	// side carries every projected column.
	CColRefArray *pdrgpcrProj = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrProj->Append((*pdrgpcrT0)[1]);
	CExpression *pexprProject = fix.PexprLogicalProject(pexprJoin, pdrgpcrProj);
	pdrgpcrProj->Release();
	pexprJoin->Release();

	*ppGetT0 = pexprGetT0;
	*ppGetT1 = pexprGetT1;
	*ppProject = pexprProject;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinElimTest::EresUnittest
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLJoinElimTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CDSLJoinElimTest::EresUnittest_LeftJoinElimFires),
		GPOS_UNITTEST_FUNC(
			CDSLJoinElimTest::EresUnittest_LeftJoinElimBelowAgg),
		GPOS_UNITTEST_FUNC(
			CDSLJoinElimTest::EresUnittest_AggregateInputRemapped),
		GPOS_UNITTEST_FUNC(
			CDSLJoinElimTest::EresUnittest_LeftJoinElimRejectsWithoutUnique),
		GPOS_UNITTEST_FUNC(
			CDSLJoinElimTest::EresUnittest_InnerJoinElimRejectsWithoutFK),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLJoinElimTest::EresUnittest_AggregateInputRemapped()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp,
		"Proj<a2 s0>(InnerJoin<a0 a1>(Input<t0>,Input<t1>))|"
		"Proj<a3 s1>(Input<t2>)|"
		"AttrsEq(a0,a2);AttrsSub(a0,t0);AttrsSub(a1,t1);"
		"AttrsSub(a2,t0);TableEq(t2,t1);AttrsEq(a3,a1);SchemaEq(s1,s0)");
	GPOS_ASSERT(nullptr != prule);

	CColRefArray *pdrgpcrLeft = nullptr;
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprLeft =
		fix.PexprLogicalGet("agg_remap_left", 1, &pdrgpcrLeft);
	CExpression *pexprRight =
		fix.PexprLogicalGet("agg_remap_right", 1, &pdrgpcrRight);
	CExpression *pexprPred =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprPred);
	pexprPred->Release();

	CColRefArray *pdrgpcrGrouping = GPOS_NEW(mp) CColRefArray(mp);
	CColRef *pcrAgg = fix.PcrCreateInt4("agg_output");
	CExpression *pexprAgg = fix.PexprLogicalGbAgg(
		pexprJoin, pdrgpcrGrouping, pcrAgg, (*pdrgpcrLeft)[0]);
	pdrgpcrGrouping->Release();
	CColRefArray *pdrgpcrProject = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrProject->Append(pcrAgg);
	CExpression *pexprProject =
		fix.PexprLogicalProject(pexprAgg, pdrgpcrProject);
	pdrgpcrProject->Release();
	pexprAgg->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprProject, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTarget ||
			COperator::EopLogicalProject != pexprTarget->Pop()->Eopid() ||
			COperator::EopLogicalGbAgg != (*pexprTarget)[0]->Pop()->Eopid() ||
			COperator::EopLogicalGet !=
				(*(*pexprTarget)[0])[0]->Pop()->Eopid() ||
			(*(*pexprTarget)[0])[0] != pexprRight ||
			!(*(*pexprTarget)[0])[1]->DeriveUsedColumns()->FMember(
				(*pdrgpcrRight)[0]) ||
			(*(*pexprTarget)[0])[1]->DeriveUsedColumns()->FMember(
				(*pdrgpcrLeft)[0]) ||
			!pexprProject->DeriveOutputColumns()->Equals(
				pexprTarget->DeriveOutputColumns()))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprProject->Release();
	pexprJoin->Release();
	pexprRight->Release();
	pexprLeft->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLJoinElimTest::EresUnittest_LeftJoinElimBelowAgg()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_LEFTJOIN_ELIM_RULE);
	GPOS_ASSERT(nullptr != prule);

	CColRefArray *pdrgpcrT0 = nullptr;
	CExpression *pexprGetT0 =
		fix.PexprLogicalGet("agg_t0", 2, &pdrgpcrT0);
	CColRefArray *pdrgpcrT1 = nullptr;
	CExpression *pexprGetT1 =
		fix.PexprLogicalGet("agg_t1", 2, &pdrgpcrT1, 0 /*unique key*/);
	CExpression *pexprPred =
		fix.PexprEqPred((*pdrgpcrT0)[0], (*pdrgpcrT1)[0]);
	CExpression *pexprJoin =
		fix.PexprLogicalLeftOuterJoin(pexprGetT0, pexprGetT1, pexprPred);
	pexprPred->Release();

	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrT0)[1]);
	pdrgpcrGroup->AddRef();
	CExpression *pexprEmptyList = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp),
		GPOS_NEW(mp) CExpressionArray(mp));
	pexprJoin->AddRef();
	CExpression *pexprAgg = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CLogicalGbAgg(
			mp, pdrgpcrGroup, COperator::EgbaggtypeGlobal),
		pexprJoin, pexprEmptyList);
	CExpression *pexprProject =
		fix.PexprLogicalProject(pexprAgg, pdrgpcrGroup);
	pexprAgg->Release();
	pdrgpcrGroup->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprProject, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTarget ||
			COperator::EopLogicalProject != pexprTarget->Pop()->Eopid() ||
			COperator::EopLogicalGbAgg != (*pexprTarget)[0]->Pop()->Eopid() ||
			COperator::EopLogicalGet !=
				(*(*pexprTarget)[0])[0]->Pop()->Eopid() ||
			(*(*pexprTarget)[0])[0] != pexprGetT0 ||
			!pexprProject->DeriveOutputColumns()->Equals(
				pexprTarget->DeriveOutputColumns()))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprProject->Release();
	pexprJoin->Release();
	pexprGetT0->Release();
	pexprGetT1->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinElimTest::EresUnittest_LeftJoinElimFires
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLJoinElimTest::EresUnittest_LeftJoinElimFires()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_LEFTJOIN_ELIM_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGetT0 = nullptr;
	CExpression *pexprGetT1 = nullptr;
	CExpression *pexprProject = nullptr;
	BuildProjOverJoin(fix, false /*fInner*/, true /*fUniqueT1*/, &pexprGetT0,
					  &pexprGetT1, &pexprProject);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTgt = nullptr;

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprProject, pmodel))
	{
		// the composite Proj-over-Join must match structurally
		eres = GPOS_FAILED;
	}
	else if (!checker.FCheck(prule, pmodel))
	{
		// Unique(t1,a1) holds (c0 is t1's key) so the check must pass
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTgt = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt ||
			COperator::EopLogicalProject != pexprTgt->Pop()->Eopid())
		{
			// target root must be a Project
			eres = GPOS_FAILED;
		}
		else if ((*pexprTgt)[0] != pexprGetT0)
		{
			// THE elimination proof: the Project's relational child is the reused
			// t0 Get (pointer identity) — the join and t1 are gone.
			eres = GPOS_FAILED;
		}
		else if (COperator::EopLogicalGet != (*pexprTgt)[0]->Pop()->Eopid())
		{
			// and it is a plain Get, not a join
			eres = GPOS_FAILED;
		}
		else
		{
			// the projected column (t0.c1) survives in the target output.
			CColRefSet *pcrsOut = pexprTgt->DeriveOutputColumns();
			CColRefArray *pdrgpcrProj = pmodel->PdrgpcrAttrs(
				prule->PfragSrc()->PopRoot()->Pdrgpsym()->operator[](0));
			if (nullptr == pdrgpcrProj || 0 == pdrgpcrProj->Size())
			{
				eres = GPOS_FAILED;
			}
			else
			{
				CColRefSet *pcrsProj = GPOS_NEW(mp) CColRefSet(mp);
				pcrsProj->Include(pdrgpcrProj);
				if (!pcrsOut->ContainsAll(pcrsProj))
				{
					eres = GPOS_FAILED;
				}
				pcrsProj->Release();
			}
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprGetT0->Release();
	pexprGetT1->Release();
	pexprProject->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinElimTest::EresUnittest_LeftJoinElimRejectsWithoutUnique
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLJoinElimTest::EresUnittest_LeftJoinElimRejectsWithoutUnique()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_LEFTJOIN_ELIM_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGetT0 = nullptr;
	CExpression *pexprGetT1 = nullptr;
	CExpression *pexprProject = nullptr;
	// t1 has NO unique key: the join could duplicate left rows.
	BuildProjOverJoin(fix, false /*fInner*/, false /*fUniqueT1*/, &pexprGetT0,
					  &pexprGetT1, &pexprProject);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprProject, pmodel))
	{
		// structural match still succeeds
		eres = GPOS_FAILED;
	}
	else if (checker.FCheck(prule, pmodel))
	{
		// but Unique(t1,a1) must gate the fire (t1.c0 is not a key)
		eres = GPOS_FAILED;
	}

	pmodel->Release();
	pexprGetT0->Release();
	pexprGetT1->Release();
	pexprProject->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinElimTest::EresUnittest_InnerJoinElimRejectsWithoutFK
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLJoinElimTest::EresUnittest_InnerJoinElimRejectsWithoutFK()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_INNERJOIN_ELIM_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGetT0 = nullptr;
	CExpression *pexprGetT1 = nullptr;
	CExpression *pexprProject = nullptr;
	// even with a unique t1 key, the Reference (FK) constraint cannot be
	// confirmed on the FK-less fixture, so the rule must not fire.
	BuildProjOverJoin(fix, true /*fInner*/, true /*fUniqueT1*/, &pexprGetT0,
					  &pexprGetT1, &pexprProject);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprProject, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else if (checker.FCheck(prule, pmodel))
	{
		// Reference(t0,a0,t1,a1) has no live FK => reject
		eres = GPOS_FAILED;
	}

	pmodel->Release();
	pexprGetT0->Release();
	pexprGetT1->Release();
	pexprProject->Release();
	prule->Release();
	return eres;
}

// EOF
