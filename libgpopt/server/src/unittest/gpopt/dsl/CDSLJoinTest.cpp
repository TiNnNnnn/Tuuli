//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLJoinTest.cpp
//
//	@doc:
//		Implementation of the Join three-stage tests (see header). Builds a live
//		CLogicalInnerJoin via the fixture, drives match -> instantiate, and asserts
//		join-key binding / predicate preservation / operator-identity gating and
//		the (FK-less) Reference rejection.
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLJoinTest.h"

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
#include "gpopt/operators/CScalarBoolOp.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

// identity InnerJoin rule: target reuses source's tables / join-key attrs.
#define GPOPT_DSL_JOIN_IDENTITY_RULE                              \
	"InnerJoin<a0 a1>(Input<t0>,Input<t1>)|"                      \
	"InnerJoin<a2 a3>(Input<t4>,Input<t5>)|"                      \
	"TableEq(t4,t0);TableEq(t5,t1);AttrsEq(a2,a0);AttrsEq(a3,a1)"

// a Reference-guarded InnerJoin rule: fires only if t0.a0 -> t1.a1 is an FK.
#define GPOPT_DSL_JOIN_REFERENCE_RULE                             \
	"InnerJoin<a0 a1>(Input<t0>,Input<t1>)|"                      \
	"InnerJoin<a2 a3>(Input<t4>,Input<t5>)|"                      \
	"TableEq(t4,t0);TableEq(t5,t1);AttrsEq(a2,a0);AttrsEq(a3,a1);" \
	"Reference(t0,a0,t1,a1)"

static CDSLRule *
PdslruleParseLocal(CMemoryPool *mp, const CHAR *sz_dsl)
{
	CWStringDynamic strErr(mp);
	return CDSLRuleParser::PdslruleParse(mp, sz_dsl, "EQ" /*verdict*/, &strErr);
}

