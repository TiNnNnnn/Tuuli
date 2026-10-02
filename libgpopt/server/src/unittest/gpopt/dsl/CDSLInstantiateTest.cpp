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
#include "gpopt/base/CColumnFactory.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/metadata/CColumnDescriptor.h"
#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLExpressionDefinitions.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLPlanTemplate.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/dsl/CDSLRulePrefixIndex.h"
#include "gpopt/operators/CExpressionUtils.h"
#include "gpopt/operators/CLogicalConstTableGet.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarSubquery.h"
#include "gpopt/operators/CScalarWindowFunc.h"
#include "gpopt/operators/CScalarSubqueryExists.h"
#include "gpopt/operators/CScalarSubqueryNotExists.h"
#include "gpopt/operators/CScalarSubqueryAny.h"
#include "gpopt/operators/CScalarSubqueryAll.h"
#include "gpopt/operators/CLogicalLeftAntiSemiApplyNotIn.h"
#include "gpopt/operators/CLogicalLeftAntiSemiCorrelatedApplyNotIn.h"
#include "gpopt/operators/CLogicalLeftSemiApply.h"
#include "gpopt/operators/CLogicalLeftSemiCorrelatedApply.h"
#include "gpopt/operators/CLogicalLeftSemiApplyIn.h"
#include "gpopt/operators/CLogicalLeftSemiCorrelatedApplyIn.h"
#include "gpopt/operators/CLogicalLeftAntiSemiApply.h"
#include "gpopt/operators/CLogicalLeftAntiSemiCorrelatedApply.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarIf.h"
#include "gpopt/operators/CScalarFunc.h"
#include "gpopt/operators/CScalarOp.h"
#include "gpopt/operators/CScalarCmp.h"
#include "naucrates/md/CMDIdGPDB.h"
#include "naucrates/md/IMDTypeInt4.h"
#include "naucrates/md/IMDTypeInt8.h"
#include "naucrates/md/CMDTypeInt4GPDB.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalInnerApply.h"
#include "gpopt/operators/CLogicalInnerCorrelatedApply.h"
#include "gpopt/operators/CLogicalLeftOuterApply.h"
#include "gpopt/operators/CLogicalLeftOuterCorrelatedApply.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CLogicalFullOuterJoin.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"
#include "gpopt/operators/CLogicalLeftAntiSemiJoin.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarBooleanTest.h"
#include "gpopt/operators/CScalarNullTest.h"
#include "gpopt/search/CGroupExpression.h"
#include "gpopt/search/CMemo.h"
#include "gpopt/xforms/CXformCollapseGbAgg.h"
#include "naucrates/md/IMDTypeBool.h"

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

static GPOS_RESULT
EresTypedQuantifiedDistinct()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	BOOL ok = true;
	for (ULONG kind = 0; kind < 3; ++kind)
	for (BOOL nested : {false, true})
	for (ULONG carrier = 0; carrier < (2 == kind ? 5U : 4U); ++carrier)
	{
		const BOOL all = 1 == kind, membership = 2 == kind;
		const std::string name = membership ? "InSubFilter" : all ? "All" : "Any";
		std::string source_text = name + (membership ? "<a0>" : "<p0 a0>") +
			"(Input<t0>,Proj*<a1 s0>(Input<t1>))";
		std::string target_text = name + (membership ? "<a2>" : "<p1 a2>") +
			"(Input<t2>,Proj<a3 s1>(Input<t3>))";
		std::string bindings = "AttrsSub(a0,t0);AttrsSub(a1,t1);t2 := t0;t3 := t1;"
			"a2 := a0;a3 := a1;s1 := s0";
		if (!membership)
			bindings += ";p1 := p0";
		if (nested)
		{
			source_text = "Filter<p2 a4>(" + source_text + ")";
			target_text = "Filter<p3 a5>(" + target_text + ")";
			bindings += ";p3 := p2;a5 := a4";
		}
		CDSLRule *rule = PdslruleParseLocal(mp, (source_text + "|" + target_text + "|" + bindings).c_str());
		GPOS_UNITTEST_ASSERT(nullptr != rule);
		CColRefArray *lc = nullptr, *rc = nullptr;
		CExpression *left = fix.PexprLogicalGet("quant_left", 1, &lc);
		CExpression *right = fix.PexprLogicalGet("quant_right", 1, &rc);
		CExpression *dedup = fix.PexprLogicalGbAgg(right, rc);
		CExpression *cmp = CUtils::PexprScalarCmp(mp,
			CUtils::PexprScalarIdent(mp, (*lc)[0]), CUtils::PexprScalarIdent(mp, (*rc)[0]),
			(all && 1 == carrier) || (membership && 4 == carrier) ? IMDType::EcmptNEq : IMDType::EcmptEq);
		CExpression *source = nullptr;
		if (0 == carrier)
		{
			auto *op = CScalarCmp::PopConvert(cmp->Pop());
			op->MdIdOp()->AddRef();
			auto *label = GPOS_NEW(mp) CWStringConst(mp, op->Pstr()->GetBuffer());
			COperator *quantifier = all
				? static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryAll(mp, op->MdIdOp(), label, (*rc)[0]))
				: GPOS_NEW(mp) CScalarSubqueryAny(mp, op->MdIdOp(), label, (*rc)[0]);
			source = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalSelect(mp), left,
				GPOS_NEW(mp) CExpression(mp, quantifier, dedup, CUtils::PexprScalarIdent(mp, (*lc)[0])));
			cmp->Release();
		}
		else
		{
			const auto origin = 3 == carrier ? COperator::EopScalarSubqueryExists
				: all ? COperator::EopScalarSubqueryAll : COperator::EopScalarSubqueryAny;
			if (all)
				source = 2 == carrier
					? CUtils::PexprLogicalApply<CLogicalLeftAntiSemiCorrelatedApplyNotIn>(mp, left, dedup, (*rc)[0], origin, cmp)
					: CUtils::PexprLogicalApply<CLogicalLeftAntiSemiApplyNotIn>(mp, left, dedup, (*rc)[0], origin, cmp);
			else
				source = 2 == carrier
					? CUtils::PexprLogicalApply<CLogicalLeftSemiCorrelatedApplyIn>(mp, left, dedup, (*rc)[0], origin, cmp)
					: CUtils::PexprLogicalApply<CLogicalLeftSemiApplyIn>(mp, left, dedup, (*rc)[0], origin, cmp);
		}
		if (nested)
			source = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalSelect(mp), source, fix.PexprPredAtom((*lc)[0]));
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		const BOOL matched = CDSLMatcher(mp, rule).FMatch(rule->PfragSrc()->PopRoot(), source, model) &&
			CDSLConstraintChecker(mp).FCheck(rule, model);
		CDSLInstantiator inst(mp);
		CExpression *target = matched ? inst.PexprInstantiate(rule, model) : nullptr;
		CDSLRulePrefixIndex index(mp);
		index.Insert(rule, 0, source->Pop()->Eopid());
		CDSLRuleArray *candidates = index.PdrgpruleCandidates(mp, source);
		const BOOL valid = matched == (carrier < 3) &&
			(carrier >= 3 || (nullptr != target && 1 == candidates->Size()));
		if (!valid)
			GPOS_TRACE_FORMAT("typed quantified kind=%lu nested=%d carrier=%lu matched=%d built=%d candidates=%lu",
				kind, nested, carrier, matched, nullptr != target, candidates->Size());
		ok &= valid;
		if (nullptr != target)
		{
			CExpression *quantifier = nested ? (*target)[0] : target;
			if (membership)
			{
				ok &= COperator::EopLogicalSelect == quantifier->Pop()->Eopid();
				CExpression *any = (*quantifier)[1];
				ok &= COperator::EopScalarSubqueryAny == any->Pop()->Eopid() &&
					CScalarSubqueryAny::PopConvert(any->Pop())->Pcr() == (*rc)[0] &&
					COperator::EopLogicalProject == (*any)[0]->Pop()->Eopid() &&
					(*(*any)[0])[0] == right && 0 == (*(*any)[0])[1]->Arity();
			}
			else
			{
				const auto expected = all
					? (2 == carrier ? COperator::EopLogicalLeftAntiSemiCorrelatedApplyNotIn : COperator::EopLogicalLeftAntiSemiApplyNotIn)
					: (2 == carrier ? COperator::EopLogicalLeftSemiCorrelatedApplyIn : COperator::EopLogicalLeftSemiApplyIn);
				ok &= expected == quantifier->Pop()->Eopid() &&
					COperator::EopLogicalProject == (*quantifier)[1]->Pop()->Eopid() &&
					(*(*quantifier)[1])[0] == right && 0 == (*(*quantifier)[1])[1]->Arity() &&
					CScalarCmp::PopConvert((*quantifier)[2]->Pop())->ParseCmpType() ==
						(all && 2 != carrier ? IMDType::EcmptNEq : IMDType::EcmptEq);
			}
		}
		candidates->Release(); CRefCount::SafeRelease(target);
		model->Release(); source->Release(); right->Release(); rule->Release();
	}
	return ok ? GPOS_OK : GPOS_FAILED;
}

static GPOS_RESULT
EresColumnValues()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	const std::string source_text =
		"Compute<Item(Case(p0,Column(a0),n0),a1,e0) a2 s0>(Input<t0>)|";
	const std::string aliases = "|t1 := t0;a4 := a2;s1 := s0";
	BOOL ok = true;
	for (ULONG trial = 0; trial < 4; ++trial)
	{
		CColRefArray *columns = nullptr;
		CExpression *input = fix.PexprLogicalGet("column_values", 2, &columns);
		IMDId *type = fix.Pmda()->PtMDType<IMDTypeInt4>()->MDId();
		type->AddRef();
		CExpression *value = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarIf(mp, type),
			fix.PexprEqConst((*columns)[0], 1),
			1 == trial ? CUtils::PexprScalarConstInt4(mp, 7) : CUtils::PexprScalarIdent(mp, (*columns)[1]),
			CUtils::PexprScalarConstInt4(mp, 9));
		CExpression *source = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp), input,
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
				GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectElement(mp, fix.PcrCreateInt4("result")), value)));
		// ATTRS may be a vector elsewhere, but Column requires exactly one;
		// scope validation must also reject reading this Project's own output.
		const CHAR *selected = 2 == trial ? "a2" : 3 == trial ? "a1" : "a3";
		CDSLRule *rule = PdslruleParseLocal(mp, (source_text +
			"Compute<Item(Case(Not(Not(p0)),Column(" + selected + "),n0),a1,e0) a4 s1>(Input<t1>)" +
			aliases + (trial < 2 ? ";a3 := a0" : "")).c_str());
		if (nullptr == rule) { source->Release(); return GPOS_FAILED; }
		CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, source);
		if ((EdsldecisionReady == decision->Status()) != (0 == trial))
			GPOS_TRACE_FORMAT("Column trial=%lu status=%d", trial, decision->Status());
		ok &= (EdsldecisionReady == decision->Status()) == (0 == trial);
		if (0 == trial && nullptr != decision->PexprTarget())
		{
			CExpression *target_value = (*(*(*decision->PexprTarget())[1])[0])[0];
			ok &= (*target_value)[1]->Matches((*value)[1]);
			std::string exported, error;
			ok &= CDSLPlanTemplate::FSlice(mp, source, "r", {"r/0"}, &exported, &error) &&
				exported.find("Column(") != std::string::npos;
			if (!ok) GPOS_TRACE_FORMAT("Column template=%s error=%s", exported.c_str(), error.c_str());
		}
		GPOS_DELETE(decision);
		rule->Release(); source->Release();
	}
	return ok ? GPOS_OK : GPOS_FAILED;
}

static GPOS_RESULT
EresColumnAliases()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	const std::string definitions = "Item(Column(a2),a3,Item(Column(a2),a4,Item()))";
	BOOL ok = true;
	for (ULONG trial = 0; trial < 7; ++trial)
	{
		const BOOL in_case = 3 <= trial;
		const BOOL in_condition = 6 == trial;
		const std::string text =
			"Compute<Item(" + std::string(in_condition ?
				"Case(Compare(c0,Args(Column(a3),Args(n0,Args()))),n3,n4)" :
				(in_case ? "Case(p0,Column(a3),n0)" : "Column(a3)")) +
			",a5,Item()) a0 s0>(Compute<" + definitions + " a1 s1>(Input<t0>))|"
			"Compute<Item(n1,a5,Item()) a6 s2>(Compute<" + definitions + " a7 s3>(Input<t1>))|" +
			(in_condition ? "n2 := Column(a4);v0 := Args();v1 := Args(n0,v0);v2 := Args(n2,v1);"
				"p1 := Compare(c0,v2);n1 := Case(p1,n3,n4);" :
				(in_case ? "n2 := Column(a4);n1 := Case(p0,n2,n0);" : "n1 := Column(a4);")) +
			"t1 := t0;s2 := s0;a7 := a1;s3 := s1;";
		CColRefArray *columns = nullptr;
		CExpression *input = fix.PexprLogicalGet("column_aliases", 2, &columns);
		CColRef *a = fix.PcrCreateInt4("a"), *b = fix.PcrCreateInt4("b");
		CExpressionArray *items = GPOS_NEW(mp) CExpressionArray(mp);
		items->Append(GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectElement(mp, a),
			CUtils::PexprScalarIdent(mp, (*columns)[0])));
		items->Append(GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectElement(mp, b),
			CUtils::PexprScalarIdent(mp, (*columns)[2 == trial ? 1 : 0])));
		CExpression *inner = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp), input,
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp), items));
		CExpression *source_value = CUtils::PexprScalarIdent(mp, a);
		if (in_case)
		{
			CExpression *condition = in_condition ? fix.PexprEqConst(a, 7) :
				CUtils::PexprScalarConstBool(mp, 4 == trial, 5 == trial);
			if (in_condition)
			{
				source_value->Release();
				source_value = CUtils::PexprScalarConstInt4(mp, 8);
			}
			IMDId *type = fix.Pmda()->PtMDType<IMDTypeInt4>()->MDId();
			type->AddRef();
			source_value = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarIf(mp, type),
				condition, source_value,
				CUtils::PexprScalarConstInt4(mp, 9));
		}
		CExpression *source = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp), inner,
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
				GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CScalarProjectElement(mp, fix.PcrCreateInt4("result")),
					source_value)));
		CDSLRule *rule = PdslruleParseLocal(mp,
			(text + (1 == trial ? "a6 := a0" : "a6 := ScalarDeps(n1)")).c_str());
		if (nullptr == rule) { source->Release(); return GPOS_FAILED; }
		CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, source);
		ok &= (EdsldecisionReady == decision->Status()) == (0 == trial || in_case);
		if (0 == trial || in_case)
		{
			CExpression *target = decision->PexprTarget();
			ok &= nullptr != target;
			if (nullptr != target)
			{
				CExpression *value = (*(*(*target)[1])[0])[0];
				if (in_case)
				{
					if (COperator::EopScalarIf != value->Pop()->Eopid() || 3 != value->Arity())
					{
						GPOS_DELETE(decision); rule->Release(); source->Release(); return GPOS_FAILED;
					}
					ok &= value->Pop()->Matches(source_value->Pop()) &&
						(*value)[2]->Matches((*source_value)[2]);
					if (in_condition)
					{
						ok &= (*value)[1]->Matches((*source_value)[1]);
						value = (*value)[0];
						if (COperator::EopScalarCmp != value->Pop()->Eopid() || 2 != value->Arity())
						{
							GPOS_DELETE(decision); rule->Release(); source->Release(); return GPOS_FAILED;
						}
						ok &= value->Pop()->Matches((*source_value)[0]->Pop()) &&
							(*value)[1]->Matches((*(*source_value)[0])[1]);
						value = (*value)[0];
					}
					else
					{
						ok &= (*value)[0]->Matches((*source_value)[0]);
						value = (*value)[1];
					}
				}
				ok &= COperator::EopScalarIdent == value->Pop()->Eopid() &&
					CScalarIdent::PopConvert(value->Pop())->Pcr() == b &&
					(*target)[0]->Matches(inner) &&
					target->DeriveOutputColumns()->Equals(source->DeriveOutputColumns());
			}
		}
		GPOS_DELETE(decision);
		rule->Release(); source->Release();
	}
	return ok ? GPOS_OK : GPOS_FAILED;
}

static GPOS_RESULT
EresComputeColumnDerivations()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	const std::string sourceText = "Compute<Item(n0,a3,Item()) a0 s0>("
		"Compute<Item(n1,a4,Item()) a1 s1>(Input<t0>))|";
	const std::string targetText = "Compute<Item(n1,a4,Item(n0,a3,Item())) a2 s2>(Input<t1>)|";
	BOOL ok = true;
	// Construction only, not an installed/proved law. Incorrect dependencies
	// and output layouts must still fail at the shared builder boundary.
	const struct { const CHAR *constraints; BOOL ready; } cases[] = {
		{"t1 := t0;AttrsUnion(a2,a0,a1);SchemaUnion(s2,s1,a3)", true},
		{"t1 := t0;AttrsUnion(a2,a0,a0);SchemaUnion(s2,s1,a3)", false},
		{"t1 := t0;AttrsUnion(a2,a0,a1);SchemaUnion(s2,s0,a4)", false},
		{"t1 := t0;AttrsUnion(a2,a5,a1);SchemaUnion(s2,s1,a3);a5 := a0", true},
		{"t1 := t0;SchemaFromAttrs(s2,a6);AttrsUnion(a6,a4,a3);AttrsUnion(a2,a0,a1)", true},
		{"t1 := t0;AttrsUnion(a2,a0,a1);AttrsUnion(a6,a4,a3);SchemaFromAttrs(s2,a6)", true},
		{"t1 := t0;SchemaFromAttrs(s2,a6);AttrsUnion(a6,a4,a3);AttrsUnion(a2,a0,a1);"
		 "SchemaUnion(s1,s0,a4)", false},
		{"t1 := t0;OutputAttrs(a2,t1);SchemaUnion(s2,s1,a3)", true},
	};
	for (ULONG trial = 0; trial < GPOS_ARRAY_SIZE(cases); ++trial)
	{
		CDSLRule *rule = PdslruleParseLocal(mp, (sourceText + targetText + cases[trial].constraints).c_str());
		if (nullptr == rule)
		{
			GPOS_TRACE_FORMAT("Compute derivation parse failed trial=%lu", trial);
			return GPOS_FAILED;
		}
		CColRefArray *inputs = nullptr;
		CExpression *input = fix.PexprLogicalGet("compute_columns", 2, &inputs);
		for (ULONG i = 0; i < inputs->Size(); ++i) (*inputs)[i]->MarkAsUsed();
		CColRef *lowerOutput = fix.PcrCreateInt4("lower");
		CColRef *upperOutput = fix.PcrCreateInt4("upper");
		input->AddRef();
		CExpression *lower = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp), input,
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
				GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectElement(mp, lowerOutput),
					CUtils::PexprScalarIdent(mp, (*inputs)[0]))));
		lower->AddRef();
		CExpression *source = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp), lower,
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
				GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectElement(mp, upperOutput),
					CUtils::PexprScalarIdent(mp, (*inputs)[1]))));
		CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, source);
		const BOOL ready = EdsldecisionReady == decision->Status();
		if (ready != cases[trial].ready)
			GPOS_TRACE_FORMAT("Compute derivation trial=%lu status=%d", trial, decision->Status());
		ok &= ready == cases[trial].ready;
		if (cases[trial].ready && ready)
		{
			CExpression *target = decision->PexprTarget();
			ok &= COperator::EopLogicalProject == target->Pop()->Eopid() &&
				(*target)[0]->Matches(input) && 2 == (*target)[1]->Arity() &&
				(*(*target)[1])[0]->Matches((*(*lower)[1])[0]) &&
				(*(*target)[1])[1]->Matches((*(*source)[1])[0]) &&
				target->DeriveOutputColumns()->Equals(source->DeriveOutputColumns());
			if (!ok) GPOS_TRACE_FORMAT("Compute derivation target failed trial=%lu", trial);
		}
		GPOS_DELETE(decision);
		source->Release(); lower->Release(); input->Release(); rule->Release();
	}
	return ok ? GPOS_OK : GPOS_FAILED;
}

enum class ComputeAliasValue { Column, Call, And, Or };

