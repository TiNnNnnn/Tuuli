//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLExistsTest.cpp
//
//	@doc:
//		Three-stage test using an unmodified real rule from
//		MONSOON/dataset/rules/rules.els.reduced.txt (line 486).
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLExistsTest.h"

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
#include "gpopt/dsl/CDSLPlanTemplate.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CLogicalLeftAntiSemiApply.h"
#include "gpopt/operators/CLogicalLeftSemiApply.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarSubqueryExists.h"
#include "gpopt/operators/CScalarSubqueryNotExists.h"
#include "gpopt/operators/CScalarBoolOp.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

#define GPOPT_DSL_CORPUS_EXISTS_AGG_PROJ_RULE                              \
	"Exists(Agg<a0 a1 f0 s0 p0>(Input<t0>),Proj<a2 s1>(Input<t1>))|"     \
	"Exists(Agg<a3 a4 f1 s2 p1>(Input<t2>),Proj<a5 s3>(Input<t3>))|"     \
	"AttrsSub(a0,t0);AttrsSub(a1,t0);AttrsSub(a2,t1);"                    \
	"TableEq(t2,t0);TableEq(t3,t1);AttrsEq(a3,a0);AttrsEq(a4,a1);"       \
	"AttrsEq(a5,a2);PredicateEq(p1,p0);SchemaEq(s2,s0);"                 \
	"SchemaEq(s3,s1);FuncEq(f1,f0)"

#define GPOPT_DSL_NOT_EXISTS_DISTINCT_DROP_RULE                            \
	"NotExists(Input<t0>,Proj*<a0 s0>(Input<t1>))|"                      \
	"NotExists(Input<t2>,Proj<a1 s1>(Input<t3>))|"                       \
	"AttrsSub(a0,t1);TableEq(t2,t0);TableEq(t3,t1);"                     \
	"AttrsEq(a1,a0);SchemaEq(s1,s0)"

#define GPOPT_DSL_PREDICATE_EXISTS_IDENTITY_RULE                         \
	"Exists<p0 a0 a1>(Input<t0>,Input<t1>)|"                            \
	"Exists<p1 a2 a3>(Input<t2>,Input<t3>)|"                            \
	"AttrsSub(a0,t0);AttrsSub(a1,t1);TableEq(t2,t0);TableEq(t3,t1);"   \
	"PredicateEq(p1,p0);AttrsEq(a2,a0);AttrsEq(a3,a1)"

#define GPOPT_DSL_EXPRESSION_DEFINED_EXISTS_RULE                         \
	"Filter<p0 a0>(Input<t0>)|Exists(Input<t1>,Input<t2>)|"             \
	"TableEq(t1,t0);PredicateExists(p0,t2)"

#define GPOPT_DSL_EXPRESSION_DEFINED_NOT_EXISTS_RULE                    \
	"Filter<p0 a0>(Input<t0>)|NotExists(Input<t1>,Input<t2>)|"         \
	"TableEq(t1,t0);PredicateNotExists(p0,t2)"

