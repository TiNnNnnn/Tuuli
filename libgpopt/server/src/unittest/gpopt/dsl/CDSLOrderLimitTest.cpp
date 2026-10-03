#include "unittest/gpopt/dsl/CDSLOrderLimitTest.h"

#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringConst.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/CDistributionSpecHashed.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLPlanTemplate.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/dsl/CDSLRulePrefixIndex.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CLogicalAssert.h"
#include "gpopt/operators/CLogicalMaxOneRow.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CLogicalSequenceProject.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarSubqueryExists.h"
#include "gpopt/operators/CScalarWindowFunc.h"
#include "gpopt/xforms/CXformUtils.h"
#include "naucrates/md/CMDIdGPDB.h"
#include "naucrates/md/CMDAggregateGPDB.h"
#include "naucrates/md/CMDTypeInt4GPDB.h"
#include "naucrates/dxl/gpdb_types.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

namespace
{
COrderSpec *
PosOne(CMemoryPool *mp, CColRef *pcr, EDslSortDir edslsort)
{
	const BOOL fAsc = EdslsortAsc == edslsort;
	IMDId *pmdid = pcr->RetrieveType()->GetMdidForCmpType(
		fAsc ? IMDType::EcmptL : IMDType::EcmptG);
	pmdid->AddRef();
	COrderSpec *pos = GPOS_NEW(mp) COrderSpec(mp);
	pos->Append(pmdid, pcr, fAsc ? COrderSpec::EntLast : COrderSpec::EntFirst);
	return pos;
}

CExpression *
PexprLimit(CMemoryPool *mp, CExpression *pexprChild, COrderSpec *pos,
		   BOOL fHasCount, LINT offset, LINT count)
{
	pexprChild->AddRef();
	return GPOS_NEW(mp) CExpression(mp,
		GPOS_NEW(mp) CLogicalLimit(
			mp, pos, true /*global*/, fHasCount, false /*top DML*/),
		pexprChild, CUtils::PexprScalarConstInt8(mp, offset),
		CUtils::PexprScalarConstInt8(mp, count, !fHasCount /*is null*/));
}

CDSLRule *
Prule(CMemoryPool *mp, const CHAR *szRule)
{
	return CDSLRuleParser::PdslruleParse(mp, szRule, "EQ", nullptr);
}

CExpression *
PexprWindowRows(CMemoryPool *mp, CDSLTestFixture &fix,
				CExpression *pexprChild, CColRef *pcrPartition,
				CColRef *pcrArgument, COrderSpec *order = nullptr,
				CWindowFrame *frame = nullptr)
{
	CExpressionArray *pdrgpexprDist = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexprDist->Append(CUtils::PexprScalarIdent(mp, pcrPartition));
	CDistributionSpec *pds =
		GPOS_NEW(mp) CDistributionSpecHashed(pdrgpexprDist, true);
	COrderSpecArray *pdrgpos = GPOS_NEW(mp) COrderSpecArray(mp);
	CWindowFrameArray *pdrgpwf = GPOS_NEW(mp) CWindowFrameArray(mp);
	if (nullptr != order) pdrgpos->Append(order);
	if (nullptr != frame) pdrgpwf->Append(frame);

	CScalarWindowFunc *popWindow = GPOS_NEW(mp) CScalarWindowFunc(
		mp, GPOS_NEW(mp) CMDIdGPDB(IMDId::EmdidGeneral, GPDB_INT4_AGG_MAX),
		GPOS_NEW(mp) CMDIdGPDB(IMDId::EmdidGeneral, GPDB_INT4_OID),
		GPOS_NEW(mp) CWStringConst(mp, GPOS_WSZ_LIT("max")),
		CScalarWindowFunc::EwsImmediate, false, false, true);
	CExpressionArray *pdrgpexprArgs = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexprArgs->Append(CUtils::PexprScalarIdent(mp, pcrArgument));
	CExpression *pexprWindow =
		GPOS_NEW(mp) CExpression(mp, popWindow, pdrgpexprArgs);
	CColRef *pcrOutput = fix.PcrCreateInt4("win");
	CExpressionArray *pdrgpexprElems = GPOS_NEW(mp) CExpressionArray(mp);
	pdrgpexprElems->Append(
		CUtils::PexprScalarProjectElement(mp, pcrOutput, pexprWindow));
	CExpression *pexprList = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprElems);

	pexprChild->AddRef();
	return CUtils::PexprLogicalSequenceProject(
		mp, COperator::EsptypeGlobalOneStep, pds, pdrgpos, pdrgpwf,
		pexprChild, pexprList);
}

