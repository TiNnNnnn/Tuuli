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

#include <cstring>

#include "gpos/base.h"
#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLExpressionDefinitions.h"
#include "gpopt/dsl/CDSLExprListUtils.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarValuesList.h"
#include "gpopt/operators/CScalarSwitchCase.h"
#include "gpopt/operators/CScalarArrayRefIndexList.h"
#include "gpopt/operators/CScalarSortGroupClause.h"
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
		GPOS_UNITTEST_FUNC(CDSLMatchTest::EresUnittest_DeepestFailure),
		GPOS_UNITTEST_FUNC(CDSLMatchTest::EresUnittest_TypedPredicateResultTypes),
		GPOS_UNITTEST_FUNC(CDSLMatchTest::EresUnittest_TypedScalarValueKinds),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLMatchTest::EresUnittest_TypedScalarValueKinds()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *rule = PdslruleParseLocal(mp,
		"Filter<ValueBool(n0) a0>(Input<t0>)|Filter<ValueBool(n1) a1>(Input<t1>)|"
		"t1 := t0;n1 := n0;a1 := a0");
	GPOS_UNITTEST_ASSERT(nullptr != rule);
	const CDSLSymbol *predicate = (*rule->PfragSrc()->PopRoot()->Pdrgpsym())[0];
	const CDSLSymbol *value = rule->Pexprdefs()->Pdef(predicate)->PsymOperand(0);
	const CDSLSymbol *target = (*rule->PfragTgt()->PopRoot()->Pdrgpsym())[0];
	CDSLMatcher matcher(mp, rule);
	CDSLRule *calls = PdslruleParseLocal(mp,
		"Filter<Compare(c0,v0) a0>(Input<t0>)|Filter<Compare(c1,v1) a1>(Input<t1>)|"
		"t1 := t0;c1 := c0;v1 := v0;a1 := a0");
	GPOS_UNITTEST_ASSERT(nullptr != calls);
	const auto *call_def = calls->Pexprdefs()->Pdef((*calls->PfragSrc()->PopRoot()->Pdrgpsym())[0]);
	CExpression *head = fix.PexprEqConst(fix.PcrCreateInt4("call_arg"), 7);
	CExpression *candidates[] = {
		CUtils::PexprScalarConstBool(mp, true),
		CUtils::PexprScalarConstBool(mp, false, true),
		CUtils::PexprScalarConstInt4(mp, 7),
		CUtils::PexprScalarConstInt8(mp, 7, true),
		fix.PexprLogicalGet("not_a_scalar", 1),
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp)),
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectElement(mp,
			fix.PcrCreateInt4("not_a_value")), CUtils::PexprScalarConstInt4(mp, 7)),
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarValuesList(mp)),
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarSwitchCase(mp)),
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarArrayRefIndexList(mp,
			CScalarArrayRefIndexList::EiltLower)),
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarSortGroupClause(mp, 1, 0, 0, false, false))};
	GPOS_RESULT result = GPOS_OK;
	for (ULONG kind = 0; kind < GPOS_ARRAY_SIZE(candidates); ++kind)
	{
		CExpression *expression = candidates[kind];
		if (CDSLMatchView::FScalarValue(expression) != (kind < 4) ||
			CDSLMatchView::FBooleanValue(expression) != (kind < 2)) result = GPOS_FAILED;
		CDSLModel *source = GPOS_NEW(mp) CDSLModel(mp);
		const BOOL matched = matcher.FMatchExpression(value, expression, source);
		if (matched != (kind < 4)) result = GPOS_FAILED;
		if (!matched && nullptr != source->PvalLookup(value)) result = GPOS_FAILED;
		source->Release();
		// Bypass source matching: reuse must enforce the same value boundary.
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		model->FBind(value, expression);
		CDSLInstantiator instantiator(mp);
		CExpression *built = instantiator.PexprInstantiatePredicate(rule, target, model);
		if ((nullptr != built) != (kind < 2)) result = GPOS_FAILED;
		CRefCount::SafeRelease(built);
		model->Release();
		expression->AddRef();
		CExpression *item = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CScalarProjectElement(mp, fix.PcrCreateInt4("typed_item")), expression);
		if (CDSLExprListUtils::FTypedProjectElement(item) != (2 == kind)) result = GPOS_FAILED;
		item->Release();
		CExpressionArray *arguments = GPOS_NEW(mp) CExpressionArray(mp);
		expression->AddRef();
		arguments->Append(expression);
		arguments->Append(CUtils::PexprScalarConstInt4(mp, 7));
		if (CDSLMatchView::FCallArgumentTypes(head, arguments) != (2 == kind)) result = GPOS_FAILED;
		CDSLModel *call_model = GPOS_NEW(mp) CDSLModel(mp);
		call_model->FBind(call_def->PsymOperand(0), head);
		call_model->FBind(call_def->PsymOperand(1), arguments);
		CDSLInstantiator call_instantiator(mp);
		CExpression *call = call_instantiator.PexprInstantiatePredicate(calls,
			(*calls->PfragTgt()->PopRoot()->Pdrgpsym())[0], call_model);
		if ((nullptr != call) != (2 == kind)) result = GPOS_FAILED;
		CRefCount::SafeRelease(call);
		call_model->Release();
		head->Pop()->AddRef();
		CExpression *candidate_call = GPOS_NEW(mp) CExpression(mp, head->Pop(), arguments);
		if (kind >= 4 && (CDSLMatchView::FScalarCall(candidate_call) ||
			CDSLMatchView::FCallArgumentTypes(candidate_call, head->PdrgPexpr()))) result = GPOS_FAILED;
		candidate_call->Release();
		expression->Release();
	}
	head->Release();
	calls->Release();
	rule->Release();
	return result;
}

