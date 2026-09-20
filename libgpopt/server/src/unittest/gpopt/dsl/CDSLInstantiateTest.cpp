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

#include <string>

#include "gpos/base.h"
#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLExpressionDefinitions.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/dsl/CDSLRulePrefixIndex.h"
#include "gpopt/operators/CExpressionUtils.h"
#include "gpopt/operators/CLogicalConstTableGet.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CLogicalFullOuterJoin.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"
#include "gpopt/operators/CLogicalLeftAntiSemiJoin.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarBooleanTest.h"
#include "gpopt/search/CGroupExpression.h"
#include "gpopt/search/CMemo.h"

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
			CDSLInstantiateTest::EresUnittest_JoinExpressionBindings),
		GPOS_UNITTEST_FUNC(
			CDSLInstantiateTest::EresUnittest_ExpressionBindings),
		GPOS_UNITTEST_FUNC(
			CDSLInstantiateTest::EresUnittest_FilterIdentityPreservesOutput),
		GPOS_UNITTEST_FUNC(
			CDSLInstantiateTest::EresUnittest_ResidualConjunctsPreserved),
		GPOS_UNITTEST_FUNC(
			CDSLInstantiateTest::EresUnittest_TargetFilterChainFlattened),
		GPOS_UNITTEST_FUNC(
			CDSLInstantiateTest::EresUnittest_DerivedFilterConjunction),
		GPOS_UNITTEST_FUNC(
			CDSLInstantiateTest::EresUnittest_DerivedPredicateNotTrue),
		GPOS_UNITTEST_FUNC(
			CDSLInstantiateTest::EresUnittest_PredicateNegationNullSemantics),
		GPOS_UNITTEST_FUNC(
			CDSLInstantiateTest::EresUnittest_PushedFilterPredicateRemapped),
		GPOS_UNITTEST_FUNC(
			CDSLInstantiateTest::EresUnittest_PredicateDomainSplit),
		GPOS_UNITTEST_FUNC(
			CDSLInstantiateTest::EresUnittest_PredicateDomainSplitRejectsMixedAtom),
		GPOS_UNITTEST_FUNC(CDSLInstantiateTest::EresUnittest_BaseSubtreeReused),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLInstantiateTest::EresUnittest_JoinExpressionBindings()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	auto input = [&](CColRefArray **columns) {
		*columns = GPOS_NEW(mp) CColRefArray(mp);
		(*columns)->Append(fix.PcrCreateInt4("first"));
		(*columns)->Append(fix.PcrCreateInt4("second"));
		// Memo property derivation needs real metadata; constant relations keep
		// this structural regression independent of fabricated catalog stats.
		return GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalConstTableGet(
			mp, *columns, GPOS_NEW(mp) IDatum2dArray(mp)));
	};
	auto negate = [mp](CExpression *input) {
		input->AddRef();
		return GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopNot), input);
	};
	const CHAR *kinds[] = {"InnerJoin", "LeftJoin", "FullJoin", "SemiJoin", "AntiJoin"};
	BOOL ok = true;
	for (ULONG kind = 0; kind < GPOS_ARRAY_SIZE(kinds); kind++)
	{
		const std::string name(kinds[kind]);
		const std::string text = name +
			"<Not(Not(p0)) a0 a1>(Filter<Not(Not(p1)) a2>(Input<t0>),"
			"Filter<Not(Not(p2)) a3>(Input<t1>))|" + name +
			"<p3 a4 a5>(Filter<p4 a6>(Input<t2>),Filter<p5 a7>(Input<t3>))|"
			"TableEq(t2,t0);TableEq(t3,t1);AttrsEq(a4,a0);AttrsEq(a5,a1);"
			"AttrsEq(a6,a2);AttrsEq(a7,a3);p3 := p0;p4 := p1;p5 := p2";
		CDSLRule *rule = PdslruleParseLocal(mp, text.c_str());
		if (nullptr == rule)
		{
			ok = false;
			continue;
		}
		auto join = [mp, kind](CExpression *l, CExpression *r, CExpression *p) {
			COperator *op = nullptr;
			switch (kind)
			{
				case 0: op = GPOS_NEW(mp) CLogicalInnerJoin(mp); break;
				case 1: op = GPOS_NEW(mp) CLogicalLeftOuterJoin(mp); break;
				case 2: op = GPOS_NEW(mp) CLogicalFullOuterJoin(mp); break;
				case 3: op = GPOS_NEW(mp) CLogicalLeftSemiJoin(mp); break;
				default: op = GPOS_NEW(mp) CLogicalLeftAntiSemiJoin(mp); break;
			}
			l->AddRef(); r->AddRef(); p->AddRef();
			return GPOS_NEW(mp) CExpression(mp, op, l, r, p);
		};
		for (ULONG shape = 0; shape < 3; shape++)
		{
			auto check = [&](BOOL valid, const CHAR *step) {
				if (!valid)
					GPOS_TRACE_FORMAT("join binding kind=%s shape=%lu check=%s", kinds[kind], shape, step);
				ok &= valid;
			};
			CColRefArray *lc = nullptr, *rc = nullptr;
			CExpression *l = input(&lc);
			CExpression *r = input(&rc);
			CExpression *lp = fix.PexprPredAtom((*lc)[0]);
			CExpression *rp = fix.PexprPredAtom((*rc)[0]);
			CExpression *ln = negate(lp), *rn = negate(rp);
			CExpression *lnn = negate(ln), *rnn = negate(rn);
			CExpression *ls = fix.PexprLogicalSelect(l, lnn);
			CExpression *rs = fix.PexprLogicalSelect(r, rnn);
			CExpression *on = nullptr;
			if (2 == shape)
				on = CUtils::PexprScalarConstBool(mp, false, true /*is_null*/);
			else
			{
				lp->AddRef(); rp->AddRef();
				on = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarBoolOp(mp,
					0 == shape ? CScalarBoolOp::EboolopAnd : CScalarBoolOp::EboolopOr), lp, rp);
			}
			CExpression *pn = negate(on), *pnn = negate(pn);
			CExpression *source = join(ls, rs, pnn);
			CDSLRulePrefixIndex index(mp);
			index.Insert(rule, 0, source->Pop()->Eopid());
			CDSLRuleArray *candidates = index.PdrgpruleCandidates(mp, source);
			check(1 == candidates->Size(), "literal source candidate");
			candidates->Release();
			CExpression *missingChild = join(l, rs, pnn);
			candidates = index.PdrgpruleCandidates(mp, missingChild);
			check(0 == candidates->Size(), "missing child candidate");
			candidates->Release();
			missingChild->Release();
			if (0 == kind && 0 == shape)
			{
				CMemo memo(mp);
				const auto insert = [&](const auto &self, CExpression *expr) -> CGroupExpression * {
					CGroupArray *children = GPOS_NEW(mp) CGroupArray(mp);
					for (ULONG i = 0; i < expr->Arity(); i++)
						children->Append(self(self, (*expr)[i])->Pgroup());
					expr->Pop()->AddRef();
					CGroupExpression *entry = GPOS_NEW(mp) CGroupExpression(mp,
						expr->Pop(), children, CXform::ExfInvalid, nullptr, false);
					CGroupExpression *canonical = nullptr;
					memo.PgroupInsert(nullptr, expr, entry, &canonical);
					if (canonical != entry)
						entry->Release();
					return canonical;
				};
				CExpressionArray *bindings = index.PdrgpexprBindings(mp, insert(insert, source));
				check(0 < bindings->Size(), "memo bindings");
				for (ULONG i = 0; i < bindings->Size(); i++)
				{
					CDSLRewriteDecision *bound = CDSLRuleEngine::Instance()->PdecisionEvaluate(
						mp, rule, (*bindings)[i]);
					check(EdsldecisionReady == bound->Status(), "memo rewrite");
					GPOS_DELETE(bound);
				}
				bindings->Release();
			}
			CDSLRewriteDecision *decision =
				CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, source);
			CExpression *target = decision->PexprTarget();
			check(EdsldecisionReady == decision->Status() && nullptr != target, "direct rewrite");
			if (nullptr != target)
			{
				check(target->Pop()->Eopid() == source->Pop()->Eopid(), "join kind");
				check((*target)[2]->Matches(on), "ON target");
				check((*(*target)[0])[1]->Matches(lp), "left predicate");
				check((*(*target)[1])[1]->Matches(rp), "right predicate");
				check((*(*target)[0])[0] == l && (*(*target)[1])[0] == r, "input identity");
				check(source->DeriveOutputColumns()->Equals(target->DeriveOutputColumns()), "schema");
				check((*source)[2] == pnn, "source unchanged");
			}
			GPOS_DELETE(decision);
			if (1 == kind)
			{
				ls->AddRef(); rs->AddRef(); pnn->AddRef();
				CExpression *full = GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CLogicalFullOuterJoin(mp), ls, rs, pnn);
				CExpression *rejectNull = fix.PexprEqConst((*lc)[0], 1);
				CExpression *wrapped = fix.PexprLogicalSelect(full, rejectNull);
				decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, wrapped);
				check(EdsldecisionMatchRejected == decision->Status(), "view isolation");
				GPOS_DELETE(decision);
				wrapped->Release(); rejectNull->Release(); full->Release();
			}
			// The ON pattern is a structural test, not an assumed definition.
			CExpression *wrong = join(ls, rs, pn);
			decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, wrong);
			check(EdsldecisionMatchRejected == decision->Status(), "wrong ON shape");
			GPOS_DELETE(decision);
			wrong->Release(); source->Release(); pn->Release(); pnn->Release(); on->Release();
			ls->Release(); rs->Release(); ln->Release(); rn->Release(); lnn->Release(); rnn->Release();
			// Column arrays are borrowed from the input operators.
			lp->Release(); rp->Release(); l->Release(); r->Release();
		}
			rule->Release();
	}
	// Dependent Apply and NOT IN need their own evaluation certificates;
	// keyed/residual forms must not masquerade as one complete ON expression.
	for (const CHAR *text : {
		"LeftApply<p0 a0 a1 a2>(Input<t0>,Input<t1>)|"
		"LeftApply<p1 a3 a4 a5>(Input<t2>,Input<t3>)|"
		"TableEq(t2,t0);TableEq(t3,t1);AttrsEq(a3,a0);AttrsEq(a4,a1);AttrsEq(a5,a2);p1 := Not(p0)",
		"AntiJoinNotIn<p0 a0 a1>(Input<t0>,Input<t1>)|"
		"AntiJoinNotIn<p1 a2 a3>(Input<t2>,Input<t3>)|"
		"TableEq(t2,t0);TableEq(t3,t1);AttrsEq(a2,a0);AttrsEq(a3,a1);p1 := Not(p0)",
		"InnerJoin<a0 a1 p0 a2 a3>(Input<t0>,Input<t1>)|"
		"InnerJoin<a4 a5 p1 a6 a7>(Input<t2>,Input<t3>)|"
		"TableEq(t2,t0);TableEq(t3,t1);AttrsEq(a4,a0);AttrsEq(a5,a1);"
		"AttrsEq(a6,a2);AttrsEq(a7,a3);p1 := Not(p0)"})
	{
		CDSLRule *unsupported = PdslruleParseLocal(mp, text);
		ok &= nullptr == unsupported;
		CRefCount::SafeRelease(unsupported);
	}
	return ok ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLInstantiateTest::EresUnittest_ExpressionBindings()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	auto negate = [mp](CExpression *input) {
		input->AddRef();
		return GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopNot),
			input);
	};
	// Same text as the native FormalSQL proof and WeTune runtime test.
	CDSLRule *rule = PdslruleParseLocal(
		mp,
		"Filter<Not(Not(p0)) a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
		"TableEq(t1,t0);AttrsEq(a1,a0);p1 := p0");
	CDSLRule *inverse = PdslruleParseLocal(
		mp,
		"Filter<p0 a0>(Input<t0>)|Filter<Not(Not(p0)) a1>(Input<t1>)|"
		"TableEq(t1,t0);AttrsEq(a1,a0)");
	if (nullptr == rule || nullptr == inverse)
	{
		CRefCount::SafeRelease(rule);
		CRefCount::SafeRelease(inverse);
		return GPOS_FAILED;
	}
	BOOL ok = true;
	CDSLRuleEngine *engine = CDSLRuleEngine::Instance();
	GPOS_ASSERT(nullptr != engine);
	for (ULONG shape = 0; shape < 3; shape++)
	{
		CExpression *get = nullptr, *baseSelect = nullptr;
		CColRefArray *columns = nullptr;
		BuildSelectOverAtoms(fix, 2, 2, &get, &baseSelect, &columns);
		// Input must also bind an entire Join, not just a scan.
		CExpression *base = get;
		base->AddRef();
		if (1 == shape)
		{
			CExpression *other = fix.PexprLogicalGet("other", 1, nullptr);
			CExpression *join =
				fix.PexprLogicalInnerJoin(get, other, (*baseSelect)[1]);
			base->Release();
			base = join;
			other->Release();
		}
		CExpression *predicate = (*baseSelect)[1];
		predicate->AddRef();
		if (2 == shape)
		{
			predicate->Release();
			predicate = CUtils::PexprScalarConstBool(mp, false, true /*is_null*/);
		}
		CExpression *once = negate(predicate);
		CExpression *twice = negate(once);
		CExpression *source = fix.PexprLogicalSelect(base, twice);
		// Constructor tests do not certify a rewrite: duplicate/reconstructed
		// AND rules below are intentionally not registered as proven rules.
		for (const CHAR *text : {
				 "Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
				 "TableEq(t1,t0);AttrsEq(a1,a0);p1 := And(p0,p0)",
				 "Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
				 "TableEq(t1,t0);AttrsEq(a1,a0);Not(p2) := p0;"
				 "Not(p3) := p2;And(p4,p5) := p3;p1 := And(p4,p5)",
				 "Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
				 "TableEq(t1,t0);AttrsEq(a1,a0);Not(p2) := p0;"
				 "Not(p3) := p2;And(p4,p5) := p3;p1 := p3"})
		{
			CDSLRule *andRule = PdslruleParseLocal(mp, text);
			if (nullptr == andRule)
			{
				ok = false;
				continue;
			}
			const BOOL duplicate = 1 == andRule->Pexprdefs()->UlDefinitions();
			CDSLRewriteDecision *andDecision =
				engine->PdecisionEvaluate(mp, andRule, source);
			if (2 == shape && !duplicate)
			{
				ok &= EdsldecisionMatchRejected == andDecision->Status();
			}
			else
			{
				CExpression *target = andDecision->PexprTarget();
				ok &= EdsldecisionReady == andDecision->Status() && nullptr != target;
				if (nullptr != target)
				{
					CExpression *result = (*target)[1];
					ok &= 2 == result->Arity() && CUtils::FScalarBoolOp(
						result, CScalarBoolOp::EboolopAnd);
					if (2 == result->Arity())
					{
						ok &= duplicate ? ((*result)[0] == twice && (*result)[1] == twice)
							: ((*result)[0]->Matches((*predicate)[0]) &&
							   (*result)[1]->Matches((*predicate)[1]));
					}
				}
			}
			GPOS_DELETE(andDecision);
			andRule->Release();
		}
		CDSLRewriteDecision *decision =
			engine->PdecisionEvaluate(mp, rule, source, true);
		ok &= EdsldecisionReady == decision->Status() &&
			  nullptr != decision->PexprTarget();
		GPOS_DELETE(decision);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLConstraintChecker checker(mp);
		CDSLInstantiator builder(mp);
		ok &= CDSLMatcher(mp, rule).FMatch(rule->PfragSrc()->PopRoot(), source,
										   model) &&
			  checker.FCheck(rule, model);
		CExpression *target =
			ok ? builder.PexprInstantiate(rule, model) : nullptr;
		ok &= nullptr != target && (*target)[0] == base &&
			  (*target)[1]->Matches(predicate) &&
			  source->DeriveOutputColumns()->Equals(
				  target->DeriveOutputColumns()) &&
			  (*source)[1] == twice && (*twice)[0] == once;
		if (nullptr != target)
		{
			// Compare with the native Boolean preprocessor on the same tree.
			// This covers its double-NOT branch, not all AND/OR/EXISTS rules.
			CExpression *native = CExpressionUtils::PexprUnnest(mp, source);
			ok &= native->Matches(target);
			native->Release();
			CDSLModel *inverseModel = GPOS_NEW(mp) CDSLModel(mp);
			CDSLInstantiator inverseBuilder(mp);
			ok &= CDSLMatcher(mp, inverse)
					  .FMatch(inverse->PfragSrc()->PopRoot(), target,
							  inverseModel) &&
				  checker.FCheck(inverse, inverseModel);
			CExpression *restored =
				ok ? inverseBuilder.PexprInstantiate(inverse, inverseModel)
				   : nullptr;
			ok &= nullptr != restored && restored->Matches(source);
			CRefCount::SafeRelease(restored);
			inverseModel->Release();
		}
		// Missing NOT shape is not an applicable source binding.
		CDSLModel *wrong = GPOS_NEW(mp) CDSLModel(mp);
		ok &= !CDSLMatcher(mp, rule).FMatch(rule->PfragSrc()->PopRoot(),
											baseSelect, wrong);
		wrong->Release();
		CRefCount::SafeRelease(target);
		model->Release();
		source->Release();
		twice->Release();
		once->Release();
		predicate->Release();
		base->Release();
		baseSelect->Release();
		get->Release();
	}
	// The same mixed expression rule is proved by the shared FormalSQL
	// constructor certificates; no AND/OR reassociation is implicit here.
	CDSLRule *orRule = PdslruleParseLocal(mp,
		"Filter<Or(Not(Not(p0)),p1) a0>(Input<t0>)|Filter<Or(p2,p3) a1>(Input<t1>)|"
		"TableEq(t1,t0);AttrsEq(a1,a0);p2 := p0;p3 := p1");
	ok &= nullptr != orRule;
	if (nullptr != orRule)
	{
		CColRefArray *cols = nullptr;
		CExpression *get = fix.PexprLogicalGet("disjunction", 2, &cols);
		for (ULONG trial = 0; trial < 4; ++trial)
		{
			CExpression *left = fix.PexprEqConst((*cols)[0], 1);
			CExpression *right = 3 == trial
				? CUtils::PexprScalarConstBool(mp, false, true /*is_null*/)
				: fix.PexprEqConst((*cols)[1], 2);
			CExpression *once = negate(left), *twice = negate(once);
			CExpressionArray *children = GPOS_NEW(mp) CExpressionArray(mp);
			twice->AddRef();
			children->Append(twice);
			right->AddRef();
			children->Append(right);
			if (2 == trial)
				children->Append(fix.PexprEqConst((*cols)[0], 3));
			CExpression *predicate = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CScalarBoolOp(mp, 1 == trial
					? CScalarBoolOp::EboolopAnd : CScalarBoolOp::EboolopOr), children);
			CExpression *source = fix.PexprLogicalSelect(get, predicate);
			CDSLRewriteDecision *decision = engine->PdecisionEvaluate(mp, orRule, source);
			if (0 == trial || 3 == trial)
			{
				CExpression *target = decision->PexprTarget();
				ok &= EdsldecisionReady == decision->Status() && nullptr != target;
				if (nullptr != target)
					ok &= (*target)[0] == get && 2 == (*target)[1]->Arity() &&
						CUtils::FScalarBoolOp((*target)[1], CScalarBoolOp::EboolopOr) &&
						(*(*target)[1])[0]->Matches(left) && (*(*target)[1])[1]->Matches(right);
			}
			else
				ok &= EdsldecisionMatchRejected == decision->Status();
			ok &= (*predicate)[0] == twice && (*twice)[0] == once;
			GPOS_DELETE(decision);
			source->Release();
			predicate->Release();
			twice->Release();
			once->Release();
			right->Release();
			left->Release();
		}
		get->Release();
		orRule->Release();
	}
	CDSLRule *sharedAnd = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
			"TableEq(t1,t0);AttrsEq(a1,a0);And(p2,p2) := p0;p1 := p0");
	ok &= nullptr != sharedAnd;
	if (nullptr != sharedAnd)
	{
		CExpression *get = nullptr, *baseSelect = nullptr;
		CColRefArray *columns = nullptr;
		BuildSelectOverAtoms(fix, 2, 2, &get, &baseSelect, &columns);
		for (ULONG trial = 0; trial < 3; trial++)
		{
			CExpressionArray *children = GPOS_NEW(mp) CExpressionArray(mp);
			children->Append(fix.PexprPredAtom((*columns)[0]));
			children->Append(fix.PexprPredAtom((*columns)[1 == trial ? 1 : 0]));
			if (2 == trial)
				children->Append(fix.PexprPredAtom((*columns)[0]));
			CExpression *predicate = GPOS_NEW(mp) CExpression(
				mp, GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopAnd), children);
			CExpression *source = fix.PexprLogicalSelect(get, predicate);
			CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
			// Distinct copies with identical column identities match; different
			// columns and a flattened three-operand AND do not match a binary pattern.
			ok &= (0 == trial) == CDSLMatcher(mp, sharedAnd).FMatch(
				sharedAnd->PfragSrc()->PopRoot(), source, model);
			model->Release();
			source->Release();
			predicate->Release();
		}
		baseSelect->Release();
		get->Release();
		sharedAnd->Release();
	}
	CDSLRule *chain = PdslruleParseLocal(
		mp,
		"Filter<p0 a0>(Filter<p4 a4>(Input<t0>))|"
		"Filter<p1 a1>(Filter<p6 a6>(Input<t1>))|"
		"TableEq(t1,t0);AttrsEq(a1,a0);AttrsEq(a6,a4);"
		"Not(p2) := p0;Not(p3) := p2;Not(p5) := p4;Not(p3) := p5;"
		"p1 := p3;p6 := p3");
	ok &= nullptr != chain;
	if (nullptr != chain)
	{
		CDSLRulePrefixIndex index(mp);
		index.Insert(chain, 0, COperator::EopLogicalSelect);
		CColRefArray *cols = GPOS_NEW(mp) CColRefArray(mp);
		cols->Append(fix.PcrCreateInt4("first"));
		cols->Append(fix.PcrCreateInt4("second"));
		CExpression *get = GPOS_NEW(mp)
			CExpression(mp, GPOS_NEW(mp) CLogicalConstTableGet(
								mp, cols, GPOS_NEW(mp) IDatum2dArray(mp)));
		for (ULONG trial = 0; trial < 3; trial++)
		{
			const ULONG different = trial % 2;
			CExpression *atom = fix.PexprPredAtom((*cols)[0]);
			CExpression *other = fix.PexprPredAtom((*cols)[different]);
			CExpression *outerOnce = negate(atom), *innerOnce = negate(other);
			CExpression *outerTwice = negate(outerOnce),
						*innerTwice = negate(innerOnce);
			CExpression *inner = fix.PexprLogicalSelect(get, innerTwice);
			CExpression *source = fix.PexprLogicalSelect(inner, outerTwice);
			if (0 == trial)
			{
				CMemo memo(mp);
				const auto insert =
					[&](const auto &self,
						CExpression *expr) -> CGroupExpression * {
					CGroupArray *children = GPOS_NEW(mp) CGroupArray(mp);
					for (ULONG i = 0; i < expr->Arity(); i++)
						children->Append(self(self, (*expr)[i])->Pgroup());
					expr->Pop()->AddRef();
					CGroupExpression *entry = GPOS_NEW(mp)
						CGroupExpression(mp, expr->Pop(), children,
										 CXform::ExfInvalid, nullptr, false);
					CGroupExpression *canonical = nullptr;
					memo.PgroupInsert(nullptr, expr, entry, &canonical);
					if (canonical != entry)
						entry->Release();
					return canonical;
				};
				CExpressionArray *bindings =
					index.PdrgpexprBindings(mp, insert(insert, source));
				ok &= 1 == bindings->Size();
				for (ULONG i = 0; i < bindings->Size(); i++)
				{
					CDSLRewriteDecision *bound =
						engine->PdecisionEvaluate(mp, chain, (*bindings)[i]);
					ok &= EdsldecisionReady == bound->Status();
					GPOS_DELETE(bound);
				}
				bindings->Release();
			}
			CDSLRuleArray *candidates = index.PdrgpruleCandidates(mp, source);
			ok &= 1 == candidates->Size();
			candidates->Release();
			candidates = index.PdrgpruleCandidates(mp, inner);
			ok &= 0 == candidates->Size();	// never collapse two literal levels
			candidates->Release();
			CDSLRewriteDecision *decision =
				engine->PdecisionEvaluate(mp, chain, source);
			if (0 == different)
			{
				CExpression *target = decision->PexprTarget();
				ok &= EdsldecisionReady == decision->Status() &&
					  nullptr != target &&
					  COperator::EopLogicalSelect ==
						  (*target)[0]->Pop()->Eopid() &&
					  (*target)[1]->Matches(atom) &&
					  (*(*target)[0])[1]->Matches(atom);
			}
			else
			{
				ok &= EdsldecisionMatchRejected == decision->Status();
			}
			GPOS_DELETE(decision);
			source->Release();
			inner->Release();
			outerTwice->Release();
			innerTwice->Release();
			outerOnce->Release();
			innerOnce->Release();
			atom->Release();
			other->Release();
		}
		get->Release();
		chain->Release();
	}
	rule->Release();
	inverse->Release();
	return ok ? GPOS_OK : GPOS_FAILED;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiateTest::EresUnittest_DerivedPredicateNotTrue
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLInstantiateTest::EresUnittest_PredicateNegationNullSemantics()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	const CHAR *constraints[] = {"PredicateNot(p1,p0)", "PredicateNotTrue(p1,p0)"};
	for (ULONG kind = 0; kind < 2; ++kind)
	{
		const std::string dsl =
			std::string("Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|") +
			"Deterministic(p1);ErrorFree(p1);" + constraints[kind];
		CDSLRule *rule = PdslruleParseLocal(mp, dsl.c_str());
		GPOS_ASSERT(nullptr != rule);
		const CDSLSymbol *input = (*rule->PfragSrc()->PopRoot()->Pdrgpsym())[0];
		const CDSLSymbol *output = (*rule->PfragTgt()->PopRoot()->Pdrgpsym())[0];
		for (ULONG value = 0; value < 3; ++value)
		{
			CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
			CDSLConstraintChecker checker(mp);
			GPOS_ASSERT(!checker.FCheck(rule, model));
			CExpression *leaf = CUtils::PexprScalarConstBool(mp, value == 1, value == 2);
			model->FBind(input, leaf);
			const BOOL checked = checker.FCheck(rule, model);
			CExpression *result = model->PexprPred(output);
			const CScalar::EBoolEvalResult expected = value == 1 ? CScalar::EberFalse
				: value == 2 && kind == 0 ? CScalar::EberNull : CScalar::EberTrue;
			const BOOL valid = checked && nullptr != result &&
				CScalar::EberEvaluate(mp, result) == expected &&
				(*result)[0]->Matches(leaf);
			// A bound NOT output cannot pass the IS NOT TRUE contract, or vice versa.
			CDSLModel *wrong = GPOS_NEW(mp) CDSLModel(mp);
			wrong->FBind(input, leaf);
			CExpression *other = nullptr;
			leaf->AddRef();
			other = kind == 0 ? GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CScalarBooleanTest(mp, CScalarBooleanTest::EbtIsNotTrue), leaf)
				: CUtils::PexprNegate(mp, leaf);
			wrong->FBind(output, other);
			const BOOL rejected = !checker.FCheck(rule, wrong);
			other->Release();
			wrong->Release();
			leaf->Release();
			model->Release();
			if (!valid || !rejected)
			{
				rule->Release();
				return GPOS_FAILED;
			}
		}
		rule->Release();
	}
	return GPOS_OK;
}