BOOL
FBindingRoundTrip(CMemoryPool *mp, const CHAR *text, CExpression *source,
				  BOOL reject = false)
{
	CWStringDynamic error(mp);
	CDSLRule *rule = CDSLRuleParser::PdslruleParse(mp, text, "EQ", &error);
	CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, rule);
	CDSLConstraintChecker checker(mp);
	CDSLInstantiator instantiator(mp);
	CExpression *target = nullptr;
	BOOL valid = nullptr != rule &&
		matcher.FMatch(rule->PfragSrc()->PopRoot(), source, model) &&
		checker.FCheck(rule, model);
	if (valid)
	{
		CDSLRulePrefixIndex index(mp);
		index.Insert(rule, 0, source->Pop()->Eopid());
		CDSLRuleArray *candidates = index.PdrgpruleCandidates(mp, source);
		valid = 1 == candidates->Size();
		candidates->Release();
		target = instantiator.PexprInstantiate(rule, model);
		valid = valid && (reject ? nullptr == target : nullptr != target && target->Matches(source));
	}
	if (!valid)
		GPOS_TRACE_FORMAT("Order/window binding check failed (parsed=%d, target=%d, reject=%d): %s; %ls",
			nullptr != rule, nullptr != target, reject, text, error.GetBuffer());
	CRefCount::SafeRelease(target);
	model->Release();
	CRefCount::SafeRelease(rule);
	return valid;
}
}  // namespace

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(
			CDSLOrderLimitTest::EresUnittest_FusedLimitSortRoundTrip),
		GPOS_UNITTEST_FUNC(
			CDSLOrderLimitTest::EresUnittest_PlanTemplateSlice),
		GPOS_UNITTEST_FUNC(
			CDSLOrderLimitTest::EresUnittest_SortOverLimitStaysNested),
		GPOS_UNITTEST_FUNC(
			CDSLOrderLimitTest::EresUnittest_PlainLimitRejectsHiddenOrder),
		GPOS_UNITTEST_FUNC(
			CDSLOrderLimitTest::EresUnittest_OffsetOnlyLimitRoundTrip),
		GPOS_UNITTEST_FUNC(
			CDSLOrderLimitTest::EresUnittest_ExactOrderSpecRoundTrip),
		GPOS_UNITTEST_FUNC(
			CDSLOrderLimitTest::EresUnittest_TargetScalarConstants),
		GPOS_UNITTEST_FUNC(
			CDSLOrderLimitTest::EresUnittest_WindowRowsRoundTrip),
		GPOS_UNITTEST_FUNC(CDSLOrderLimitTest::EresUnittest_WindowFilterBindings),
		GPOS_UNITTEST_FUNC(CDSLOrderLimitTest::EresUnittest_WindowContextBindings),
		GPOS_UNITTEST_FUNC(
			CDSLOrderLimitTest::EresUnittest_RowNumberConstructiveTarget),
		GPOS_UNITTEST_FUNC(
			CDSLOrderLimitTest::EresUnittest_MaxOneRowReplacement),
	};
	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_PlanTemplateSlice()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *pdrgpcr = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("template_limit", 1, &pdrgpcr);
	CExpression *pexprLimit = PexprLimit(
		mp, pexprGet, PosOne(mp, (*pdrgpcr)[0], EdslsortAsc), true, 0, 7);
	std::string dsl;
	std::string error;
	BOOL valid = CDSLPlanTemplate::FSlice(
		mp, pexprLimit, "r", {"r/0"}, &dsl, &error) &&
		dsl == "Limit<n0 n1>(SortAsc<a0>(Input<t0>))" && error.empty();
	CExpression *pexprWindow = PexprWindowRows(
		mp, fix, pexprGet, (*pdrgpcr)[0], (*pdrgpcr)[0]);
	valid = valid && CDSLPlanTemplate::FSlice(
		mp, pexprWindow, "r", {"r/0"}, &dsl, &error) &&
		dsl == "WindowRows<a0 o0 w0>(Input<t0>)" && error.empty();
	pexprWindow->Release();
	pexprLimit->Release();
	pexprGet->Release();
	return valid ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_RowNumberConstructiveTarget()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *pdrgpcr = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("ranked", 1, &pdrgpcr);
	CDSLRule *prule = Prule(mp,
		"Input<t0>|RowNumber<a0 o0 r0>(Input<t1>)|"
		"TableEq(t1,t0);AttrsEmpty(a0);OrderEmpty(o0);RankAttrs(a1,r0)");
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	CDSLConstraintChecker checker(mp);
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget = nullptr;
	GPOS_RESULT eres = GPOS_OK;
	if (nullptr == prule ||
		!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGet, pmodel) ||
		!checker.FCheck(prule, pmodel) ||
		nullptr == (pexprTarget = instantiator.PexprInstantiate(prule, pmodel)) ||
		COperator::EopLogicalSequenceProject != pexprTarget->Pop()->Eopid() ||
		2 != pexprTarget->Arity() || 1 != (*pexprTarget)[1]->Arity())
	{
		eres = GPOS_FAILED;
	}

	CRefCount::SafeRelease(pexprTarget);
	for (const CHAR *metadata : {
		"OrderEmpty(o0);RankAttrs(a1,r0)",
		"RankAttrs(a1,r0);OrderEmpty(o0)",
		"o0 := o1;OrderEmpty(o1);r0 := r1;RankAttrs(a1,r1)",
		"RankAttrs(a1,r0);RankAttrs(a2,r0);OrderEmpty(o0)",
		"OrderEmpty(o0);RankAttrs(a1,r0);ErrorFree(r0);Deterministic(r0)"})
	{
		const std::string text = "Input<t0>|RowNumber<a0 o0 r0>(Input<t1>)|"
			"t1 := t0;AttrsEmpty(a0);" + std::string(metadata);
		CDSLRule *typed = Prule(mp, text.c_str());
		CDSLModel *bindings = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher capture(mp, typed);
		CExpression *target = nullptr;
		if (nullptr == typed || !capture.FMatch(typed->PfragSrc()->PopRoot(), pexprGet, bindings) ||
			!checker.FCheck(typed, bindings)) eres = GPOS_FAILED;
		else
		{
			CDSLInstantiator build(mp);
			target = build.PexprInstantiate(typed, bindings);
			if (nullptr == target || target->Pop()->Eopid() != COperator::EopLogicalSequenceProject ||
				(*target)[1]->Arity() != 1) eres = GPOS_FAILED;
		}
		CRefCount::SafeRelease(target);
		bindings->Release();
		CRefCount::SafeRelease(typed);
	}
	CExpression *pexprLive = CXformUtils::PexprWindowWithRowNumber(
		mp, pexprGet, pdrgpcr);
	CDSLRule *pruleIdentity = Prule(mp,
		"RowNumber<a0 o0 r0>(Input<t0>)|"
		"RowNumber<a1 o1 r1>(Input<t1>)|"
		"TableEq(t1,t0);AttrsEq(a1,a0);OrderEq(o1,o0);RankEq(r1,r0);"
		"OrderEmpty(o1);ErrorFree(r0);Deterministic(r0)");
	CDSLModel *pmodelIdentity = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcherIdentity(mp, pruleIdentity);
	CDSLInstantiator instantiatorIdentity(mp);
	CExpression *pexprIdentity = nullptr;
	if (GPOS_OK == eres &&
		(nullptr == pruleIdentity ||
		 !matcherIdentity.FMatch(pruleIdentity->PfragSrc()->PopRoot(),
								 pexprLive, pmodelIdentity) ||
		 !checker.FCheck(pruleIdentity, pmodelIdentity) ||
		 nullptr == (pexprIdentity =
			 instantiatorIdentity.PexprInstantiate(pruleIdentity, pmodelIdentity)) ||
		 !pexprIdentity->Matches(pexprLive)))
	{
		eres = GPOS_FAILED;
	}
	CRefCount::SafeRelease(pexprIdentity);
	pmodelIdentity->Release();
	CRefCount::SafeRelease(pruleIdentity);
	// RankAttrs must reuse captured rank identities through aliases. A partition
	// column is not a rank merely because both target symbols were unbound.
	for (ULONG mode = 0; mode < 4; mode++)
	{
		const std::string aliases =
			"TableEq(t1,t0);AttrsEq(a1,a0);OrderEq(o1,o0);RankEq(r1,r0)";
		const std::string rank = mode == 3 ? "RankAttrs(a1,r1)" :
			mode == 1 ? "RankAttrs(a2,r0)" : "RankAttrs(a2,r1)";
		const std::string text =
			"RowNumber<a0 o0 r0>(Input<t0>)|RowNumber<a1 o1 r1>(Input<t1>)|" +
			(mode == 2 ? rank + ";" + aliases : aliases + ";" + rank);
		CDSLRule *rule = Prule(mp, text.c_str());
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher capture(mp, rule);
		if (nullptr == rule || !capture.FMatch(rule->PfragSrc()->PopRoot(), pexprLive, model) ||
			checker.FCheck(rule, model) != (mode != 3)) eres = GPOS_FAILED;
		if (nullptr != rule && mode != 3)
		{
			CDSLInstantiator rebuild(mp);
			CExpression *target = rebuild.PexprInstantiate(rule, model);
			if (nullptr == target || !target->Matches(pexprLive)) eres = GPOS_FAILED;
			CRefCount::SafeRelease(target);
			for (ULONG i = 0; i < rule->Pdrgpcon()->Size(); i++)
			{
				const auto *constraint = (*rule->Pdrgpcon())[i];
				if (EdslconRankAttrs != constraint->Edslcon()) continue;
				const auto *columns = model->PdrgpcrAttrs((*constraint->Pdrgpsym())[0]);
				if (nullptr == columns || columns->Size() != 1 ||
					(*columns)[0] != (*pexprLive)[1]->DeriveDefinedColumns()->PcrFirst())
					eres = GPOS_FAILED;
			}
		}
		model->Release();
		CRefCount::SafeRelease(rule);
	}
	if (!FBindingRoundTrip(mp,
		"RowNumber<a0 o0 r0>(Input<t0>)|RowNumber<a1 o1 r1>(Input<t1>)|"
		"t1 := t0;a1 := a0;o1 := o0;r1 := r2;r2 := r0;ErrorFree(r0)", pexprLive))
	{
		eres = GPOS_FAILED;
	}
	pexprLive->Release();
	// Removing a nested rank can remove a required partition/order column.
	// Reusing its output identity above the retained child is also invalid.
	for (BOOL order : {false, true})
	{
		CExpression *inner = CXformUtils::PexprWindowWithRowNumber(mp, pexprGet, pdrgpcr);
		CColRef *rank = (*inner)[1]->DeriveDefinedColumns()->PcrFirst();
		CColRefArray *partition = GPOS_NEW(mp) CColRefArray(mp);
		COrderSpecArray *orders = GPOS_NEW(mp) COrderSpecArray(mp);
		if (order) orders->Append(PosOne(mp, rank, EdslsortAsc));
		else partition->Append(rank);
		CExpression *outer = CXformUtils::PexprWindowWithRowNumber(
			mp, inner, partition, nullptr, orders);
		const std::string source =
			"RowNumber<a0 o0 r0>(RowNumber<a2 o2 r2>(Input<t0>))|";
		const std::string target =
			"RowNumber<a1 o1 r1>(RowNumber<a3 o3 r3>(Input<t1>))|"
			"t1 := t0;a1 := a0;o1 := o0;a3 := a2;o3 := o2;r3 := r2;";
		if (!FBindingRoundTrip(mp, (source + target + "r1 := r0").c_str(), outer))
			eres = GPOS_FAILED;
		for (const std::string &invalid : {
			source + target + "r1 := r2", source +
			"RowNumber<a1 o1 r1>(Input<t1>)|"
			"t1 := t0;a1 := a0;o1 := o0;r1 := r0"})
		{
			if (!FBindingRoundTrip(mp, invalid.c_str(), outer, true))
				eres = GPOS_FAILED;
		}
		outer->Release();
		inner->Release();
		partition->Release();
		orders->Release();
	}
	pmodel->Release();
	CRefCount::SafeRelease(prule);
	pexprGet->Release();
	return eres;
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_MaxOneRowReplacement()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CExpression *pexprGet = fix.PexprLogicalGet("scalar_input", 1, nullptr);
	pexprGet->AddRef();
	CExpression *pexprLive = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalMaxOneRow(mp), pexprGet);

	CDSLRule *prule = Prule(mp,
		"MaxOneRow(Input<t0>)|AssertMaxOneRow(Input<t1>)|t1 := t0");
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	GPOS_RESULT eres = GPOS_OK;
	if (nullptr == prule ||
		!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprLive, pmodel))
	{
		eres = GPOS_FAILED;
	}

	CExpression *pexprTarget = nullptr;
	if (GPOS_OK == eres)
	{
		CDSLConstraintChecker checker(mp);
		CDSLInstantiator instantiator(mp);
		pexprTarget = instantiator.PexprInstantiate(prule, pmodel);
		if (!checker.FCheck(prule, pmodel) || nullptr == pexprTarget ||
			COperator::EopLogicalAssert != pexprTarget->Pop()->Eopid() ||
			2 != pexprTarget->Arity() ||
			COperator::EopLogicalSequenceProject !=
				(*pexprTarget)[0]->Pop()->Eopid() ||
			gpos::CException::ExmiSQLMaxOneRow !=
				CLogicalAssert::PopConvert(pexprTarget->Pop())
					->Pexc()->Minor())
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	CRefCount::SafeRelease(prule);
	pexprLive->Release();
	pexprGet->Release();
	return eres;
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_WindowContextBindings()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	BOOL ok = true;
	for (BOOL framed : {false, true})
	{
		CColRefArray *columns = nullptr;
		CExpression *input = fix.PexprLogicalGet("context_window", 2, &columns);
		CWindowFrame *frame = framed ? GPOS_NEW(mp) CWindowFrame(mp,
			CWindowFrame::EfsRows, CWindowFrame::EfbUnboundedPreceding,
			CWindowFrame::EfbCurrentRow, nullptr, nullptr,
			CWindowFrame::EfesNone, 0, 0, 0, true, false) : nullptr;
		CExpression *source = PexprWindowRows(mp, fix, input, (*columns)[0], (*columns)[1],
			framed ? PosOne(mp, (*columns)[1], EdslsortDesc) : nullptr, frame);
		const std::string pair = framed
			? "Window<a0 o0 m0 w0>(Input<t0>)|Window<a1 o1 m1 w1>(Input<t1>)|m1 := m0;"
			: "WindowRows<a0 o0 w0>(Input<t0>)|WindowRows<a1 o1 w1>(Input<t1>)|";
		const std::string bindings = "t1 := t0;a1 := a0;o1 := o0;Context(n0) := w0;"
			"Column(a2) := n0;n1 := Column(a2);w1 := w3;w3 := Context(w2,n1);w2 := w0;"
			"ErrorFree(w1);Deterministic(w1);ErrorFree(n1)";
		ok = FBindingRoundTrip(mp, (pair + bindings).c_str(), source) && ok;
		CDSLRule *unsafe = Prule(mp, (pair + "t1 := t0;a1 := a0;o1 := o0;"
			"Context(n0) := w0;Column(a2) := n0;n1 := Subquery(a0,t0);"
			"w1 := Context(w0,n1);ErrorFree(w1)").c_str());
		GPOS_UNITTEST_ASSERT(nullptr != unsafe);
		CDSLModel *unsafe_model = GPOS_NEW(mp) CDSLModel(mp);
		ok = CDSLMatcher(mp, unsafe).FMatch(unsafe->PfragSrc()->PopRoot(), source, unsafe_model) &&
			!CDSLConstraintChecker(mp).FCheck(unsafe, unsafe_model) && ok;
		unsafe_model->Release();
		unsafe->Release();
		// A constructor check, not an equivalence rule: replace exactly the
		// captured argument with another existing int4 column. Keep the function
		// head, output label and complete window spec from the source carrier.
		const std::string changed = pair + "t1 := t0;a1 := a0;o1 := o0;Context(n0) := w0;"
			"Column(a2) := n0;n1 := Column(a0);w1 := Context(w0,n1)";
		CDSLRule *rule = Prule(mp, changed.c_str());
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		GPOS_UNITTEST_ASSERT(nullptr != rule);
		BOOL matched = CDSLMatcher(mp, rule).FMatch(rule->PfragSrc()->PopRoot(), source, model);
		CDSLInstantiator instantiator(mp);
		CExpression *target = matched ? instantiator.PexprInstantiate(rule, model) : nullptr;
		CExpression *source_item = (*(*source)[1])[0];
		CExpression *target_item = nullptr == target ? nullptr : (*(*target)[1])[0];
		ok = ok && nullptr != target && source->Pop()->Matches(target->Pop()) &&
			source_item->Pop()->Matches(target_item->Pop()) &&
			(*source_item)[0]->Pop()->Matches((*target_item)[0]->Pop()) &&
			CScalarIdent::PopConvert((*(*target_item)[0])[0]->Pop())->Pcr() == (*columns)[0] &&
			CScalarIdent::PopConvert((*(*source_item)[0])[0]->Pop())->Pcr() == (*columns)[1];
		CRefCount::SafeRelease(target);
		model->Release();
		rule->Release();
		source->Release();
		input->Release();
	}
	return ok ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_WindowFilterBindings()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	BOOL ok = true;
	for (BOOL framed : {false, true})
	for (ULONG wrappers = 0; wrappers < 3; ++wrappers)
	{
		const std::string window = framed ? "Window<a0 o0 m0 w0>" : "WindowRows<a0 o0 w0>";
		const std::string target_window = framed ? "Window<a3 o1 m1 w1>" : "WindowRows<a3 o1 w1>";
		const std::string source_prefix = (wrappers ? "Compute<e0 a8 s0>(" : std::string()) +
			std::string(2 == wrappers ? "Filter<p2 a6>(" : "");
		const std::string target_prefix = (wrappers ? "Compute<e1 a9 s1>(" : std::string()) +
			std::string(2 == wrappers ? "Filter<p3 a7>(" : "");
		const std::string tree = source_prefix + window + "(Filter<p0 a1 a2>(Input<t0>))" +
			std::string(wrappers, ')') + "|Filter<p1 a4 a5>(" + target_prefix + target_window +
			"(Input<t1>)" + std::string(wrappers, ')') + ")|";
		const std::string premises =
			"CorrelationEquality(p0,a1,a2);AttrsNonEmpty(a1);AttrsSub(a1,a0);"
			"DepsDisjoint(a0,a2);DepsDisjoint(o0,a2);DepsDisjoint(w0,a2);"
			"Deterministic(p0);ErrorFree(p0);ErrorFree(p1);ErrorFree(w0);ErrorFree(w1)" +
			std::string(framed ? ";DepsDisjoint(m0,a2)" : "") +
			(wrappers ? ";DepsDisjoint(p0,s0);ErrorFree(e0);Deterministic(e0);ErrorFree(e1);Deterministic(e1)" : "") +
			(2 == wrappers ? ";ErrorFree(p2);Deterministic(p2);ErrorFree(p3);Deterministic(p3);Deterministic(p1)" : "");
		for (ULONG scenario = 0; scenario < 2 + wrappers; ++scenario)
		{
			const ULONG partition = 1 == scenario ? 1 : 0;
			const BOOL external_items = 2 == scenario;
			const BOOL external_residual = 3 == scenario;
			CColRefArray *columns = nullptr;
			CExpression *input = fix.PexprLogicalGet("motion", 2, &columns);
			CColRef *outer = fix.PcrCreateInt4("outer");
			CExpression *predicate = CUtils::PexprScalarEqCmp(mp, (*columns)[0], outer);
			CExpression *filtered = fix.PexprLogicalSelect(input, predicate);
			CWindowFrame *frame = framed ? GPOS_NEW(mp) CWindowFrame(mp,
				CWindowFrame::EfsRows, CWindowFrame::EfbUnboundedPreceding,
				CWindowFrame::EfbUnboundedFollowing, nullptr, nullptr,
				CWindowFrame::EfesNone, 0, 0, 0, true, false) : nullptr;
			CExpression *source = PexprWindowRows(mp, fix, filtered,
				(*columns)[partition], (*columns)[1],
				framed ? GPOS_NEW(mp) COrderSpec(mp) : nullptr, frame);
			if (2 == wrappers)
			{
				CColRef *window_column = CScalarProjectElement::PopConvert(
					(*(*source)[1])[0]->Pop())->Pcr();
				CExpression *residual = CUtils::PexprScalarEqCmp(mp, window_column,
					external_residual ? outer : (*columns)[1]);
				CExpression *selected = fix.PexprLogicalSelect(source, residual);
				residual->Release();
				source->Release();
				source = selected;
			}
			if (wrappers)
			{
				CExpression *items = GPOS_NEW(mp) CExpression(mp,
					GPOS_NEW(mp) CScalarProjectList(mp), CUtils::PexprScalarProjectElement(
						mp, fix.PcrCreateInt4("computed"), external_items
							? CUtils::PexprScalarIdent(mp, outer) : CUtils::PexprScalarConstInt4(mp, 1)));
				source = GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CLogicalProject(mp), source, items);
			}
			CExpression *legacy = nullptr;
			for (BOOL bindings : {false, true})
			{
				const std::string aliases = bindings
					? "t1 := t0;a3 := a0;o1 := o0;w1 := w0;p1 := p0;a4 := a1;a5 := a2;"
					: "TableEq(t1,t0);AttrsEq(a3,a0);OrderEq(o1,o0);WindowEq(w1,w0);"
					  "PredicateEq(p1,p0);AttrsEq(a4,a1);AttrsEq(a5,a2);";
				const std::string compute_aliases = !wrappers ? "" : bindings
					? "e1 := e0;a9 := a8;s1 := s0;"
					: "ExprListEq(e1,e0);AttrsEq(a9,a8);SchemaEq(s1,s0);";
				const std::string residual_aliases = 2 != wrappers ? "" : bindings
					? "p3 := p2;a7 := a6;" : "PredicateEq(p3,p2);AttrsEq(a7,a6);";
				CDSLRule *rule = Prule(mp, (tree + aliases + compute_aliases + residual_aliases + (framed
					? (bindings ? "m1 := m0;" : "FrameEq(m1,m0);") : "") + premises).c_str());
				GPOS_UNITTEST_ASSERT(nullptr != rule);
				CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
				const BOOL matched = CDSLMatcher(mp, rule).FMatch(rule->PfragSrc()->PopRoot(), source, model);
				const BOOL accepted = matched && CDSLConstraintChecker(mp).FCheck(rule, model);
				// A two-argument Filter is local-only. Typed capture rejects
				// an outer residual immediately; legacy target construction rejects it later.
				const BOOL expected_match = !external_residual || !bindings;
				const BOOL expected_accept = expected_match && 0 == partition;
				ok &= matched == expected_match && accepted == expected_accept;
				if (matched != expected_match || accepted != expected_accept)
					GPOS_TRACE_FORMAT("Window motion framed=%d wrappers=%lu bindings=%d partition=%lu matched=%d accepted=%d",
						framed, wrappers, bindings, partition, matched, accepted);
				if (accepted)
				{
					CDSLRulePrefixIndex index(mp);
					index.Insert(rule, 0, source->Pop()->Eopid());
					CDSLRuleArray *candidates = index.PdrgpruleCandidates(mp, source);
					ok &= 1 == candidates->Size();
					if (1 != candidates->Size())
						GPOS_TRACE_FORMAT("Window motion prefix framed=%d bindings=%d candidates=%lu",
							framed, bindings, candidates->Size());
					candidates->Release();
					CExpression *target = CDSLInstantiator(mp).PexprInstantiate(rule, model);
					// Typed references retain the captured Compute's outer scope.
					// Legacy construction only accepts child-local dependencies.
					const BOOL expected_target = !external_residual && (!external_items || bindings);
					ok &= (nullptr != target) == expected_target;
					if (nullptr != target)
					{
						ok &= COperator::EopLogicalSelect == target->Pop()->Eopid() &&
							(*target)[1]->Matches(predicate);
						CExpression *before = source;
						CExpression *after = (*target)[0];
						for (ULONG depth = 0; depth <= wrappers; ++depth)
						{
							ok &= before->Pop()->Matches(after->Pop()) && (*before)[1]->Matches((*after)[1]);
							before = (*before)[0];
							after = (*after)[0];
						}
						ok &= before->Matches(filtered) && after->Matches(input);
					}
					if (expected_target && nullptr == target)
						GPOS_TRACE_FORMAT("Window motion target missing framed=%d bindings=%d", framed, bindings);
					if (!bindings) legacy = target;
					else
					{
						ok &= external_items ? nullptr == legacy :
							nullptr != legacy && nullptr != target && legacy->Matches(target);
						if (nullptr != legacy && nullptr != target && !legacy->Matches(target))
							GPOS_TRACE_FORMAT("Window motion targets differ framed=%d", framed);
						CRefCount::SafeRelease(target);
					}
				}
				model->Release();
				rule->Release();
			}
			CRefCount::SafeRelease(legacy);
			source->Release();
			filtered->Release();
			predicate->Release();
			input->Release();
		}
	}
	return ok ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_WindowRowsRoundTrip()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *pdrgpcr = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("windowed", 2, &pdrgpcr);
	CExpression *pexprLive =
		PexprWindowRows(mp, fix, pexprGet, (*pdrgpcr)[0], (*pdrgpcr)[1]);

	CDSLRule *prule = Prule(mp,
		"WindowRows<a0 o0 w0>(Input<t0>)|"
		"WindowRows<a1 o1 w1>(Input<t1>)|"
		"TableEq(t1,t0);AttrsEq(a1,a0);OrderEq(o1,o0);WindowEq(w1,w0);"
		"ErrorFree(w0);ErrorFree(w1)");
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp, prule);
	GPOS_RESULT eres = GPOS_OK;
	if (nullptr == prule ||
		!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprLive, pmodel))
	{
		eres = GPOS_FAILED;
	}

	CExpression *pexprTarget = nullptr;
	if (GPOS_OK == eres)
	{
		CDSLConstraintChecker checker(mp);
		CDSLInstantiator instantiator(mp);
		if (!checker.FCheck(prule, pmodel) ||
			nullptr == (pexprTarget = instantiator.PexprInstantiate(prule, pmodel)) ||
			COperator::EopLogicalSequenceProject !=
				pexprTarget->Pop()->Eopid() ||
			!pexprTarget->Matches(pexprLive))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	CRefCount::SafeRelease(prule);
	if (!FBindingRoundTrip(mp,
		"WindowRows<a0 o0 w0>(Input<t0>)|WindowRows<a1 o1 w1>(Input<t1>)|"
		"t1 := t0;a1 := a0;o1 := o0;w1 := w2;w2 := w0;ErrorFree(w0)", pexprLive))
	{
		eres = GPOS_FAILED;
	}
	// A row-level predicate may contain a window in its subquery's scope.
	pexprLive->AddRef();
	CExpression *exists = GPOS_NEW(mp) CExpression(mp,
		GPOS_NEW(mp) CScalarSubqueryExists(mp), pexprLive);
	CExpression *filtered = fix.PexprLogicalSelect(pexprGet, exists);
	if (!FBindingRoundTrip(mp,
		"Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
		"p1 := p0;a1 := a0;t1 := t0", filtered))
		eres = GPOS_FAILED;
	filtered->Release();
	exists->Release();
	pexprLive->Release();
	// Nested windows expose distinct partition/order/frame captures. Reusing
	// the outer item list must not silently ignore a different target spec.
	CExpression *inner = PexprWindowRows(mp, fix, pexprGet,
		(*pdrgpcr)[1], (*pdrgpcr)[0], PosOne(mp, (*pdrgpcr)[1], EdslsortDesc),
		GPOS_NEW(mp) CWindowFrame(mp, CWindowFrame::EfsRows,
			CWindowFrame::EfbUnboundedPreceding, CWindowFrame::EfbUnboundedFollowing,
			nullptr, nullptr, CWindowFrame::EfesNone, 0, 0, 0, true, false));
	CExpression *outer = PexprWindowRows(mp, fix, inner,
		(*pdrgpcr)[0], (*pdrgpcr)[1], PosOne(mp, (*pdrgpcr)[0], EdslsortAsc),
		GPOS_NEW(mp) CWindowFrame(mp, CWindowFrame::EfsRows,
			CWindowFrame::EfbUnboundedPreceding, CWindowFrame::EfbCurrentRow,
			nullptr, nullptr, CWindowFrame::EfesNone, 0, 0, 0, true, false));
	const std::string tree =
		"Window<a0 o0 m0 w0>(Window<a2 o2 m2 w2>(Input<t0>))|"
		"Window<a1 o1 m1 w1>(Window<a4 o4 m4 w4>(Input<t1>))|"
		"t1 := t0;w1 := w3;w3 := w0;a4 := a2;o4 := o2;m4 := m2;w4 := w2;";
	if (!FBindingRoundTrip(mp,
		(tree + "a1 := a0;o1 := o0;m1 := m3;m3 := m0").c_str(), outer))
		eres = GPOS_FAILED;
	for (const CHAR *bindings : {
		"a1 := a2;o1 := o0;m1 := m0", "a1 := a0;o1 := o2;m1 := m0",
		"a1 := a0;o1 := o0;m1 := m2"})
	{
		if (!FBindingRoundTrip(mp, (tree + bindings).c_str(), outer, true))
			eres = GPOS_FAILED;
	}
	if (!FBindingRoundTrip(mp,
		"Window<a0 o0 m0 w0>(Window<a2 o2 m2 w2>(Input<t0>))|"
		"Window<a1 o1 m1 w1>(Window<a3 o3 m3 w3>(Input<t1>))|"
		"t1 := t0;a1 := a2;o1 := o2;m1 := m2;w1 := w2;"
		"a3 := a2;o3 := o2;m3 := m2;w3 := w2", outer, true))
		eres = GPOS_FAILED;
	outer->Release();
	inner->Release();
	pexprGet->Release();
	return eres;
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_FusedLimitSortRoundTrip()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *pdrgpcr = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("ordered", 2, &pdrgpcr);
	CExpression *pexprLive = PexprLimit(
		mp, pexprGet, PosOne(mp, (*pdrgpcr)[0], EdslsortAsc), true, 0, 7);
	const BOOL indexed = FBindingRoundTrip(mp,
		"Limit<n0 n1>(SortBy<o0>(Input<t0>))|"
		"Limit<n2 n3>(SortBy<o1>(Input<t1>))|"
		"t1 := t0;n2 := n0;n3 := n1;o1 := o0", pexprLive);

	CDSLRule *prule = Prule(mp,
		"Limit<n0 n1>(SortAsc<a0>(Input<t0>))|"
		"Limit<n2 n3>(SortAsc<a1>(Input<t1>))|"
		"TableEq(t1,t0);AttrsEq(a1,a0);ScalarEq(n2,n0);"
		"ScalarEq(n3,n1)");
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_RESULT eres = indexed ? GPOS_OK : GPOS_FAILED;
	CExpression *nested = PexprLimit(
		mp, pexprLive, PosOne(mp, (*pdrgpcr)[0], EdslsortDesc), false, 0, 0);
	if (!FBindingRoundTrip(mp,
		"SortBy<o2>(Limit<n0 n1>(SortBy<o0>(Input<t0>)))|"
		"SortBy<o3>(Limit<n2 n3>(SortBy<o1>(Input<t1>)))|"
		"t1 := t0;n2 := n0;n3 := n1;o1 := o0;o3 := o2", nested))
		eres = GPOS_FAILED;
	nested->Release();
	for (const CHAR *capture : {"Column(a0) := n0", "Column(a0) := n1"})
	{
		const std::string text =
			"Limit<n0 n1>(SortBy<o0>(Input<t0>))|"
			"Limit<n2 n3>(SortBy<o1>(Input<t1>))|"
			"t1 := t0;n2 := n0;n3 := n1;o1 := o0;" + std::string(capture);
		CDSLRule *wrong = Prule(mp, text.c_str());
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		if (nullptr == wrong || CDSLMatcher(mp, wrong).FMatch(
			wrong->PfragSrc()->PopRoot(), pexprLive, model))
			eres = GPOS_FAILED;
		model->Release();
		CRefCount::SafeRelease(wrong);
	}
	if (nullptr == prule
		|| !matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprLive, pmodel)
		|| 4 != pmodel->Size())
	{
		eres = GPOS_FAILED;
	}

	CExpression *pexprTgt = nullptr;
	if (GPOS_OK == eres)
	{
		CDSLInstantiator instantiator(mp);
		pexprTgt = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt
			|| COperator::EopLogicalLimit != pexprTgt->Pop()->Eopid()
			|| 1
				!= CLogicalLimit::PopConvert(pexprTgt->Pop())
					   ->Pos()
					   ->UlSortColumns()
			|| !CLogicalLimit::PopConvert(pexprTgt->Pop())->FHasCount()
			|| COperator::EopLogicalLimit == (*pexprTgt)[0]->Pop()->Eopid()
			|| !(*pexprTgt)[2]->Matches((*pexprLive)[2]))
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	CRefCount::SafeRelease(prule);
	pexprLive->Release();
	pexprGet->Release();
	return eres;
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_SortOverLimitStaysNested()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *pdrgpcr = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("nested", 2, &pdrgpcr);
	CExpression *pexprInner
		= PexprLimit(mp, pexprGet, GPOS_NEW(mp) COrderSpec(mp), true, 0, 5);
	CExpression *pexprOuter = PexprLimit(
		mp, pexprInner, PosOne(mp, (*pdrgpcr)[0], EdslsortDesc), false, 0, 0);
	const BOOL indexed = FBindingRoundTrip(mp,
		"SortBy<o0>(Limit<n0 n1>(Input<t0>))|"
		"SortBy<o1>(Limit<n2 n3>(Input<t1>))|"
		"t1 := t0;n2 := n0;n3 := n1;o1 := o0", pexprOuter);

	CDSLRule *prule = Prule(mp,
		"SortDesc<a0>(Limit<n0 n1>(Input<t0>))|"
		"SortDesc<a1>(Limit<n2 n3>(Input<t1>))|"
		"TableEq(t1,t0);AttrsEq(a1,a0);ScalarEq(n2,n0);"
		"ScalarEq(n3,n1)");
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_RESULT eres = indexed ? GPOS_OK : GPOS_FAILED;
	if (nullptr == prule
		|| !matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprOuter, pmodel))
	{
		eres = GPOS_FAILED;
	}

	CExpression *pexprTgt = nullptr;
	if (GPOS_OK == eres)
	{
		CDSLInstantiator instantiator(mp);
		pexprTgt = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt
			|| COperator::EopLogicalLimit != pexprTgt->Pop()->Eopid()
			|| CLogicalLimit::PopConvert(pexprTgt->Pop())->FHasCount()
			|| COperator::EopLogicalLimit != (*pexprTgt)[0]->Pop()->Eopid()
			|| !CLogicalLimit::PopConvert((*pexprTgt)[0]->Pop())->FHasCount())
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	CRefCount::SafeRelease(prule);
	pexprOuter->Release();
	pexprInner->Release();
	pexprGet->Release();
	return eres;
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_PlainLimitRejectsHiddenOrder()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *pdrgpcr = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("hidden_order", 1, &pdrgpcr);
	CExpression *pexprLive = PexprLimit(
		mp, pexprGet, PosOne(mp, (*pdrgpcr)[0], EdslsortAsc), true, 0, 3);
	CDSLRule *prule = Prule(mp,
		"Limit<n0 n1>(Input<t0>)|Limit<n2 n3>(Input<t1>)|"
		"TableEq(t1,t0);ScalarEq(n2,n0);ScalarEq(n3,n1)");
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_RESULT eres = (nullptr != prule
						   && !matcher.FMatch(
							   prule->PfragSrc()->PopRoot(), pexprLive, pmodel))
		? GPOS_OK
		: GPOS_FAILED;

	pmodel->Release();
	CRefCount::SafeRelease(prule);
	pexprLive->Release();
	pexprGet->Release();
	return eres;
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_OffsetOnlyLimitRoundTrip()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CExpression *pexprGet = fix.PexprLogicalGet("offset_only", 1, nullptr);
	CExpression *pexprLive
		= PexprLimit(mp, pexprGet, GPOS_NEW(mp) COrderSpec(mp), false, 2, 0);
	CDSLRule *prule = Prule(mp,
		"Limit<n0 n1>(Input<t0>)|Limit<n2 n3>(Input<t1>)|"
		"TableEq(t1,t0);ScalarEq(n2,n0);ScalarEq(n3,n1)");
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	GPOS_RESULT eres = GPOS_OK;
	if (nullptr == prule
		|| !matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprLive, pmodel))
	{
		eres = GPOS_FAILED;
	}

	CExpression *pexprTgt = nullptr;
	if (GPOS_OK == eres)
	{
		CDSLInstantiator instantiator(mp);
		pexprTgt = instantiator.PexprInstantiate(prule, pmodel);
		if (nullptr == pexprTgt
			|| CLogicalLimit::PopConvert(pexprTgt->Pop())->FHasCount()
			|| CUtils::FHasZeroOffset(pexprTgt)
			|| !CLogicalLimit::PopConvert(pexprTgt->Pop())->Pos()->IsEmpty())
		{
			eres = GPOS_FAILED;
		}
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	CRefCount::SafeRelease(prule);
	pexprLive->Release();
	pexprGet->Release();
	return eres;
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_ExactOrderSpecRoundTrip()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CColRefArray *pdrgpcr = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("null_order", 1, &pdrgpcr);
	IMDId *pmdid
		= (*pdrgpcr)[0]->RetrieveType()->GetMdidForCmpType(IMDType::EcmptL);
	pmdid->AddRef();
	COrderSpec *pos = GPOS_NEW(mp) COrderSpec(mp);
	pos->Append(pmdid, (*pdrgpcr)[0], COrderSpec::EntFirst);
	CExpression *pexprLive = PexprLimit(mp, pexprGet, pos, false, 0, 0);
	CDSLRule *prule = Prule(mp,
		"SortBy<o0>(Input<t0>)|SortBy<o1>(Input<t1>)|"
		"TableEq(t1,t0);OrderEq(o1,o0)");
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	CDSLInstantiator instantiator(mp);
	CExpression *pexprTarget = nullptr;
	std::string dsl;
	std::string error;
	GPOS_RESULT eres = (nullptr != prule &&
		matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprLive, pmodel) &&
		checker.FCheck(prule, pmodel) &&
		nullptr != (pexprTarget = instantiator.PexprInstantiate(prule, pmodel)) &&
		pexprTarget->Matches(pexprLive) &&
		CDSLPlanTemplate::FSlice(mp, pexprLive, "r", {"r/0"}, &dsl, &error) &&
		dsl == "SortBy<o0>(Input<t0>)" && error.empty())
		? GPOS_OK : GPOS_FAILED;

	// An empty-order condition on an alias must inspect the captured order,
	// irrespective of the order/orientation of the equality clauses.
	for (const CHAR *clauses : {
		"TableEq(t1,t0);OrderEq(o1,o0);OrderEmpty(o1)",
		"OrderEmpty(o1);OrderEq(o1,o0);TableEq(t1,t0)",
		"TableEq(t1,t0);OrderEq(o0,o1);OrderEmpty(o1)"})
	{
		const std::string text =
			"SortBy<o0>(Input<t0>)|SortBy<o1>(Input<t1>)|" + std::string(clauses);
		CDSLRule *restricted = Prule(mp, text.c_str());
		CDSLModel *captured = GPOS_NEW(mp) CDSLModel(mp);
		if (nullptr == restricted ||
			!matcher.FMatch(restricted->PfragSrc()->PopRoot(), pexprLive, captured) ||
			checker.FCheck(restricted, captured)) eres = GPOS_FAILED;
		captured->Release();
		CRefCount::SafeRelease(restricted);
	}

	CRefCount::SafeRelease(pexprTarget);
	pmodel->Release();
	CRefCount::SafeRelease(prule);
	pexprLive->Release();
	pexprGet->Release();
	return eres;
}