GPOS_RESULT
CDSLMatchTest::EresUnittest_TypedPredicateResultTypes()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	const CHAR *rules[] = {
		"Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
		"t1 := t0;p1 := p0;a1 := a0",
		"Filter<p0 a0>(Input<t0>)|Filter<Not(Not(p0)) a1>(Input<t1>)|"
		"t1 := t0;a1 := a0"};
	CDSLRule *nested_rule = PdslruleParseLocal(mp,
		"Filter<Not(Not(p0)) a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
		"t1 := t0;p1 := p0;a1 := a0");
	GPOS_UNITTEST_ASSERT(nullptr != nested_rule);
	CDSLMatcher nested_matcher(mp, nested_rule);
	GPOS_RESULT result = GPOS_OK;
	for (const CHAR *text : rules)
	{
		CDSLRule *rule = PdslruleParseLocal(mp, text);
		GPOS_UNITTEST_ASSERT(nullptr != rule);
		CDSLOp *root = rule->PfragSrc()->PopRoot();
		for (ULONG kind = 0; kind < 5; ++kind)
		{
			const BOOL valid = kind < 3;
			CExpression *predicate = valid
				? CUtils::PexprScalarConstBool(mp, 0 == kind, 2 == kind)
				: CUtils::PexprScalarConstInt8(mp, 7, 4 == kind);
			CExpression *input = fix.PexprLogicalGet("typed_predicate", 1);
			predicate->AddRef();
			CExpression *nested = CUtils::PexprNegate(mp, CUtils::PexprNegate(mp, predicate));
			CDSLModel *nested_model = GPOS_NEW(mp) CDSLModel(mp);
			if (valid != nested_matcher.FMatchPredicate(
				(*nested_rule->PfragSrc()->PopRoot()->Pdrgpsym())[0], nested, nested_model))
				result = GPOS_FAILED;
			nested_model->Release();
			nested->Release();
			CExpression *source = fix.PexprLogicalSelect(input, predicate);
			CDSLModel *matched = GPOS_NEW(mp) CDSLModel(mp);
			CDSLMatcher matcher(mp, rule);
			if (valid != matcher.FMatch(root, source, matched))
				result = GPOS_FAILED;
			matched->Release();
			// Bypass source matching to test reuse and Boolean construction too.
			CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
			CColRefArray *attrs = GPOS_NEW(mp) CColRefArray(mp);
			model->FBind((*(*root)[0]->Pdrgpsym())[0], input);
			model->FBind((*root->Pdrgpsym())[0], predicate);
			model->FBind((*root->Pdrgpsym())[1], attrs);
			CDSLInstantiator instantiator(mp);
			CExpression *target = instantiator.PexprInstantiate(rule, model);
			if (valid != (nullptr != target))
				result = GPOS_FAILED;
			CRefCount::SafeRelease(target);
			attrs->Release();
			model->Release();
			source->Release();
			predicate->Release();
			input->Release();
		}
		rule->Release();
	}
	nested_rule->Release();
	return result;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLMatchTest::EresUnittest_InputBindsAnySubtree