// build InnerJoin(Get t0[2], Get t1[2], t0.c0 = t1.c0). Returns the two Gets and
// the join; caller owns them.
static void
BuildInnerJoinEqui(CDSLTestFixture &fix, CExpression **ppLeft,
				   CExpression **ppRight, CExpression **ppJoin)
{
	CColRefArray *pdrgpcrLeft = nullptr;
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprLeft = fix.PexprLogicalGet("t0", 2, &pdrgpcrLeft);
	CExpression *pexprRight = fix.PexprLogicalGet("t1", 2, &pdrgpcrRight);

	CExpression *pexprPred =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprPred);
	pexprPred->Release();

	*ppLeft = pexprLeft;
	*ppRight = pexprRight;
	*ppJoin = pexprJoin;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinTest::EresUnittest
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLJoinTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CDSLJoinTest::EresUnittest_MatchBindsJoinKeys),
		GPOS_UNITTEST_FUNC(CDSLJoinTest::EresUnittest_InstantiatePreservesJoin),
		GPOS_UNITTEST_FUNC(CDSLJoinTest::EresUnittest_NonEquiPredicateResidual),
		GPOS_UNITTEST_FUNC(CDSLJoinTest::EresUnittest_NoFireOnWrongRoot),
		GPOS_UNITTEST_FUNC(CDSLJoinTest::EresUnittest_ReferenceRejectsWithoutFK),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinTest::EresUnittest_MatchBindsJoinKeys
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLJoinTest::EresUnittest_MatchBindsJoinKeys()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_JOIN_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprLeft = nullptr;
	CExpression *pexprRight = nullptr;
	CExpression *pexprJoin = nullptr;
	BuildInnerJoinEqui(fix, &pexprLeft, &pexprRight, &pexprJoin);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprJoin, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		// <a0> left key (1 col), <a1> right key (1 col), t0/t1 bound, predicate
		// recorded on the model.
		CDSLSymbolArray *pdrgpsym = prule->PfragSrc()->PopRoot()->Pdrgpsym();
		const CDSLSymbol *psymLeft = (*pdrgpsym)[0];
		const CDSLSymbol *psymRight = (*pdrgpsym)[1];
		CColRefArray *pdrgpcrL = pmodel->PdrgpcrAttrs(psymLeft);
		CColRefArray *pdrgpcrR = pmodel->PdrgpcrAttrs(psymRight);
		if (nullptr == pdrgpcrL || 1 != pdrgpcrL->Size() ||
			nullptr == pdrgpcrR || 1 != pdrgpcrR->Size() ||
			nullptr == pmodel->PexprJoinPred())
		{
			eres = GPOS_FAILED;
		}
	}

	pmodel->Release();
	pexprLeft->Release();
	pexprRight->Release();
	pexprJoin->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinTest::EresUnittest_InstantiatePreservesJoin
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLJoinTest::EresUnittest_InstantiatePreservesJoin()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_JOIN_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprLeft = nullptr;
	CExpression *pexprRight = nullptr;
	CExpression *pexprJoin = nullptr;
	BuildInnerJoinEqui(fix, &pexprLeft, &pexprRight, &pexprJoin);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CExpression *pexprTgt = nullptr;

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprJoin, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTgt = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt ||
			COperator::EopLogicalInnerJoin != pexprTgt->Pop()->Eopid())
		{
			eres = GPOS_FAILED;
		}
		else if (!pexprJoin->DeriveOutputColumns()->Equals(
					 pexprTgt->DeriveOutputColumns()))
		{
			// output-column invariant
			eres = GPOS_FAILED;
		}
		else if ((*pexprTgt)[0] != pexprLeft || (*pexprTgt)[1] != pexprRight)
		{
			// both relational children are the grafted subtrees (pointer identity)
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprLeft->Release();
	pexprRight->Release();
	pexprJoin->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinTest::EresUnittest_NonEquiPredicateResidual
//
//	@doc:
//		Join predicate = (t0.c0 = t1.c0) AND IsNull(t0.c1). The equi conjunct binds
//		as a key; the non-equi atom is preserved as residual; instantiation still
//		produces a join whose output columns match the source.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLJoinTest::EresUnittest_NonEquiPredicateResidual()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_JOIN_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrLeft = nullptr;
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprLeft = fix.PexprLogicalGet("t0", 2, &pdrgpcrLeft);
	CExpression *pexprRight = fix.PexprLogicalGet("t1", 2, &pdrgpcrRight);

	// (t0.c0 = t1.c0) AND IsNull(t0.c1)
	CExpression *pexprEq =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprAtom = fix.PexprPredAtom((*pdrgpcrLeft)[1]);
	CExpressionArray *pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexpr->Append(pexprEq);
	pdrgpexpr->Append(pexprAtom);
	CExpression *pexprPred = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopAnd),
		pdrgpexpr);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprPred);
	pexprPred->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CExpression *pexprTgt = nullptr;

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprJoin, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		// one equi key each side, and the non-equi atom recorded as residual.
		CDSLSymbolArray *pdrgpsym = prule->PfragSrc()->PopRoot()->Pdrgpsym();
		CColRefArray *pdrgpcrL = pmodel->PdrgpcrAttrs((*pdrgpsym)[0]);
		CExpressionArray *pdrgpexprResidual = pmodel->PdrgpexprResidual();
		if (nullptr == pdrgpcrL || 1 != pdrgpcrL->Size() ||
			nullptr == pdrgpexprResidual || 1 != pdrgpexprResidual->Size())
		{
			eres = GPOS_FAILED;
		}
		else
		{
			CDSLInstantiator inst(mp);
			pexprTgt = inst.PexprInstantiate(prule, pmodel);
			if (nullptr == pexprTgt ||
				!pexprJoin->DeriveOutputColumns()->Equals(
					pexprTgt->DeriveOutputColumns()))
			{
				eres = GPOS_FAILED;
			}
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprLeft->Release();
	pexprRight->Release();
	pexprJoin->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinTest::EresUnittest_NoFireOnWrongRoot
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLJoinTest::EresUnittest_NoFireOnWrongRoot()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_JOIN_IDENTITY_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 2, &pdrgpcrOut);
	CExpression *pexprPred = fix.PexprPredAtom((*pdrgpcrOut)[0]);
	CExpression *pexprSelect = fix.PexprLogicalSelect(pexprGet, pexprPred);
	pexprPred->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);

	// a join-rooted rule must NOT match a Select.
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

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinTest::EresUnittest_ReferenceRejectsWithoutFK
//
//	@doc:
//		The match binds fine, but FCheckConstraints must REJECT because the
//		programmatic fixture carries no FK metadata (FCheckReference cannot confirm
//		the FK). Live FK verification is a base-C concern (doc M2 §C).
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLJoinTest::EresUnittest_ReferenceRejectsWithoutFK()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_JOIN_REFERENCE_RULE);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprLeft = nullptr;
	CExpression *pexprRight = nullptr;
	CExpression *pexprJoin = nullptr;
	BuildInnerJoinEqui(fix, &pexprLeft, &pexprRight, &pexprJoin);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);

	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprJoin, pmodel))
	{
		// the structural match should still succeed
		eres = GPOS_FAILED;
	}
	else if (checker.FCheck(prule, pmodel))
	{
		// but the Reference constraint must gate the fire (no FK => reject)
		eres = GPOS_FAILED;
	}

	pmodel->Release();
	pexprLeft->Release();
	pexprRight->Release();
	pexprJoin->Release();
	prule->Release();
	return eres;
}

// EOF