static GPOS_RESULT
EresComputeAliasFusionVariant(ComputeAliasValue kind, BOOL multiple = false)
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	const BOOL call = ComputeAliasValue::Call == kind;
	const BOOL junction = ComputeAliasValue::And == kind || ComputeAliasValue::Or == kind;
	GPOS_ASSERT(!multiple || junction);
	// Same native Compute rule as the FormalSQL public-DSL proof regression.
	auto template_value = [&](const CHAR *left, const CHAR *right) -> std::string {
		const std::string value = "Column(" + std::string(left) + ")";
		const std::string other = "Column(" + std::string(right) + ")";
		if (call) return "Call(h0,Args(" + value + ",Args(" + other + ",Args())))";
		if (junction) return "BoolValue(" + std::string(ComputeAliasValue::And == kind ? "And" : "Or") +
			"(ValueBool(" + value + "),Not(ValueBool(" + other + "))))";
		return value;
	};
	const std::string source_value = template_value("a1", multiple ? "a9" : "a1");
	const std::string target_value = template_value("a0", multiple ? "a8" : "a0");
	const std::string prefix = std::string("Item(Column(a0),a1,") + (multiple ? "Item(Column(a8),a9," : "");
	const std::string suffix = multiple ? "))" : ")";
	CDSLRule *rule = PdslruleParseLocal(mp, (
		"Compute<Item(" + source_value + ",a2,Item()) a5 s1>("
		"Compute<" + prefix + "Item()" + suffix + " a4 s0>(Input<t0>))|"
		"Compute<" + prefix + "Item(" + target_value + ",a2,Item())" + suffix + " a6 s3>(Input<t1>)|"
		"t1 := t0;AttrsUnion(a6,a0," + (multiple ? "a8" : "a0") + ");SchemaUnion(s3,s0,a2)").c_str());
	if (nullptr == rule) return GPOS_FAILED;
	auto value = [&](CColRef *left, CColRef *right) -> CExpression * {
		if (junction) return GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CScalarBoolOp(mp, ComputeAliasValue::And == kind ? CScalarBoolOp::EboolopAnd : CScalarBoolOp::EboolopOr),
			CUtils::PexprScalarIdent(mp, left),
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopNot),
				CUtils::PexprScalarIdent(mp, right)));
		if (!call) return CUtils::PexprScalarIdent(mp, left);
		IMDId *type = fix.Pmda()->PtMDType<IMDTypeBool>()->MDId();
		type->AddRef();
		return GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarFunc(mp,
			GPOS_NEW(mp) CMDIdGPDB(IMDId::EmdidGeneral, 65 /*int4eq*/), type,
			default_type_modifier, GPOS_NEW(mp) CWStringConst(GPOS_WSZ_LIT("int4eq")), 0, false),
			CUtils::PexprScalarIdent(mp, left), CUtils::PexprScalarIdent(mp, right));
	};
	BOOL ok = true;
	for (ULONG trial = 0; trial < 3; ++trial)
	{
		CColRefArray *inputs = nullptr;
		CExpression *input = nullptr;
		if (junction)
		{
			inputs = GPOS_NEW(mp) CColRefArray(mp);
			for (ULONG col = 0; col < (0 == trial ? (multiple ? 2UL : 1UL) : 3UL); ++col)
				inputs->Append(COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
					fix.Pmda()->PtMDType<IMDTypeBool>(), default_type_modifier));
			input = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CLogicalConstTableGet(mp, inputs, GPOS_NEW(mp) IDatum2dArray(mp)));
		}
		else input = fix.PexprLogicalGet("compute_alias_fusion", 0 == trial ? 1 : 3, &inputs);
		CColRef *lowerOutput = junction ? COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
			fix.Pmda()->PtMDType<IMDTypeBool>(), default_type_modifier) : fix.PcrCreateInt4("lower");
		CColRef *upperOutput = call || junction ? COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
			fix.Pmda()->PtMDType<IMDTypeBool>(), default_type_modifier) : fix.PcrCreateInt4("upper");
		CColRef *secondOutput = multiple ? COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
			fix.Pmda()->PtMDType<IMDTypeBool>(), default_type_modifier) : lowerOutput;
		CExpressionArray *definitions = GPOS_NEW(mp) CExpressionArray(mp);
		definitions->Append(GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectElement(mp, lowerOutput),
			CUtils::PexprScalarIdent(mp, (*inputs)[0])));
		if (multiple) definitions->Append(GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CScalarProjectElement(mp, secondOutput), CUtils::PexprScalarIdent(mp, (*inputs)[1])));
		input->AddRef();
		CExpression *lower = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp), input,
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp), definitions));
		lower->AddRef();
		CColRef *outerLeft = 2 == trial ? (*inputs)[multiple ? 2 : 1] : lowerOutput;
		CExpression *source = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp), lower,
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
				GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectElement(mp, upperOutput),
					value(outerLeft, multiple ? secondOutput : outerLeft))));
		CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, source);
		const BOOL ready = EdsldecisionReady == decision->Status();
		ok &= ready == (trial < 2);
		if (ready)
		{
			CExpression *target = decision->PexprTarget();
			CExpression *expected = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CScalarProjectElement(mp, upperOutput),
				value((*inputs)[0], (*inputs)[multiple ? 1 : 0]));
			ok &= COperator::EopLogicalProject == target->Pop()->Eopid() &&
				(*target)[0]->Matches(input) && (multiple ? 3UL : 2UL) == (*target)[1]->Arity() &&
				(*(*target)[1])[0]->Matches((*(*lower)[1])[0]) &&
				(!multiple || (*(*target)[1])[1]->Matches((*(*lower)[1])[1])) &&
				(*(*target)[1])[multiple ? 2 : 1]->Matches(expected) &&
				target->DeriveOutputColumns()->Equals(source->DeriveOutputColumns());
			expected->Release();
			CExpression *applied = CDSLRuleEngine::Instance()->PexprApply(mp, rule, source);
			ok &= nullptr != applied && applied->DeriveOutputColumns()->Equals(source->DeriveOutputColumns());
			CRefCount::SafeRelease(applied);
		}
		GPOS_DELETE(decision);
		source->Release(); lower->Release(); input->Release();
	}
	rule->Release();
	return ok ? GPOS_OK : GPOS_FAILED;
}

static GPOS_RESULT
EresComputeAliasFusion()
{
	for (ComputeAliasValue kind : {ComputeAliasValue::Column, ComputeAliasValue::Call,
		ComputeAliasValue::And, ComputeAliasValue::Or})
		if (GPOS_OK != EresComputeAliasFusionVariant(kind)) return GPOS_FAILED;
	for (ComputeAliasValue kind : {ComputeAliasValue::And, ComputeAliasValue::Or})
		if (GPOS_OK != EresComputeAliasFusionVariant(kind, true)) return GPOS_FAILED;
	return GPOS_OK;
}

static GPOS_RESULT
EresComputedResultOutputContract()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	// This exact rule is proved for strict DSL Proj by FormalSQL. Native
	// LogicalProject also exposes the lower computed column: dropping it is
	// not a valid alternative for arbitrary parents of the same memo group.
	CDSLRule *rule = PdslruleParseLocal(mp,
		"Proj<a0 s0 Item(Column(a2),a3,Item())>("
		"Proj<a1 s1 Item(n0,a2,Item())>(Input<t0>))|"
		"Proj<a4 s2 Item(n0,a3,Item())>(Input<t1>)|"
		"t1 := t0;a4 := a0;s2 := s0");
	if (nullptr == rule) return GPOS_FAILED;
	BOOL ok = true;
	for (ULONG width = 1; width <= 3; width += 2)
	{
		CExpression *input = fix.PexprLogicalGet("computed_result", width);
		CColRef *lowerOutput = fix.PcrCreateInt4("lower");
		CColRef *upperOutput = fix.PcrCreateInt4("upper");
		CExpression *lower = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp), input,
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
				GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectElement(mp, lowerOutput),
					CUtils::PexprScalarConstInt4(mp, 7))));
		CExpression *source = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp), lower,
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
				GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectElement(mp, upperOutput),
					CUtils::PexprScalarIdent(mp, lowerOutput))));
		CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, source);
		CExpression *target = decision->PexprTarget();
		ok &= EdsldecisionReady == decision->Status() && nullptr != target &&
			target->DeriveOutputColumns()->FMember(upperOutput) &&
			!target->DeriveOutputColumns()->FMember(lowerOutput) &&
			source->DeriveOutputColumns()->Size() == width + 2 &&
			target->DeriveOutputColumns()->Size() == width + 1;
		CExpression *applied = CDSLRuleEngine::Instance()->PexprApply(mp, rule, source);
		ok &= nullptr == applied;
		CRefCount::SafeRelease(applied);
		GPOS_DELETE(decision);
		source->Release();
	}
	rule->Release();
	return ok ? GPOS_OK : GPOS_FAILED;
}

static GPOS_RESULT
EresSystemColumnOutputContract()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	// Deliberately lossy rule, only for testing the output boundary. Never
	// admit this as an equivalence: the schema guard is not a rule prover.
	CDSLRule *rule = PdslruleParseLocal(mp,
		"Proj*<a0 s0>(Filter<p0 a2>(Input<t0>))|"
		"Proj*<a1 s1>(Filter<Not(Not(p0)) a3>(Input<t1>))|"
		"t1 := t0;AttrsIntersect(a1,a0,a2);SchemaFromAttrs(s1,a1);a3 := a2");
	GPOS_UNITTEST_ASSERT(nullptr != rule);
	BOOL ok = true;
	for (INT attno : {-1, 2})
	{
		for (CColRef::EUsedStatus usage : {CColRef::EUnknown, CColRef::EUnused, CColRef::EUsed})
		{
			CColRef *key = fix.PcrCreateInt4("key");
			CColumnDescriptor *descriptor = GPOS_NEW(mp) CColumnDescriptor(
				mp, key->RetrieveType(), default_type_modifier, key->Name(), attno, false);
			CColRef *extra = COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
				descriptor, key->Name(), 0, false, nullptr);
			descriptor->Release();
			if (CColRef::EUnused == usage) extra->MarkAsUnused();
			if (CColRef::EUsed == usage) extra->MarkAsUsed();
			CColRefArray *cols = GPOS_NEW(mp) CColRefArray(mp);
			cols->Append(key); cols->Append(extra);
			CExpression *input = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CLogicalConstTableGet(mp, cols, GPOS_NEW(mp) IDatum2dArray(mp)));
			CExpression *predicate = fix.PexprPredAtom(key);
			CExpression *filtered = fix.PexprLogicalSelect(input, predicate);
			CExpression *source = fix.PexprLogicalGbAgg(filtered, cols);
			CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, source);
			ok &= EdsldecisionReady == decision->Status() &&
				nullptr != decision->PexprTarget() &&
				!decision->PexprTarget()->DeriveOutputColumns()->FMember(extra);
			CExpression *target = CDSLRuleEngine::Instance()->PexprApply(mp, rule, source);
			ok &= (nullptr != target) == (attno < 0 && CColRef::EUsed != usage);
			CRefCount::SafeRelease(target);
			GPOS_DELETE(decision);
			source->Release(); filtered->Release(); predicate->Release(); input->Release();
		}
	}
	rule->Release();
	return ok ? GPOS_OK : GPOS_FAILED;
}

static GPOS_RESULT
EresMemoSourceOutputContract()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *rule = PdslruleParseLocal(mp,
		"Proj*<a0 s0>(Filter<p0 a2>(Input<t0>))|"
		"Proj*<a1 s1>(Filter<Not(Not(p0)) a3>(Input<t1>))|"
		"t1 := t0;a1 := a0;s1 := s0;a3 := a2");
	if (nullptr == rule) return GPOS_FAILED;
	BOOL ok = true;
	{
		CMemo memo(mp);
		const auto insert = [&](const auto &self, CExpression *expr,
			CGroup *group) -> CGroupExpression * {
			CGroupArray *children = GPOS_NEW(mp) CGroupArray(mp);
			for (ULONG i = 0; i < expr->Arity(); ++i)
				children->Append(self(self, (*expr)[i], nullptr)->Pgroup());
			expr->Pop()->AddRef();
			CGroupExpression *entry = GPOS_NEW(mp) CGroupExpression(mp,
				expr->Pop(), children, CXform::ExfInvalid, nullptr, false);
			CGroupExpression *canonical = nullptr;
			memo.PgroupInsert(group, expr, entry, &canonical);
			if (entry != canonical) entry->Release();
			return canonical;
		};
		CColRefArray *cols = GPOS_NEW(mp) CColRefArray(mp);
		for (ULONG i = 0; i < 3; ++i) cols->Append(fix.PcrCreateInt4("memo_output"));
		CExpression *input = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CLogicalConstTableGet(mp, cols, GPOS_NEW(mp) IDatum2dArray(mp)));
		CExpression *predicate = fix.PexprPredAtom((*cols)[0]);
		CExpression *filtered = fix.PexprLogicalSelect(input, predicate);
		predicate->Release();
		CExpression *wide = fix.PexprLogicalGbAgg(filtered, cols);
		CGroup *group = insert(insert, wide, nullptr)->Pgroup();
		CColRefArray *key = GPOS_NEW(mp) CColRefArray(mp);
		key->Append((*cols)[0]);
		CExpression *narrow = fix.PexprLogicalGbAgg(filtered, key);
		key->Release();
		CGroupExpression *entry = insert(insert, narrow, group);
		// Model native column pruning: the selected alternative emits one column,
		// while extraction inherits the original group's three-column properties.
		narrow->Pop()->AddRef();
		narrow->PdrgPexpr()->AddRef();
		CExpression *bound = GPOS_NEW(mp) CExpression(mp, narrow->Pop(), entry,
			narrow->PdrgPexpr(), nullptr, nullptr);
		ok &= 3 == bound->DeriveOutputColumns()->Size() &&
			1 == narrow->DeriveOutputColumns()->Size();
		if (!ok) GPOS_TRACE_FORMAT("memo output source widths: %lu/%lu",
			bound->DeriveOutputColumns()->Size(), narrow->DeriveOutputColumns()->Size());
		// Re-root a memo Input without remapping its operator or children. Only
		// the expression's Memo origin/properties must be detached.
		CDSLRule *identity = PdslruleParseLocal(mp, "Input<t0>|Input<t1>|t1 := t0");
		GPOS_UNITTEST_ASSERT(nullptr != identity);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		GPOS_UNITTEST_ASSERT(CDSLMatcher(mp, identity).FMatch(
			identity->PfragSrc()->PopRoot(), bound, model));
		GPOS_UNITTEST_ASSERT(CDSLConstraintChecker(mp).FCheck(identity, model));
		CExpression *fresh = CDSLInstantiator(mp).PexprInstantiate(identity, model);
		GPOS_UNITTEST_ASSERT(nullptr != fresh && fresh != bound && nullptr == fresh->Pgexpr());
		GPOS_UNITTEST_ASSERT(fresh->Pop() == bound->Pop());
		GPOS_UNITTEST_ASSERT((*fresh)[0] == (*bound)[0] && (*fresh)[1] == (*bound)[1]);
		GPOS_UNITTEST_ASSERT(fresh->DeriveOutputColumns()->Equals(narrow->DeriveOutputColumns()));
		fresh->Release();
		model->Release();
		identity->Release();
		for (CExpression *source : {narrow, bound})
		{
			CExpression *target = CDSLRuleEngine::Instance()->PexprApply(mp, rule, source);
			if (nullptr == target) GPOS_TRACE_FORMAT("memo output target rejected: bound=%d", source == bound);
			ok &= nullptr != target && target->DeriveOutputColumns()->Equals(
				narrow->DeriveOutputColumns());
			CRefCount::SafeRelease(target);
		}
		// An invalid target that really removes grouping columns must still fail.
		// This tests the runtime contract, not an admitted/proved rewrite rule.
		CDSLRule *lossy = PdslruleParseLocal(mp,
			"Proj*<a0 s0>(Filter<p0 a2>(Input<t0>))|"
			"Proj*<a1 s1>(Filter<Not(Not(p0)) a3>(Input<t1>))|"
			"t1 := t0;AttrsIntersect(a1,a0,a2);SchemaFromAttrs(s1,a1);a3 := a2");
		ok &= nullptr != lossy;
		if (nullptr != lossy)
		{
			CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, lossy, wide);
			ok &= EdsldecisionReady == decision->Status() &&
				nullptr != decision->PexprTarget() && 1 == decision->PexprTarget()->DeriveOutputColumns()->Size();
			GPOS_DELETE(decision);
			CExpression *target = CDSLRuleEngine::Instance()->PexprApply(mp, lossy, wide);
			ok &= nullptr == target;
			CRefCount::SafeRelease(target);
			lossy->Release();
		}
		// Logical binding enumeration can pair a wide top grouping with a
		// pruned bottom alternative. Collapse must test its own subset premise.
		for (CExpression *bottom : {narrow, wide})
		{
			CExpression *source = fix.PexprLogicalGbAgg(bottom, cols);
			CXformCollapseGbAgg *collapse = GPOS_NEW(mp) CXformCollapseGbAgg(mp);
			CXformContext *context = GPOS_NEW(mp) CXformContext(mp);
			CXformResult *result = GPOS_NEW(mp) CXformResult(mp);
			collapse->Transform(context, result, source);
			ok &= result->Size() == (bottom == wide ? 1 : 0);
			result->Release(); context->Release(); collapse->Release();
			source->Release();
		}
		bound->Release(); narrow->Release(); wide->Release(); filtered->Release(); input->Release();
	}
	rule->Release();
	return ok ? GPOS_OK : GPOS_FAILED;
}

static GPOS_RESULT
EresIntroducedOuterReference()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	// Runtime boundary check, not a generally valid admitted rule: a predicate
	// may move above grouping only if its columns survive that grouping.
	CDSLRule *rule = PdslruleParseLocal(mp,
		"Proj*<a0 s0>(Filter<p0 a2 a4>(Input<t0>))|"
		"Filter<p1 a3 a5>(Proj*<a1 s1>(Input<t1>))|"
		"TableEq(t1,t0);AttrsEq(a1,a0);SchemaEq(s1,s0);"
		"PredicateEq(p1,p0);AttrsEq(a3,a2);AttrsEq(a5,a4)");
	if (nullptr == rule) return GPOS_FAILED;
	CColRefArray *cols = nullptr;
	CExpression *input = fix.PexprLogicalGet("outer_reference", 2, &cols);
	CColRefArray *keys = GPOS_NEW(mp) CColRefArray(mp);
	keys->Append((*cols)[0]);
	CColRef *external = fix.PcrCreateInt4("existing_outer_reference");
	BOOL ok = true;
	for (ULONG i = 0; i < 3; ++i)
	{
		CExpression *predicate =
			fix.PexprPredAtom(2 == i ? external : (*cols)[i]);
		CExpression *filtered = fix.PexprLogicalSelect(input, predicate);
		CExpression *source = fix.PexprLogicalGbAgg(filtered, keys);
		CDSLRewriteDecision *decision =
			CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, source);
		// Existing correlations remain legal; only the lost local column fails.
		ok &= decision->Status() ==
			(1 == i ? EdsldecisionInstantiateRejected : EdsldecisionReady);
		GPOS_DELETE(decision);
		source->Release(); filtered->Release(); predicate->Release();
	}
	keys->Release(); input->Release(); rule->Release();
	return ok ? GPOS_OK : GPOS_FAILED;
}