GPOS_RESULT
CDSLExistsTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(
			CDSLExistsTest::EresUnittest_CorpusAggProjRoundTrip),
		GPOS_UNITTEST_FUNC(
			CDSLExistsTest::EresUnittest_PreApplyCorpusAggProjRoundTrip),
		GPOS_UNITTEST_FUNC(
			CDSLExistsTest::EresUnittest_PreApplyPreservesResidual),
		GPOS_UNITTEST_FUNC(
			CDSLExistsTest::EresUnittest_PreApplyNotExistsDistinctDrop),
		GPOS_UNITTEST_FUNC(
			CDSLExistsTest::EresUnittest_PostApplyNotExistsDistinctDrop),
		GPOS_UNITTEST_FUNC(
			CDSLExistsTest::EresUnittest_ExistsPolarityIsolation),
		GPOS_UNITTEST_FUNC(
			CDSLExistsTest::EresUnittest_PredicateSemiJoinRoundTrip),
		GPOS_UNITTEST_FUNC(
			CDSLExistsTest::EresUnittest_ExpressionDefinedExistence),
		GPOS_UNITTEST_FUNC(CDSLExistsTest::EresUnittest_TypedScalarExists)};
	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLExistsTest::EresUnittest_TypedScalarExists()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CWStringDynamic error(mp);
	CDSLRule *rule = CDSLRuleParser::PdslruleParse(mp,
		"Filter<Not(Not(Exists(t1))) a0>(Input<t0>)|"
		"Filter<Exists(t3) a1>(Input<t2>)|t2 := t0;t3 := t1;a1 := a0", "EQ", &error);
	GPOS_ASSERT(nullptr != rule);
	for (ULONG correlated = 0; correlated < 2; ++correlated)
	{
		CColRefArray *outer_cols = nullptr, *inner_cols = nullptr;
		CExpression *outer = fix.PexprLogicalGet("typed_exists_outer", 2, &outer_cols);
		CExpression *inner = fix.PexprLogicalGet("typed_exists_inner", 2, &inner_cols);
		// The TABLE operand can be a query with outer references, not just a Get.
		CExpression *query = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalSelect(mp),
			inner, fix.PexprEqPred((*inner_cols)[0],
				correlated ? (*outer_cols)[0] : (*inner_cols)[1]));
		CExpression *exists = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CScalarSubqueryExists(mp), query);
		CExpression *source = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalSelect(mp),
			outer, CUtils::PexprNegate(mp, CUtils::PexprNegate(mp, exists)));
		CDSLMatcher matcher(mp, rule);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		GPOS_ASSERT(matcher.FMatch(rule->PfragSrc()->PopRoot(), source, model));
		CDSLConstraintChecker checker(mp);
		GPOS_ASSERT(checker.FCheck(rule, model));
		CDSLInstantiator inst(mp);
		CExpression *target = inst.PexprInstantiate(rule, model);
		GPOS_ASSERT(nullptr != target && COperator::EopLogicalSelect == target->Pop()->Eopid());
		GPOS_ASSERT(COperator::EopScalarSubqueryExists == (*target)[1]->Pop()->Eopid());
		GPOS_ASSERT((*(*target)[1])[0] == query);
		GPOS_ASSERT(source->DeriveOutputColumns()->Equals(target->DeriveOutputColumns()));
		// The same typed TABLE capture must also feed a relational target Input.
		// Both polarities retain the complete correlated query, not a synthetic Get.
		for (ULONG negated = 0; negated < 2; ++negated)
		{
			CDSLRule *bridge = CDSLRuleParser::PdslruleParse(mp, negated
				? "Filter<Not(Not(Not(Exists(t1)))) a0>(Input<t0>)|"
				  "NotExists(Input<t2>,Input<t3>)|t2 := t0;t3 := t1"
				: "Filter<Not(Not(Exists(t1))) a0>(Input<t0>)|"
				  "Exists(Input<t2>,Input<t3>)|t2 := t0;t3 := t1", "EQ", &error);
			GPOS_ASSERT(nullptr != bridge);
			outer->AddRef();
			exists->AddRef();
			CExpression *predicate = CUtils::PexprNegate(mp, CUtils::PexprNegate(mp, exists));
			if (negated) predicate = CUtils::PexprNegate(mp, predicate);
			CExpression *bridge_source = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CLogicalSelect(mp), outer, predicate);
			CDSLModel *bridge_model = GPOS_NEW(mp) CDSLModel(mp);
			CDSLMatcher bridge_matcher(mp, bridge);
			GPOS_ASSERT(bridge_matcher.FMatch(bridge->PfragSrc()->PopRoot(), bridge_source, bridge_model));
			GPOS_ASSERT(checker.FCheck(bridge, bridge_model));
			CExpression *bridge_target = inst.PexprInstantiate(bridge, bridge_model);
			GPOS_ASSERT(nullptr != bridge_target);
			GPOS_ASSERT(COperator::EopLogicalSelect == bridge_target->Pop()->Eopid());
			GPOS_ASSERT((negated ? COperator::EopScalarSubqueryNotExists
				: COperator::EopScalarSubqueryExists) == (*bridge_target)[1]->Pop()->Eopid());
			GPOS_ASSERT((*bridge_target)[0] == outer && (*(*bridge_target)[1])[0] == query);
			GPOS_ASSERT(source->DeriveOutputColumns()->Equals(bridge_target->DeriveOutputColumns()));
			bridge_target->Release();
			bridge_model->Release();
			bridge_source->Release();
			bridge->Release();
		}
		std::string exported, export_error;
		GPOS_ASSERT(CDSLPlanTemplate::FSlice(mp, source, "r", {"r/0"}, &exported, &export_error));
		GPOS_ASSERT(std::string::npos != exported.find("Not(Not(Exists("));
		// Negated/native non-subquery inputs must not masquerade as Exists.
		const CDSLSymbol *exists_symbol = nullptr;
		for (ULONG i = 0; i < rule->Pexprdefs()->UlDefinitions(); ++i)
		{
			const auto *def = rule->Pexprdefs()->PdefAt(i);
			if (EdslexprExists == def->Edslexpr() && CDSLExpressionDefinitions::EMatch == def->Binding())
				exists_symbol = def->PsymOutput();
		}
		GPOS_ASSERT(nullptr != exists_symbol);
		CDSLModel *conflict = GPOS_NEW(mp) CDSLModel(mp);
		conflict->FBind(rule->Pexprdefs()->Pdef(exists_symbol)->PsymOperand(0), outer);
		GPOS_ASSERT(!matcher.FMatchExpression(exists_symbol, exists, conflict));
		conflict->Release();
		CDSLRule *opaque = CDSLRuleParser::PdslruleParse(mp,
			"Filter<p0 a0>(Input<t0>)|Filter<Not(Not(p0)) a1>(Input<t1>)|"
			"t1 := t0;a1 := a0", "EQ", &error);
		GPOS_ASSERT(nullptr != opaque);
		CDSLModel *opaque_model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher opaque_matcher(mp, opaque);
		GPOS_ASSERT(!opaque_matcher.FMatch(opaque->PfragSrc()->PopRoot(), source, opaque_model));
		opaque_model->Release();
		opaque->Release();
		query->AddRef();
		CExpression *wrong[] = {
			CUtils::PexprScalarConstBool(mp, false),
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarSubqueryNotExists(mp), query)};
		for (CExpression *value : wrong)
		{
			CDSLModel *rejected = GPOS_NEW(mp) CDSLModel(mp);
			GPOS_ASSERT(!matcher.FMatchExpression(exists_symbol, value, rejected));
			rejected->Release();
			if (COperator::EopScalarSubqueryNotExists == value->Pop()->Eopid())
			{
				CDSLRule *negated = CDSLRuleParser::PdslruleParse(mp,
					"Filter<Not(Exists(t1)) a0>(Input<t0>)|"
					"Filter<Not(Not(Not(Exists(t1)))) a1>(Input<t2>)|t2 := t0;a1 := a0", "EQ", &error);
				GPOS_ASSERT(nullptr != negated);
				outer->AddRef();
				value->AddRef();
				CExpression *negative_source = GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CLogicalSelect(mp), outer, value);
				CDSLModel *negative_model = GPOS_NEW(mp) CDSLModel(mp);
				CDSLMatcher negative_matcher(mp, negated);
				GPOS_ASSERT(negative_matcher.FMatch(negated->PfragSrc()->PopRoot(), negative_source, negative_model));
				CDSLInstantiator negative_inst(mp);
				CExpression *negative_target = negative_inst.PexprInstantiate(negated, negative_model);
				GPOS_ASSERT(nullptr != negative_target);
				CExpression *predicate = (*negative_target)[1];
				for (ULONG i = 0; i < 3; ++i)
				{
					GPOS_ASSERT(CUtils::FScalarBoolOp(predicate, CScalarBoolOp::EboolopNot));
					predicate = (*predicate)[0];
				}
				GPOS_ASSERT(COperator::EopScalarSubqueryExists == predicate->Pop()->Eopid() && (*predicate)[0] == query);
				negative_target->Release();
				negative_model->Release();
				negative_source->Release();
				negated->Release();
			}
			value->Release();
		}
		target->Release();
		model->Release();
		source->Release();
	}
	rule->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLExistsTest::EresUnittest_ExpressionDefinedExistence()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	for (ULONG ul = 0; ul < 2; ul++)
	{
		const BOOL fNegated = 1 == ul;
		CExpression *pexprOuter = fix.PexprLogicalGet(
			fNegated ? "expression_not_exists_outer" : "expression_exists_outer",
			2);
		CExpression *pexprInner = fix.PexprLogicalGet(
			fNegated ? "expression_not_exists_inner" : "expression_exists_inner",
			2);
		COperator *popScalar =
			fNegated
				? static_cast<COperator *>(
					  GPOS_NEW(mp) CScalarSubqueryNotExists(mp))
				: static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryExists(mp));
		CExpression *pexprPredicate =
			GPOS_NEW(mp) CExpression(mp, popScalar, pexprInner);
		CExpression *pexprSource = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CLogicalSelect(mp), pexprOuter, pexprPredicate);

		CWStringDynamic strErr(mp);
		CDSLRule *prule = CDSLRuleParser::PdslruleParse(
			mp, fNegated ? GPOPT_DSL_EXPRESSION_DEFINED_NOT_EXISTS_RULE
							 : GPOPT_DSL_EXPRESSION_DEFINED_EXISTS_RULE,
			"EQ", &strErr);
		GPOS_ASSERT(nullptr != prule);
		CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher matcher(mp);
		GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
								   pmodel));
		CDSLConstraintChecker checker(mp);
		const CDSLConstraint *extraction = (*prule->Pdrgpcon())[1];
		const CDSLSymbol *predicate = (*extraction->Pdrgpsym())[0];
		const CDSLSymbol *input = (*extraction->Pdrgpsym())[1];
		// The legacy check produces a binding, not just a boolean result.
		// Missing/non-subquery captures, opposite polarity and a conflicting
		// prebound input must fail at this constraint without inventing an input.
		for (ULONG shape = 0; shape < 4; ++shape)
		{
			CDSLModel *rejected = GPOS_NEW(mp) CDSLModel(mp);
			if (1 == shape)
			{
				CExpression *value = CUtils::PexprScalarConstBool(mp, true);
				rejected->FBind(predicate, value);
				value->Release();
			}
			else if (2 == shape)
			{
				pexprInner->AddRef();
				COperator *opposite = fNegated
					? static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryExists(mp))
					: static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryNotExists(mp));
				CExpression *value = GPOS_NEW(mp) CExpression(mp, opposite, pexprInner);
				rejected->FBind(predicate, value);
				value->Release();
			}
			else if (3 == shape)
			{
				rejected->FBind(predicate, pexprPredicate);
				rejected->FBind(input, pexprOuter);
			}
			const CDSLConstraint *failed = nullptr;
			ULONG index = gpos::ulong_max;
			GPOS_ASSERT(!checker.FCheck(prule, rejected, &failed, &index));
			GPOS_ASSERT(extraction == failed && 1 == index);
			GPOS_ASSERT(rejected->PexprTable(input) == (3 == shape ? pexprOuter : nullptr));
			rejected->Release();
		}
		GPOS_ASSERT(checker.FCheck(prule, pmodel));
		GPOS_ASSERT(pmodel->PexprTable(input) == pexprInner);

		CDSLInstantiator instantiator(mp);
		CExpression *pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		GPOS_ASSERT(nullptr != pexprTarget);
		GPOS_ASSERT((fNegated ? COperator::EopLogicalLeftAntiSemiApply
							  : COperator::EopLogicalLeftSemiApply) ==
					pexprTarget->Pop()->Eopid());
		GPOS_ASSERT((fNegated ? COperator::EopScalarSubqueryNotExists
							  : COperator::EopScalarSubqueryExists) ==
					dynamic_cast<CLogicalApply *>(pexprTarget->Pop())
						->EopidOriginSubq());
		GPOS_ASSERT(pexprSource->DeriveOutputColumns()->Equals(
			pexprTarget->DeriveOutputColumns()));

		pexprTarget->Release();
		pmodel->Release();
		prule->Release();
		pexprSource->Release();
	}
	return GPOS_OK;
}

