//---------------------------------------------------------------------------
// Cardinality experiment tests.
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLStatsExperimentTest.h"

#include <sstream>

#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"
#include "gpos/task/CAutoTraceFlag.h"

#include "gpopt/dsl/CDSLStatsExperiment.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/base/CDrvdPropRelational.h"
#include "gpopt/base/CUtils.h"
#include "naucrates/traceflags/traceflags.h"
#include "gpopt/operators/CScalarConst.h"
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
		GPOS_UNITTEST_FUNC(CDSLStatsExperimentTest::EresUnittest_RehashAlreadyEquivalentGroups),
	};
	return CUnittest::EresExecute(tests, GPOS_ARRAY_SIZE(tests));
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
					path.c_str(), "memo_consumes", "memo_inserted");
				context->RegisterDSLGroupExpressionOrigin(parentExpr, alias,
					path.c_str(), "memo_consumes", "memo_inserted");
				GPOS_ASSERT(context->DSLGroupExpressionOrigins(parentExpr)->size() == 1);
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
			if (nullptr != origins)
				for (const auto &origin : *origins)
				{
					inserted += origin.m_outcome == "memo_inserted";
					inherited += origin.m_outcome == "memo_rehashed";
				}
			valid = valid && inserted == 1 && inherited == (dsl ? 3 : 0);
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
	CExpression *other = fixture.PexprLogicalGet("renamed", 1);
	const std::string context = CDSLStatsExperimentSnapshot::InputContext(get);
	BOOL valid = nullptr == get->Pstats() &&
		context == CDSLStatsExperimentSnapshot::InputContext(other) &&
		std::string::npos != context.find("\"rows\":null") &&
		std::string::npos != context.find("\"memo_state\":null") &&
		std::string::npos != context.find("\"source_tree\":{\"nodes\":[{\"operator\":\"CLogicalGet\",\"arity\":0}],\"complete\":true}") &&
		std::string::npos != context.find("\"stats_source\":\"missing\"");
	const std::string keyed = CDSLStatsExperimentSnapshot::InputContext(get, mp);
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
	}

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