GPOS_RESULT
CDSLInstantiateTest::EresUnittest_DerivedPredicateNotTrue()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
			"TableEq(t1,t0);PredicateNotTrue(p1,p0);AttrsEq(a1,a0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CExpression *pexprGet = nullptr;
	CExpression *pexprSelect = nullptr;
	CColRefArray *pdrgpcrOut = nullptr;
	BuildSelectOverAtoms(fix, 1, 1, &pexprGet, &pexprSelect, &pdrgpcrOut);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLConstraintChecker checker(mp);
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!CDSLMatcher(mp, prule)
			 .FMatch(prule->PfragSrc()->PopRoot(), pexprSelect, pmodel) ||
		!checker.FCheck(prule, pmodel) ||
		nullptr == (pexprTarget = instantiator.PexprInstantiate(prule, pmodel)) ||
		COperator::EopLogicalSelect != pexprTarget->Pop()->Eopid() ||
		COperator::EopScalarBooleanTest != (*pexprTarget)[1]->Pop()->Eopid() ||
		CScalarBooleanTest::EbtIsNotTrue !=
			CScalarBooleanTest::PopConvert((*pexprTarget)[1]->Pop())->Ebt())
	{
		eres = GPOS_FAILED;
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprSelect->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiateTest::EresUnittest_PredicateDomainSplit
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLInstantiateTest::EresUnittest_PredicateDomainSplit()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp,
		"Filter<p0 a2 a3>(InnerJoin<p1 a0 a1>(Input<t0>,Input<t1>))|"
		"Filter<p3 a6 a7>(InnerJoin<p2 a4 a5>(Input<t2>,Input<t3>))|"
		"TableEq(t2,t0);TableEq(t3,t1);"
		"PredicateAnd(p4,p0,p1);"
		"PredicateDomainSplit(p4,p2,p3,a4,a5,a6,a7,t0,t1);"
		"ErrorFree(p4);Deterministic(p4)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrOuter = nullptr;
	CColRefArray *pdrgpcrInner = nullptr;
	CColRefArray *pdrgpcrExternal = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("split_outer", 1, &pdrgpcrOuter);
	CExpression *pexprInner =
		fix.PexprLogicalGet("split_inner", 2, &pdrgpcrInner);
	CExpression *pexprExternal =
		fix.PexprLogicalGet("split_external", 1, &pdrgpcrExternal);
	CExpression *pexprOuterAtom =
		fix.PexprPredAtom((*pdrgpcrOuter)[0]);
	CExpression *pexprInnerAtom =
		fix.PexprPredAtom((*pdrgpcrInner)[0]);
	CExpression *pexprCrossDomain = CPredicateUtils::PexprDisjunction(
		mp, pexprOuterAtom, pexprInnerAtom);
	pexprOuterAtom->Release();
	pexprInnerAtom->Release();
	CExpression *pexprInnerOnly =
		fix.PexprPredAtom((*pdrgpcrInner)[1]);
	CExpression *pexprResidual = CPredicateUtils::PexprConjunction(
		mp, pexprCrossDomain, pexprInnerOnly);
	pexprCrossDomain->Release();
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprOuter, pexprInner, pexprResidual);
	CExpression *pexprExternalPred =
		fix.PexprEqPred((*pdrgpcrInner)[1], (*pdrgpcrExternal)[0]);
	CExpression *pexprSource =
		fix.PexprLogicalSelect(pexprJoin, pexprExternalPred);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	const BOOL fMatched =
		matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource, pmodel);
	const BOOL fChecked = fMatched && checker.FCheck(prule, pmodel);
	if (!fMatched || !fChecked)
	{
		eres = GPOS_FAILED;
	}
	else
	{
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		BOOL fInnerOnlyResidual = false;
		BOOL fInnerOnlyExternal = false;
		if (nullptr != pexprTarget && 2 == pexprTarget->Arity() &&
			COperator::EopLogicalSelect == pexprTarget->Pop()->Eopid() &&
			COperator::EopLogicalInnerJoin == (*pexprTarget)[0]->Pop()->Eopid() &&
			3 == (*pexprTarget)[0]->Arity())
		{
			CExpressionArray *pdrgpexprJoin =
				CPredicateUtils::PdrgpexprConjuncts(mp, (*(*pexprTarget)[0])[2]);
			for (ULONG ul = 0; ul < pdrgpexprJoin->Size(); ul++)
			{
				fInnerOnlyResidual = fInnerOnlyResidual ||
					CUtils::Equals((*pdrgpexprJoin)[ul], pexprInnerOnly);
			}
			pdrgpexprJoin->Release();
			CExpressionArray *pdrgpexprFilter =
				CPredicateUtils::PdrgpexprConjuncts(mp, (*pexprTarget)[1]);
			for (ULONG ul = 0; ul < pdrgpexprFilter->Size(); ul++)
			{
				fInnerOnlyExternal = fInnerOnlyExternal ||
					CUtils::Equals((*pdrgpexprFilter)[ul], pexprInnerOnly);
			}
			pdrgpexprFilter->Release();
		}
		if (nullptr == pexprTarget ||
			COperator::EopLogicalSelect != pexprTarget->Pop()->Eopid() ||
			COperator::EopLogicalInnerJoin != (*pexprTarget)[0]->Pop()->Eopid() ||
			!fInnerOnlyResidual || fInnerOnlyExternal ||
			!(*pexprTarget)[1]->DeriveUsedColumns()->FMember(
				(*pdrgpcrInner)[1]) ||
			!(*pexprTarget)[1]->DeriveUsedColumns()->FMember(
				(*pdrgpcrExternal)[0]) ||
			(*pexprTarget)[1]->DeriveUsedColumns()->FMember(
				(*pdrgpcrOuter)[0]) ||
			!(*(*pexprTarget)[0])[2]->DeriveUsedColumns()->FMember(
				(*pdrgpcrOuter)[0]) ||
			!(*(*pexprTarget)[0])[2]->DeriveUsedColumns()->FMember(
				(*pdrgpcrInner)[0]) ||
			(*(*pexprTarget)[0])[2]->DeriveUsedColumns()->FMember(
				(*pdrgpcrExternal)[0]))
		{
			eres = GPOS_FAILED;
		}
	}
	CDSLRule *pruleUnsafe = PdslruleParseLocal(
		mp,
		"Filter<p0 a2 a3>(InnerJoin<p1 a0 a1>(Input<t0>,Input<t1>))|"
		"Filter<p3 a6 a7>(InnerJoin<p2 a4 a5>(Input<t2>,Input<t3>))|"
		"TableEq(t2,t0);TableEq(t3,t1);"
		"PredicateAnd(p4,p0,p1);"
		"PredicateDomainSplit(p4,p2,p3,a4,a5,a6,a7,t0,t1)");
	CDSLModel *pmodelUnsafe = GPOS_NEW(mp) CDSLModel(mp);
	if (nullptr == pruleUnsafe ||
		!CDSLMatcher(mp, pruleUnsafe)
			 .FMatch(pruleUnsafe->PfragSrc()->PopRoot(), pexprSource,
					 pmodelUnsafe) ||
		checker.FCheck(pruleUnsafe, pmodelUnsafe))
	{
		eres = GPOS_FAILED;
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodelUnsafe->Release();
	CRefCount::SafeRelease(pruleUnsafe);
	pmodel->Release();
	pexprSource->Release();
	pexprExternalPred->Release();
	pexprJoin->Release();
	pexprResidual->Release();
	pexprInnerOnly->Release();
	pexprExternal->Release();
	pexprInner->Release();
	pexprOuter->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiateTest::EresUnittest_PredicateDomainSplitRejectsMixedAtom
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLInstantiateTest::EresUnittest_PredicateDomainSplitRejectsMixedAtom()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp,
		"Filter<p0 a2 a3>(InnerJoin<p1 a0 a1>(Input<t0>,Input<t1>))|"
		"Filter<p3 a6 a7>(InnerJoin<p2 a4 a5>(Input<t2>,Input<t3>))|"
		"TableEq(t2,t0);TableEq(t3,t1);"
		"PredicateAnd(p4,p0,p1);"
		"PredicateDomainSplit(p4,p2,p3,a4,a5,a6,a7,t0,t1);"
		"ErrorFree(p4);Deterministic(p4)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrOuter = nullptr;
	CColRefArray *pdrgpcrInner = nullptr;
	CColRefArray *pdrgpcrExternal = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("mixed_outer", 1, &pdrgpcrOuter);
	CExpression *pexprInner =
		fix.PexprLogicalGet("mixed_inner", 2, &pdrgpcrInner);
	CExpression *pexprExternal =
		fix.PexprLogicalGet("mixed_external", 1, &pdrgpcrExternal);
	CExpression *pexprOuterAtom =
		fix.PexprPredAtom((*pdrgpcrOuter)[0]);
	CExpression *pexprInnerAtom =
		fix.PexprPredAtom((*pdrgpcrInner)[0]);
	CExpression *pexprResidual = CPredicateUtils::PexprDisjunction(
		mp, pexprOuterAtom, pexprInnerAtom);
	pexprOuterAtom->Release();
	pexprInnerAtom->Release();
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprOuter, pexprInner, pexprResidual);
	CExpression *pexprCurrentDomains =
		fix.PexprEqPred((*pdrgpcrOuter)[0], (*pdrgpcrInner)[1]);
	CExpression *pexprExternalDomain =
		fix.PexprEqPred((*pdrgpcrInner)[1], (*pdrgpcrExternal)[0]);
	// OR is one top-level conjunct. It cannot be split without changing its
	// Boolean structure, and together its dependencies span all three domains.
	CExpression *pexprMixed = CPredicateUtils::PexprDisjunction(
		mp, pexprCurrentDomains, pexprExternalDomain);
	CExpression *pexprSource = fix.PexprLogicalSelect(pexprJoin, pexprMixed);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	const BOOL fMatched =
		matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource, pmodel);
	const BOOL fChecked = fMatched && checker.FCheck(prule, pmodel);
	if (!fMatched || fChecked)
	{
		eres = GPOS_FAILED;
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprSource->Release();
	pexprMixed->Release();
	pexprExternalDomain->Release();
	pexprCurrentDomains->Release();
	pexprJoin->Release();
	pexprResidual->Release();
	pexprExternal->Release();
	pexprInner->Release();
	pexprOuter->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiateTest::EresUnittest_DerivedFilterConjunction
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLInstantiateTest::EresUnittest_DerivedFilterConjunction()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp,
		"Filter<p0 a0>(Filter<p1 a1>(Input<t0>))|"
		"Filter<p2 a2>(Input<t1>)|TableEq(t1,t0);"
		"PredicateAnd(p2,p0,p1);AttrsUnion(a2,a0,a1)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet =
		fix.PexprLogicalGet("derived_filter", 2, &pdrgpcrOut);
	CColRef *pcrInner = (*pdrgpcrOut)[0];
	CExpression *pexprPredInner =
		fix.PexprConjunctionOfAtoms(&pcrInner, 1);
	CExpression *pexprInner =
		fix.PexprLogicalSelect(pexprGet, pexprPredInner);
	pexprPredInner->Release();
	CColRef *pcrOuter = (*pdrgpcrOut)[1];
	CExpression *pexprPredOuter =
		fix.PexprConjunctionOfAtoms(&pcrOuter, 1);
	CExpression *pexprSelect =
		fix.PexprLogicalSelect(pexprInner, pexprPredOuter);
	pexprPredOuter->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSelect, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLConstraintChecker checker(mp);
		CDSLInstantiator instantiator(mp);
		if (!checker.FCheck(prule, pmodel))
		{
			eres = GPOS_FAILED;
		}
		else
		{
			pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
			if (nullptr == pexprTarget ||
				COperator::EopLogicalSelect != pexprTarget->Pop()->Eopid() ||
				2 != pexprTarget->Arity() ||
				COperator::EopLogicalGet != (*pexprTarget)[0]->Pop()->Eopid())
			{
				eres = GPOS_FAILED;
			}
			else
			{
				CExpressionArray *pdrgpexprTarget =
					CPredicateUtils::PdrgpexprConjuncts(mp, (*pexprTarget)[1]);
				if (2 != pdrgpexprTarget->Size())
				{
					eres = GPOS_FAILED;
				}
				pdrgpexprTarget->Release();
			}
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	pexprSelect->Release();
	pexprInner->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiateTest::EresUnittest_PushedFilterPredicateRemapped
//
//	@doc:
//		The live ORCA tree has already pushed a right-key Filter through a nested
//		inner join. Match it through the pre-pushdown view, peel the Select from
//		the bound right Input, and instantiate the target over the left root key.
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
		fix.PexprLogicalGet("right_t", 2, &pdrgpcrRight);
	// Put an unrelated conjunct first. The source AttrsEq(a1,a2) must guide
	// assignment to the second conjunct, which references the right join key.
	CColRef *rgpcrRightPreds[] = {(*pdrgpcrRight)[1],
								 (*pdrgpcrRight)[0]};
	CExpression *pexprRightPred =
		fix.PexprConjunctionOfAtoms(rgpcrRightPreds, 2);
	CExpression *pexprRightSelect =
		fix.PexprLogicalSelect(pexprRight, pexprRightPred);
	CColRefArray *pdrgpcrRightExtra = nullptr;
	CExpression *pexprRightExtra =
		fix.PexprLogicalGet("right_extra_t", 1, &pdrgpcrRightExtra);
	CExpression *pexprNestedPred =
		fix.PexprEqPred((*pdrgpcrRight)[0], (*pdrgpcrRightExtra)[0]);
	CExpression *pexprNestedRight = fix.PexprLogicalInnerJoin(
		pexprRightSelect, pexprRightExtra, pexprNestedPred);
	CExpression *pexprJoinPred =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprJoin = fix.PexprLogicalInnerJoin(
		pexprLeft, pexprNestedRight, pexprJoinPred);

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
		CExpression *pexprBoundRight = pmodel->PexprTable(psymRightTable);
		if (nullptr == pexprBoundRight ||
			COperator::EopLogicalInnerJoin !=
				pexprBoundRight->Pop()->Eopid() ||
			COperator::EopLogicalGet !=
				(*pexprBoundRight)[0]->Pop()->Eopid())
		{
			eres = GPOS_FAILED;
		}
		const CDSLSymbol *psymSourcePred =
			(*prule->PfragSrc()->PopRoot()->Pdrgpsym())[0];
		CExpression *pexprSourcePred = pmodel->PexprPred(psymSourcePred);
		if (nullptr == pexprSourcePred ||
			!pexprSourcePred->DeriveUsedColumns()->FMember(
				(*pdrgpcrRight)[0]) ||
			pexprSourcePred->DeriveUsedColumns()->FMember(
				(*pdrgpcrRight)[1]))
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
			COperator::EopLogicalInnerJoin !=
				(*(*pexprTgt)[0])[1]->Pop()->Eopid() ||
			COperator::EopLogicalGet !=
				(*(*(*pexprTgt)[0])[1])[0]->Pop()->Eopid() ||
			!(*pexprTgt)[1]->DeriveUsedColumns()->FMember(
				(*pdrgpcrLeft)[0]) ||
			(*pexprTgt)[1]->DeriveUsedColumns()->FMember(
				(*pdrgpcrRight)[0]) ||
			!(*pexprTgt)[1]->DeriveUsedColumns()->FMember(
				(*pdrgpcrRight)[1]))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	pexprJoin->Release();
	pexprJoinPred->Release();
	pexprNestedRight->Release();
	pexprNestedPred->Release();
	pexprRightExtra->Release();
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
		CDSLTargetInputOriginArray inputOrigins;
		pexprTgt = inst.PexprInstantiate(prule, pmodel, &inputOrigins);
		// target Select's relational child[0] must be the very Get subtree the
		// source Select was built over (pointer identity — grafted, not rebuilt).
		if (nullptr == pexprTgt || (*pexprTgt)[0] != pexprGet ||
			1 != inputOrigins.size() ||
			"r/0" != inputOrigins[0].m_template_path ||
			"r/0" != inputOrigins[0].m_expression_path)
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