GPOS_RESULT
CDSLExistsTest::EresUnittest_PredicateSemiJoinRoundTrip()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CColRefArray *pdrgpcrOuter = nullptr;
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("predicate_exists_outer", 2, &pdrgpcrOuter);
	CExpression *pexprInner =
		fix.PexprLogicalGet("predicate_exists_inner", 2, &pdrgpcrInner);
	CExpression *pexprPred = fix.PexprPredAtom((*pdrgpcrOuter)[1]);
	CExpression *pexprSemiJoin =
		CUtils::PexprLogicalJoin<CLogicalLeftSemiJoin>(
			mp, pexprOuter, pexprInner, pexprPred);

	CWStringDynamic strErr(mp);
	CDSLRule *prule = CDSLRuleParser::PdslruleParse(
		mp, GPOPT_DSL_PREDICATE_EXISTS_IDENTITY_RULE, "EQ", &strErr);
	if (nullptr == prule)
	{
		pexprSemiJoin->Release();
		return GPOS_FAILED;
	}

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSemiJoin, pmodel))
	{
		eres = GPOS_FAILED;
	}
	else
	{
		CDSLInstantiator inst(mp);
		pexprTarget = inst.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTarget ||
			COperator::EopLogicalLeftSemiJoin != pexprTarget->Pop()->Eopid() ||
			!(*pexprTarget)[2]->Matches((*pexprSemiJoin)[2]))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	prule->Release();
	pexprSemiJoin->Release();
	return eres;
}