//
//	@doc:
//		WeTune: Match.matchOne INPUT branch — an Input placeholder binds to ANY
//		logical plan node without fixing its operator kind. Input<t0> matches
//		a Select(Get) tree; t0 must bind to the whole non-leaf subtree rather than
//		being narrowed to the Get leaf.
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
	CColRef *rgpcr[1] = {(*pdrgpcrOut)[0]};
	CExpression *pexprPred = fix.PexprConjunctionOfAtoms(rgpcr, 1);
	CExpression *pexprSubtree = fix.PexprLogicalSelect(pexprGet, pexprPred);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);

	GPOS_RESULT eres = GPOS_OK;
	// A relational placeholder must reject scalar expressions without binding
	// them. Use a fresh model so an existing table binding cannot mask this.
	if (nullptr != popInput &&
		(matcher.FMatch(popInput, pexprPred, pmodel) || 0 != pmodel->Size()))
	{
		eres = GPOS_FAILED;
	}
	if (nullptr == popInput || EdslopInput != popInput->Edslop() ||
		!matcher.FMatch(popInput, pexprSubtree, pmodel) || 1 != pmodel->Size())
	{
		eres = GPOS_FAILED;
	}
	else
	{
		// t0 must be bound to the complete Select(Get) subtree we matched.
		const CDSLSymbol *psymT0 = (*popInput->Pdrgpsym())[0];
		if (pexprSubtree != pmodel->PexprTable(psymT0))
		{
			eres = GPOS_FAILED;
		}
	}

	pmodel->Release();
	pexprPred->Release();
	pexprGet->Release();
	pexprSubtree->Release();
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
//		pred): both relational children recurse and bind, and the two <a> join-key
//		symbols bind (M2 — CDSLJoinMatcher). The predicate is a cross-input
//		equality, as required by the two-slot form; the model binds
//		t0, t1, a0, a1 (4 symbols). WeTune: Match.matchOne join branch.
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
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprRight = fix.PexprLogicalGet("t1", 2, &pdrgpcrRight);
	CExpression *pexprPred = fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
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
		// both Input symbols must be bound to the respective Get subtrees; the
		// model also carries the two join-key <a> bindings (M2), so Size()==4.
		CDSLOp *popIn0 = (*popRoot)[0];
		CDSLOp *popIn1 = (*popRoot)[1];
		const CDSLSymbol *psymT0 = (*popIn0->Pdrgpsym())[0];
		const CDSLSymbol *psymT1 = (*popIn1->Pdrgpsym())[0];
		if (pexprLeft != pmodel->PexprTable(psymT0) ||
			pexprRight != pmodel->PexprTable(psymT1) || 4 != pmodel->Size())
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

GPOS_RESULT
CDSLMatchTest::EresUnittest_DeepestFailure()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp, "InnerJoin<a0 a1>(Input<t0>,Filter<p0 a2>(Input<t1>))|"
			"Input<t2>|TableEq(t2,t0)");
	if (nullptr == prule)
		return GPOS_FAILED;

	CColRefArray *pdrgpcrLeft = nullptr;
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprLeft = fix.PexprLogicalGet("left", 2, &pdrgpcrLeft);
	CExpression *pexprRight = fix.PexprLogicalGet("right", 2, &pdrgpcrRight);
	CExpression *pexprProject =
		fix.PexprLogicalProject(pexprRight, pdrgpcrRight);
	CExpression *pexprPred = fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprProject, pexprPred);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);

	const BOOL fUnexpectedMatch = matcher.FMatch(
		prule->PfragSrc()->PopRoot(), pexprJoin, pmodel);
	const GPOS_RESULT eres = !fUnexpectedMatch && matcher.FHasFailure() &&
		1 == matcher.UlFailureDepth() &&
		EdslopFilter == matcher.EdslopFailureExpected() &&
		0 == std::strcmp("CLogicalProject", matcher.SzFailureActual())
		? GPOS_OK
		: GPOS_FAILED;

	pmodel->Release();
	pexprPred->Release();
	pexprLeft->Release();
	pexprRight->Release();
	pexprProject->Release();
	pexprJoin->Release();
	prule->Release();
	return eres;
}

// EOF
