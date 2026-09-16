//---------------------------------------------------------------------------
// Cardinality experiment tests.
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLStatsExperimentTest.h"
#include "gpopt/operators/CLogicalGet.h"

#include <sstream>
#include <cstdlib>

#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"
#include "gpos/task/CAutoTraceFlag.h"

#include "gpopt/dsl/CDSLStatsExperiment.h"
#include "gpopt/dsl/CDSLPlanTemplate.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/base/CDrvdPropRelational.h"
#include "gpopt/base/CUtils.h"
#include "naucrates/traceflags/traceflags.h"
#include "gpopt/operators/CScalarConst.h"
#include "gpopt/operators/CScalarSubqueryExists.h"
#include "gpopt/operators/CLogicalUnionAll.h"
#include "gpopt/operators/CLogicalConstTableGet.h"
#include "gpopt/search/CGroup.h"
#include "gpopt/search/CGroupExpression.h"
#include "gpopt/search/CGroupProxy.h"
#include "gpopt/search/CMemo.h"
#include "naucrates/statistics/CStatistics.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

GPOS_RESULT
CDSLStatsExperimentTest::EresUnittest()
{
	CUnittest tests[] = {
		GPOS_UNITTEST_FUNC(
			CDSLStatsExperimentTest::EresUnittest_ResolveSPJBoundaries),
		GPOS_UNITTEST_FUNC(
			CDSLStatsExperimentTest::EresUnittest_ExpressionFingerprintRoundTrip),
		GPOS_UNITTEST_FUNC(CDSLStatsExperimentTest::EresUnittest_StrictInput),
		GPOS_UNITTEST_FUNC(CDSLStatsExperimentTest::EresUnittest_InputContextDoesNotDeriveStats),
		GPOS_UNITTEST_FUNC(CDSLStatsExperimentTest::EresUnittest_CachedLogicalContext),
		GPOS_UNITTEST_FUNC(CDSLStatsExperimentTest::EresUnittest_ShapesAndBindings),
		GPOS_UNITTEST_FUNC(CDSLStatsExperimentTest::EresUnittest_PlanTemplateContext),
		GPOS_UNITTEST_FUNC(CDSLStatsExperimentTest::EresUnittest_RehashAlreadyEquivalentGroups),
	};
	return CUnittest::EresExecute(tests, GPOS_ARRAY_SIZE(tests));
}