GPOS_RESULT
CDSLExistsTest::EresUnittest_CorpusAggProjRoundTrip()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	// Outer child: genuine Global GbAgg, group c0 and MAX(c1).
	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuterGet =
		fix.PexprLogicalGet("exists_outer", 2, &pdrgpcrOuter);
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrOuter)[0]);
	CColRef *pcrMax = fix.PcrCreateInt4("exists_max");
	CExpression *pexprAgg = fix.PexprLogicalGbAgg(
		pexprOuterGet, pdrgpcrGroup, pcrMax, (*pdrgpcrOuter)[1]);
	pdrgpcrGroup->Release();

	// Inner child: Proj, wrapped in the exact LIMIT 1 inserted by ORCA for an
	// uncorrelated EXISTS.
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprInnerGet =
		fix.PexprLogicalGet("exists_inner", 2, &pdrgpcrInner);
	CColRefArray *pdrgpcrProjected = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrProjected->Append((*pdrgpcrInner)[0]);
	CExpression *pexprProject =
		fix.PexprLogicalProject(pexprInnerGet, pdrgpcrProjected);
	pdrgpcrProjected->Release();
	CColRef *pcrExistsCheck =
		pexprProject->DeriveOutputColumns()->PcrFirst();
	CExpression *pexprLimit = CUtils::PexprLimit(mp, pexprProject, 0, 1);
	CExpression *pexprSource =
		CUtils::PexprLogicalApply<CLogicalLeftSemiApply>(
			mp, pexprAgg, pexprLimit, pcrExistsCheck,
			COperator::EopScalarSubqueryExists);

	CWStringDynamic strErr(mp);
	CDSLRule *prule = CDSLRuleParser::PdslruleParse(
		mp, GPOPT_DSL_CORPUS_EXISTS_AGG_PROJ_RULE, "EQ", &strErr);
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(COperator::EopLogicalLeftSemiApply ==
				prule->EopidSrcRoot());

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
							   pmodel));
	CDSLConstraintChecker checker(mp);
	GPOS_ASSERT(checker.FCheck(prule, pmodel));

	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget =
		instantiator.PexprInstantiate(prule, pmodel);
	GPOS_ASSERT(nullptr != pexprTarget);
	GPOS_ASSERT(COperator::EopLogicalLeftSemiApply ==
				pexprTarget->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalGbAgg == (*pexprTarget)[0]->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalLimit == (*pexprTarget)[1]->Pop()->Eopid());
	// EXISTS does not observe target-list values, so an ordinary scalar Project
	// is removed before the Limit carrier is constructed.
	GPOS_ASSERT(COperator::EopLogicalGet ==
				(*(*pexprTarget)[1])[0]->Pop()->Eopid());
	GPOS_ASSERT(CUtils::FScalarConstTrue((*pexprTarget)[2]));
	GPOS_ASSERT(COperator::EopScalarSubqueryExists ==
				dynamic_cast<CLogicalApply *>(pexprTarget->Pop())
					->EopidOriginSubq());
	GPOS_ASSERT(pexprSource->DeriveOutputColumns()->Equals(
		pexprTarget->DeriveOutputColumns()));

	pexprTarget->Release();
	pmodel->Release();
	prule->Release();
	pexprSource->Release();
	pexprOuterGet->Release();
	pexprInnerGet->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLExistsTest::EresUnittest_PreApplyCorpusAggProjRoundTrip()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuterGet =
		fix.PexprLogicalGet("exists_preapply_outer", 2, &pdrgpcrOuter);
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrOuter)[0]);
	CColRef *pcrMax = fix.PcrCreateInt4("exists_preapply_max");
	CExpression *pexprAgg = fix.PexprLogicalGbAgg(
		pexprOuterGet, pdrgpcrGroup, pcrMax, (*pdrgpcrOuter)[1]);
	pdrgpcrGroup->Release();

	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprInnerGet =
		fix.PexprLogicalGet("exists_preapply_inner", 2, &pdrgpcrInner);
	CColRefArray *pdrgpcrProjected = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrProjected->Append((*pdrgpcrInner)[0]);
	CExpression *pexprProject =
		fix.PexprLogicalProject(pexprInnerGet, pdrgpcrProjected);
	pdrgpcrProjected->Release();

	CExpression *pexprScalarExists = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarSubqueryExists(mp), pexprProject);
	CExpression *pexprSource = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), pexprAgg, pexprScalarExists);

	CWStringDynamic strErr(mp);
	CDSLRule *prule = CDSLRuleParser::PdslruleParse(
		mp, GPOPT_DSL_CORPUS_EXISTS_AGG_PROJ_RULE, "EQ", &strErr);
	GPOS_ASSERT(nullptr != prule);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
							   pmodel));
	CDSLConstraintChecker checker(mp);
	GPOS_ASSERT(checker.FCheck(prule, pmodel));

	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget =
		instantiator.PexprInstantiate(prule, pmodel);
	GPOS_ASSERT(nullptr != pexprTarget);
	GPOS_ASSERT(COperator::EopLogicalLeftSemiApply ==
				pexprTarget->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalGbAgg == (*pexprTarget)[0]->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalLimit == (*pexprTarget)[1]->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalGet ==
				(*(*pexprTarget)[1])[0]->Pop()->Eopid());
	GPOS_ASSERT(CUtils::FScalarConstTrue((*pexprTarget)[2]));
	GPOS_ASSERT(COperator::EopScalarSubqueryExists ==
				dynamic_cast<CLogicalApply *>(pexprTarget->Pop())
					->EopidOriginSubq());
	GPOS_ASSERT(pexprSource->DeriveOutputColumns()->Equals(
		pexprTarget->DeriveOutputColumns()));

	pexprTarget->Release();
	pmodel->Release();
	prule->Release();
	pexprSource->Release();
	pexprOuterGet->Release();
	pexprInnerGet->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLExistsTest::EresUnittest_PreApplyPreservesResidual()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuterGet =
		fix.PexprLogicalGet("exists_residual_outer", 2, &pdrgpcrOuter);
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrOuter)[0]);
	CColRef *pcrMax = fix.PcrCreateInt4("exists_residual_max");
	CExpression *pexprAgg = fix.PexprLogicalGbAgg(
		pexprOuterGet, pdrgpcrGroup, pcrMax, (*pdrgpcrOuter)[1]);
	pdrgpcrGroup->Release();

	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprInnerGet =
		fix.PexprLogicalGet("exists_residual_inner", 2, &pdrgpcrInner);
	CColRefArray *pdrgpcrProjected = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrProjected->Append((*pdrgpcrInner)[0]);
	CExpression *pexprProject =
		fix.PexprLogicalProject(pexprInnerGet, pdrgpcrProjected);
	pdrgpcrProjected->Release();

	CExpression *pexprResidual = fix.PexprPredAtom(pcrMax);
	CExpression *pexprScalarExists = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarSubqueryExists(mp), pexprProject);
	CExpressionArray *pdrgpexprConj = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexprConj->Append(pexprResidual);
	pdrgpexprConj->Append(pexprScalarExists);
	CExpression *pexprPred =
		CPredicateUtils::PexprConjunction(mp, pdrgpexprConj);
	CExpression *pexprSource = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), pexprAgg, pexprPred);

	CWStringDynamic strErr(mp);
	CDSLRule *prule = CDSLRuleParser::PdslruleParse(
		mp, GPOPT_DSL_CORPUS_EXISTS_AGG_PROJ_RULE, "EQ", &strErr);
	GPOS_ASSERT(nullptr != prule);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
							   pmodel));
	// The sibling predicate is part of the matched Agg's HAVING binding,
	// so it survives target construction without an unscoped residual slot.

	CDSLConstraintChecker checker(mp);
	GPOS_ASSERT(checker.FCheck(prule, pmodel));
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget =
		instantiator.PexprInstantiate(prule, pmodel);
	GPOS_ASSERT(nullptr != pexprTarget);
	GPOS_ASSERT(COperator::EopLogicalLeftSemiApply ==
				pexprTarget->Pop()->Eopid());
	// Normalization pushes an outer-only sibling predicate below SemiApply. It
	// must remain attached to the outer Agg rather than being discarded.
	GPOS_ASSERT(COperator::EopLogicalSelect ==
				(*pexprTarget)[0]->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalGbAgg ==
				(*(*pexprTarget)[0])[0]->Pop()->Eopid());
	GPOS_ASSERT((*(*pexprTarget)[0])[1]->Matches(pexprResidual));
	GPOS_ASSERT(pexprSource->DeriveOutputColumns()->Equals(
		pexprTarget->DeriveOutputColumns()));

	pexprTarget->Release();
	pmodel->Release();
	prule->Release();
	pexprSource->Release();
	pexprOuterGet->Release();
	pexprInnerGet->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLExistsTest::EresUnittest_PreApplyNotExistsDistinctDrop()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("not_exists_pre_outer", 2, &pdrgpcrOuter);
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprInnerGet =
		fix.PexprLogicalGet("not_exists_pre_inner", 2, &pdrgpcrInner);
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrInner)[0]);
	CExpression *pexprDistinct =
		fix.PexprLogicalGbAgg(pexprInnerGet, pdrgpcrGroup);
	pdrgpcrGroup->Release();
	CExpression *pexprNotExists = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarSubqueryNotExists(mp), pexprDistinct);
	CExpression *pexprSource = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), pexprOuter, pexprNotExists);

	CWStringDynamic strErr(mp);
	CDSLRule *prule = CDSLRuleParser::PdslruleParse(
		mp, GPOPT_DSL_NOT_EXISTS_DISTINCT_DROP_RULE, "EQ", &strErr);
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(COperator::EopLogicalLeftAntiSemiApply ==
				prule->EopidSrcRoot());
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
							   pmodel));
	GPOS_ASSERT(pmodel->FDedupDrop());
	CDSLConstraintChecker checker(mp);
	GPOS_ASSERT(checker.FCheck(prule, pmodel));
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget =
		instantiator.PexprInstantiate(prule, pmodel);
	GPOS_ASSERT(nullptr != pexprTarget);
	GPOS_ASSERT(COperator::EopLogicalLeftAntiSemiApply ==
				pexprTarget->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalGet == (*pexprTarget)[1]->Pop()->Eopid());
	GPOS_ASSERT(CUtils::FScalarConstTrue((*pexprTarget)[2]));
	GPOS_ASSERT(COperator::EopScalarSubqueryNotExists ==
				CLogicalApply::PopConvert(pexprTarget->Pop())
					->EopidOriginSubq());

	pexprTarget->Release();
	pmodel->Release();
	prule->Release();
	pexprSource->Release();
	pexprInnerGet->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLExistsTest::EresUnittest_PostApplyNotExistsDistinctDrop()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CColRefArray *pdrgpcrOuter = nullptr;
	CExpression *pexprOuter =
		fix.PexprLogicalGet("not_exists_apply_outer", 2, &pdrgpcrOuter);
	CColRefArray *pdrgpcrInner = nullptr;
	CExpression *pexprInnerGet =
		fix.PexprLogicalGet("not_exists_apply_inner", 2, &pdrgpcrInner);
	CColRefArray *pdrgpcrGroup = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrGroup->Append((*pdrgpcrInner)[0]);
	CExpression *pexprDistinct =
		fix.PexprLogicalGbAgg(pexprInnerGet, pdrgpcrGroup);
	pdrgpcrGroup->Release();
	CExpression *pexprSource =
		CUtils::PexprLogicalApply<CLogicalLeftAntiSemiApply>(
			mp, pexprOuter, pexprDistinct, (*pdrgpcrInner)[0],
			COperator::EopScalarSubqueryNotExists);

	CWStringDynamic strErr(mp);
	CDSLRule *prule = CDSLRuleParser::PdslruleParse(
		mp, GPOPT_DSL_NOT_EXISTS_DISTINCT_DROP_RULE, "EQ", &strErr);
	GPOS_ASSERT(nullptr != prule);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	GPOS_ASSERT(matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprSource,
							   pmodel));
	GPOS_ASSERT(pmodel->FDedupDrop());
	CDSLConstraintChecker checker(mp);
	GPOS_ASSERT(checker.FCheck(prule, pmodel));
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget =
		instantiator.PexprInstantiate(prule, pmodel);
	GPOS_ASSERT(nullptr != pexprTarget);
	GPOS_ASSERT(COperator::EopLogicalLeftAntiSemiApply ==
				pexprTarget->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopLogicalGet == (*pexprTarget)[1]->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopScalarSubqueryNotExists ==
				CLogicalApply::PopConvert(pexprTarget->Pop())
					->EopidOriginSubq());

	pexprTarget->Release();
	pmodel->Release();
	prule->Release();
	pexprSource->Release();
	pexprInnerGet->Release();
	return GPOS_OK;
}

