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

#define GPOPT_DSL_NESTED_JOIN_IDENTITY_RULE                              \
	"InnerJoin<a0 a1>(InnerJoin<a2 a3>(Input<t0>,Input<t1>),Input<t2>)|" \
	"InnerJoin<a4 a5>(InnerJoin<a6 a7>(Input<t3>,Input<t4>),Input<t5>)|" \
	"TableEq(t3,t0);TableEq(t4,t1);TableEq(t5,t2);"                       \
	"AttrsEq(a4,a0);AttrsEq(a5,a1);AttrsEq(a6,a2);AttrsEq(a7,a3)"

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
		GPOS_UNITTEST_FUNC(
			CDSLJoinTest::EresUnittest_NestedJoinPredicatesStayLocal),
		GPOS_UNITTEST_FUNC(CDSLJoinTest::EresUnittest_NonEquiPredicateResidual),
		GPOS_UNITTEST_FUNC(CDSLJoinTest::EresUnittest_NoFireOnWrongRoot),
		GPOS_UNITTEST_FUNC(CDSLJoinTest::EresUnittest_ReferenceRejectsWithoutFK),
		GPOS_UNITTEST_FUNC(
			CDSLJoinTest::EresUnittest_ReferenceAcceptsReflexiveBaseColumn),
		GPOS_UNITTEST_FUNC(
			CDSLJoinTest::EresUnittest_ReferenceRejectsFilteredReflexiveTarget),
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
			nullptr == pmodel->PexprJoinPred(psymLeft, psymRight))
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

GPOS_RESULT
CDSLJoinTest::EresUnittest_NestedJoinPredicatesStayLocal()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule =
		PdslruleParseLocal(mp, GPOPT_DSL_NESTED_JOIN_IDENTITY_RULE);
	GPOS_ASSERT(nullptr != prule);

	CColRefArray *pdrgpcr0 = nullptr;
	CColRefArray *pdrgpcr1 = nullptr;
	CColRefArray *pdrgpcr2 = nullptr;
	CExpression *pexpr0 = fix.PexprLogicalGet("nested_t0", 2, &pdrgpcr0);
	CExpression *pexpr1 = fix.PexprLogicalGet("nested_t1", 2, &pdrgpcr1);
	CExpression *pexpr2 = fix.PexprLogicalGet("nested_t2", 2, &pdrgpcr2);
	CExpression *pexprInnerPred =
		fix.PexprEqPred((*pdrgpcr0)[0], (*pdrgpcr1)[0]);
	CExpression *pexprInner =
		fix.PexprLogicalInnerJoin(pexpr0, pexpr1, pexprInnerPred);
	CExpression *pexprOuterPred =
		fix.PexprEqPred((*pdrgpcr0)[1], (*pdrgpcr2)[1]);
	CExpression *pexprSource =
		fix.PexprLogicalInnerJoin(pexprInner, pexpr2, pexprOuterPred);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
							   pmodel));
	CDSLConstraintChecker checker(mp);
	GPOS_ASSERT(checker.FCheck(prule, pmodel));
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
	GPOS_ASSERT(nullptr != pexprTarget);
	GPOS_ASSERT(COperator::EopLogicalInnerJoin ==
				pexprTarget->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalInnerJoin ==
				(*pexprTarget)[0]->Pop()->Eopid());
	GPOS_ASSERT((*pexprTarget)[2]->Matches(pexprOuterPred));
	GPOS_ASSERT((*(*pexprTarget)[0])[2]->Matches(pexprInnerPred));
	GPOS_ASSERT(!pexprOuterPred->Matches(pexprInnerPred));

	pexprTarget->Release();
	pmodel->Release();
	pexprSource->Release();
	pexprOuterPred->Release();
	pexprInner->Release();
	pexprInnerPred->Release();
	pexpr0->Release();
	pexpr1->Release();
	pexpr2->Release();
	prule->Release();
	return GPOS_OK;
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
		// One equi key each side; the complete node-local predicate retains the
		// non-equi atom.
		CDSLSymbolArray *pdrgpsym = prule->PfragSrc()->PopRoot()->Pdrgpsym();
		CColRefArray *pdrgpcrL = pmodel->PdrgpcrAttrs((*pdrgpsym)[0]);
		if (nullptr == pdrgpcrL || 1 != pdrgpcrL->Size())
		{
			eres = GPOS_FAILED;
		}
		else
		{
			CDSLInstantiator inst(mp);
			pexprTgt = inst.PexprInstantiate(prule, pmodel);
			if (nullptr == pexprTgt ||
				!(*pexprTgt)[2]->Matches((*pexprJoin)[2]) ||
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

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinTest::EresUnittest_ReferenceAcceptsReflexiveBaseColumn
//
//	@doc:
//		Two aliases of one base relation satisfy Reference(t0,a0,t1,a1) when
//		the bound attribute vectors name the same column.  No FK metadata is
//		needed: inclusion of a relation's column in itself is reflexive.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLJoinTest::EresUnittest_ReferenceAcceptsReflexiveBaseColumn()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_JOIN_REFERENCE_RULE);
	GPOS_ASSERT(nullptr != prule);

	CTableDescriptor *ptabdesc =
		fix.PtabdescCreate("self_reference", 2, gpos::ulong_max, false);
	CColRefArray *pdrgpcrLeft = nullptr;
	CExpression *pexprLeft =
		fix.PexprLogicalGet(ptabdesc, "self_reference_left", &pdrgpcrLeft);
	ptabdesc->AddRef();
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprRight =
		fix.PexprLogicalGet(ptabdesc, "self_reference_right", &pdrgpcrRight);
	CExpression *pexprPred =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprPred);
	pexprPred->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	GPOS_RESULT eres =
		matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprJoin, pmodel) &&
				checker.FCheck(prule, pmodel)
			? GPOS_OK
			: GPOS_FAILED;

	pmodel->Release();
	pexprLeft->Release();
	pexprRight->Release();
	pexprJoin->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinTest::EresUnittest_ReferenceRejectsFilteredReflexiveTarget
//
//	@doc:
//		The reflexive shortcut must not treat a filtered alias as the complete
//		referred relation: the filter may have removed a value from the inclusion
//		dependency's target domain.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLJoinTest::EresUnittest_ReferenceRejectsFilteredReflexiveTarget()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(mp, GPOPT_DSL_JOIN_REFERENCE_RULE);
	GPOS_ASSERT(nullptr != prule);

	CTableDescriptor *ptabdesc =
		fix.PtabdescCreate("filtered_self_reference", 2, gpos::ulong_max, false);
	CColRefArray *pdrgpcrLeft = nullptr;
	CExpression *pexprLeft = fix.PexprLogicalGet(
		ptabdesc, "filtered_self_reference_left", &pdrgpcrLeft);
	ptabdesc->AddRef();
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprRightGet = fix.PexprLogicalGet(
		ptabdesc, "filtered_self_reference_right", &pdrgpcrRight);
	CExpression *pexprFilterPred = fix.PexprPredAtom((*pdrgpcrRight)[1]);
	CExpression *pexprRight =
		fix.PexprLogicalSelect(pexprRightGet, pexprFilterPred);
	pexprRightGet->Release();
	pexprFilterPred->Release();
	CExpression *pexprJoinPred =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprJoinPred);
	pexprJoinPred->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprJoin, pmodel) ||
		checker.FCheck(prule, pmodel))
	{
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