static GPOS_RESULT
EresTargetScalarConstants(ULONG mode)
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CExpression *pexprGet = fix.PexprLogicalGet("target_constants", 1, nullptr);
	const CHAR *rules[] = {
		"Input<t0>|Limit<n0 n1>(Input<t1>)|TableEq(t1,t0);ScalarOne(n0);ScalarZero(n1)",
		"Input<t0>|Limit<n0 n1>(Input<t1>)|t1 := t0;ScalarOne(n0);ScalarZero(n1)",
		"Input<t0>|Limit<n0 n1>(Input<t1>)|t1 := t0;n0 := n2;ScalarOne(n2);ScalarZero(n1)",
		"Input<t0>|Limit<n0 n1>(Input<t1>)|ScalarZero(n1);ScalarOne(n0);t1 := t0"};
	CDSLRule *prule = Prule(mp, rules[mode]);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	CDSLConstraintChecker checker(mp);
	GPOS_RESULT eres = GPOS_OK;
	if (nullptr == prule ||
		!matcher.FMatch(prule->PfragSrc()->PopRoot(), pexprGet, pmodel) ||
		!checker.FCheck(prule, pmodel))
	{
		eres = GPOS_FAILED;
	}

	CExpression *pexprTgt = nullptr;
	if (GPOS_OK == eres)
	{
		CDSLInstantiator instantiator(mp);
		pexprTgt = instantiator.PexprInstantiate(prule, pmodel);
		CExpression *pexprOne = CUtils::PexprScalarConstInt8(mp, 1);
		if (nullptr == pexprTgt ||
			COperator::EopLogicalLimit != pexprTgt->Pop()->Eopid() ||
			!CLogicalLimit::PopConvert(pexprTgt->Pop())->FHasCount() ||
			!CUtils::FHasZeroOffset(pexprTgt) ||
			!(*pexprTgt)[2]->Matches(pexprOne))
		{
			eres = GPOS_FAILED;
		}
		pexprOne->Release();
	}

	CRefCount::SafeRelease(pexprTgt);
	pmodel->Release();
	CRefCount::SafeRelease(prule);
	pexprGet->Release();
	return eres;
}