GPOS_RESULT
CDSLStatsExperimentTest::EresUnittest_PlanTemplateContext()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fixture(mp);
	CColRefArray *cols = nullptr;
	CExpression *get = fixture.PexprLogicalGet("outer", 1, &cols);
	CExpression *inner = fixture.PexprLogicalGet("inner", 1);
	CExpression *exists = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarSubqueryExists(mp), inner);
	CExpression *select = fixture.PexprLogicalSelect(get, exists);
	const std::string artifact =
		CDSLPlanTemplate::Serialize(mp, select);
	const std::string query_context =
		CDSLStatsExperimentSnapshot::InputContext(select, mp, true);
	const std::string candidate_context =
		CDSLStatsExperimentSnapshot::InputContext(select, mp, false);
	std::string selection_error;
	const BOOL selection_valid =
		CDSLPlanTemplate::FValidateSelection(
			select, "r", {"r/0", "r/1"}, &selection_error);
	std::string exists_slice;
	std::string exists_error;
	const BOOL exists_sliced = CDSLPlanTemplate::FSlice(
		mp, select, "r", {"r/0", "r/1"}, &exists_slice, &exists_error);
	CExpression *predicate = fixture.PexprEqConst((*cols)[0], 7);
	CExpression *plain_select = fixture.PexprLogicalSelect(get, predicate);
	CExpression *outer_predicate = fixture.PexprEqConst((*cols)[0], 8);
	CExpression *nested_select =
		fixture.PexprLogicalSelect(plain_select, outer_predicate);
	std::string sliced;
	std::string slice_error;
	const BOOL sliced_ok = CDSLPlanTemplate::FSlice(
		mp, plain_select, "r", {"r/0"}, &sliced, &slice_error);
	CColRefArray *right_cols = nullptr;
	CExpression *right = fixture.PexprLogicalGet("right", 1, &right_cols);
	CExpression *join_predicate =
		fixture.PexprEqPred((*cols)[0], (*right_cols)[0]);
	CExpression *join =
		fixture.PexprLogicalInnerJoin(get, right, join_predicate);
	std::string join_slice;
	std::string join_error;
	const BOOL join_sliced = CDSLPlanTemplate::FSlice(
		mp, join, "r", {"r/0", "r/1"}, &join_slice, &join_error);
	CColRefArray *grouping = GPOS_NEW(mp) CColRefArray(mp);
	grouping->Append((*cols)[0]);
	CExpression *dedup = fixture.PexprLogicalGbAgg(get, grouping);
	grouping->Release();
	std::string dedup_slice;
	std::string dedup_error;
	const BOOL dedup_sliced = CDSLPlanTemplate::FSlice(
		mp, dedup, "r", {"r/0"}, &dedup_slice, &dedup_error);
	CWStringDynamic request_errors(mp);
	CDSLStatsExperimentSnapshot *request =
		CDSLStatsExperimentSnapshot::PsnapshotLoadBuffer(mp,
			"experiment: template-selection\n"
			"discover: false\n"
			"template_root: r\n"
			"template_cuts: [r/0]\n"
			"cardinalities:\n",
			plain_select, &request_errors);
	const std::string requested_slice = nullptr == request ? "" :
		request->TemplateSelectionArtifact(plain_select);
	const BOOL valid = std::string::npos != artifact.find(
		"\"schema\":\"pgorca.dsl.plan-template.v1\"") &&
		std::string::npos != artifact.find(
			"\"path\":\"r\",\"orca_operator\":\"CLogicalSelect\"") &&
		std::string::npos != artifact.find("\"Exists\"") &&
		std::string::npos != artifact.find(
			"\"relational_children\":[\"r/0\",\"r/1\"]") &&
		std::string::npos != artifact.find(
			"\"path\":\"r/0\",\"orca_operator\":\"CLogicalGet\"") &&
		std::string::npos != artifact.find(
			"\"dsl_views\":[\"Input\"],\"requires_cut\":false") &&
		std::string::npos != artifact.find(
			"\"path\":\"r/1\",\"orca_operator\":\"CLogicalGet\"") &&
		std::string::npos != artifact.find("\"tree\":[{\"path\":\"s1\"") &&
		std::string::npos != artifact.find("\"operator_text\":\"CScalarSubqueryExists\"") &&
		std::string::npos != artifact.find("\"complete\":true}") &&
		std::string::npos != query_context.find("\"plan_template\":" + artifact) &&
		std::string::npos == candidate_context.find("\"plan_template\"") &&
		selection_valid && selection_error.empty() && exists_sliced &&
		exists_slice == "Exists(Input<t0>,Input<t1>)" &&
		exists_error.empty() &&
		!CDSLPlanTemplate::FValidateSelection(
			select, "r/0", {"r/1"}, &selection_error) &&
		selection_error == "cut path is outside selected root: r/1" &&
		!CDSLPlanTemplate::FValidateSelection(
			select, "r", {"r/0", "r/0"}, &selection_error) &&
		selection_error == "duplicate cut path: r/0" && sliced_ok &&
		sliced == "Filter<p0 a0 a1>(Input<t0>)" && slice_error.empty() &&
		join_sliced &&
		join_slice == "InnerJoin<p0 a0 a1>(Input<t0>,Input<t1>)" &&
		join_error.empty() &&
		dedup_sliced && dedup_slice == "Proj*<a0 s0>(Input<t0>)" &&
		dedup_error.empty() &&
		nullptr != request && request->FHasTemplateSelection() &&
		requested_slice ==
			"{\"schema\":\"pgorca.dsl.plan-slice.v1\",\"root_path\":\"r\","
			"\"cut_paths\":[\"r/0\"],\"status\":\"ok\","
			"\"source_template\":\"Filter<p0 a0 a1>(Input<t0>)\",\"error\":null}" &&
		!CDSLPlanTemplate::FValidateSelection(
			nested_select, "r", {"r/0", "r/0/0"}, &selection_error) &&
		selection_error == "cut paths must form an antichain";
	GPOS_DELETE(request);
	dedup->Release();
	join->Release();
	join_predicate->Release();
	right->Release();
	nested_select->Release();
	outer_predicate->Release();
	plain_select->Release();
	predicate->Release();
	select->Release();
	exists->Release();
	get->Release();
	return valid ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLStatsExperimentTest::EresUnittest_RehashAlreadyEquivalentGroups()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	// Four bag-equivalent VALUES orders, partitioned between two parent groups.
	// Try every partition so the test does not depend on hash bucket traversal.
	for (ULONG dsl = 0; dsl < 2; ++dsl)
	{
		for (ULONG pair = 1; pair < 4; ++pair)
		{
			CDSLTestFixture fixture(mp);
			CAutoTraceFlag trace(EopttracePrintDSLRule, dsl != 0);
			COptCtxt *context = COptCtxt::PoctxtFromTLS();
			const CHAR *identity = "Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
				"TableEq(t1,t0);AttrsEq(a1,a0);PredicateEq(p1,p0)";
			CDSLRule *rule = CDSLRuleParser::PdslruleParse(mp, identity, "EQ", nullptr);
			CDSLRule *alias = CDSLRuleParser::PdslruleParse(mp, identity, "EQ", nullptr);
			GPOS_ASSERT(nullptr != rule && nullptr != alias);
			CMemo memo(mp);
			const auto insert = [&](CExpression *expr, CGroup *owner, CGroupArray *children,
				CGroupExpression **result = nullptr)
			{
				CGroupExpression *origin = nullptr;
				if (dsl && children->Size() > 0)
				{
					CGroupProxy child((*children)[0]);
					origin = child.PgexprFirst();
				}
				expr->Pop()->AddRef();
				CGroupExpression *gexpr = GPOS_NEW(mp) CGroupExpression(mp,
					expr->Pop(), children, nullptr != origin ? CXform::ExfDSLRuleSelect : CXform::ExfInvalid,
					origin, false);
				CGroupExpression *canonical = nullptr;
				CGroup *group = memo.PgroupInsert(owner, expr, gexpr, &canonical);
				GPOS_ASSERT(nullptr != gexpr->Pgroup());
				GPOS_ASSERT(canonical == gexpr);
				const ULONG size = memo.UlGrpExprs();
				expr->Pop()->AddRef();
				children->AddRef();
				CGroupExpression *duplicate = GPOS_NEW(mp) CGroupExpression(mp,
					expr->Pop(), children, CXform::ExfInvalid, nullptr, false);
				CGroup *same = memo.PgroupInsert(group, expr, duplicate, &canonical);
				GPOS_ASSERT(same == group && canonical == gexpr &&
					nullptr == duplicate->Pgroup() && size == memo.UlGrpExprs());
				duplicate->Release();
				if (nullptr != result)
					*result = gexpr;
				return group;
			};
			CExpression *predicate = CUtils::PexprScalarConstBool(mp, true);
			CGroup *scalar = insert(predicate, nullptr, GPOS_NEW(mp) CGroupArray(mp));
			CColRefArray *columns = GPOS_NEW(mp) CColRefArray(mp);
			columns->Append(fixture.PcrCreateInt4("v"));
			CExpression *inputs[4];
			CGroup *leaves[4];
			for (ULONG i = 0; i < 4; ++i)
			{
				IDatum2dArray *rows = GPOS_NEW(mp) IDatum2dArray(mp);
				for (ULONG j = 0; j < 4; ++j)
				{
					CExpression *value = CUtils::PexprScalarConstInt4(mp, (i + j) % 4);
					IDatum *datum = CScalarConst::PopConvert(value->Pop())->GetDatum();
					datum->AddRef();
					IDatumArray *row = GPOS_NEW(mp) IDatumArray(mp);
					row->Append(datum);
					rows->Append(row);
					value->Release();
				}
				columns->AddRef();
				inputs[i] = GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CLogicalConstTableGet(mp, columns, rows));
				leaves[i] = insert(inputs[i], nullptr, GPOS_NEW(mp) CGroupArray(mp));
			}
			CGroup *parents[2] = {nullptr, nullptr};
			for (ULONG i = 0; i < 4; ++i)
			{
				const ULONG owner = (i == 0 || i == pair) ? 0 : 1;
				CExpression *filter = fixture.PexprLogicalSelect(inputs[i], predicate);
				CGroupArray *children = GPOS_NEW(mp) CGroupArray(mp);
				children->Append(leaves[i]);
				children->Append(scalar);
				CGroupExpression *parentExpr = nullptr;
				parents[owner] = insert(filter, parents[owner], children, &parentExpr);
				const std::string path = "r/" + std::to_string(i);
				context->RegisterDSLGroupExpressionOrigin(parentExpr, rule,
					path.c_str(), "memo_consumes", "memo_inserted", i + 1);
				context->RegisterDSLGroupExpressionOrigin(parentExpr, alias,
					path.c_str(), "memo_consumes", "memo_inserted", i + 1);
				GPOS_ASSERT(context->DSLGroupExpressionOrigins(parentExpr)->size() == 1);
				// Same rule/path, different producing attempt: retain both only
				// while tracing. Canonical rule aliases of one attempt still dedup.
				context->RegisterDSLGroupExpressionOrigin(parentExpr, alias,
					path.c_str(), "memo_consumes", "memo_inserted", i + 5);
				GPOS_ASSERT(context->DSLGroupExpressionOrigins(parentExpr)->size() == (dsl ? 2 : 1));
				filter->Release();
			}
			for (ULONG i = 0; i < memo.UlpGroups(); ++i)
			{
				CGroup *group = memo.Pgroup(i);
				CGroupProxy proxy(group);
				proxy.SetState(CGroup::estExploring);
				proxy.SetState(CGroup::estExplored);
				for (CGroupExpression *expr = proxy.PgexprFirst(); nullptr != expr;
					 expr = proxy.PgexprNext(expr))
				{
					expr->SetState(CGroupExpression::estExploring);
					expr->SetState(CGroupExpression::estExplored);
				}
			}
			memo.SetRoot(parents[0]);
			for (ULONG i = 0; i < 3; ++i)
				CMemo::MarkDuplicates(leaves[i], leaves[3]);
			memo.GroupMerge();
			BOOL valid = CGroup::FDuplicateGroups(parents[0], parents[1]) &&
				memo.PgroupRoot()->UlGExprs() == 1 && leaves[3]->UlGExprs() == 4 &&
				!CGroup::FReachable(mp, leaves[3], memo.PgroupRoot());
			const ULONG count = memo.UlGrpExprs();
			CGroupProxy root(memo.PgroupRoot());
			const auto *origins = context->DSLGroupExpressionOrigins(root.PgexprFirst());
			ULONG inserted = 0;
			ULONG inherited = 0;
			ULONG producers = 0;
			if (nullptr != origins)
				for (const auto &origin : *origins)
				{
					inserted += origin.m_outcome == "memo_inserted";
					inherited += origin.m_outcome == "memo_rehashed";
					if (dsl)
					{
						valid = valid && origin.m_candidate_sequence >= 1 && origin.m_candidate_sequence <= 8;
						if (origin.m_candidate_sequence >= 1 && origin.m_candidate_sequence <= 8)
							producers |= 1UL << (origin.m_candidate_sequence - 1);
					}
				}
			valid = valid && inserted == (dsl ? 2 : 1) && inherited == (dsl ? 6 : 0) &&
				(!dsl || producers == 255);
			const auto originCount = nullptr == origins ? 0 : origins->size();
			memo.GroupMerge();
			valid = valid && memo.UlGrpExprs() == count && nullptr != origins &&
				origins->size() == originCount;
			for (CExpression *input : inputs)
				input->Release();
			predicate->Release();
			columns->Release();
			alias->Release();
			rule->Release();
			if (!valid)
				return GPOS_FAILED;
		}
	}
	return GPOS_OK;
}