static GPOS_RESULT
EresColumnProjectionFusion()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	const std::string inner = "Item(Column(a2),a2,Item(Column(a6),a6,Item()))";
	const std::string values = "Item(n0,a2,Item(n1,a6,Item()))";
	BOOL ok = true;
	for (ULONG trial = 0; trial < 4; ++trial)
	{
		const std::string outer = 0 == trial ? "Item(Column(a2),a2,Item())" :
			"Item(Column(a6),a6,Item(Column(a2),a2,Item()))";
		const std::string text = "Proj<a0 s0 " + outer + ">(Proj<a1 s1 " + inner +
			">(Proj<a3 s2 " + values + ">(Input<t0>)))|Proj<a4 s3 " + outer +
			">(Proj<a5 s4 " + values + ">(Input<t1>))|"
			"t1 := t0;a4 := a0;s3 := s0;a5 := a3;s4 := s2";
		CDSLRule *rule = PdslruleParseLocal(mp, text.c_str());
		if (nullptr == rule) return GPOS_FAILED;
		CColRefArray *inputs = nullptr;
		CExpression *input = fix.PexprLogicalGet("column_fusion", 2, &inputs);
		CColRefArray *columns = GPOS_NEW(mp) CColRefArray(mp);
		CExpressionArray *definitions = GPOS_NEW(mp) CExpressionArray(mp);
		for (ULONG i = 0; i < 2; ++i)
		{
			CColRef *output = fix.PcrCreateInt4("defined");
			columns->Append(output);
			definitions->Append(GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CScalarProjectElement(mp, output),
				CUtils::PexprScalarIdent(mp, (*inputs)[i])));
		}
		input->AddRef();
		CExpression *bottom = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp), input,
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp), definitions));
		CExpression *middle = fix.PexprLogicalProject(bottom, columns);
		// A renamed or computed middle item is not a pure identity selection.
		if (2 <= trial)
		{
			middle->Release();
			CColRef *output = 2 == trial ? fix.PcrCreateInt4("renamed") : (*columns)[0];
			CExpressionArray *items = GPOS_NEW(mp) CExpressionArray(mp);
			items->Append(GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectElement(mp, output),
				2 == trial ? CUtils::PexprScalarIdent(mp, (*columns)[0]) : CUtils::PexprScalarConstInt4(mp, 7)));
			items->Append(GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectElement(mp, (*columns)[1]),
				CUtils::PexprScalarIdent(mp, (*columns)[1])));
			bottom->AddRef();
			middle = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp), bottom,
				GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp), items));
		}
		CColRefArray *selected = GPOS_NEW(mp) CColRefArray(mp);
		if (0 != trial) selected->Append((*columns)[1]);
		selected->Append((*columns)[0]);
		CExpression *source = fix.PexprLogicalProject(middle, selected);
		CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, source);
		ok &= (EdsldecisionReady == decision->Status()) == (trial < 2);
		if (trial < 2)
		{
			CExpression *target = decision->PexprTarget();
			ok &= nullptr != target && COperator::EopLogicalProject == target->Pop()->Eopid() &&
				(*target)[0]->Matches(bottom) && 0 == (*target)[1]->Arity() &&
				target->DeriveOutputColumns()->Equals(source->DeriveOutputColumns());
		}
		GPOS_DELETE(decision);
		source->Release(); selected->Release(); middle->Release(); bottom->Release(); input->Release();
		columns->Release();
		rule->Release();
	}
	return ok ? GPOS_OK : GPOS_FAILED;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiateTest::EresUnittest
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLInstantiateTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(EresColumnValues),
		GPOS_UNITTEST_FUNC(EresTypedQuantifiedDistinct),
		GPOS_UNITTEST_FUNC(EresColumnAliases),
		GPOS_UNITTEST_FUNC(EresColumnProjectionFusion),
		GPOS_UNITTEST_FUNC(EresComputeColumnDerivations),
		GPOS_UNITTEST_FUNC(EresComputeAliasFusion),
		GPOS_UNITTEST_FUNC(EresMemoSourceOutputContract),
		GPOS_UNITTEST_FUNC(EresSystemColumnOutputContract),
		GPOS_UNITTEST_FUNC(EresIntroducedOuterReference),
		GPOS_UNITTEST_FUNC(EresComputedResultOutputContract),
		GPOS_UNITTEST_FUNC(CDSLInstantiateTest::EresUnittest_CorrelatedFilterBindings),
		GPOS_UNITTEST_FUNC(CDSLInstantiateTest::EresUnittest_LegacyBindingBoundary),
		GPOS_UNITTEST_FUNC(CDSLInstantiateTest::EresUnittest_ExistsExpressionBindings),
		GPOS_UNITTEST_FUNC(CDSLInstantiateTest::EresUnittest_DistinctProjectionBindings),
		GPOS_UNITTEST_FUNC(CDSLInstantiateTest::EresUnittest_DistinctProjectionPrefix),
		GPOS_UNITTEST_FUNC(CDSLInstantiateTest::EresUnittest_CallValues),
		GPOS_UNITTEST_FUNC(CDSLInstantiateTest::EresUnittest_SelectItems),
		GPOS_UNITTEST_FUNC(CDSLInstantiateTest::EresUnittest_CaseValues),
		GPOS_UNITTEST_FUNC(CDSLInstantiateTest::EresUnittest_ValueBool),
		GPOS_UNITTEST_FUNC(
			CDSLInstantiateTest::EresUnittest_ProjectExpressionBindings),
		GPOS_UNITTEST_FUNC(
			CDSLInstantiateTest::EresUnittest_JoinExpressionBindings),
		GPOS_UNITTEST_FUNC(CDSLInstantiateTest::EresUnittest_ApplyExpressionBindings),
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
		GPOS_UNITTEST_FUNC(CDSLInstantiateTest::EresUnittest_NotTrueBindings),
		GPOS_UNITTEST_FUNC(CDSLInstantiateTest::EresUnittest_NullSafeEqBindings),
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
CDSLInstantiateTest::EresUnittest_DistinctProjectionBindings()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	BOOL ok = true;
	CDSLRule *rule = PdslruleParseLocal(mp,
		"Proj*<a0 s0 Item(Case(p0,n0,n1),a2,e0)>(Input<t0>)|"
		"Proj*<a1 s1 Item(Case(Not(Not(p0)),n0,n1),a2,e0)>(Input<t1>)|"
		"t1 := t0;a1 := a0;s1 := s0");
	CDSLRule *filter = PdslruleParseLocal(mp,
		"Proj*<a0 s0>(Filter<p0 a2>(Input<t0>))|"
		"Proj*<a1 s1>(Filter<Not(Not(p0)) a3>(Input<t1>))|"
		"t1 := t0;a1 := a0;s1 := s0;a3 := a2");
	CDSLRule *columnsRule = PdslruleParseLocal(mp,
		"Proj*<a0 s0>(Input<t0>)|Proj*<a1 s1>(Input<t1>)|"
		"t1 := t0;a1 := a0;s1 := s0");
	if (nullptr == rule || nullptr == filter || nullptr == columnsRule)
	{
		CRefCount::SafeRelease(rule); CRefCount::SafeRelease(filter);
		CRefCount::SafeRelease(columnsRule);
		return GPOS_FAILED;
	}
	CColRefArray *columns = nullptr;
	CExpression *input = fix.PexprLogicalGet("distinct_binding", 2, &columns);
	CExpression *predicate = fix.PexprPredAtom((*columns)[0]);
	CExpression *select = fix.PexprLogicalSelect(input, predicate);
	CColRefArray *keys = GPOS_NEW(mp) CColRefArray(mp);
	keys->Append((*columns)[0]);
	CExpression *dedup = fix.PexprLogicalGbAgg(select, keys);
	CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, filter, dedup);
	CExpression *target = decision->PexprTarget();
	ok &= EdsldecisionReady == decision->Status() && nullptr != target;
	if (nullptr != target)
		ok &= COperator::EopLogicalGbAgg == target->Pop()->Eopid() &&
			CColRef::Equals(CLogicalGbAgg::PopConvert(target->Pop())->Pdrgpcr(), keys) &&
			dedup->DeriveOutputColumns()->Equals(target->DeriveOutputColumns()) &&
			COperator::EopLogicalSelect == (*target)[0]->Pop()->Eopid();
	GPOS_DELETE(decision); dedup->Release(); keys->Release();

	CColRef *output = fix.PcrCreateInt4("computed");
	for (ULONG shape = 0; shape < 8; ++shape)
	{
		IMDId *type = fix.Pmda()->PtMDType<IMDTypeInt4>()->MDId();
		type->AddRef(); predicate->AddRef();
		CExpression *value = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CScalarIf(mp, type), predicate,
			CUtils::PexprScalarConstInt4(mp, 7), CUtils::PexprScalarConstInt4(mp, 9));
		CExpressionArray *items = GPOS_NEW(mp) CExpressionArray(mp);
		items->Append(GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CScalarProjectElement(mp, output), value));
		CColRef *extra = fix.PcrCreateInt4("extra");
		if (4 == shape || 7 == shape)
			items->Append(GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CScalarProjectElement(mp, extra),
				CUtils::PexprScalarConstInt4(mp, 42)));
		input->AddRef();
		CExpression *project = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CLogicalProject(mp), input,
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp), items));
		keys = GPOS_NEW(mp) CColRefArray(mp);
		if (3 != shape)
		{
			// Group-key order must not silently reorder computed expressions.
			if (7 == shape)
				keys->Append(extra);
			keys->Append(output);
			keys->Append((*columns)[1]); // passthrough, not computed
		}
		dedup = fix.PexprLogicalGbAgg(project, keys,
			1 == shape ? fix.PcrCreateInt4("aggregate") : nullptr,
			(*columns)[0]);
		if (2 == shape)
		{
			keys->AddRef(); project->AddRef(); (*dedup)[1]->AddRef();
			CExpression *local = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CLogicalGbAgg(mp, keys, COperator::EgbaggtypeLocal), project, (*dedup)[1]);
			dedup->Release(); dedup = local;
		}
		if (5 == shape)
		{
			dedup->Release(); project->AddRef(); dedup = project;
		}
		if (6 == shape)
		{
			// A hidden Limit remains part of the arbitrary input, not peeled away.
			input->AddRef();
			CExpression *limit = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CLogicalLimit(mp, GPOS_NEW(mp) COrderSpec(mp), true, true, false),
				input, CUtils::PexprScalarConstInt8(mp, 0), CUtils::PexprScalarConstInt8(mp, 1));
			(*project)[1]->AddRef();
			CExpression *limited_project = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CLogicalProject(mp), limit, (*project)[1]);
			dedup->Release(); dedup = fix.PexprLogicalGbAgg(limited_project, keys);
			limited_project->Release();
		}
		if (0 == shape || 4 == shape || 6 == shape || 7 == shape)
		{
			// Two slots capture grouping columns, never absorb computed items
			// (including unselected/reordered items) out of the opaque child.
			CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
			CDSLMatcher matcher(mp, columnsRule);
			const CDSLOp *root = columnsRule->PfragSrc()->PopRoot();
			const BOOL matched = matcher.FMatch(root, dedup, model);
			ok &= matched;
			if (matched)
			{
				ok &= model->PexprTable((*(*root)[0]->Pdrgpsym())[0]) == (*dedup)[0];
				CExpression *captured = model->PexprProjList((*root->Pdrgpsym())[1]);
				ok &= nullptr != captured && captured->Arity() == keys->Size();
				if (nullptr != captured)
					for (ULONG i = 0; i < captured->Arity(); ++i)
						ok &= COperator::EopScalarIdent == (*(*captured)[i])[0]->Pop()->Eopid() &&
							CScalarIdent::PopConvert((*(*captured)[i])[0]->Pop())->Pcr() ==
								CScalarProjectElement::PopConvert((*captured)[i]->Pop())->Pcr();
			}
			model->Release();
		}
		decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, dedup);
		target = decision->PexprTarget();
		if (0 == shape || 6 == shape)
		{
			ok &= EdsldecisionReady == decision->Status() && nullptr != target;
			if (nullptr != target)
			{
				ok &= COperator::EopLogicalGbAgg == target->Pop()->Eopid() &&
					CColRef::Equals(CLogicalGbAgg::PopConvert(target->Pop())->Pdrgpcr(), keys) &&
					dedup->DeriveOutputColumns()->Equals(target->DeriveOutputColumns()) &&
					1 == (*(*target)[0])[1]->Arity();
				CExpression *rewritten = (*(*(*(*target)[0])[1])[0])[0];
				ok &= COperator::EopScalarIf == rewritten->Pop()->Eopid() &&
					(*(*(*rewritten)[0])[0])[0]->Matches(predicate);
				if (6 == shape)
					ok &= COperator::EopLogicalLimit == (*(*target)[0])[0]->Pop()->Eopid();
			}
		}
		else
			ok &= EdsldecisionReady != decision->Status();
		GPOS_DELETE(decision); dedup->Release(); project->Release(); keys->Release();
	}
	predicate->Release(); select->Release(); input->Release();
	rule->Release(); filter->Release(); columnsRule->Release();
	return ok ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLInstantiateTest::EresUnittest_DistinctProjectionPrefix()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	BOOL ok = true;
	for (BOOL nested : {false, true})
	{
		const std::string source_pattern =
			"Proj*<a0 s0 Item(Case(p0,n0,n1),a2,e0)>(Filter<p2 a3>(Input<t0>))";
		const std::string target_pattern =
			"Proj*<a1 s1 Item(Case(Not(Not(p0)),n0,n1),a2,e0)>(Filter<p4 a5>(Input<t1>))";
		CDSLRule *rule = PdslruleParseLocal(mp, ((nested ? "Filter<p3 a4>(" + source_pattern + ")" : source_pattern) + "|" +
			(nested ? "Filter<p5 a6>(" + target_pattern + ")" : target_pattern) +
			"|t1 := t0;a1 := a0;s1 := s0;p4 := p2;a5 := a3" +
			(nested ? ";p5 := p3;a6 := a4" : "")).c_str());
		if (nullptr == rule) return GPOS_FAILED;
		CColRefArray *columns = GPOS_NEW(mp) CColRefArray(mp);
		columns->Append(fix.PcrCreateInt4("predicate_input"));
		columns->Append(fix.PcrCreateInt4("other"));
		CExpression *input = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CLogicalConstTableGet(mp, columns, GPOS_NEW(mp) IDatum2dArray(mp)));
		CExpression *predicate = fix.PexprPredAtom((*columns)[0]);
		CExpression *select = fix.PexprLogicalSelect(input, predicate);
		CColRef *output = fix.PcrCreateInt4("computed");
		IMDId *type = fix.Pmda()->PtMDType<IMDTypeInt4>()->MDId();
		type->AddRef(); predicate->AddRef(); select->AddRef();
		CExpression *project = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CLogicalProject(mp), select,
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
				GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectElement(mp, output),
					GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarIf(mp, type), predicate,
						CUtils::PexprScalarConstInt4(mp, 7), CUtils::PexprScalarConstInt4(mp, 9)))));
		CColRefArray *keys = GPOS_NEW(mp) CColRefArray(mp);
		keys->Append(output);
		CExpression *source = fix.PexprLogicalGbAgg(project, keys);
		if (nested)
		{
			CExpression *outer_predicate = fix.PexprPredAtom(output);
			CExpression *outer = fix.PexprLogicalSelect(source, outer_predicate);
			outer_predicate->Release(); source->Release(); source = outer;
		}
		CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, source);
		const BOOL direct = EdsldecisionReady == decision->Status();
		GPOS_DELETE(decision);
		CDSLRulePrefixIndex index(mp);
		index.Insert(rule, 0, source->Pop()->Eopid());
		CDSLRuleArray *candidates = index.PdrgpruleCandidates(mp, source);
		const BOOL indexed = 1 == candidates->Size();
		candidates->Release();
		// The adapter exposes one Project; it does not erase a required Filter.
		input->AddRef(); (*project)[1]->AddRef();
		CExpression *bare_project = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CLogicalProject(mp), input, (*project)[1]);
		CExpression *missing = fix.PexprLogicalGbAgg(bare_project, keys);
		bare_project->Release();
		if (nested)
		{
			CExpression *outer = fix.PexprLogicalSelect(missing, (*source)[1]);
			missing->Release(); missing = outer;
		}
		candidates = index.PdrgpruleCandidates(mp, missing);
		ok &= 0 == candidates->Size();
		candidates->Release(); missing->Release();
		CMemo memo(mp);
		const auto insert = [&](const auto &self, CExpression *expr) -> CGroupExpression * {
			CGroupArray *children = GPOS_NEW(mp) CGroupArray(mp);
			for (ULONG i = 0; i < expr->Arity(); ++i)
				children->Append(self(self, (*expr)[i])->Pgroup());
			expr->Pop()->AddRef();
			CGroupExpression *entry = GPOS_NEW(mp) CGroupExpression(mp,
				expr->Pop(), children, CXform::ExfInvalid, nullptr, false);
			CGroupExpression *canonical = nullptr;
			memo.PgroupInsert(nullptr, expr, entry, &canonical);
			if (canonical != entry) entry->Release();
			return canonical;
		};
		CGroupExpression *root = insert(insert, source);
		// A second equivalent Filter must remain visible below the absorbed
		// Project; retaining only a representative would miss rewrite chains.
		predicate->AddRef();
		CExpression *twice = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopNot),
			GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopNot), predicate));
		CExpression *alternative = fix.PexprLogicalSelect(input, twice);
		CGroupArray *children = GPOS_NEW(mp) CGroupArray(mp);
		children->Append(insert(insert, input)->Pgroup());
		children->Append(insert(insert, twice)->Pgroup());
		alternative->Pop()->AddRef();
		CGroupExpression *entry = GPOS_NEW(mp) CGroupExpression(mp,
			alternative->Pop(), children, CXform::ExfInvalid, nullptr, false);
		CGroupExpression *canonical = nullptr;
		memo.PgroupInsert(insert(insert, select)->Pgroup(), alternative, entry, &canonical);
		if (entry != canonical) entry->Release();
		alternative->Release(); twice->Release();
		CExpressionArray *bindings = index.PdrgpexprBindings(mp, root);
		ULONG ready = 0;
		for (ULONG i = 0; i < bindings->Size(); ++i)
		{
			decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, (*bindings)[i]);
			ready += EdsldecisionReady == decision->Status();
			GPOS_DELETE(decision);
		}
		if (!direct || !indexed || 2 != ready)
			GPOS_TRACE_FORMAT("distinct prefix nested=%d direct=%d indexed=%d ready=%lu bindings=%lu",
				nested, direct, indexed, ready, bindings->Size());
		ok &= direct && indexed && 2 == ready;
		bindings->Release();
		source->Release(); keys->Release(); project->Release(); select->Release(); predicate->Release(); input->Release(); rule->Release();
	}
	return ok ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLInstantiateTest::EresUnittest_CallValues()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	const std::string lhs = "Filter<ValueBool(Call(h0,Args(Case(p0,n0,n1),v0))) a0>(Input<t0>)|";
	const std::string aliases = "|t1 := t0;a1 := a0;h1 := h2;h2 := h0;v1 := v0";
	CDSLRule *rule = PdslruleParseLocal(mp, (lhs +
		"Filter<ValueBool(Call(h1,Args(Case(Not(Not(p0)),n0,n1),v1))) a1>(Input<t1>)" + aliases).c_str());
	if (nullptr == rule) return GPOS_FAILED;
	BOOL ok = true;
	for (ULONG kind = 0; kind < 3; ++kind)
	{
		CColRefArray *columns = nullptr;
		CExpression *input = fix.PexprLogicalGet("call_values", 1, &columns);
		CExpression *eq = fix.PexprEqConst((*columns)[0], 7);
		COperator *op = eq->Pop();
		if (0 == kind) op->AddRef();
		else if (1 == kind)
		{
			IMDId *mdid = CScalarCmp::PopConvert(op)->MdIdOp();
			mdid->AddRef();
			op = GPOS_NEW(mp) CScalarOp(mp, mdid, nullptr,
				GPOS_NEW(mp) CWStringConst(GPOS_WSZ_LIT("=")));
		}
		else
		{
			IMDId *boolean_type = fix.Pmda()->PtMDType<IMDTypeBool>()->MDId();
			boolean_type->AddRef();
			op = GPOS_NEW(mp) CScalarFunc(mp,
				GPOS_NEW(mp) CMDIdGPDB(IMDId::EmdidGeneral, 65 /*int4eq*/),
				boolean_type,
				default_type_modifier, GPOS_NEW(mp) CWStringConst(GPOS_WSZ_LIT("int4eq")), 0, false);
		}
		eq->Release();
		IMDId *type = fix.Pmda()->PtMDType<IMDTypeInt4>()->MDId();
		type->AddRef();
		CExpression *value = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarIf(mp, type),
			CUtils::PexprScalarConstBool(mp, true),
			CUtils::PexprScalarConstInt4(mp, 7), CUtils::PexprScalarConstInt4(mp, 9));
		CExpression *call = GPOS_NEW(mp) CExpression(mp, op, value, CUtils::PexprScalarConstInt4(mp, 7));
		if (2 == kind)
		{
			CDSLRule *repeated = PdslruleParseLocal(mp,
				"Filter<And(ValueBool(n0),ValueBool(n0)) a0>(Input<t0>)|"
				"Filter<ValueBool(n0) a1>(Input<t1>)|t1 := t0;a1 := a0");
			if (nullptr == repeated) return GPOS_FAILED;
			// Native operator Matches ignores these fields; a captured Call head
			// must not identify different signatures or invocation metadata.
			for (ULONG variant = 0; variant < 4; ++variant)
			{
				IMDId *result_type = fix.Pmda()->PtMDType<IMDTypeBool>()->MDId();
				result_type->AddRef();
				COperator *other_op = GPOS_NEW(mp) CScalarFunc(mp,
					GPOS_NEW(mp) CMDIdGPDB(IMDId::EmdidGeneral, 65), result_type,
					1 == variant ? 42 : default_type_modifier,
					GPOS_NEW(mp) CWStringConst(GPOS_WSZ_LIT("int4eq")),
					2 == variant ? 1 : 0, 3 == variant);
				(*call)[0]->AddRef();
				(*call)[1]->AddRef();
				CExpression *other = GPOS_NEW(mp) CExpression(mp, other_op, (*call)[0], (*call)[1]);
				ok &= call->Matches(other) &&
					CDSLMatchView::FSameCallHead(call, other) == (0 == variant);
				call->AddRef();
				CExpression *both = GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopAnd), call, other);
				CExpression *selected = fix.PexprLogicalSelect(input, both);
				CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
				ok &= CDSLMatcher(mp, repeated).FMatch(repeated->PfragSrc()->PopRoot(), selected, model)
					== (0 == variant);
				model->Release();
				// Explicit Eq must enforce the same capture identity as repetition.
				for (const CHAR *text : {
					"Filter<And(ValueBool(n0),ValueBool(n1)) a0>(Input<t0>)|"
					"Filter<ValueBool(n0) a1>(Input<t1>)|t1 := t0;a1 := a0;Eq(n0,n1)",
					"Filter<And(p2,p3) a0>(Input<t0>)|"
					"Filter<p1 a1>(Input<t1>)|t1 := t0;a1 := a0;p1 := p2;Eq(p2,p3)"})
				{
					CDSLRule *equality = PdslruleParseLocal(mp, text);
					if (nullptr == equality) return GPOS_FAILED;
					CDSLRewriteDecision *checked = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, equality, selected);
					const BOOL admitted = EdsldecisionReady == checked->Status();
					if (admitted != (0 == variant))
						GPOS_TRACE_FORMAT("capture Eq variant=%lu status=%d", variant, checked->Status());
					ok &= 0 == variant ? admitted : EdsldecisionConstraintRejected == checked->Status();
					GPOS_DELETE(checked); equality->Release();
				}
				// Legacy extraction and carrier reuse must enforce that same
				// identity, including calls nested in a captured query body.
				CDSLRule *exists_rule = PdslruleParseLocal(mp,
					"Filter<p0 a0>(Input<t0>)|Exists(Input<t1>,Input<t2>)|TableEq(t1,t0);PredicateExists(p0,t2)");
				if (nullptr == exists_rule) return GPOS_FAILED;
				CDSLModel *captures = GPOS_NEW(mp) CDSLModel(mp);
				const auto *existence = (*exists_rule->Pdrgpcon())[1]->Pdrgpsym();
				CExpression *exists = GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CScalarSubqueryExists(mp), fix.PexprLogicalSelect(input, call));
				CExpression *other_query = fix.PexprLogicalSelect(input, other);
				ok &= captures->FBind((*existence)[0], exists);
				ok &= captures->FBind((*existence)[1], other_query);
				ok &= CDSLConstraintChecker(mp).FCheck(exists_rule, captures) == (0 == variant);
				const auto *attrs = (*exists_rule->PfragSrc()->PopRoot()->Pdrgpsym())[1];
				ok &= captures->FSetJoinPred(attrs, attrs, call);
				ok &= captures->FSetJoinPred(attrs, attrs, other) == (0 == variant);
				ok &= captures->PexprJoinPred(attrs, attrs) == call;
				exists->AddRef();
				ok &= captures->FSetFilterCarrier((*existence)[0], exists);
				other_query->AddRef();
				CExpression *other_exists = GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CScalarSubqueryExists(mp), other_query);
				ok &= captures->FSetFilterCarrier((*existence)[0], other_exists) == (0 == variant);
				captures->Release();
				exists->Release(); other_query->Release(); exists_rule->Release();
				CDSLRule *functions = PdslruleParseLocal(mp,
					"Agg<a0 a1 f0 s0 p0>(Agg<a2 a3 f1 s1 p1>(Input<t0>))|"
					"Input<t1>|TableEq(t1,t0);FuncEq(f0,f1)");
				if (nullptr == functions) return GPOS_FAILED;
				CDSLModel *function_model = GPOS_NEW(mp) CDSLModel(mp);
				const auto *function_symbols = (*functions->Pdrgpcon())[1]->Pdrgpsym();
				for (ULONG side = 0; side < 2; ++side)
				{
					CExpressionArray *values = GPOS_NEW(mp) CExpressionArray(mp);
					CExpression *value = side == 0 ? call : other;
					value->AddRef(); values->Append(value);
					ok &= function_model->FBind((*function_symbols)[side], values);
					values->Release();
				}
				ok &= CDSLConstraintChecker(mp).FCheck(functions, function_model) == (0 == variant);
				function_model->Release(); functions->Release();
				// Check the same metadata recursively inside SELECT lists. This
				// isolates Eq premises; it does not certify a projection rewrite.
				CDSLRule *lists = PdslruleParseLocal(mp,
					"Proj<a0 s0 e0>(Proj<a1 s1 e1>(Input<t0>))|"
					"Input<t1>|t1 := t0;Eq(e0,e1)");
				if (nullptr == lists) return GPOS_FAILED;
				CDSLModel *list_model = GPOS_NEW(mp) CDSLModel(mp);
				CColRef *output = COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
					fix.Pmda()->PtMDType<IMDTypeBool>(), default_type_modifier);
				const auto *projection = lists->PfragSrc()->PopRoot();
				for (ULONG side = 0; side < 2; ++side)
				{
					CExpression *value = 0 == side ? call : other;
					value->AddRef();
					CExpression *list = GPOS_NEW(mp) CExpression(mp,
						GPOS_NEW(mp) CScalarProjectList(mp), GPOS_NEW(mp) CExpression(mp,
							GPOS_NEW(mp) CScalarProjectElement(mp, output), value));
					ok &= list_model->FBind((*projection->Pdrgpsym())[2], list);
					list->Release();
					if (0 == side) projection = (*projection)[0];
				}
				ok &= CDSLConstraintChecker(mp).FCheck(lists, list_model) == (0 == variant);
				list_model->Release();
				lists->Release();
				selected->Release();
				both->Release();
			}
			repeated->Release();
		}
		CExpression *source = fix.PexprLogicalSelect(input, call);
		CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, source);
		CExpression *target = decision->PexprTarget();
		ok &= EdsldecisionReady == decision->Status() && nullptr != target;
		if (nullptr != target)
		{
			CExpression *rewritten = (*target)[1];
			ok &= rewritten->Pop() == call->Pop() && rewritten->Arity() == call->Arity() &&
				CDSLMatchView::FSameCallHead(call, rewritten) &&
				(*rewritten)[1]->Matches((*call)[1]) &&
				(*(*(*(*rewritten)[0])[0])[0])[0]->Matches((*value)[0]);
		}
		GPOS_DELETE(decision);
		std::string exported, error;
		ok &= CDSLPlanTemplate::FSlice(mp, source, "r", {"r/0"}, &exported, &error) &&
			exported == "Filter<ValueBool(Call(h0,Args(Case(p1,n0,n1),Args(n2,Args())))) a0>(Input<t0>)";
		for (const CHAR *bad_args : {"Args()", "Args(BoolValue(p0),v1)", "Args(n0,Args())"})
		{
			CDSLRule *bad = PdslruleParseLocal(mp, (lhs + "Filter<ValueBool(Call(h1," + bad_args +
				")) a1>(Input<t1>)" + aliases).c_str());
			if (nullptr == bad) { ok = false; continue; }
			decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, bad, source);
			ok &= EdsldecisionReady != decision->Status();
			GPOS_DELETE(decision);
			bad->Release();
		}
		CExpression *srf = fix.PexprGenerateSeries((*columns)[0]);
		ok &= !CDSLMatchView::FScalarCall(srf);
		srf->Release();
		source->Release();
		call->Release();
		input->Release();
	}
	rule->Release();
	rule = PdslruleParseLocal(mp,
		"Proj<a0 s0 Item(Call(h0,Args()),a2,e0)>(Input<t0>)|"
		"Proj<a1 s1 Item(Call(h1,Args()),a2,e0)>(Input<t1>)|t1 := t0;a1 := a0;s1 := s0;h1 := h0");
	if (nullptr == rule) return GPOS_FAILED;
	for (ULONG stability = 0; stability < IMDFunction::EfsSentinel; ++stability)
	{
		IMDId *type = fix.Pmda()->PtMDType<IMDTypeInt4>()->MDId();
		type->AddRef();
		CExpression *call = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarFunc(mp,
			GPOS_NEW(mp) CMDIdGPDB(IMDId::EmdidGeneral, 100300 + stability), type,
			default_type_modifier, GPOS_NEW(mp) CWStringConst(GPOS_WSZ_LIT("nullary")), 0, false));
		CExpression *source = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp),
			fix.PexprLogicalGet("nullary_input", 1),
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
				GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectElement(mp, fix.PcrCreateInt4("value")), call)));
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		const BOOL matched = CDSLMatcher(mp, rule).FMatch(rule->PfragSrc()->PopRoot(), source, model);
		ok &= matched == (IMDFunction::EfsImmutable == stability);
		if (matched)
		{
			CDSLInstantiator instantiator(mp);
			CExpression *target = instantiator.PexprInstantiate(rule, model);
			ok &= nullptr != target && target->Matches(source);
			CRefCount::SafeRelease(target);
		}
		model->Release();
		source->Release();
	}
	rule->Release();
	return ok ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLInstantiateTest::EresUnittest_ValueBool()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *rule = PdslruleParseLocal(mp,
		"Filter<ValueBool(Case(p0,n0,n1)) a0>(Input<t0>)|"
		"Filter<ValueBool(Case(Not(Not(p0)),n0,n1)) a1>(Input<t1>)|t1 := t0;a1 := a0");
	if (nullptr == rule)
		return GPOS_FAILED;
	BOOL ok = true;
	for (ULONG truth = 0; truth < 3; ++truth)
	{
		IMDId *type = fix.Pmda()->PtMDType<IMDTypeBool>()->MDId();
		type->AddRef();
		CExpression *predicate = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarIf(mp, type),
			CUtils::PexprScalarConstBool(mp, 1 == truth, 2 == truth),
			CUtils::PexprScalarConstBool(mp, true), CUtils::PexprScalarConstBool(mp, false, true));
		CExpression *input = fix.PexprLogicalGet("value_bool", 1);
		CExpression *source = fix.PexprLogicalSelect(input, predicate);
		input->Release();
		std::string exported, error;
		ok &= CDSLPlanTemplate::FSlice(mp, source, "r", {"r/0"}, &exported, &error) &&
			exported == "Filter<ValueBool(Case(p1,BoolValue(p2),BoolValue(p3))) a0>(Input<t0>)";
		CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, source);
		CExpression *target = decision->PexprTarget();
		ok &= EdsldecisionReady == decision->Status() && nullptr != target;
		if (nullptr != target)
		{
			CExpression *rewritten = (*target)[1];
			ok &= COperator::EopScalarIf == rewritten->Pop()->Eopid() && 3 == rewritten->Arity() &&
				(*rewritten)[1]->Matches((*predicate)[1]) && (*rewritten)[2]->Matches((*predicate)[2]) &&
				(*(*(*rewritten)[0])[0])[0]->Matches((*predicate)[0]);
		}
		GPOS_DELETE(decision);
		// The source adapter cannot capture a numeric value as a predicate.
		CExpression *number = CUtils::PexprScalarConstInt4(mp, 7);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		ok &= !CDSLMatcher(mp, rule).FMatchPredicate((*rule->PfragSrc()->PopRoot()->Pdrgpsym())[0], number, model);
		model->Release();
		number->Release();
		source->Release();
		predicate->Release();
	}
	rule->Release();
	// An arbitrary SCALAR captured from SELECT must also be type-checked when
	// a target tries to use it as a predicate. Preserve the projection schema
	// here, so rejection cannot be attributed to a dropped output column.
	rule = PdslruleParseLocal(mp,
		"Proj<a0 s0 Item(n0,a2,e0)>(Input<t0>)|"
		"Proj<a1 s1 Item(n0,a2,e0)>(Filter<ValueBool(n0) a3>(Input<t1>))|"
		"t1 := t0;a1 := a0;s1 := s0;a3 := a0");
	if (nullptr == rule)
		return GPOS_FAILED;
	CExpression *numeric = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp),
		fix.PexprLogicalGet("numeric_value", 1),
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectElement(mp, fix.PcrCreateInt4("number")),
				CUtils::PexprScalarConstInt4(mp, 7))));
	CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, numeric);
	ok &= EdsldecisionInstantiateRejected == decision->Status();
	GPOS_DELETE(decision);
	numeric->Release();
	rule->Release();
	return ok ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLInstantiateTest::EresUnittest_CaseValues()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	const std::string source_text = "Proj<a0 s0 Item(Case(p0,n0,n1),a2,e2)>(Input<t0>)|";
	const std::string aliases = "|t1 := t0;a1 := a0;s1 := s0";
	CDSLRule *rule = PdslruleParseLocal(mp, (source_text +
		"Proj<a1 s1 Item(Case(Not(Not(p0)),n0,n1),a2,e2)>(Input<t1>)" + aliases).c_str());
	if (nullptr == rule)
		return GPOS_FAILED;
	BOOL ok = true;
	for (ULONG truth = 0; truth < 3; ++truth)
	{
		CExpression *input = fix.PexprLogicalGet("case_values", 1);
		IMDId *type = fix.Pmda()->PtMDType<IMDTypeInt4>()->MDId();
		type->AddRef();
		CExpression *value = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarIf(mp, type),
			CUtils::PexprScalarConstBool(mp, 1 == truth, 2 == truth),
			CUtils::PexprScalarConstInt4(mp, 7), CUtils::PexprScalarConstInt4(mp, 9));
		CExpression *source = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp), input,
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
				GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectElement(mp, fix.PcrCreateInt4("result")), value)));
		std::string exported, error;
		ok &= CDSLPlanTemplate::FSlice(mp, source, "r", {"r/0"}, &exported, &error) &&
			exported == "Compute<Item(Case(p0,n0,n1),a1,e0) a0 s0>(Input<t0>)";
		CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, source);
		CExpression *target = decision->PexprTarget();
		ok &= EdsldecisionReady == decision->Status() && nullptr != target;
		if (nullptr != target)
		{
			CExpression *result = (*(*(*target)[1])[0])[0];
			ok &= COperator::EopScalarIf == result->Pop()->Eopid() && 3 == result->Arity() &&
				(*result)[1]->Matches((*value)[1]) && (*result)[2]->Matches((*value)[2]) &&
				(*(*(*result)[0])[0])[0]->Matches((*value)[0]) &&
				target->DeriveOutputColumns()->Equals(source->DeriveOutputColumns());
		}
		GPOS_DELETE(decision);
		CDSLRule *bad = PdslruleParseLocal(mp, (source_text +
			"Proj<a1 s1 Item(Case(p0,n0,BoolValue(p0)),a2,e2)>(Input<t1>)" + aliases).c_str());
		if (nullptr == bad)
			ok = false;
		else
		{
			decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, bad, source);
			ok &= EdsldecisionReady != decision->Status();
			GPOS_DELETE(decision);
			bad->Release();
		}
		source->Release();
	}
	rule->Release();
	return ok ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLInstantiateTest::EresUnittest_SelectItems()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	const std::string source_rules[] = {
		"Proj<a0 s0 Item(BoolValue(Not(Not(p0))),a2,Item(n0,a3,e2))>(Input<t0>)|",
		"Compute<Item(BoolValue(Not(Not(p0))),a2,Item(n0,a3,e2)) a0 s0>(Input<t0>)|"};
	const std::string aliases = "|t1 := t0;a1 := a0;s1 := s0;n1 := n2;n2 := n0";
	const std::string target_rules[] = {
		"Proj<a1 s1 Item(BoolValue(p0),a2,Item(n1,a3,e2))>(Input<t1>)",
		"Compute<Item(BoolValue(p0),a2,Item(n1,a3,e2)) a1 s1>(Input<t1>)"};
	BOOL ok = true;
	for (ULONG tail_size : {0UL, 1UL, 4UL})
	{
		for (ULONG truth = 0; truth < 3; ++truth)
		{
			CColRefArray *columns = GPOS_NEW(mp) CColRefArray(mp);
			columns->Append(fix.PcrCreateInt4("input"));
			CExpression *input = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CLogicalConstTableGet(mp, columns, GPOS_NEW(mp) IDatum2dArray(mp)));
			CWStringConst name(GPOS_WSZ_LIT("boolean_output"));
			CColRef *boolean_output = COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
				fix.Pmda()->PtMDType<IMDTypeBool>(), default_type_modifier, CName(&name));
			CExpression *predicate = CUtils::PexprScalarConstBool(mp, 1 == truth, 2 == truth);
			predicate->AddRef();
			CExpression *twice = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopNot),
				GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopNot), predicate));
			CExpressionArray *items = GPOS_NEW(mp) CExpressionArray(mp);
			items->Append(GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CScalarProjectElement(mp, boolean_output), twice));
			for (ULONG i = 0; i <= tail_size; ++i)
			{
				// SCALAR references preserve arbitrary values, not only numbers.
				const BOOL boolean = 0 == i && 0 == truth;
				CColRef *output = boolean ? COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
					fix.Pmda()->PtMDType<IMDTypeBool>(), default_type_modifier, CName(&name))
					: fix.PcrCreateInt4("output");
				items->Append(GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CScalarProjectElement(mp, output),
					boolean ? CUtils::PexprScalarConstBool(mp, true) :
					GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarIdent(mp, (*columns)[0]))));
			}
			CExpression *list = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp), items);
			CExpression *source = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp), input, list);
			std::string exported, error;
			ok &= CDSLPlanTemplate::FSlice(mp, source, "r", {"r/0"}, &exported, &error) &&
				0 == exported.find("Compute<Item(BoolValue(Not(Not(p0))),a1,") &&
				std::string::npos != exported.find(0 == truth ? "Item(BoolValue(p1),a2," : "Item(Column(a2),a3,");
			ULONG item_count = 0;
			for (size_t pos = 0; (pos = exported.find("Item(", pos)) != std::string::npos; pos += 5)
				++item_count;
			ok &= item_count == list->Arity();
			// The exported template must reconstruct the native compute, including
			// child columns that are not mentioned by any output definition.
			std::string rebuilt = exported;
			rebuilt.replace(rebuilt.rfind(" a0 s0>"), std::string::npos,
				" a100 s100>(Input<t100>)");
			CDSLRule *roundtrip = PdslruleParseLocal(mp, (exported + "|" + rebuilt +
				"|a100 := a0;s100 := s0;t100 := t0").c_str());
			if (nullptr == roundtrip)
				ok = false;
			else
			{
				CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, roundtrip, source);
				CExpression *target = decision->PexprTarget();
				ok &= EdsldecisionDuplicate == decision->Status() && nullptr != target;
				if (nullptr != target)
					ok &= target->Matches(source) &&
						target->DeriveOutputColumns()->Equals(source->DeriveOutputColumns());
				GPOS_DELETE(decision);
				roundtrip->Release();
			}
			ok &= CDSLPlanTemplate::FSlice(mp, source, "r", {"r"}, &exported, &error) &&
				exported == "Input<t0>";
			CExpression *filtered = fix.PexprLogicalSelect(source, predicate);
			ok &= CDSLPlanTemplate::FSlice(mp, filtered, "r", {"r/0/0"}, &exported, &error) &&
				0 == exported.find("Filter<") && std::string::npos != exported.find("Compute<") &&
				std::string::npos != exported.find("BoolValue(Not(Not(");
			ok &= CDSLPlanTemplate::FSlice(mp, filtered, "r/0", {"r/0/0"}, &exported, &error) &&
				0 == exported.find("Compute<");
			ok &= CDSLPlanTemplate::FSlice(mp, filtered, "r", {"r/0"}, &exported, &error) &&
				std::string::npos == exported.find("Item(");
			filtered->Release();
			for (ULONG kind = 0; kind < 2; ++kind)
			{
				const std::string &source_rule = source_rules[kind];
				CDSLRule *rule = PdslruleParseLocal(mp, (source_rule + target_rules[kind] + aliases).c_str());
				if (nullptr == rule)
				{
					ok = false;
					continue;
				}
				CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, source);
				CExpression *target = decision->PexprTarget();
				ok &= EdsldecisionReady == decision->Status() && nullptr != target;
				if (nullptr != target)
				{
					CExpression *target_list = (*target)[1];
					ok &= target_list->Arity() == list->Arity() &&
						(*(*target_list)[0])[0]->Matches(predicate) &&
						!target_list->Matches(list) &&
						target->DeriveOutputColumns()->Equals(source->DeriveOutputColumns());
					for (ULONG i = 1; i < list->Arity(); ++i)
						ok &= (*target_list)[i]->Matches((*list)[i]);
					// Native scalar trees re-enter the matcher, not only the builder.
					CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
					ok &= !CDSLMatcher(mp, rule).FMatch(rule->PfragSrc()->PopRoot(), target, model);
					model->Release();
				}
				GPOS_DELETE(decision);
				// Closing the tail is an exact width check, not a wildcard.
				std::string closed = source_rule + target_rules[kind] + aliases;
				for (size_t pos = 0; (pos = closed.find("e2", pos)) != std::string::npos; pos += 6)
					closed.replace(pos, 2, "Item()");
				CDSLRule *closed_rule = PdslruleParseLocal(mp, closed.c_str());
				if (nullptr == closed_rule)
					ok = false;
				else
				{
					CDSLModel *closed_model = GPOS_NEW(mp) CDSLModel(mp);
					ok &= (0 == tail_size) == CDSLMatcher(mp, closed_rule).FMatch(
						closed_rule->PfragSrc()->PopRoot(), source, closed_model);
					closed_model->Release();
					decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, closed_rule, source);
					ok &= (0 == tail_size) == (EdsldecisionReady == decision->Status());
					ok &= (0 == tail_size) == (nullptr != decision->PexprTarget());
					if (0 == tail_size && nullptr != decision->PexprTarget())
					{
						CExpression *closed_list = (*decision->PexprTarget())[1];
						ok &= 2 == closed_list->Arity() &&
							(*(*closed_list)[0])[0]->Matches(predicate) &&
							(*closed_list)[1]->Matches((*list)[1]);
					}
					GPOS_DELETE(decision);
					closed_rule->Release();
				}
				if (1 == truth)
				{
					// BoolValue is not a cast from a numeric value to a predicate.
					(*list)[1]->AddRef();
					input->AddRef();
					CExpression *numeric = GPOS_NEW(mp) CExpression(mp,
						GPOS_NEW(mp) CLogicalProject(mp), input,
						GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp), (*list)[1]));
					CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
					ok &= !CDSLMatcher(mp, rule).FMatch(rule->PfragSrc()->PopRoot(), numeric, model);
					model->Release();
					numeric->Release();
				}
				for (const CHAR *bad_target : {
					// Type mismatch, swapped output positions, lost item, duplicate output.
					"Item(BoolValue(p0),a3,Item(n1,a2,e2))",
					"Item(n1,a3,Item(BoolValue(p0),a2,e2))",
					"Item(BoolValue(p0),a2,e2)",
					"Item(BoolValue(p0),a2,Item(n1,a3,Item(n1,a3,e2)))"})
				{
					const std::string target = 0 == kind ? std::string("Proj<a1 s1 ") + bad_target + ">"
						: std::string("Compute<") + bad_target + " a1 s1>";
					CDSLRule *bad = PdslruleParseLocal(mp, (source_rule + target + "(Input<t1>)" + aliases).c_str());
					if (nullptr == bad)
						ok = false;
					else
					{
						decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, bad, source);
						ok &= EdsldecisionReady != decision->Status();
						GPOS_DELETE(decision);
						bad->Release();
					}
				}
				rule->Release();
			}
			source->Release();
			predicate->Release();
		}
	}
	// The exact Compute path accepts empty lists, but rejects sibling dependencies,
	// SRFs, child-column collisions, duplicate definitions and same-level
	// aggregate/window calls. An aggregate inside a subquery has its own scope.
	CDSLRule *rule = PdslruleParseLocal(mp,
		"Compute<e0 a0 s0>(Input<t0>)|Compute<e1 a1 s1>(Input<t1>)|"
		"e1 := e0;a1 := a0;s1 := s0;t1 := t0");
	if (nullptr == rule)
		return GPOS_FAILED;
	for (ULONG shape = 0; shape < 11; ++shape)
	{
		const BOOL previous_ok = ok;
		ok = true;
		CColRefArray *columns = nullptr;
		CExpression *input = fix.PexprLogicalGet("select_export", 1, &columns);
		CColRef *output = 3 == shape ? (*columns)[0] : fix.PcrCreateInt4("definition");
		CExpressionArray *items = GPOS_NEW(mp) CExpressionArray(mp);
		if (shape > 0)
		{
			CExpression *value = nullptr;
			if (6 == shape || 8 == shape)
				value = CUtils::PexprAgg(mp, fix.Pmda(), IMDType::EaggMax, (*columns)[0], false, false);
			else if (7 == shape || 9 == shape)
				value = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarWindowFunc(
					mp, GPOS_NEW(mp) CMDIdGPDB(IMDId::EmdidGeneral, GPDB_INT4_AGG_MAX),
					GPOS_NEW(mp) CMDIdGPDB(IMDId::EmdidGeneral, GPDB_INT4_OID),
					GPOS_NEW(mp) CWStringConst(mp, GPOS_WSZ_LIT("max")),
					CScalarWindowFunc::EwsImmediate, false, false, true),
					CUtils::PexprScalarIdent(mp, (*columns)[0]));
			else if (10 == shape)
			{
				CColRefArray *inner_columns = nullptr;
				CExpression *inner = fix.PexprLogicalGet("aggregate_subquery", 1, &inner_columns);
				CColRefArray *grouping = GPOS_NEW(mp) CColRefArray(mp);
				CColRef *aggregate = fix.PcrCreateInt4("maximum");
				CExpression *query = fix.PexprLogicalGbAgg(inner, grouping, aggregate, (*inner_columns)[0]);
				inner->Release(); grouping->Release();
				value = GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CScalarSubquery(mp, aggregate, false, false), query);
			}
			else
				value = 2 == shape ? fix.PexprGenerateSeries((*columns)[0])
					: CUtils::PexprScalarConstInt4(mp, 7);
			if (8 == shape || 9 == shape)
				value = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarIf(mp,
					GPOS_NEW(mp) CMDIdGPDB(IMDId::EmdidGeneral, GPDB_INT4_OID)),
					CUtils::PexprScalarConstBool(mp, true), value, CUtils::PexprScalarConstInt4(mp, 0));
			items->Append(CUtils::PexprScalarProjectElement(mp, output, value));
		}
		if (1 == shape)
			items->Append(GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CScalarProjectElement(mp, fix.PcrCreateInt4("dependent")),
				GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarIdent(mp, output))));
		if (4 == shape)
			items->Append(GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CScalarProjectElement(mp, output), CUtils::PexprScalarConstInt4(mp, 8)));
		CExpression *source = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp), input,
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp), items));
		std::string exported, error;
		if (0 == shape || 2 == shape)
			ok &= CDSLPlanTemplate::FSlice(mp, source, "r", {"r/0"}, &exported, &error) &&
				exported == "Compute<e0 a0 s0>(Input<t0>)";
		else if (shape < 5 || (6 <= shape && shape <= 9))
			// A sibling definition is not an outer reference. The production
			// matcher rejects it along with colliding/duplicate outputs.
			ok &= !CDSLPlanTemplate::FSlice(mp, source, "r", {"r/0"}, &exported, &error);
		else
			ok &= CDSLPlanTemplate::FSlice(mp, source, "r", {"r/0"}, &exported, &error);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		const BOOL valid = 0 == shape || 5 == shape || 10 == shape;
		// An opaque list retains the complete subquery just like its explicit
		// typed spelling; row-phase checks stop at the relational boundary.
		const BOOL match_valid = valid;
		const BOOL matched = CDSLMatcher(mp, rule).FMatch(rule->PfragSrc()->PopRoot(), source, model);
		if (matched != match_valid)
			GPOS_TRACE_FORMAT("Compute matching domain: shape=%lu", shape);
		ok &= matched == match_valid;
		model->Release();
		// Probe construction independently: source matching must not be the
		// only guard for lists assembled by target expressions or other captures.
		model = GPOS_NEW(mp) CDSLModel(mp);
		const CDSLOp *pattern = rule->PfragSrc()->PopRoot();
		CExpression *list = (*source)[1];
		CColRefArray *used = list->DeriveUsedColumns()->Pdrgpcr(mp);
		CColRefArray *defined = GPOS_NEW(mp) CColRefArray(mp);
		for (ULONG i = 0; i < list->Arity(); ++i)
			defined->Append(CScalarProjectElement::PopConvert((*list)[i]->Pop())->Pcr());
		ok &= model->FBind((*pattern->Pdrgpsym())[0], list) &&
			model->FBind((*pattern->Pdrgpsym())[1], used) &&
			model->FBind((*pattern->Pdrgpsym())[2], defined) &&
			model->FBind((*(*pattern)[0]->Pdrgpsym())[0], input);
		CDSLInstantiator builder(mp);
		CExpression *target = builder.PexprInstantiate(rule, model);
		if ((nullptr != target) != valid)
			GPOS_TRACE_FORMAT("Compute construction domain: shape=%lu", shape);
		ok &= (nullptr != target) == valid;
		CRefCount::SafeRelease(target);
		if (5 <= shape || 2 == shape)
		{
			// SELECT and SELECT DISTINCT consume the same row-level values;
			// target validation must also cover independently bound lists.
			for (BOOL distinct : {false, true})
			{
				const std::string project_text = std::string("Proj<a0 s0 e0>(Input<t0>)|Proj") +
					(distinct ? "*" : "") + "<a1 s1 e1>(Input<t1>)|"
					"a1 := a0;s1 := s0;e1 := e0;t1 := t0";
				CDSLRule *project_rule = PdslruleParseLocal(mp, project_text.c_str());
				GPOS_ASSERT(nullptr != project_rule);
				CDSLModel *project_model = GPOS_NEW(mp) CDSLModel(mp);
				const CDSLOp *project_pattern = project_rule->PfragSrc()->PopRoot();
				ok &= CDSLMatcher(mp, project_rule).FMatch(project_pattern, source, project_model) == match_valid;
				project_model->Release();
				project_model = GPOS_NEW(mp) CDSLModel(mp);
				ok &= project_model->FBind((*project_pattern->Pdrgpsym())[0], used) &&
					project_model->FBind((*project_pattern->Pdrgpsym())[1], defined) &&
					project_model->FBind((*project_pattern->Pdrgpsym())[2], list) &&
					project_model->FBind((*(*project_pattern)[0]->Pdrgpsym())[0], input);
				CDSLInstantiator project_builder(mp);
				target = project_builder.PexprInstantiate(project_rule, project_model);
				if ((nullptr != target) != valid)
					GPOS_TRACE_FORMAT("SELECT construction domain: shape=%lu distinct=%d", shape, distinct);
				ok &= (nullptr != target) == valid;
				CRefCount::SafeRelease(target);
				project_model->Release();
				project_rule->Release();
			}
		}
		defined->Release();
		used->Release();
		model->Release();
		if (5 <= shape || 2 == shape)
		{
			// The same row-scalar boundary applies to Filter, on both entry paths.
			CDSLRule *filter_rule = PdslruleParseLocal(mp,
				"Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|p1 := p0;a1 := a0;t1 := t0");
			CExpression *value = (*(*source)[1])[0];
			value = (*value)[0];
			value->AddRef();
			CExpression *predicate = nullptr;
			if (8 == shape || 9 == shape)
			{
				// A quantified subquery starts a new scope only for its relational
				// child, not for the left comparison operand evaluated here.
				CColRefArray *inner_columns = nullptr;
				CExpression *inner = fix.PexprLogicalGet("quantified_scope", 1, &inner_columns);
				IMDId *comparison = (*columns)[0]->RetrieveType()->GetMdidForCmpType(IMDType::EcmptEq);
				comparison->AddRef();
				predicate = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarSubqueryAny(mp,
					comparison, GPOS_NEW(mp) CWStringConst(GPOS_WSZ_LIT("=")), (*inner_columns)[0]), inner, value);
			}
			else
				predicate = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarNullTest(mp), value);
			CExpression *filter = fix.PexprLogicalSelect(input, predicate);
			model = GPOS_NEW(mp) CDSLModel(mp);
			const BOOL filter_matched = CDSLMatcher(mp, filter_rule).FMatch(filter_rule->PfragSrc()->PopRoot(), filter, model);
			if (filter_matched != valid)
				GPOS_TRACE_FORMAT("Filter matching domain: shape=%lu", shape);
			ok &= filter_matched == valid;
			model->Release();
			model = GPOS_NEW(mp) CDSLModel(mp);
			const CDSLOp *filter_pattern = filter_rule->PfragSrc()->PopRoot();
			used = predicate->DeriveUsedColumns()->Pdrgpcr(mp);
			ok &= model->FBind((*filter_pattern->Pdrgpsym())[0], predicate) &&
				model->FBind((*filter_pattern->Pdrgpsym())[1], used) &&
				model->FBind((*(*filter_pattern)[0]->Pdrgpsym())[0], input);
			CDSLInstantiator filter_builder(mp);
			target = filter_builder.PexprInstantiate(filter_rule, model);
			if ((nullptr != target) != valid)
				GPOS_TRACE_FORMAT("Filter construction domain: shape=%lu", shape);
			ok &= (nullptr != target) == valid;
			CRefCount::SafeRelease(target);
			for (BOOL apply : {false, true})
			{
				const std::string op = apply ? "LeftApply" : "InnerJoin";
				const std::string slots = apply ? "p0 a0 a1 a2" : "p0 a0 a1";
				const std::string target_slots = apply ? "p1 a3 a4 a5" : "p1 a3 a4";
				CDSLRule *join_rule = PdslruleParseLocal(mp, (op + "<" + slots +
					">(Input<t0>,Input<t1>)|" + op + "<" + target_slots +
					">(Input<t2>,Input<t3>)|t2 := t0;t3 := t1;p1 := p0;a3 := a0;a4 := a1" +
					(apply ? ";a5 := a2" : "")).c_str());
				GPOS_ASSERT(nullptr != join_rule);
				CColRefArray *right_columns = nullptr;
				CExpression *right = fix.PexprLogicalGet("row_scope_right", 1, &right_columns);
				COperator *join_op = nullptr;
				if (apply)
				{
					right_columns->AddRef();
					join_op = GPOS_NEW(mp) CLogicalLeftOuterApply(mp, right_columns, COperator::EopScalarSubquery);
				}
				else
					join_op = GPOS_NEW(mp) CLogicalInnerJoin(mp);
				input->AddRef(); predicate->AddRef();
				CExpression *join = GPOS_NEW(mp) CExpression(mp, join_op, input, right, predicate);
				const CDSLOp *join_pattern = join_rule->PfragSrc()->PopRoot();
				CDSLModel *join_model = GPOS_NEW(mp) CDSLModel(mp);
				ok &= CDSLMatcher(mp, join_rule).FMatch(join_pattern, join, join_model) == valid;
				join_model->Release();
				join_model = GPOS_NEW(mp) CDSLModel(mp);
				CColRefArray *empty = GPOS_NEW(mp) CColRefArray(mp);
				ok &= join_model->FBind((*join_pattern->Pdrgpsym())[0], predicate) &&
					join_model->FBind((*join_pattern->Pdrgpsym())[1], used) &&
					join_model->FBind((*join_pattern->Pdrgpsym())[2], empty) &&
					(!apply || join_model->FBind((*join_pattern->Pdrgpsym())[3], empty)) &&
					join_model->FBind((*(*join_pattern)[0]->Pdrgpsym())[0], input) &&
					join_model->FBind((*(*join_pattern)[1]->Pdrgpsym())[0], right);
				if (apply)
				{
					join->AddRef();
					ok &= join_model->FSetApplyCarrier((*join_pattern->Pdrgpsym())[0], join);
				}
				CDSLInstantiator join_builder(mp);
				target = join_builder.PexprInstantiate(join_rule, join_model);
				if ((nullptr != target) != valid)
					GPOS_TRACE_FORMAT("ON construction domain: shape=%lu apply=%d", shape, apply);
				ok &= (nullptr != target) == valid;
				CRefCount::SafeRelease(target);
				empty->Release(); join_model->Release(); join->Release(); join_rule->Release();
			}
			used->Release(); model->Release(); filter->Release(); predicate->Release(); filter_rule->Release();
		}
		if (!ok)
			GPOS_TRACE_FORMAT("Row scalar domain: shape=%lu", shape);
		ok &= previous_ok;
		source->Release();
	}
	rule->Release();
	return ok ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLInstantiateTest::EresUnittest_ProjectExpressionBindings()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	BOOL ok = true;
	for (BOOL expand : {false, true})
	{
		const std::string text = expand
			? "Proj<a0 s0>(Filter<p0 a1>(Input<t0>))|"
			  "Proj<a2 s1>(Filter<Not(Not(p0)) a3>(Input<t1>))|"
			  "Eq(t1,t0);Eq(a2,a0);Eq(s1,s0);Eq(a3,a1)"
			: "Proj<a0 s0>(Filter<Not(Not(p0)) a1>(Input<t0>))|"
			  "Proj<a2 s1>(Filter<p1 a3>(Input<t1>))|"
			  "Eq(t1,t0);Eq(a2,a0);Eq(s1,s0);Eq(a3,a1);p1 := p0";
		CDSLRule *rule = PdslruleParseLocal(mp, text.c_str());
		if (nullptr == rule)
			return GPOS_FAILED;
		std::string reference_text = text;
		reference_text.replace(reference_text.find("Eq(a2,a0)"), 9, "a2 := a4;a4 := a0");
		reference_text.replace(reference_text.find("Eq(t1,t0)"), 9, "t1 := t2;t2 := t0");
		reference_text.replace(reference_text.find("Eq(s1,s0)"), 9, "s1 := s2;s2 := s0");
		reference_text.replace(reference_text.find("a0 s0>"), 6, "a0 s0 e0>");
		reference_text.replace(reference_text.find("a2 s1>"), 6, "a2 s1 e1>");
		reference_text += ";e1 := e2;e2 := e0";
		CDSLRule *reference = PdslruleParseLocal(mp, reference_text.c_str());
		if (nullptr == reference)
		{
			rule->Release();
			return GPOS_FAILED;
		}
		for (ULONG shape = 0; shape < 3; ++shape)
		{
			CColRefArray *columns = GPOS_NEW(mp) CColRefArray(mp);
			columns->Append(fix.PcrCreateInt4("predicate_input"));
			columns->Append(fix.PcrCreateInt4("projection_input"));
			CExpression *input = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CLogicalConstTableGet(mp, columns, GPOS_NEW(mp) IDatum2dArray(mp)));
			CExpression *predicate = 2 == shape ? CUtils::PexprScalarConstBool(mp, false, true)
				: fix.PexprPredAtom((*columns)[0]);
			if (1 == shape)
				predicate = GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopOr), predicate,
					CUtils::PexprScalarConstBool(mp, false, true));
			predicate->AddRef();
			CExpression *twice = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopNot),
				GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopNot), predicate));
			CExpression *select = fix.PexprLogicalSelect(input, expand ? predicate : twice);
			CExpressionArray *items = GPOS_NEW(mp) CExpressionArray(mp);
			for (ULONG i = 0; i < 3; ++i)
				items->Append(GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CScalarProjectElement(mp, fix.PcrCreateInt4("output")),
					1 == i ? CUtils::PexprScalarConstInt4(mp, 7) :
					GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarIdent(mp, (*columns)[1]))));
			CExpression *list = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp), items);
			select->AddRef();
			CExpression *source = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp), select, list);
			CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(
				mp, 1 == shape ? reference : rule, source);
			CExpression *target = decision->PexprTarget();
			ok &= EdsldecisionReady == decision->Status() && nullptr != target;
			if (nullptr != target)
			{
				ok &= COperator::EopLogicalProject == target->Pop()->Eopid() &&
					(*target)[1]->Matches(list) &&
					(*(*target)[0])[1]->Matches(expand ? twice : predicate) &&
					source->DeriveOutputColumns()->Equals(target->DeriveOutputColumns());
			}
			GPOS_DELETE(decision);
			if (1 == shape)
			{
				std::string alias_text = reference_text;
				alias_text.replace(alias_text.find("e1 := e2;e2 := e0"), 18, "Eq(e1,e0)");
				CDSLRule *alias = PdslruleParseLocal(mp, alias_text.c_str());
				if (nullptr == alias)
					ok = false;
				else
				{
					decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, alias, source);
					ok &= EdsldecisionReady == decision->Status() &&
						nullptr != decision->PexprTarget() && (*decision->PexprTarget())[1]->Matches(list);
					GPOS_DELETE(decision); alias->Release();
				}
			}
			// A schema capture must not be paired with the filter's unrelated columns.
			std::string bad_text = text;
			bad_text.replace(bad_text.find("Eq(a2,a0)"), 9, "Eq(a2,a1)");
			CDSLRule *bad = PdslruleParseLocal(mp, bad_text.c_str());
			if (nullptr == bad)
				ok = false;
			else
			{
				decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, bad, source);
				ok &= EdsldecisionReady != decision->Status();
				GPOS_DELETE(decision);
				bad->Release();
			}
			if (0 == shape)
			{
				// The target cannot combine an inner schema with an outer SELECT list.
				std::string crossed = reference_text;
				crossed.replace(crossed.find("e2 := e0"), 9, "e2 := e3");
				const auto split = crossed.find('|');
				crossed = "Proj<a5 s3 e3>(" + crossed.substr(0, split) + ")" + crossed.substr(split);
				CDSLRule *wrong_list = PdslruleParseLocal(mp, crossed.c_str());
				if (nullptr == wrong_list)
					ok = false;
				else
				{
					source->AddRef();
					CExpression *nested = GPOS_NEW(mp) CExpression(mp,
						GPOS_NEW(mp) CLogicalProject(mp), source,
						GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp)));
					decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, wrong_list, nested);
					ok &= EdsldecisionReady != decision->Status();
					GPOS_DELETE(decision); nested->Release(); wrong_list->Release();
				}
				// An unmentioned Limit must not disappear into the legacy Proj view.
				select->AddRef(); list->AddRef();
				CExpression *limited = GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CLogicalLimit(mp, GPOS_NEW(mp) COrderSpec(mp), true, true, false),
					select, CUtils::PexprScalarConstInt8(mp, 0), CUtils::PexprScalarConstInt8(mp, 1));
				CExpression *hidden = GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CLogicalProject(mp), limited, list);
				decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, hidden);
				ok &= EdsldecisionReady != decision->Status();
				GPOS_DELETE(decision); hidden->Release();
				CExpression *srf = fix.PexprGenerateSeries((*columns)[1]);
				CExpression *srf_list = GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CScalarProjectList(mp), GPOS_NEW(mp) CExpression(mp,
						GPOS_NEW(mp) CScalarProjectElement(mp, fix.PcrCreateInt4("series")), srf));
				select->AddRef();
				CExpression *set_project = GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CLogicalProject(mp), select, srf_list);
				decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, set_project);
				ok &= EdsldecisionReady != decision->Status();
				GPOS_DELETE(decision); set_project->Release();
			}
			source->Release(); select->Release(); twice->Release(); predicate->Release(); input->Release();
		}
		reference->Release();
		rule->Release();
	}
	return ok ? GPOS_OK : GPOS_FAILED;
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
	const CHAR *kinds[] = {"InnerJoin", "LeftJoin", "FullJoin", "SemiJoin", "AntiJoin", "Exists"};
	BOOL ok = true;
	for (ULONG kind = 0; kind < GPOS_ARRAY_SIZE(kinds); kind++)
	{
		const std::string name(kinds[kind]);
		// Predicate-form Exists is a SemiJoin, not plain existential demand.
		const std::string nativeName = 5 == kind ? "SemiJoin" : name;
		const std::string text = nativeName +
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
				case 3:
				case 5: op = GPOS_NEW(mp) CLogicalLeftSemiJoin(mp); break;
				default: op = GPOS_NEW(mp) CLogicalLeftAntiSemiJoin(mp); break;
			}
			l->AddRef(); r->AddRef(); p->AddRef();
			return GPOS_NEW(mp) CExpression(mp, op, l, r, p);
		};
		for (ULONG shape = 0; shape < 5; shape++)
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
			else if (3 <= shape)
			{
				on = 3 == shape ? lp : rp;
				on->AddRef();
			}
			else
			{
				lp->AddRef(); rp->AddRef();
				on = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarBoolOp(mp,
					0 == shape ? CScalarBoolOp::EboolopAnd : CScalarBoolOp::EboolopOr), lp, rp);
			}
			CExpression *pn = negate(on), *pnn = negate(pn);
			CExpression *source = join(ls, rs, pnn);
			std::string exported, export_error;
			check(CDSLPlanTemplate::FSlice(mp, source, "r", {"r/0/0", "r/1/0"},
				&exported, &export_error), "export branch expressions");
			const std::string on_template = 2 <= shape ? "p5"
				: 0 == shape ? "And(p5,p6)" : "Or(p5,p6)";
			check(exported == nativeName + "<Not(Not(" + on_template + ")) a4 a5>("
				"Filter<Not(Not(p3)) a0>(Input<t0>),Filter<Not(Not(p4)) a2>(Input<t1>))",
				"lossless ON and two independent child templates");
			check(CDSLPlanTemplate::FSlice(mp, source, "r", {"r/0", "r/1"},
				&exported, &export_error) && std::string::npos == exported.find("Filter<"),
				"arbitrary input cuts hide child expression structure");
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
				check(target->Pop()->Eopid() == source->Pop()->Eopid() && 3 == target->Arity(), "join kind");
			}
			if (nullptr != target && 3 == target->Arity())
			{
				check((*target)[2]->Matches(on), "ON target");
				check((*(*target)[0])[1]->Matches(lp), "left predicate");
				check((*(*target)[1])[1]->Matches(rp), "right predicate");
				check((*(*target)[0])[0] == l && (*(*target)[1])[0] == r, "input identity");
				check(source->DeriveOutputColumns()->Equals(target->DeriveOutputColumns()), "schema");
				check((*source)[2] == pnn, "source unchanged");
			}
			GPOS_DELETE(decision);
			if (5 == kind)
			{
				std::string reverse = text;
				reverse.replace(0, nativeName.size(), name);
				reverse.replace(reverse.find("|Exists"), 7, "|SemiJoin");
				CDSLRule *alias = PdslruleParseLocal(mp, reverse.c_str());
				check(nullptr != alias, "predicate Exists source parses");
				if (nullptr != alias)
				{
					decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, alias, source);
					check(EdsldecisionReady == decision->Status() &&
						nullptr != decision->PexprTarget() &&
						3 == decision->PexprTarget()->Arity() &&
						(*decision->PexprTarget())[2]->Matches(on), "predicate Exists source capture");
					GPOS_DELETE(decision);
					alias->Release();
				}
			}
			// A dependency union can be correct while its two scopes are wrong.
			// Reject swapped/duplicated partitions, but permit empty partitions
			// and a genuinely commuted InnerJoin with both inputs swapped.
			for (ULONG variant = 0; variant < 3; variant++)
			{
				if (2 == variant && 0 != kind)
					continue;
				std::string partitionText = text;
				const std::string original = "AttrsEq(a4,a0);AttrsEq(a5,a1)";
				partitionText.replace(partitionText.find(original), original.size(),
					1 == variant ? "AttrsEq(a4,a0);AttrsEq(a5,a0)"
								 : "AttrsEq(a4,a1);AttrsEq(a5,a0)");
				if (2 == variant)
				{
					const std::string children =
						"Filter<p4 a6>(Input<t2>),Filter<p5 a7>(Input<t3>)";
					partitionText.replace(partitionText.find(children), children.size(),
						"Filter<p5 a7>(Input<t3>),Filter<p4 a6>(Input<t2>)");
				}
				CDSLRule *partitionRule = PdslruleParseLocal(mp, partitionText.c_str());
				check(nullptr != partitionRule, "partition rule parses");
				if (nullptr != partitionRule)
				{
					decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, partitionRule, source);
					const BOOL valid = 2 == shape || 2 == variant;
					check((valid ? EdsldecisionReady : EdsldecisionInstantiateRejected) ==
						decision->Status(), "target dependency partitions");
					GPOS_DELETE(decision);
					partitionRule->Release();
				}
			}
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
	// A predicate Exists observes columns produced by its right Project.
	// It must never inherit plain EXISTS's unused-target-list stripping.
	for (const CHAR *bindings : {
		"TableEq(t2,t0);TableEq(t3,t1);PredicateEq(p1,p0);AttrsEq(a2,a0);AttrsEq(a3,a1)",
		"t2 := t0;t3 := t1;p1 := p0;a2 := a0;a3 := a1"})
	{
		CColRefArray *lc = nullptr, *rc = nullptr;
		CExpression *left = input(&lc), *right = input(&rc);
		CColRef *column = fix.PcrCreateInt4("computed");
		CExpression *project = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CLogicalProject(mp), right,
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
				GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectElement(mp, column),
					CUtils::PexprScalarConstInt4(mp, 7))));
		CExpression *on = fix.PexprEqConst(column, 7);
		CExpression *source = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CLogicalLeftSemiJoin(mp), left, project, on);
		const std::string text =
			"SemiJoin<p0 a0 a1>(Input<t0>,Input<t1>)|"
			"Exists<p1 a2 a3>(Input<t2>,Input<t3>)|" + std::string(bindings);
		CDSLRule *rule = PdslruleParseLocal(mp, text.c_str());
		if (nullptr == rule) { source->Release(); return GPOS_FAILED; }
		CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, source);
		CExpression *target = decision->PexprTarget();
		const BOOL preserved = EdsldecisionDuplicate == decision->Status() && nullptr != target &&
			3 == target->Arity() && (*target)[1] == project && (*target)[2]->Matches(on);
		if (!preserved)
			GPOS_TRACE_FORMAT("predicate Exists Project status=%d target=%p source=%s",
				decision->Status(), target, bindings);
		ok &= preserved;
		GPOS_DELETE(decision);
		rule->Release(); source->Release();
	}
	// Keyed forms keep comparison metadata, but construct the named residual.
	// Test both residual layouts, both join kinds and swapped dependency scopes.
	for (BOOL outer : {false, true})
	for (BOOL output : {false, true})
	for (ULONG variant = 0; variant < 9; variant++)
	{
		auto check = [&](BOOL valid, const CHAR *reason) {
			if (!valid) GPOS_TRACE_FORMAT("keyed residual outer=%d output=%d variant=%lu: %s",
				outer, output, variant, reason);
			ok &= valid;
		};
		CColRefArray *lc = nullptr, *rc = nullptr;
		CExpression *left = fix.PexprLogicalGet("residual_left", 2, &lc);
		CExpression *right = fix.PexprLogicalGet("residual_right", 2, &rc);
		CExpression *key = fix.PexprEqPred((*lc)[0], (*rc)[0]);
		CExpression *atom = fix.PexprPredAtom(5 == variant
			? fix.PcrCreateInt4("outside_join") : (*lc)[1]);
		CExpression *residual = CUtils::PexprNegate(mp, atom);
		CExpression *on = CPredicateUtils::PexprConjunction(mp, key, residual);
		if (6 == variant)
		{
			on->Release(); key->AddRef(); on = key;
		}
		CExpression *source = outer
			? fix.PexprLogicalLeftOuterJoin(left, right, on)
			: fix.PexprLogicalInnerJoin(left, right, on);
		const std::string name = outer ? "LeftJoin" : "InnerJoin";
		std::string text = name + (output ? "<a0 a1 a10 s0 p0 a2 a3>" : "<a0 a1 p0 a2 a3>") +
			"(Input<t0>,Input<t1>)|" + name +
			(output ? "<a4 a5 a11 s1 p1 a6 a7>" : "<a4 a5 p1 a6 a7>") +
			"(Input<t2>,Input<t3>)|t2 := t0;t3 := t1;a4 := a0;a5 := a1;" +
			(3 == variant ? "a6 := a3;a7 := a2;" : "a6 := a2;a7 := a3;") +
			(output ? "a11 := a10;s1 := s0;" : "") +
			(0 == variant ? "p1 := p0" : "p1 := Not(p0)");
		if (2 == variant) text += ";Not(p9) := p0";
		if (4 == variant) text += ";And(p8,p9) := p0";
		if (7 == variant)
		{
			const std::string keys = "a4 := a0;a5 := a1";
			const std::string deps = "a6 := a2;a7 := a3";
			const std::string children = "(Input<t2>,Input<t3>)";
			text.replace(text.find(keys), keys.size(), "a4 := a1;a5 := a0");
			text.replace(text.find(deps), deps.size(), "a6 := a3;a7 := a2");
			text.replace(text.find(children), children.size(), "(Input<t3>,Input<t2>)");
		}
		if (8 == variant)
		{
			text.erase(text.find('|', text.find('|') + 1) + 1);
			text += "TableEq(t2,t0);TableEq(t3,t1);AttrsEq(a4,a0);AttrsEq(a5,a1);"
				"AttrsEq(a6,a2);AttrsEq(a7,a3);PredicateNot(p1,p0)";
			if (output) text += ";AttrsEq(a11,a10);SchemaEq(s1,s0)";
		}
		CDSLRule *rule = PdslruleParseLocal(mp, text.c_str());
		check(nullptr != rule, "keyed residual parses");
		if (nullptr != rule)
		{
			CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
			const BOOL matched = CDSLMatcher(mp, rule).FMatch(rule->PfragSrc()->PopRoot(), source, model);
			check(matched == (variant < 4 || variant >= 7), "residual source structure/scope");
			CExpression *target = matched ? CDSLInstantiator(mp).PexprInstantiate(rule, model) : nullptr;
			if (3 <= variant && variant <= 6)
				check(nullptr == target, "residual partition rejects");
			else if (nullptr != target)
			{
				CExpression *expectedResidual = residual;
				if (0 == variant) expectedResidual->AddRef();
				else { residual->AddRef(); expectedResidual = CUtils::PexprNegate(mp, residual); }
				CExpression *expected = CPredicateUtils::PexprConjunction(mp, key, expectedResidual);
				check((*target)[2]->Matches(expected), "target uses new residual and original key");
				expected->Release(); expectedResidual->Release();
			}
			else check(false, "keyed residual constructs");
			CRefCount::SafeRelease(target); model->Release(); rule->Release();
		}
		source->Release(); on->Release(); residual->Release(); key->Release();
		left->Release(); right->Release();
	}
	// A NOT IN Apply target needs its correlated carrier metadata preserved.
	for (const CHAR *text : {
		"AntiJoinNotIn<p0 a0 a1>(Input<t0>,Input<t1>)|"
		"AntiApplyNotIn<p1 a2 a3 a4>(Input<t2>,Input<t3>)|"
		"t2 := t0;t3 := t1;a2 := a0;a3 := a1;AttrsEmpty(a4);p1 := p0"})
	{
		CDSLRule *unsupported = PdslruleParseLocal(mp, text);
		ok &= nullptr == unsupported;
		CRefCount::SafeRelease(unsupported);
	}
	return ok ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLInstantiateTest::EresUnittest_ApplyExpressionBindings()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	auto input = [&]() {
		CColRefArray *cols = GPOS_NEW(mp) CColRefArray(mp);
		cols->Append(fix.PcrCreateInt4("apply_value"));
		return GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalConstTableGet(
			mp, cols, GPOS_NEW(mp) IDatum2dArray(mp)));
	};
	auto twice = [mp](CExpression *p) {
		p->AddRef();
		return GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopNot),
			GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopNot), p));
	};
	BOOL ok = true;
	for (const std::string name : {"InnerApply", "LeftApply", "SemiApply", "AntiApply"})
	for (BOOL correlated : {false, true})
	for (BOOL construct : {false, true})
	for (BOOL emptyInner : {false, true})
	{
		const BOOL left = "LeftApply" == name;
		const BOOL existential = "SemiApply" == name || "AntiApply" == name;
		if (emptyInner && !existential)
			continue;
		const COperator::EOperatorId origin = "SemiApply" == name ? COperator::EopScalarSubqueryExists
			: "AntiApply" == name ? COperator::EopScalarSubqueryNotExists : COperator::EopScalarSubquery;
		const std::string src = construct ? "p0" : "Not(Not(p0))";
		const std::string dst = construct ? "Not(Not(p3))" : "p3";
		const std::string text = name + "<" + src + " a0 a1 a2>"
			"(Input<t0>,Filter<Not(Not(p1)) a3 a4>(Input<t1>))|" + name +
			"<" + dst + " a5 a6 a7>(Input<t2>,Filter<p2 a8 a9>(Input<t3>))|"
			"t2 := t0;t3 := t1;a5 := a0;a6 := a1;a7 := a2;"
			"a8 := a3;a9 := a4;p2 := p1;p3 := p0";
		CWStringDynamic parseError(mp);
		CDSLRule *rule = CDSLRuleParser::PdslruleParse(mp, text.c_str(), "EQ", &parseError);
		if (nullptr == rule)
			GPOS_TRACE_FORMAT("Apply binding parse: %ls", parseError.GetBuffer());
		GPOS_UNITTEST_ASSERT(nullptr != rule);
		CExpression *l = input(), *r = input();
		CColRef *lc = (*CLogicalConstTableGet::PopConvert(l->Pop())->PdrgpcrOutput())[0];
		CColRef *rc = (*CLogicalConstTableGet::PopConvert(r->Pop())->PdrgpcrOutput())[0];
		CExpression *on = fix.PexprEqPred(lc, rc);
		CExpression *pred = fix.PexprEqPred(lc, rc);
		CExpression *predNN = twice(pred);
		CExpression *right = fix.PexprLogicalSelect(r, predNN);
		CExpression *onNN = twice(on);
		CColRefArray *inner = GPOS_NEW(mp) CColRefArray(mp);
		if (!emptyInner)
			inner->Append(rc);
		COperator *op = "SemiApply" == name
			? (correlated ? static_cast<COperator *>(GPOS_NEW(mp) CLogicalLeftSemiCorrelatedApply(mp, inner, origin))
				: GPOS_NEW(mp) CLogicalLeftSemiApply(mp, inner, origin))
			: "AntiApply" == name
			? (correlated ? static_cast<COperator *>(GPOS_NEW(mp) CLogicalLeftAntiSemiCorrelatedApply(mp, inner, origin))
				: GPOS_NEW(mp) CLogicalLeftAntiSemiApply(mp, inner, origin))
			: left
			? (correlated ? static_cast<COperator *>(GPOS_NEW(mp) CLogicalLeftOuterCorrelatedApply(
				mp, inner, COperator::EopScalarSubquery)) : GPOS_NEW(mp) CLogicalLeftOuterApply(
				mp, inner, COperator::EopScalarSubquery))
			: (correlated ? static_cast<COperator *>(GPOS_NEW(mp) CLogicalInnerCorrelatedApply(
				mp, inner, COperator::EopScalarSubquery)) : GPOS_NEW(mp) CLogicalInnerApply(
				mp, inner, COperator::EopScalarSubquery));
		l->AddRef(); right->AddRef();
		CExpression *sourcePred = construct ? on : onNN;
		sourcePred->AddRef();
		CExpression *source = GPOS_NEW(mp) CExpression(mp, op, l, right, sourcePred);
		auto check = [&](BOOL valid, const CHAR *step) {
			if (!valid)
				GPOS_TRACE_FORMAT("apply binding kind=%s correlated=%d construct=%d check=%s",
					name.c_str(), correlated, construct, step);
			ok &= valid;
		};
		std::string exported, exportError;
		check(CDSLPlanTemplate::FSlice(mp, source, "r", {"r/0", "r/1/0"},
			&exported, &exportError) && exported.find(name + "<") == 0 &&
			exported.find("Filter<Not(Not(") != std::string::npos,
			"export native Apply and correlated child expression");
		CDSLRulePrefixIndex index(mp);
		index.Insert(rule, 0, source->Pop()->Eopid());
		CDSLRuleArray *candidates = index.PdrgpruleCandidates(mp, source);
		check(1 == candidates->Size(), "literal Apply candidate");
		candidates->Release();
		op->AddRef(); l->AddRef(); r->AddRef(); sourcePred->AddRef();
		CExpression *missing = GPOS_NEW(mp) CExpression(mp, op, l, r, sourcePred);
		candidates = index.PdrgpruleCandidates(mp, missing);
		check(0 == candidates->Size(), "missing child rejected by prefix");
		candidates->Release(); missing->Release();
		CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, source);
		CExpression *target = decision->PexprTarget();
		check(EdsldecisionReady == decision->Status() && nullptr != target, "rewrite");
		if (nullptr != target)
		{
			CLogicalApply *built = CLogicalApply::PopConvert(target->Pop());
			check(target->Pop()->Eopid() == op->Eopid() &&
				built->FCorrelated() == correlated &&
				built->EopidOriginSubq() == origin &&
				built->PdrgPcrInner()->Equals(inner), "carrier metadata");
			check((*target)[0] == l && (*(*target)[1])[0] == r, "input identity");
			check((*target)[2]->Matches(construct ? onNN : on) &&
				(*(*target)[1])[1]->Matches(pred), "predicate construction");
			check((*source)[2] == sourcePred && (*right)[1] == predNN, "source unchanged");
		}
		GPOS_DELETE(decision);
		// Equal unions are insufficient, and a target correlation cannot be
		// borrowed from an unrelated local dependency slot.
		for (const CHAR *wrong : {"a7 := a1", "a7 := a0"})
		{
			std::string invalid = text;
			invalid.replace(invalid.find("a7 := a2"), 8, wrong);
			CDSLRule *bad = PdslruleParseLocal(mp, invalid.c_str());
			GPOS_UNITTEST_ASSERT(nullptr != bad);
			decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, bad, source);
			check(EdsldecisionInstantiateRejected == decision->Status(), "unbound carrier metadata");
			GPOS_DELETE(decision);
			bad->Release();
		}
		// A Boolean ANY/ALL carrier is not the row-producing Apply in the DSL.
		if (correlated && !existential)
		{
			inner->AddRef(); l->AddRef(); right->AddRef(); sourcePred->AddRef();
			COperator *valueOp = left
				? static_cast<COperator *>(GPOS_NEW(mp) CLogicalLeftOuterCorrelatedApply(
					mp, inner, COperator::EopScalarSubqueryAny))
				: static_cast<COperator *>(GPOS_NEW(mp) CLogicalInnerCorrelatedApply(
					mp, inner, COperator::EopScalarSubqueryAny));
			CExpression *value = GPOS_NEW(mp) CExpression(mp, valueOp, l, right, sourcePred);
			decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, value);
			check(EdsldecisionMatchRejected == decision->Status(), "boolean carrier isolation");
			GPOS_DELETE(decision);
			check(!CDSLPlanTemplate::FSlice(mp, value, "r", {"r/0", "r/1"},
				&exported, &exportError), "boolean carrier export isolation");
			value->Release();
		}
		if ("SemiApply" == name && !emptyInner)
		{
			inner->AddRef(); l->AddRef(); right->AddRef(); sourcePred->AddRef();
			COperator *inOp = correlated
				? static_cast<COperator *>(GPOS_NEW(mp) CLogicalLeftSemiCorrelatedApplyIn(
					mp, inner, COperator::EopScalarSubqueryAny))
				: GPOS_NEW(mp) CLogicalLeftSemiApplyIn(mp, inner, COperator::EopScalarSubqueryAny);
			CExpression *in = GPOS_NEW(mp) CExpression(mp, inOp, l, right, sourcePred);
			decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, in);
			check(EdsldecisionMatchRejected == decision->Status(), "IN carrier isolation");
			GPOS_DELETE(decision);
			in->Release();
		}
		source->Release(); right->Release(); predNN->Release(); pred->Release();
		onNN->Release(); on->Release(); l->Release(); r->Release(); rule->Release();
	}
	// Equal source predicates do not imply equal carrier metadata. Both target
	// predicates can use the same capture, but each must retain its own carrier.
	for (BOOL wrongCarrier : {false, true})
	{
		const std::string rightAliases = wrongCarrier
			? "a13 := a0;a14 := a1;a15 := a2"
			: "a13 := a3;a14 := a4;a15 := a5";
		const std::string text =
			"InnerJoin<p1 a6 a7>(InnerApply<p0 a0 a1 a2>(Input<t0>,Input<t1>),"
			"InnerApply<p3 a3 a4 a5>(Input<t2>,Input<t3>))|"
			"InnerJoin<p2 a8 a9>(InnerApply<Not(Not(p0)) a10 a11 a12>(Input<t4>,Input<t5>),"
			"InnerApply<Not(Not(p0)) a13 a14 a15>(Input<t6>,Input<t7>))|"
			"t4 := t0;t5 := t1;t6 := t2;t7 := t3;p2 := p1;a8 := a6;a9 := a7;"
			"a10 := a0;a11 := a1;a12 := a2;PredicateEq(p0,p3);" + rightAliases;
		CDSLRule *rule = PdslruleParseLocal(mp, text.c_str());
		GPOS_UNITTEST_ASSERT(nullptr != rule);
		auto apply = [&]() {
			CExpression *l = input(), *r = input();
			CColRef *rc = (*CLogicalConstTableGet::PopConvert(r->Pop())->PdrgpcrOutput())[0];
			return CUtils::PexprLogicalApply<CLogicalInnerApply>(
				mp, l, r, rc, COperator::EopScalarSubquery);
		};
		CExpression *l = apply(), *r = apply();
		CExpression *source = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CLogicalInnerJoin(mp), l, r, CUtils::PexprScalarConstBool(mp, true));
		CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, source);
		ok &= (wrongCarrier ? EdsldecisionInstantiateRejected : EdsldecisionReady) == decision->Status();
		if (nullptr != decision->PexprTarget())
		{
			for (ULONG i = 0; i < 2; i++)
			{
				CLogicalApply *original = CLogicalApply::PopConvert((*source)[i]->Pop());
				CLogicalApply *built = CLogicalApply::PopConvert((*decision->PexprTarget())[i]->Pop());
				ok &= original->PdrgPcrInner()->Equals(built->PdrgPcrInner()) &&
					original->EopidOriginSubq() == built->EopidOriginSubq();
			}
		}
		GPOS_DELETE(decision);
		source->Release(); rule->Release();
	}
	return ok ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLInstantiateTest::EresUnittest_CorrelatedFilterBindings()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	BOOL ok = true;
	CColRefArray *columns = nullptr;
	CExpression *input = fix.PexprLogicalGet("correlated_filter", 1, &columns);
	CColRef *local = (*columns)[0];
	CColRef *outer = fix.PcrCreateInt4("outer");
	CExpression *guard = fix.PexprPredAtom(local);
	// Input captures an arbitrary tree, including its own existing filter.
	CExpression *child = fix.PexprLogicalSelect(input, guard);
	guard->Release();
	for (ULONG shape = 0; shape < 4; ++shape)
	{
		CExpression *predicate = 0 == shape ? fix.PexprEqPred(local, outer)
			: 1 == shape ? fix.PexprPredAtom(local)
			: 2 == shape ? fix.PexprPredAtom(outer)
			: CUtils::PexprScalarConstBool(mp, true);
		CExpression *source = fix.PexprLogicalSelect(child, predicate);
		for (ULONG variant = 0; variant < 4; ++variant)
		{
			const std::string source_text = 3 == variant
				? "Filter<p0 a0>(Input<t0>)" : "Filter<p0 a0 a1>(Input<t0>)";
			const std::string text = source_text + "|" +
				(3 == variant ? "Filter<Not(Not(p0)) a2>(Input<t1>)"
					: "Filter<Not(Not(p0)) a2 a3>(Input<t1>)") + "|t1 := t0;" +
				(0 == variant ? "a2 := a0;a3 := a1"
				 : 1 == variant ? "a2 := a1;a3 := a0"
				 : 2 == variant ? "a2 := a0;a3 := a0" : "a2 := a0");
			CDSLRule *rule = PdslruleParseLocal(mp, text.c_str());
			if (nullptr == rule)
			{
				ok = false;
				continue;
			}
			CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
			const BOOL matched = CDSLMatcher(mp, rule).FMatch(
				rule->PfragSrc()->PopRoot(), source, model);
			const BOOL expected_match = 3 != variant || 1 == shape || 3 == shape;
			ok &= matched == expected_match;
			if (matched)
			{
				CDSLInstantiator builder(mp);
				CExpression *target = builder.PexprInstantiate(rule, model);
				// Swapping or duplicating partitions must fail even if their union
				// still equals all predicate dependencies. Both empty is valid.
				const BOOL expected_target = 0 == variant || 3 == variant || 3 == shape;
				ok &= (nullptr != target) == expected_target;
				if (nullptr != target)
				{
					ok &= (*target)[0] == child &&
						source->DeriveOuterReferences()->Equals(target->DeriveOuterReferences()) &&
						source->DeriveOutputColumns()->Equals(target->DeriveOutputColumns()) &&
						CUtils::FScalarBoolOp((*target)[1], CScalarBoolOp::EboolopNot) &&
						CUtils::FScalarBoolOp((*(*target)[1])[0], CScalarBoolOp::EboolopNot) &&
						(*(*(*target)[1])[0])[0]->Matches(predicate);
					std::string exported, error;
					ok &= CDSLPlanTemplate::FSlice(mp, target, "r", {"r/0"}, &exported, &error) &&
						std::string::npos != exported.find("Not(Not(") &&
						std::string::npos != exported.find(
							0 == shape || 2 == shape ? " a0 a1>(Input<t0>)" : " a0>(Input<t0>)");
					target->Release();
				}
			}
			model->Release();
			rule->Release();
		}
		source->Release();
		predicate->Release();
	}
	child->Release();
	input->Release();
	// Dependency vectors are sets, not output schemas. AttrsUnion may list
	// the upper filter's newer columns before the lower filter's older ones.
	for (BOOL correlated : {false, true})
	{
		CColRefArray *cols = nullptr;
		CExpression *base = fix.PexprLogicalGet("filter_dependency_order", 2, &cols);
		CColRef *outer_first = fix.PcrCreateInt4("outer_first");
		CColRef *outer_second = fix.PcrCreateInt4("outer_second");
		CExpression *lower_pred = correlated ? fix.PexprEqPred((*cols)[0], outer_first)
			: fix.PexprPredAtom((*cols)[0]);
		CExpression *upper_pred = correlated ? fix.PexprEqPred((*cols)[1], outer_second)
			: fix.PexprPredAtom((*cols)[1]);
		CExpression *lower = fix.PexprLogicalSelect(base, lower_pred);
		CExpression *source = fix.PexprLogicalSelect(lower, upper_pred);
		CDSLRule *rule = PdslruleParseLocal(mp,
			"Filter<p0 a0 a1>(Filter<p1 a2 a3>(Input<t0>))|Filter<p2 a4 a5>(Input<t1>)|"
			"t1 := t0;p2 := And(p0,p1);AttrsUnion(a4,a0,a2);AttrsUnion(a5,a1,a3)");
		GPOS_UNITTEST_ASSERT(nullptr != rule);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		GPOS_UNITTEST_ASSERT(CDSLMatcher(mp, rule).FMatch(rule->PfragSrc()->PopRoot(), source, model));
		CDSLInstantiator builder(mp);
		CExpression *target = builder.PexprInstantiate(rule, model);
		ok &= nullptr != target;
		if (nullptr != target)
		{
			ok &= (*target)[0] == base &&
				source->DeriveOuterReferences()->Equals(target->DeriveOuterReferences()) &&
				(*(*target)[1])[0]->Matches(upper_pred) &&
				(*(*target)[1])[1]->Matches(lower_pred);
			target->Release();
		}
		model->Release();
		rule->Release();
		source->Release();
		lower->Release();
		base->Release();
		upper_pred->Release();
		lower_pred->Release();
	}
	return ok ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLInstantiateTest::EresUnittest_ExistsExpressionBindings()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	BOOL ok = true;
	for (BOOL negated : {false, true})
	for (BOOL correlated : {false, true})
	{
		const std::string name = negated ? "NotExists" : "Exists";
		const std::string src_deps = correlated ? "a0 a2" : "a0";
		const std::string dst_deps = correlated ? "a1 a3" : "a1";
		const std::string text = name +
			"(Input<t0>,Filter<Not(Not(p0)) " + src_deps + ">(Input<t1>))|" + name +
			"(Input<t2>,Filter<p1 " + dst_deps + ">(Input<t3>))|"
			"Eq(t2,t0);Eq(t3,t1);Eq(a1,a0);p1 := p0" +
			(correlated ? ";a3 := a2" : "");
		CDSLRule *rule = PdslruleParseLocal(mp, text.c_str());
		if (nullptr == rule)
			return GPOS_FAILED;
		// Constant inputs provide real statistics when the subquery enters Memo.
		auto input = [&](CColRefArray *columns) {
			return GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalConstTableGet(
				mp, columns, GPOS_NEW(mp) IDatum2dArray(mp)));
		};
		CColRefArray *outer_cols = GPOS_NEW(mp) CColRefArray(mp);
		outer_cols->Append(fix.PcrCreateInt4("outer"));
		CExpression *outer = input(outer_cols);
		CColRefArray *cols = GPOS_NEW(mp) CColRefArray(mp);
		cols->Append(fix.PcrCreateInt4("inner"));
		CExpression *get = input(cols);
		CExpression *atom = correlated ? fix.PexprEqPred((*cols)[0], (*outer_cols)[0])
			: fix.PexprPredAtom((*cols)[0]);
		CExpression *inner = fix.PexprLogicalSelect(get, atom);
		atom->Release();
		// Input is an arbitrary tree: retain a Project under the inner Filter.
		CExpressionArray *items = GPOS_NEW(mp) CExpressionArray(mp);
		items->Append(GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CScalarProjectElement(mp, fix.PcrCreateInt4("computed")),
			CUtils::PexprScalarConstInt4(mp, 1)));
		get->AddRef();
		CExpression *project = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CLogicalProject(mp), get,
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp), items));
		(*inner)[1]->AddRef();
		CExpression *twice = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopNot),
			GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopNot), (*inner)[1]));
		CExpression *filtered = fix.PexprLogicalSelect(project, twice);
		COperator *op = negated
			? static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryNotExists(mp))
			: static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryExists(mp));
		filtered->AddRef();
		CExpression *predicate = GPOS_NEW(mp) CExpression(mp, op, filtered);
		CExpression *source = fix.PexprLogicalSelect(outer, predicate);
		CDSLRewriteDecision *decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, source);
		CExpression *target = decision->PexprTarget();
		ok &= EdsldecisionReady == decision->Status() && nullptr != target;
		if (nullptr != target)
		{
			ok &= COperator::EopLogicalSelect == target->Pop()->Eopid() &&
				(*target)[1]->Pop()->Eopid() == op->Eopid();
			CExpression *result = (*(*target)[1])[0];
			ok &= COperator::EopLogicalSelect == result->Pop()->Eopid() &&
				(*result)[1]->Matches((*inner)[1]) && (*result)[0]->Matches(project) &&
				target->DeriveOutputColumns()->Equals(outer->DeriveOutputColumns());
		}
		GPOS_DELETE(decision);
		// Do not silently turn AND(guard, EXISTS) into EXISTS(Filter(guard), ...).
		predicate->AddRef();
		CExpression *guarded = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopAnd),
			CUtils::PexprScalarConstBool(mp, true), predicate);
		CExpression *bad = fix.PexprLogicalSelect(outer, guarded);
		decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, bad);
		ok &= EdsldecisionMatchRejected == decision->Status();
		GPOS_DELETE(decision);
		bad->Release();
		guarded->Release();
		COperator *opposite = negated
			? static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryExists(mp))
			: static_cast<COperator *>(GPOS_NEW(mp) CScalarSubqueryNotExists(mp));
		filtered->AddRef();
		CExpression *wrong = GPOS_NEW(mp) CExpression(mp, opposite, filtered);
		bad = fix.PexprLogicalSelect(outer, wrong);
		decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, bad);
		ok &= EdsldecisionMatchRejected == decision->Status();
		GPOS_DELETE(decision);
		bad->Release();
		wrong->Release();
		// A TRUE-ON native EXISTS carrier has the same capped demand; the
		// opposite polarity remains rejected. Preserve its complete inner tree.
		outer->AddRef();
		filtered->AddRef();
		bad = CUtils::PexprLogicalApply<CLogicalLeftSemiApply>(mp, outer, filtered,
			(*cols)[0], COperator::EopScalarSubqueryExists);
		decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, bad);
		ok &= (negated ? EdsldecisionMatchRejected : EdsldecisionReady) == decision->Status();
		if (!negated && nullptr != decision->PexprTarget())
			ok &= (*(*(*decision->PexprTarget())[1])[0])[0]->Matches(project);
		GPOS_DELETE(decision);
		bad->Release();
		{
			CDSLRulePrefixIndex index(mp);
			index.Insert(rule, 0, source->Pop()->Eopid());
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
			ok &= 0 < bindings->Size();
			for (ULONG i = 0; i < bindings->Size(); i++)
			{
				decision = CDSLRuleEngine::Instance()->PdecisionEvaluate(mp, rule, (*bindings)[i]);
				ok &= EdsldecisionReady == decision->Status();
				GPOS_DELETE(decision);
			}
			bindings->Release();
		}
		source->Release();
		predicate->Release();
		filtered->Release();
		twice->Release();
		project->Release();
		inner->Release();
		get->Release();
		outer->Release();
		rule->Release();
	}
	return ok ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLInstantiateTest::EresUnittest_LegacyBindingBoundary()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	BOOL ok = true;
	// Construction probes only: these are not registered equivalent rewrites.
	for (const CHAR *text : {
		"Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Filter<p2 a2>(Input<t1>))|"
		"TableEq(t1,t0);AttrsEq(a1,a0);AttrsEq(a2,a0);PredicateNot(p2,p0);PredicateNot(p1,p2)",
		"Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Filter<p2 a2>(Input<t1>))|"
		"t1 := t0;a1 := a0;a2 := a0;p2 := Not(p0);p1 := Not(p2)",
		"Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Filter<p2 a2>(Input<t1>))|"
		"TableEq(t1,t0);AttrsEq(a1,a0);AttrsEq(a2,a0);PredicateNotTrue(p2,p0);PredicateNotTrue(p1,p2)",
		"Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Filter<p2 a2>(Input<t1>))|"
		"t1 := t0;a1 := a0;a2 := a0;p2 := NotTrue(p0);p1 := NotTrue(p2)"})
	{
		CDSLRule *rule = PdslruleParseLocal(mp, text);
		GPOS_ASSERT(nullptr != rule);
		for (ULONG value = 0; value < 3; value++)
		{
			CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
			CDSLInstantiator builder(mp);
			const CDSLSymbol *source = (*rule->PfragSrc()->PopRoot()->Pdrgpsym())[0];
			const CDSLSymbol *target = (*rule->PfragTgt()->PopRoot()->Pdrgpsym())[0];
			const BOOL notTrue = EdslexprNotTrue == rule->Pexprdefs()->Pdef(target)->Edslexpr();
			// Neither path may manufacture an absent capture.
			CExpression *missing = builder.PexprInstantiatePredicate(rule, target, model);
			ok &= nullptr == missing;
			CRefCount::SafeRelease(missing);
			CExpression *leaf = CUtils::PexprScalarConstBool(mp, 1 == value, 2 == value);
			ok &= model->FBind(source, leaf);
			CExpression *result = builder.PexprInstantiatePredicate(rule, target, model);
			ok &= nullptr != result;
			if (nullptr != result)
			{
				// Double NOT preserves NULL; double IS NOT TRUE maps it to FALSE.
				ok &= CScalar::EberEvaluate(mp, result) ==
					(notTrue && 2 == value ? CScalar::EberFalse : CScalar::EberEvaluate(mp, leaf));
				auto sameOperator = [notTrue](CExpression *expr) {
					return notTrue ? COperator::EopScalarBooleanTest == expr->Pop()->Eopid() &&
						CScalarBooleanTest::EbtIsNotTrue == CScalarBooleanTest::PopConvert(expr->Pop())->Ebt()
						: CUtils::FScalarBoolOp(expr, CScalarBoolOp::EboolopNot);
				};
				ok &= sameOperator(result) && sameOperator((*result)[0]) &&
					(*(*result)[0])[0] == leaf;
				result->Release();
			}
			leaf->Release();
			model->Release();
		}
		rule->Release();
	}
	// A source pattern is evidence to match, never a target construction recipe.
	CDSLRule *capture = PdslruleParseLocal(mp,
		"Filter<Not(p0) a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
		"t1 := t0;a1 := a0;p1 := p0");
	GPOS_ASSERT(nullptr != capture);
	CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
	const CDSLSymbol *pattern = (*capture->PfragSrc()->PopRoot()->Pdrgpsym())[0];
	CExpression *leaf = CUtils::PexprScalarConstBool(mp, false);
	ok &= model->FBind(capture->Pexprdefs()->Pdef(pattern)->PsymOperand(0), leaf);
	leaf->Release();
	CDSLInstantiator builder(mp);
	CExpression *fabricated = builder.PexprInstantiatePredicate(capture, pattern, model);
	ok &= nullptr == fabricated;
	CRefCount::SafeRelease(fabricated);
	model->Release();
	capture->Release();
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
	// Flat, audited Boolean inputs expose one ordered prefix and one last
	// operand. Rebuilding them must not lose NULLs, duplicates or child identity.
	for (BOOL disjunction : {false, true})
	for (ULONG count : {3UL, 4UL})
	for (BOOL unsafe : {false, true})
	{
		const std::string op = disjunction ? "Or" : "And";
		const auto kind = disjunction ? CScalarBoolOp::EboolopOr : CScalarBoolOp::EboolopAnd;
		CDSLRule *flatRule = PdslruleParseLocal(mp, ("Filter<" + op + "(p0,p1) a0>(Input<t0>)|"
			"Filter<" + op + "(p2,p3) a1>(Input<t1>)|t1 := t0;a1 := a0;p2 := p0;p3 := p1").c_str());
		GPOS_UNITTEST_ASSERT(nullptr != flatRule);
		CColRefArray *cols = nullptr;
		CExpression *get = fix.PexprLogicalGet("flat_boolean", 1, &cols);
		CExpressionArray *children = GPOS_NEW(mp) CExpressionArray(mp);
		for (ULONG i = 0; i < count; ++i)
		{
			CExpression *child = i + 1 == count
				? CUtils::PexprScalarConstBool(mp, false, true)
				: fix.PexprPredAtom((*cols)[0]);
			if (unsafe && 0 == i)
			{
				child->Release();
				child = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarCmp(mp,
					GPOS_NEW(mp) CMDIdGPDB(IMDId::EmdidGeneral, 100402),
					GPOS_NEW(mp) CWStringConst(GPOS_WSZ_LIT("=")), IMDType::EcmptEq),
					CUtils::PexprScalarIdent(mp, (*cols)[0]), CUtils::PexprScalarConstInt4(mp, 7));
			}
			children->Append(child);
		}
		CExpression *predicate = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarBoolOp(mp, kind), children);
		CExpression *source = fix.PexprLogicalSelect(get, predicate);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		const BOOL matched = CDSLMatcher(mp, flatRule).FMatch(flatRule->PfragSrc()->PopRoot(), source, model);
		ok &= matched == !unsafe;
		CExpression *target = matched ? CDSLInstantiator(mp).PexprInstantiate(flatRule, model) : nullptr;
		if (!unsafe)
		{
			ok &= nullptr != target;
			if (nullptr != target)
			{
				CExpression *result = (*target)[1];
				ok &= CUtils::FScalarBoolOp(result, kind) && 2 == result->Arity() &&
					CUtils::FScalarBoolOp((*result)[0], kind) && count - 1 == (*result)[0]->Arity() &&
					(*result)[1] == (*predicate)[count - 1];
				for (ULONG i = 0; i + 1 < count; ++i)
					ok &= (*(*result)[0])[i] == (*predicate)[i];
			}
		}
		ok &= count == predicate->Arity();
		CRefCount::SafeRelease(target);
		model->Release(); source->Release(); predicate->Release(); get->Release(); flatRule->Release();
	}
	CDSLRule *sharedAnd = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
			"TableEq(t1,t0);AttrsEq(a1,a0);And(p2,p2) := p0;p1 := p0");
	// Unlike a repeated capture, these are independent source symbols with an
	// explicit premise. Reusing the first capture in both target operands is
	// valid only after that premise passes; distinct objects may have equal trees.
	CDSLRule *equalAnd = PdslruleParseLocal(
		mp, "Filter<And(p2,p3) a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
			"TableEq(t1,t0);AttrsEq(a1,a0);p1 := And(p2,p2);PredicateEq(p2,p3)");
	ok &= nullptr != sharedAnd && nullptr != equalAnd;
	if (nullptr != sharedAnd && nullptr != equalAnd)
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
			// columns and a prefix unequal to the last operand cannot share a capture.
			ok &= (0 == trial) == CDSLMatcher(mp, sharedAnd).FMatch(
				sharedAnd->PfragSrc()->PopRoot(), source, model);
			model->Release();
			CDSLRewriteDecision *decision = engine->PdecisionEvaluate(mp, equalAnd, source);
			ok &= (0 == trial ? EdsldecisionDuplicate :
				EdsldecisionConstraintRejected)
				== decision->Status();
			if (0 == trial)
				ok &= nullptr != decision->PexprTarget() &&
					source->Matches(decision->PexprTarget());
			GPOS_DELETE(decision);
			source->Release();
			predicate->Release();
		}
		baseSelect->Release();
		get->Release();
	}
	CRefCount::SafeRelease(sharedAnd);
	CRefCount::SafeRelease(equalAnd);
	// Unary source premises must check the captured subtree, not act as
	// construction aliases. An SRF is rejected earlier by the row-scalar domain.
	for (const CHAR *property : {"ErrorFree", "Deterministic"})
	{
		const std::string text =
			std::string("Filter<Not(Not(p0)) a0>(Input<t0>)|") +
			"Filter<p1 a1>(Input<t1>)|TableEq(t1,t0);AttrsEq(a1,a0);"
			"p1 := p0;" + property + "(p0)";
		CDSLRule *guarded = PdslruleParseLocal(mp, text.c_str());
		ok &= nullptr != guarded;
		if (nullptr == guarded)
			continue;
		CColRefArray *cols = nullptr;
		CExpression *get = fix.PexprLogicalGet("guarded_capture", 1, &cols);
		for (ULONG setReturning = 0; setReturning < 2; ++setReturning)
		{
			CExpression *atom = nullptr;
			if (setReturning)
			{
				atom = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarNullTest(mp),
					fix.PexprGenerateSeries((*cols)[0]));
			}
			else
				atom = fix.PexprPredAtom((*cols)[0]);
			CExpression *once = negate(atom), *twice = negate(once);
			CExpression *source = fix.PexprLogicalSelect(get, twice);
			CDSLRewriteDecision *decision = engine->PdecisionEvaluate(mp, guarded, source);
			ok &= (setReturning ? EdsldecisionMatchRejected : EdsldecisionReady)
				== decision->Status();
			if (!setReturning)
				ok &= nullptr != decision->PexprTarget() &&
					(*decision->PexprTarget())[1]->Matches(atom);
			GPOS_DELETE(decision);
			source->Release();
			twice->Release();
			once->Release();
			atom->Release();
		}
		get->Release();
		guarded->Release();
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
CDSLInstantiateTest::EresUnittest_NotTrueBindings()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	// Constructor/matcher checks only, not proofs or registered rewrite rules.
	CDSLRule *build = PdslruleParseLocal(mp,
		"Filter<p0 a0>(Input<t0>)|Filter<NotTrue(p0) a1>(Input<t1>)|"
		"Eq(t1,t0);Eq(a1,a0)");
	CDSLRule *capture = PdslruleParseLocal(mp,
		"Filter<NotTrue(p0) a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
		"Eq(t1,t0);Eq(a1,a0);p1 := p0");
	if (nullptr == build || nullptr == capture)
	{
		CRefCount::SafeRelease(build);
		CRefCount::SafeRelease(capture);
		return GPOS_FAILED;
	}
	BOOL ok = true;
	for (ULONG shape = 0; shape < 2; ++shape)
	{
		CExpression *get = nullptr, *base_select = nullptr;
		CColRefArray *columns = nullptr;
		BuildSelectOverAtoms(fix, 1, 1, &get, &base_select, &columns);
		CExpression *base = get;
		base->AddRef();
		if (1 == shape)
		{
			CExpression *other = fix.PexprLogicalGet("other", 1, nullptr);
			base->Release();
			base = fix.PexprLogicalInnerJoin(get, other, (*base_select)[1]);
			other->Release();
		}
		for (ULONG value = 0; value < 4; ++value)
		{
			CExpression *leaf = value < 3
				? CUtils::PexprScalarConstBool(mp, value == 1, value == 2)
				: (*base_select)[1];
			if (3 == value)
				leaf->AddRef();
			CExpression *source = fix.PexprLogicalSelect(base, leaf);
			CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
			CDSLConstraintChecker checker(mp);
			CDSLInstantiator instantiator(mp);
			CExpression *target = nullptr;
			if (CDSLMatcher(mp, build).FMatch(build->PfragSrc()->PopRoot(), source, model) &&
				checker.FCheck(build, model))
				target = instantiator.PexprInstantiate(build, model);
			const BOOL built = nullptr != target && target->Arity() == 2 &&
				COperator::EopScalarBooleanTest == (*target)[1]->Pop()->Eopid() &&
				CScalarBooleanTest::EbtIsNotTrue == CScalarBooleanTest::PopConvert((*target)[1]->Pop())->Ebt();
			ok = ok && built;
			if (built)
			{
				// Do not erase/re-evaluate the child, including opaque/error-bearing expressions.
				ok = ok && (*(*target)[1])[0]->Matches(leaf);
				if (value < 3)
					ok = ok && CScalar::EberEvaluate(mp, (*target)[1]) ==
						(value == 1 ? CScalar::EberFalse : CScalar::EberTrue);
				CDSLModel *captured = GPOS_NEW(mp) CDSLModel(mp);
				ok = ok && CDSLMatcher(mp, capture).FMatch(capture->PfragSrc()->PopRoot(), target, captured);
				const auto *operand = capture->Pexprdefs()->PdefAt(0)->PsymOperand(0);
				ok = ok && nullptr != captured->PexprPred(operand) && captured->PexprPred(operand)->Matches(leaf);
				captured->Release();
				std::string exported, error;
				ok = ok && CDSLPlanTemplate::FSlice(mp, target, "r", {"r/0"}, &exported, &error) &&
					exported.find("Filter<NotTrue(") == 0;
			}
			// No other SQL Boolean test, nor NOT itself, may satisfy NotTrue.
			for (ULONG test = 0; test <= CScalarBooleanTest::EbtSentinel; ++test)
			{
				if (test == CScalarBooleanTest::EbtIsNotTrue)
					continue;
				leaf->AddRef();
				CExpression *predicate = test == CScalarBooleanTest::EbtSentinel
					? GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopNot), leaf)
					: GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarBooleanTest(mp,
						static_cast<CScalarBooleanTest::EBoolTest>(test)), leaf);
				CExpression *wrong = fix.PexprLogicalSelect(base, predicate);
				CDSLModel *rejected = GPOS_NEW(mp) CDSLModel(mp);
				ok = ok && !CDSLMatcher(mp, capture).FMatch(capture->PfragSrc()->PopRoot(), wrong, rejected);
				rejected->Release();
				wrong->Release();
				predicate->Release();
			}
			CRefCount::SafeRelease(target);
			model->Release();
			source->Release();
			leaf->Release();
		}
		base->Release();
		base_select->Release();
		get->Release();
	}
	build->Release();
	capture->Release();
	return ok ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLInstantiateTest::EresUnittest_NullSafeEqBindings()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	// Structural constructor tests, not proved or registered production rules.
	CDSLRule *rule = PdslruleParseLocal(mp,
		"Filter<NullSafeEq(a2,a3) a0>(Input<t0>)|"
		"Filter<NullSafeEq(a8,a9) a1>(Input<t1>)|"
		"Eq(t1,t0);a1 := a0;a8 := a7;a7 := a2;a9 := a3");
	CDSLRule *repeated = PdslruleParseLocal(mp,
		"Filter<And(NullSafeEq(a2,a3),NullSafeEq(a2,a4)) a0>(Input<t0>)|"
		"Filter<And(NullSafeEq(a2,a3),NullSafeEq(a2,a4)) a1>(Input<t1>)|"
		"Eq(t1,t0);Eq(a1,a0)");
	if (nullptr == rule || nullptr == repeated)
	{
		CRefCount::SafeRelease(rule);
		CRefCount::SafeRelease(repeated);
		return GPOS_FAILED;
	}
	BOOL ok = true;
	CColRefArray *columns = nullptr;
	CExpression *get = fix.PexprLogicalGet("pairs", 3, &columns);
	// Both spellings reject unavailable comparisons and malformed vectors.
	// These are constructor checks, not claimed equivalent rewrite rules.
	CColRef *boolean = COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
		fix.Pmda()->PtMDType<IMDTypeBool>(), default_type_modifier);
	CColRef *wide = COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
		fix.Pmda()->PtMDType<IMDTypeInt8>(), default_type_modifier);
	for (const CHAR *text : {
		"InnerJoin<p0 a0 a1>(Input<t0>,Input<t1>)|Filter<p1 a2>(Input<t2>)|"
		"TableEq(t2,t0);AttrsEq(a2,a0);PredicateNullSafeEq(p1,a0,a1)",
		"InnerJoin<p0 a0 a1>(Input<t0>,Input<t1>)|Filter<NullSafeEq(a0,a1) a2>(Input<t2>)|"
		"t2 := t0;a2 := a0"})
	{
		CWStringDynamic construction_error(mp);
		CDSLRule *construction = CDSLRuleParser::PdslruleParse(mp, text, nullptr, &construction_error);
		if (nullptr == construction)
		{
			GPOS_TRACE(construction_error.GetBuffer());
			ok = false;
			continue;
		}
		for (ULONG variant = 0; variant < 6; ++variant)
		{
			// Cross-type integer equality is available in the metadata catalog.
			const BOOL comparable = 0 == variant || 5 == variant;
			CColRefArray *left = GPOS_NEW(mp) CColRefArray(mp);
			CColRefArray *right = GPOS_NEW(mp) CColRefArray(mp);
			if (3 != variant)
			{
				left->Append((*columns)[0]);
				right->Append(1 == variant ? boolean : 5 == variant ? wide : (*columns)[1]);
			}
			if (2 == variant) right->Append((*columns)[2]);
			if (4 == variant)
			{
				left->Append((*columns)[2]);
				right->Append(boolean);
			}
			CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
			ok &= model->FBind((*construction->PfragSrc()->PopRoot()->Pdrgpsym())[1], left) &&
				model->FBind((*construction->PfragSrc()->PopRoot()->Pdrgpsym())[2], right);
			CDSLInstantiator builder(mp);
			CExpression *predicate = builder.PexprInstantiatePredicate(construction,
				(*construction->PfragTgt()->PopRoot()->Pdrgpsym())[0], model);
			if ((nullptr != predicate) != comparable)
				GPOS_TRACE_FORMAT("NullSafeEq construction variant=%d present=%d rule=%s",
					variant, nullptr != predicate, text);
			ok &= (nullptr != predicate) == comparable;
			if (nullptr != predicate && comparable)
			{
				CExpression *expected = CPredicateUtils::PexprINDFConjunction(mp, left, right);
				ok &= predicate->Matches(expected);
				expected->Release();
			}
			if (!construction->Pexprdefs()->FHasBindings())
			{
				ok &= model->FBind((*(*construction->PfragSrc()->PopRoot())[0]->Pdrgpsym())[0], get);
				CDSLConstraintChecker checker(mp);
				ok &= checker.FCheck(construction, model) == comparable;
				const CDSLSymbol *output = (*construction->PfragTgt()->PopRoot()->Pdrgpsym())[0];
				// FCheck materializes a valid target predicate. Reuse that binding:
				// FBind deliberately rejects even equivalent, separately built trees.
				CExpression *bound = model->PexprPred(output);
				ok &= (nullptr != bound) == comparable;
				if (nullptr == bound)
				{
					bound = CUtils::PexprScalarConstBool(mp, true);
					ok &= model->FBind(output, bound);
					bound->Release();
				}
				ok &= checker.FCheck(construction, model) == comparable;
			}
			CRefCount::SafeRelease(predicate);
			model->Release();
			left->Release();
			right->Release();
		}
		construction->Release();
	}
	for (ULONG count : {1U, 2U, 3U})
	{
		CColRefArray *left = GPOS_NEW(mp) CColRefArray(mp);
		CColRefArray *right = GPOS_NEW(mp) CColRefArray(mp);
		for (ULONG i = 0; i < count; ++i)
		{
			left->Append((*columns)[i % 2]);
			right->Append((*columns)[1 + i % 2]);
		}
		CExpression *predicate = CPredicateUtils::PexprINDFConjunction(mp, left, right);
		CExpression *base = get;
		base->AddRef();
		if (3 == count)
		{
			CExpression *other = fix.PexprLogicalGet("opaque", 1);
			CExpression *truth = CUtils::PexprScalarConstBool(mp, true);
			base->Release();
			base = fix.PexprLogicalInnerJoin(get, other, truth);
			other->Release();
			truth->Release();
		}
		CExpression *source = fix.PexprLogicalSelect(base, predicate);
		base->Release();
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		const auto *capture = rule->Pexprdefs()->PdefAt(0);
		const BOOL matched = CDSLMatcher(mp, rule).FMatch(rule->PfragSrc()->PopRoot(), source, model);
		ok = ok && matched;
		if (matched)
		{
			ok = ok && model->PdrgpcrAttrs(capture->PsymOperand(0))->Equals(left) &&
				model->PdrgpcrAttrs(capture->PsymOperand(1))->Equals(right);
			CDSLConstraintChecker checker(mp);
			ok = ok && checker.FCheck(rule, model);
			CDSLInstantiator instantiator(mp);
			CExpression *target = instantiator.PexprInstantiate(rule, model);
			ok = ok && nullptr != target && (*target)[1]->Matches(predicate);
			CRefCount::SafeRelease(target);
		}
		model->Release();

		// The mining interface exposes the same typed operands the production
		// matcher captures, not a generic NOT(IS DISTINCT FROM) placeholder.
		std::string text, error;
		ok &= CDSLPlanTemplate::FSlice(mp, source, "r", {"r/0"}, &text, &error) &&
			error.empty() && text == "Filter<NullSafeEq(a2,a3) a0>(Input<t0>)";
		CWStringDynamic parse_error(mp);
		CDSLRule *exported = CDSLRuleParser::PdslruleParse(mp,
			(text + "|Input<t1>|Eq(t1,t0)").c_str(), nullptr, &parse_error);
		CDSLModel *export_model = GPOS_NEW(mp) CDSLModel(mp);
		const BOOL export_match = nullptr != exported && CDSLMatcher(mp, exported).FMatch(
			exported->PfragSrc()->PopRoot(), source, export_model);
		ok &= export_match;
		if (export_match)
		{
			const auto *definition = exported->Pexprdefs()->PdefAt(0);
			ok &= EdslexprNullSafeEq == definition->Edslexpr() &&
				export_model->PdrgpcrAttrs(definition->PsymOperand(0))->Equals(left) &&
				export_model->PdrgpcrAttrs(definition->PsymOperand(1))->Equals(right);
		}
		export_model->Release();
		CRefCount::SafeRelease(exported);
		ok &= CDSLPlanTemplate::FSlice(mp, source, "r", {"r"}, &text, &error) &&
			text == "Input<t0>";

		// Mixed Boolean trees keep their nesting and independent occurrences;
		// neither partial comparisons nor relation cuts are flattened away.
		predicate->AddRef();
		CExpression *nested_predicate = GPOS_NEW(mp) CExpression(mp,
			GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopOr), predicate,
			CUtils::PexprScalarEqCmp(mp, (*columns)[0], (*columns)[1]));
		CExpression *nested_source = fix.PexprLogicalSelect(source, nested_predicate);
		ok &= CDSLPlanTemplate::FSlice(mp, nested_source, "r", {"r/0/0"}, &text, &error) &&
			text == "Filter<Or(NullSafeEq(a6,a7),ValueBool(Call(h0,Args(Column(a8),Args(Column(a9),Args()))))) a2>(Filter<NullSafeEq(a4,a5) a0>(Input<t0>))";
		nested_source->Release();
		nested_predicate->Release();

		// A shared capture means equal ordered content, not equal allocation.
		// Reordering a vector must fail even when the column set is unchanged.
		CColRefArray *permuted = GPOS_NEW(mp) CColRefArray(mp);
		for (ULONG i = 0; i < count; ++i)
			permuted->Append(1 == count ? (*right)[0] : (*left)[(i + 1) % count]);
		for (ULONG variant = 0; variant < 2; ++variant)
		{
			CExpression *second = CPredicateUtils::PexprINDFConjunction(mp,
				0 == variant ? left : permuted, right);
			predicate->AddRef();
			CExpression *both = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CScalarBoolOp(mp, CScalarBoolOp::EboolopAnd), predicate, second);
			CExpression *query = fix.PexprLogicalSelect(get, both);
			CDSLModel *bound = GPOS_NEW(mp) CDSLModel(mp);
			ok = ok && CDSLMatcher(mp, repeated).FMatch(repeated->PfragSrc()->PopRoot(), query, bound) ==
				(0 == variant);
			bound->Release();
			query->Release();
			both->Release();
		}
		permuted->Release();

		// Bad column-vector values cannot produce an empty/truncated target.
		for (ULONG invalidSize : {0U, 4U})
		{
			CDSLModel *invalid = GPOS_NEW(mp) CDSLModel(mp);
			CColRefArray *bad = GPOS_NEW(mp) CColRefArray(mp);
			for (ULONG i = 0; i < invalidSize; ++i)
				bad->Append((*columns)[0]);
			ok = ok && invalid->FBind((*rule->PfragSrc()->PopRoot()->Pdrgpsym())[1], columns) &&
				invalid->FBind((*(*rule->PfragSrc()->PopRoot())[0]->Pdrgpsym())[0], get) &&
				invalid->FBind(capture->PsymOperand(0), left) &&
				invalid->FBind(capture->PsymOperand(1), bad);
			CDSLInstantiator instantiator(mp);
			CExpression *target = instantiator.PexprInstantiate(rule, invalid);
			ok = ok && nullptr == target;
			CRefCount::SafeRelease(target);
			invalid->Release();
			bad->Release();
		}
		source->Release();
		predicate->Release();
		left->Release();
		right->Release();
	}
	// Ordinary equality and IS DISTINCT FROM are not NULL-safe equality.
	CExpression *wrong[] = {
		CUtils::PexprScalarEqCmp(mp, (*columns)[0], (*columns)[1]),
		CUtils::PexprINDF(mp, CUtils::PexprScalarIdent(mp, (*columns)[0]),
			CUtils::PexprScalarIdent(mp, (*columns)[1]),
			(*columns)[0]->RetrieveType()->GetMdidForCmpType(IMDType::EcmptNEq)),
		CUtils::PexprIDF(mp, CUtils::PexprScalarIdent(mp, (*columns)[0]),
			CUtils::PexprScalarIdent(mp, (*columns)[1])),
		CUtils::PexprINDF(mp, CUtils::PexprScalarIdent(mp, (*columns)[0]),
			CUtils::PexprScalarConstInt4(mp, 1))};
	for (CExpression *predicate : wrong)
	{
		CExpression *source = fix.PexprLogicalSelect(get, predicate);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		ok = ok && !CDSLMatcher(mp, rule).FMatch(rule->PfragSrc()->PopRoot(), source, model);
		CColRefArray *left = nullptr;
		CColRefArray *right = nullptr;
		ok &= !CDSLMatchView::FNullSafeEqColumns(mp, predicate, &left, &right) &&
			nullptr == left && nullptr == right;
		CRefCount::SafeRelease(left);
		CRefCount::SafeRelease(right);
		std::string text, error;
		ok &= CDSLPlanTemplate::FSlice(mp, source, "r", {"r/0"}, &text, &error) &&
			std::string::npos == text.find("NullSafeEq(");
		model->Release();
		source->Release();
		predicate->Release();
	}
	get->Release();
	rule->Release();
	repeated->Release();
	return ok ? GPOS_OK : GPOS_FAILED;
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