GPOS_RESULT
CDSLOrderLimitTest::EresUnittest_TargetScalarConstants()
{
	for (ULONG mode = 0; mode < 4; mode++)
		if (GPOS_OK != EresTargetScalarConstants(mode)) return GPOS_FAILED;
	// A cross-side alias must check the captured value after materialization.
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CExpression *get = fix.PexprLogicalGet("literal_alias", 1, nullptr);
	BOOL valid = true;
	for (BOOL typed : {false, true})
	{
		CDSLRule *rule = Prule(mp, typed
			? "Limit<n0 n1>(Input<t0>)|Limit<n2 n3>(Input<t1>)|t1 := t0;ScalarEq(n2,n0);n3 := n1;ScalarOne(n2)"
			: "Limit<n0 n1>(Input<t0>)|Limit<n2 n3>(Input<t1>)|TableEq(t1,t0);ScalarEq(n2,n0);ScalarEq(n3,n1);ScalarOne(n2)");
		for (LINT count = 0; count < 2; count++)
		{
			CExpression *source = PexprLimit(mp, get, GPOS_NEW(mp) COrderSpec(mp), true, 0, count);
			CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
			CDSLMatcher matcher(mp, rule);
			CDSLConstraintChecker checker(mp);
			valid &= nullptr != rule && matcher.FMatch(rule->PfragSrc()->PopRoot(), source, model) &&
				checker.FCheck(rule, model) == (count == 1);
			model->Release();
			source->Release();
		}
		CRefCount::SafeRelease(rule);
	}
	get->Release();
	return valid ? GPOS_OK : GPOS_FAILED;
}

// EOF