GPOS_RESULT
CDSLStatsExperimentTest::EresUnittest_ShapesAndBindings()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fixture(mp);
	CColRefArray *cols = nullptr;
	CExpression *get = fixture.PexprLogicalGet("private_name", 1, &cols);
	CExpression *pred = fixture.PexprEqConst((*cols)[0], 7);
	CExpression *other = fixture.PexprEqConst((*cols)[0], 999);
	CExpression *select = fixture.PexprLogicalSelect(get, pred);
	const std::string shape = CDSLStatsExperimentSnapshot::ExpressionShape(select);
	BOOL valid = nullptr == select->Pstats() && nullptr == get->Pstats() &&
		std::string::npos == shape.find("private_name") &&
		std::string::npos != shape.find("\"CScalarCmp\":1") &&
		std::string::npos != shape.find("\"depth\":3") &&
		CDSLStatsExperimentSnapshot::ExpressionShape(pred) ==
			CDSLStatsExperimentSnapshot::ExpressionShape(other);
	const std::string select_route = CDSLStatsExperimentSnapshot::RouteContext(select);
	valid = valid && std::string::npos != select_route.find(
		"\"relational_tree\":{\"encoding\":\"preorder_operator_arity\","
		"\"nodes\":\"CLogicalSelect/1 CLogicalGet/0\",\"complete\":true}");
	CExpression *inner = fixture.PexprLogicalGet("private_inner", 1);
	CExpression *exists = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarSubqueryExists(mp), inner);
	CExpression *exists_select = fixture.PexprLogicalSelect(get, exists);
	valid = valid && std::string::npos !=
		CDSLStatsExperimentSnapshot::RouteContext(exists_select).find(
			"\"nodes\":\"CLogicalSelect/2 CLogicalGet/0 CLogicalGet/0\"");
	exists_select->Release();
	exists->Release();
	CExpression *pattern = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp));
	valid = valid && std::string::npos !=
		CDSLStatsExperimentSnapshot::ExpressionShape(pattern).find("\"pattern_nodes\":1");
	pattern->Release();
	CExpressionArray *children = GPOS_NEW(mp) CExpressionArray(mp);
	CColRef2dArray *inputCols = GPOS_NEW(mp) CColRef2dArray(mp);
	for (ULONG i = 0; i < 4100; ++i)
	{
		get->AddRef();
		children->Append(get);
		cols->AddRef();
		inputCols->Append(cols);
	}
	cols->AddRef();
	CExpression *wide = GPOS_NEW(mp) CExpression(mp,
		GPOS_NEW(mp) CLogicalUnionAll(mp, cols, inputCols), children);
	const std::string partial = CDSLStatsExperimentSnapshot::ExpressionShape(wide);
	valid = valid && std::string::npos != partial.find("\"complete\":false") &&
		std::string::npos != partial.find("\"nodes\":4096");
	const std::string wide_context = CDSLStatsExperimentSnapshot::InputContext(wide);
	valid = valid && wide_context.size() >= 19 &&
		wide_context.substr(wide_context.size() - 19) == "],\"complete\":true}}";
	const size_t tree_begin = wide_context.find("\"source_tree\":");
	ULONG observed_gets = 0;
	for (size_t pos = wide_context.find("CLogicalGet", tree_begin);
		pos != std::string::npos; pos = wide_context.find("CLogicalGet", pos + 1))
		++observed_gets;
	valid = valid && observed_gets == 4100 && nullptr == wide->Pstats() && nullptr == get->Pstats();
	const std::string route_context = CDSLStatsExperimentSnapshot::RouteContext(wide);
	valid = valid && std::string::npos != route_context.find("\"encoding\":\"preorder_operator_arity\"") &&
		std::string::npos == route_context.find("logical_properties") &&
		std::string::npos == route_context.find("relation_oid");
	observed_gets = 0;
	for (size_t pos = route_context.find("CLogicalGet"); pos != std::string::npos;
		 pos = route_context.find("CLogicalGet", pos + 1))
		++observed_gets;
	valid = valid && observed_gets == 8200;
	const auto records = CDSLStatsExperimentSnapshot::ContextRecords(7, "input_context", wide_context);
	std::string restored;
	for (size_t i = 0; i < records.size(); ++i)
	{
		const std::string &record = records[i];
		valid = valid && record.size() < 8192 && std::string::npos !=
			record.find("\"part\":" + std::to_string(i) + ",\"parts\":" + std::to_string(records.size()));
		const size_t begin = record.find("\"fragment\":\"") + 12;
		const size_t end = record.find('"', begin);
		for (size_t pos = begin; pos < end; pos += 2)
			restored.push_back(static_cast<CHAR>(std::strtoul(record.substr(pos, 2).c_str(), nullptr, 16)));
	}
	valid = valid && records.size() > 1 && restored == wide_context;
	const std::string small = "{\"empty\":true}";
	const auto direct = CDSLStatsExperimentSnapshot::ContextRecords(8, "binding_context", small);
	valid = valid && direct.size() == 1 && std::string::npos != direct[0].find("\"value\":" + small);
	const auto route = CDSLStatsExperimentSnapshot::ContextRecords(9, "route_input_context", small);
	valid = valid && route.size() == 1 && std::string::npos !=
		route[0].find("\"field\":\"route_input_context\"");
	wide->Release();
	pred->Pop()->AddRef();
	(*pred)[0]->AddRef();
	(*pred)[1]->AddRef();
	CExpression *reversed = GPOS_NEW(mp) CExpression(mp, pred->Pop(), (*pred)[1], (*pred)[0]);
	valid = valid && CDSLStatsExperimentSnapshot::ExpressionShape(pred) ==
		CDSLStatsExperimentSnapshot::ExpressionShape(reversed) &&
		CDSLStatsExperimentSnapshot::InputContext(pred) !=
		CDSLStatsExperimentSnapshot::InputContext(reversed) && nullptr == pred->Pstats();
	reversed->Release();
	CWStringDynamic errors(mp);
	CDSLRule *rule = CDSLRuleParser::PdslruleParse(mp,
		"Filter<p0 a0>(Input<t0>)|Input<t1>|TableEq(t1,t0)", "EQ", &errors);
	if (nullptr != rule)
	{
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		const std::string empty = CDSLStatsExperimentSnapshot::BindingContext(rule, model);
		valid = valid && std::string::npos != empty.find("\"bound\":false") &&
			std::string::npos != empty.find("\"total_symbols\":2");
		for (ULONG i = 0; i < rule->PfragSrc()->Pdrgpsym()->Size(); ++i)
		{
			const CDSLSymbol *sym = (*rule->PfragSrc()->Pdrgpsym())[i];
			if (EdslsymTable == sym->Esymkind())
				valid = model->FBind(sym, select) && valid;
			else if (EdslsymPred == sym->Esymkind())
				valid = model->FBind(sym, pred) && valid;
		}
		const std::string bound = CDSLStatsExperimentSnapshot::BindingContext(rule, model);
		valid = valid && std::string::npos != bound.find("after_evaluation") &&
			std::string::npos == bound.find("\"bound\":false") &&
			std::string::npos != bound.find("\"CLogicalSelect\":1") &&
			std::string::npos != bound.find("\"memo_group\":null") &&
			std::string::npos != bound.find("\"stats_source\":\"missing\",\"rows\":null") &&
			std::string::npos != bound.find("\"omitted_symbols\":0") && nullptr == select->Pstats();
		model->Release();
		rule->Release();
	}
	else
		valid = false;
	select->Release();
	other->Release();
	pred->Release();
	get->Release();
	return valid ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLStatsExperimentTest::EresUnittest_InputContextDoesNotDeriveStats()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fixture(mp);
	CColRefArray *cols = nullptr;
	CExpression *get = fixture.PexprLogicalGet("first", 1, &cols);
	CTableDescriptor *table = CLogicalGet::PopConvert(get->Pop())->Ptabdesc();
	table->AddRef();
	CExpression *other = fixture.PexprLogicalGet(table, "renamed");
	const std::string context = CDSLStatsExperimentSnapshot::InputContext(get);
	BOOL valid = nullptr == get->Pstats() &&
		context == CDSLStatsExperimentSnapshot::InputContext(other) &&
		std::string::npos != context.find("\"rows\":null") &&
		std::string::npos != context.find("\"memo_state\":null") &&
		std::string::npos != context.find("\"source_tree\":{\"request_binding\":null,\"nodes\":[{\"operator\":\"CLogicalGet\",\"arity\":0,\"relation_oid\":") &&
		std::string::npos != context.find("\"request_index\":null") &&
		std::string::npos == context.find("\"relation_oid\":null") &&
		std::string::npos != context.find("\"stats_source\":\"missing\"");
	const std::string keyed = CDSLStatsExperimentSnapshot::InputContext(get, mp);
	const std::string query_input = CDSLStatsExperimentSnapshot::InputContext(get, mp, true);
	valid = valid && std::string::npos != query_input.find("\"capture\":\"before_memo_initialization\"") &&
		std::string::npos != query_input.find("\"scope\":\"query_after_preprocessing\"") &&
		std::string::npos != query_input.find("\"rows\":null") && nullptr == get->Pstats();
	const auto query_records = CDSLStatsExperimentSnapshot::ContextRecords(1, "query_input_context", query_input);
	valid = valid && !query_records.empty() &&
		std::string::npos != query_records[0].find("\"field\":\"query_input_context\"");
	valid = valid && nullptr == get->Pstats() &&
		std::string::npos != keyed.find("\"reference_key\":\"" +
			CDSLStatsExperimentSnapshot::Fingerprint(mp, get) + "\"") &&
		keyed != CDSLStatsExperimentSnapshot::InputContext(other, mp);
	CExpressionArray *children = GPOS_NEW(mp) CExpressionArray(mp);
	CColRef2dArray *input_cols = GPOS_NEW(mp) CColRef2dArray(mp);
	for (ULONG i = 0; i < 9; ++i)
	{
		get->AddRef();
		children->Append(get);
		cols->AddRef();
		input_cols->Append(cols);
	}
	cols->AddRef();
	CExpression *join = GPOS_NEW(mp) CExpression(mp,
		GPOS_NEW(mp) CLogicalUnionAll(mp, cols, input_cols), children);
	const std::string wide = CDSLStatsExperimentSnapshot::InputContext(join);
	valid = valid && nullptr == join->Pstats() && nullptr == get->Pstats() &&
		std::string::npos != wide.find("\"relational_children\":9") &&
		std::string::npos != wide.find("\"omitted_children\":1");
	CGroup *group = GPOS_NEW(mp) CGroup(mp, false);
	{
		CGroupProxy proxy(group);
		proxy.SetId(0);
		proxy.InitProperties(GPOS_NEW(mp) CDrvdPropRelational(mp));
		get->Pop()->AddRef();
		CGroupExpression *gexpr = GPOS_NEW(mp) CGroupExpression(mp, get->Pop(),
			GPOS_NEW(mp) CGroupArray(mp), CXform::ExfInvalid, nullptr, false);
		proxy.Insert(gexpr);
		get->Pop()->AddRef();
		CExpression *bound = GPOS_NEW(mp) CExpression(mp, get->Pop(), gexpr);
		ULongPtrArray *ids = GPOS_NEW(mp) ULongPtrArray(mp);
		proxy.InitStats(gpnaucrates::CStatistics::MakeDummyStats(mp, ids, CDouble(42.0)));
		ids->Release();
		const std::string cached = CDSLStatsExperimentSnapshot::InputContext(bound);
		valid = valid && nullptr == bound->Pstats() && group->Pstats()->Rows() == CDouble(42.0) &&
			std::string::npos != cached.find("\"memo_group_expressions\":1") &&
			std::string::npos != cached.find("\"group_explored\":false") &&
			std::string::npos != cached.find("\"expression_implemented\":false") &&
			std::string::npos != cached.find("\"logical_properties\":null") &&
			std::string::npos != cached.find("\"stats_source\":\"memo_group\"") &&
			std::string::npos != cached.find("\"rows\":42");
		bound->Release();
		get->Pop()->AddRef();
		bound = GPOS_NEW(mp) CExpression(mp, get->Pop(), gexpr);
		const std::string direct = CDSLStatsExperimentSnapshot::InputContext(bound);
		valid = valid && nullptr != bound->Pstats() &&
			std::string::npos != direct.find("\"stats_source\":\"expression\"") &&
			std::string::npos != direct.find("\"rows\":42");
		bound->Release();
	}
	(void) group->FResetStats();
	valid = valid && nullptr == group->Pstats() &&
		0 == COptCtxt::PoctxtFromTLS()->UlDSLStatsLifecycleEvents();
	group->Release();
	join->Release();
	other->Release();
	get->Release();
	return valid ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLStatsExperimentTest::EresUnittest_CachedLogicalContext()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fixture(mp);
	CColRefArray *columns = nullptr;
	CExpression *table = fixture.PexprLogicalGet("cached", 2, &columns);
	columns->AddRef();
	CExpression *get = GPOS_NEW(mp) CExpression(mp,
		GPOS_NEW(mp) CLogicalConstTableGet(mp, columns, GPOS_NEW(mp) IDatum2dArray(mp)));
	table->Release();
	// Explicit fixture preparation, not a side effect of observation.
	CDrvdProp *props = get->PdpDerive();
	props->AddRef();
	CGroup *group = GPOS_NEW(mp) CGroup(mp, false);
	BOOL valid;
	{
		CGroupProxy proxy(group);
		proxy.SetId(0);
		proxy.InitProperties(props);
		get->Pop()->AddRef();
		CGroupExpression *gexpr = GPOS_NEW(mp) CGroupExpression(mp, get->Pop(),
			GPOS_NEW(mp) CGroupArray(mp), CXform::ExfInvalid, nullptr, false);
		proxy.Insert(gexpr);
		get->Pop()->AddRef();
		CExpression *bound = GPOS_NEW(mp) CExpression(mp, get->Pop(), gexpr);
		const std::string context = CDSLStatsExperimentSnapshot::InputContext(bound);
		valid = nullptr == bound->Pstats() && nullptr == group->Pstats() &&
			group->Pdp() == props && group->UlGExprs() == 1 &&
			std::string::npos != context.find("\"output_columns\":2") &&
			std::string::npos != context.find("\"outer_columns\":0") &&
			std::string::npos != context.find("\"source\":\"complete_memo_group\"");
		bound->Release();
	}
	group->Release();
	get->Release();
	return valid ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLStatsExperimentTest::EresUnittest_ResolveSPJBoundaries()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fixture(mp);
	CColRefArray *a_cols = nullptr;
	CColRefArray *b_cols = nullptr;
	CExpression *a = fixture.PexprLogicalGet("a", 2, &a_cols);
	CExpression *predicate = fixture.PexprPredAtom((*a_cols)[0]);
	CExpression *inner_select = fixture.PexprLogicalSelect(a, predicate);
	predicate->Release();
	predicate = fixture.PexprPredAtom((*a_cols)[1]);
	CExpression *outer_select =
		fixture.PexprLogicalSelect(inner_select, predicate);
	predicate->Release();
	CExpression *b = fixture.PexprLogicalGet("b", 1, &b_cols);
	CExpression *join_predicate =
		fixture.PexprEqPred((*a_cols)[0], (*b_cols)[0]);
	CExpression *join = fixture.PexprLogicalInnerJoin(
		outer_select, b, join_predicate);
	join_predicate->Release();

	CWStringDynamic errors(mp);
	const CHAR *config =
		"experiment: boundary-test\n"
		"cardinalities:\n"
		"  - relations: [a]\n"
		"    rows: 7\n"
		"  - relations: [b, a]\n"
		"    rows: 11.5\n";
	CDSLStatsExperimentSnapshot *snapshot =
		CDSLStatsExperimentSnapshot::PsnapshotLoadBuffer(
			mp, config, join, &errors);
	BOOL valid = nullptr != snapshot;
	if (valid)
	{
		const auto *base = snapshot->Ptarget(outer_select);
		const auto *joined = snapshot->Ptarget(join);
		valid = nullptr != base && 7.0 == base->m_rows && nullptr != joined &&
			11.5 == joined->m_rows && nullptr == snapshot->Ptarget(inner_select) &&
			nullptr == snapshot->Ptarget(a);
		ULONG index = 99;
		valid = valid && snapshot->FRequestIndex(outer_select->Pop(), &index) && index == 0 &&
			snapshot->FRequestIndex(join->Pop(), &index) && index == 1 &&
			!snapshot->FRequestIndex(inner_select->Pop(), &index) && index == 1 &&
			!snapshot->FRequestIndex(a->Pop(), &index) && nullptr == join->Pstats();
	}

	GPOS_DELETE(snapshot);
	errors.Reset();
	snapshot = CDSLStatsExperimentSnapshot::PsnapshotLoadBuffer(
		mp,
		"experiment: discover-test\n"
		"discover: true\n"
		"cardinalities:\n",
		join, &errors);
	valid = valid && nullptr != snapshot && 5 == snapshot->UlTargets() &&
		nullptr != snapshot->Ptarget(outer_select) &&
		nullptr != snapshot->Ptarget(b) && nullptr != snapshot->Ptarget(join) &&
		nullptr != snapshot->Ptarget(inner_select) &&
		nullptr != snapshot->Ptarget(a);
	if (nullptr != snapshot)
	{
		ULONG index = 99;
		valid = valid && !snapshot->FRequestIndex(join->Pop(), &index) &&
			!snapshot->FRequestIndex(a->Pop(), &index) && index == 99;
	}
	GPOS_DELETE(snapshot);
	join->Release();
	b->Release();
	outer_select->Release();
	inner_select->Release();
	a->Release();
	return valid ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLStatsExperimentTest::EresUnittest_ExpressionFingerprintRoundTrip()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fixture(mp);
	CColRefArray *cols = nullptr;
	CExpression *get = fixture.PexprLogicalGet("a", 1, &cols);
	CExpression *agg = fixture.PexprLogicalGbAgg(get, cols);
	CWStringDynamic errors(mp);
	CDSLStatsExperimentSnapshot *snapshot =
		CDSLStatsExperimentSnapshot::PsnapshotLoadBuffer(
			mp,
			"experiment: discover-expression\n"
			"discover: true\n"
			"cardinalities:\n",
			agg, &errors);
	const SDSLStatsExperimentTarget *discovered =
		nullptr == snapshot ? nullptr : snapshot->Ptarget(agg);
	BOOL valid = nullptr != discovered && discovered->m_relations.empty() &&
		"CLogicalGbAgg" == discovered->m_operator &&
		16 == discovered->m_fingerprint.size();
	std::string fingerprint =
		nullptr == discovered ? "" : discovered->m_fingerprint;
	GPOS_DELETE(snapshot);

	std::ostringstream config;
	config << "experiment: inject-expression\n"
			   << "cardinalities:\n"
			   << "  - expression: " << fingerprint << "\n"
			   << "    operator: CLogicalGbAgg\n"
			   << "    rows: 13\n";
	errors.Reset();
	snapshot = CDSLStatsExperimentSnapshot::PsnapshotLoadBuffer(
		mp, config.str().c_str(), agg, &errors);
	const SDSLStatsExperimentTarget *injected =
		nullptr == snapshot ? nullptr : snapshot->Ptarget(agg);
	valid = valid && nullptr != injected && injected->m_inject &&
		13.0 == injected->m_rows && nullptr == snapshot->Ptarget(get);

	GPOS_DELETE(snapshot);
	agg->Release();
	get->Release();
	return valid ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLStatsExperimentTest::EresUnittest_StrictInput()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fixture(mp);
	CExpression *get = fixture.PexprLogicalGet("a", 1);
	CWStringDynamic errors(mp);
	const CHAR *duplicate =
		"experiment: invalid\n"
		"cardinalities:\n"
		"  - relations: [a]\n"
		"    rows: 1\n"
		"  - relations: [a]\n"
		"    rows: 2\n";
	CDSLStatsExperimentSnapshot *snapshot =
		CDSLStatsExperimentSnapshot::PsnapshotLoadBuffer(
			mp, duplicate, get, &errors);
	BOOL valid = nullptr == snapshot && 0 < errors.Length();
	GPOS_DELETE(snapshot);

	// Parsing requests is independent of runtime target resolution. In
	// particular, no metadata lookup or derived statistics are needed here.
	std::string id;
	std::vector<SDSLStatsExperimentRequest> requests;
	BOOL discover = false;
	const CHAR *unbound = "experiment: requested\ndiscover: true\ncardinalities:\n"
		"- relations: [z, a]\n  rows: 12.25\n"
		"- expression: ABCDEF0123456789\n  operator: CLogicalGet\n  rows: 96\n";
	errors.Reset();
	valid = valid && CDSLStatsExperimentSnapshot::FParseRequests(
		unbound, &id, &requests, &discover, &errors);
	valid = valid && id == "requested" && discover && requests.size() == 2 &&
		requests[0].m_aliases == std::vector<std::string>({"a", "z"}) &&
		requests[0].m_rows == 12.25 && requests[1].m_rows == 96 &&
		requests[1].m_fingerprint == "abcdef0123456789" &&
		requests[1].m_operator == "CLogicalGet" && 0 == errors.Length();
	snapshot = CDSLStatsExperimentSnapshot::PsnapshotLoadBuffer(mp, unbound, get, &errors);
	valid = valid && nullptr == snapshot && 0 < errors.Length();
	GPOS_DELETE(snapshot);
	for (const CHAR *invalid : {duplicate,
		"experiment: bad\ncardinalities:\n- expression: abcdef0123456789\n rows: 2\n",
		"experiment: bad\ncardinalities:\n- relations: [a]\n rows: 2\n unknown: 3\n"})
	{
		errors.Reset();
		valid = valid && !CDSLStatsExperimentSnapshot::FParseRequests(
			invalid, &id, &requests, &discover, &errors) && 0 < errors.Length();
		// Failed calls must not publish partial inputs or erase the last result.
		valid = valid && id == "requested" && discover && requests.size() == 2;
	}

	// Positive fractional estimates below MinRows violate downstream join
	// scale-factor invariants. Reject at the experiment boundary, never clamp.
	for (const CHAR *rows : {"0", "0.5", "nan", "inf"})
	{
		std::ostringstream invalid_rows;
		invalid_rows << "experiment: invalid-rows\ncardinalities:\n"
					 << "  - relations: [a]\n    rows: " << rows << "\n";
		errors.Reset();
		snapshot = CDSLStatsExperimentSnapshot::PsnapshotLoadBuffer(
			mp, invalid_rows.str().c_str(), get, &errors);
		valid = valid && nullptr == snapshot && 0 < errors.Length();
		GPOS_DELETE(snapshot);
		errors.Reset();
		valid = valid && !CDSLStatsExperimentSnapshot::FParseRequests(
			invalid_rows.str().c_str(), &id, &requests, &discover, &errors) &&
			0 < errors.Length();
	}
	errors.Reset();
	valid = valid && CDSLStatsExperimentSnapshot::FParseRequests(
		"experiment: observe\ncardinalities:\n", &id, &requests, &discover, &errors) &&
		id == "observe" && !discover && requests.empty();

	CColRefArray *cols = nullptr;
	CExpression *repeated = fixture.PexprLogicalGet("repeated", 1, &cols);
	CExpression *predicate = fixture.PexprEqPred((*cols)[0], (*cols)[0]);
	CExpression *join =
		fixture.PexprLogicalInnerJoin(repeated, repeated, predicate);
	predicate->Release();
	std::ostringstream ambiguous;
	ambiguous << "experiment: ambiguous\n"
			  << "cardinalities:\n"
			  << "  - expression: "
			  << CDSLStatsExperimentSnapshot::Fingerprint(mp, repeated) << "\n"
			  << "    operator: CLogicalGet\n"
			  << "    rows: 3\n";
	errors.Reset();
	snapshot = CDSLStatsExperimentSnapshot::PsnapshotLoadBuffer(
		mp, ambiguous.str().c_str(), join, &errors);
	valid = valid && nullptr == snapshot && 0 < errors.Length();
	GPOS_DELETE(snapshot);
	join->Release();
	repeated->Release();
	get->Release();
	return valid ? GPOS_OK : GPOS_FAILED;
}