GPOS_RESULT
CDSLExistsTest::EresUnittest_ExistsPolarityIsolation()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CExpression *pexprOuter = fix.PexprLogicalGet("polarity_outer", 1);
	CExpression *pexprInner = fix.PexprLogicalGet("polarity_inner", 1);
	CExpression *pexprScalarExists = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarSubqueryExists(mp), pexprInner);
	CExpression *pexprPositive = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), pexprOuter, pexprScalarExists);

	CWStringDynamic strErr(mp);
	CDSLRule *pruleNotExists = CDSLRuleParser::PdslruleParse(
		mp,
		"NotExists(Input<t0>,Input<t1>)|NotExists(Input<t2>,Input<t3>)|"
		"TableEq(t2,t0);TableEq(t3,t1)",
		"EQ", &strErr);
	GPOS_ASSERT(nullptr != pruleNotExists);
	CDSLModel *pmodelPositive = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcherNotExists(mp, pruleNotExists);
	GPOS_ASSERT(!matcherNotExists.FMatch(
		pruleNotExists->PfragSrc()->PopRoot(), pexprPositive, pmodelPositive));

	CExpression *pexprOuter2 = fix.PexprLogicalGet("polarity_outer2", 1);
	CExpression *pexprInner2 = fix.PexprLogicalGet("polarity_inner2", 1);
	CExpression *pexprScalarNotExists = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarSubqueryNotExists(mp), pexprInner2);
	CExpression *pexprNegative = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), pexprOuter2,
		pexprScalarNotExists);
	CWStringDynamic strErrExists(mp);
	CDSLRule *pruleExists = CDSLRuleParser::PdslruleParse(
		mp,
		"Exists(Input<t0>,Input<t1>)|Exists(Input<t2>,Input<t3>)|"
		"TableEq(t2,t0);TableEq(t3,t1)",
		"EQ", &strErrExists);
	GPOS_ASSERT(nullptr != pruleExists);
	CDSLModel *pmodelNegative = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcherExists(mp, pruleExists);
	GPOS_ASSERT(!matcherExists.FMatch(pruleExists->PfragSrc()->PopRoot(),
								   pexprNegative, pmodelNegative));

	pmodelNegative->Release();
	pruleExists->Release();
	pexprNegative->Release();
	pmodelPositive->Release();
	pruleNotExists->Release();
	pexprPositive->Release();
	return GPOS_OK;
}

// EOF
