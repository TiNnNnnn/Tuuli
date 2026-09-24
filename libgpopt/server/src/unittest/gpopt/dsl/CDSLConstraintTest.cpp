//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLConstraintTest.cpp
//
//	@doc:
//		Implementation of the constraint-checker tests (see header). Each test
//		parses a rule for its constraint + symbols, builds a Get, manually binds
//		the table/attrs symbols into a model, and asserts FCheck admits/rejects.
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLConstraintTest.h"

#include <string>

#include "gpos/base.h"
#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringConst.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CScalarNullTest.h"
#include "gpopt/operators/CScalarConst.h"
#include "naucrates/base/IDatumInt8.h"
#include "gpopt/operators/CScalarSubquery.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

// parse a rule DSL string to IR (verdict EQ); NULL on failure.
static CDSLRule *
PdslruleParseLocal(CMemoryPool *mp, const CHAR *sz_dsl)
{
	CWStringDynamic strErr(mp);
	return CDSLRuleParser::PdslruleParse(mp, sz_dsl, "EQ" /*verdict*/, &strErr);
}

// find the first source-fragment symbol with the given name (e.g. "t0", "a0").
// Returns NULL if absent.
static const CDSLSymbol *
PsymByName(CDSLRule *prule, const CHAR *sz_name)
{
	CDSLSymbolArray *pdrgpsym = prule->PfragSrc()->Pdrgpsym();
	const ULONG ulSyms = pdrgpsym->Size();

	// narrow name length
	ULONG ulNameLen = 0;
	while (0 != sz_name[ulNameLen])
	{
		ulNameLen++;
	}

	for (ULONG ul = 0; ul < ulSyms; ul++)
	{
		const CDSLSymbol *psym = (*pdrgpsym)[ul];
		const CWStringConst *pstr = psym->PstrName();
		if (pstr->Length() != ulNameLen)
		{
			continue;
		}
		const WCHAR *wsz = pstr->GetBuffer();
		BOOL fEq = true;
		for (ULONG i = 0; i < ulNameLen; i++)
		{
			if (wsz[i] != (WCHAR) sz_name[i])
			{
				fEq = false;
				break;
			}
		}
		if (fEq)
		{
			return psym;
		}
	}
	return nullptr;
}

// bind table symbol -> Get subtree, attrs symbol -> the given single column.
static void
BindTableAndAttr(CDSLModel *pmodel, const CDSLSymbol *psymTable,
				 CExpression *pexprGet, const CDSLSymbol *psymAttrs,
				 CColRef *pcr, CMemoryPool *mp)
{
	pmodel->FBind(psymTable, pexprGet);
	CColRefArray *pdrgpcr = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcr->Append(pcr);
	pmodel->FBind(psymAttrs, pdrgpcr);
	pdrgpcr->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintTest::EresUnittest
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLConstraintTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CDSLConstraintTest::EresUnittest_ExactBindingEquality),
		GPOS_UNITTEST_FUNC(CDSLConstraintTest::EresUnittest_SliceCompose),
		GPOS_UNITTEST_FUNC(CDSLConstraintTest::EresUnittest_DeterministicSubqueryBoundary),
		GPOS_UNITTEST_FUNC(CDSLConstraintTest::EresUnittest_AttrsSubAdmit),
		GPOS_UNITTEST_FUNC(CDSLConstraintTest::EresUnittest_AttrsSubReject),
		GPOS_UNITTEST_FUNC(
			CDSLConstraintTest::EresUnittest_AttrsSubAttrsAdmit),
		GPOS_UNITTEST_FUNC(
			CDSLConstraintTest::EresUnittest_AttrsSubAttrsReject),
		GPOS_UNITTEST_FUNC(CDSLConstraintTest::EresUnittest_UniqueAdmit),
		GPOS_UNITTEST_FUNC(
			CDSLConstraintTest::EresUnittest_UniqueAdmitOnFixedKey),
		GPOS_UNITTEST_FUNC(
			CDSLConstraintTest::EresUnittest_UniqueAdmitThroughJoin),
		GPOS_UNITTEST_FUNC(CDSLConstraintTest::EresUnittest_UniqueReject),
		GPOS_UNITTEST_FUNC(CDSLConstraintTest::EresUnittest_OutputAttrsAdmit),
		GPOS_UNITTEST_FUNC(
			CDSLConstraintTest::EresUnittest_OutputAttrsAdmitWithoutKey),
		GPOS_UNITTEST_FUNC(CDSLConstraintTest::EresUnittest_NotNullAdmit),
		GPOS_UNITTEST_FUNC(
			CDSLConstraintTest::EresUnittest_NotNullThroughLeftJoin),
		GPOS_UNITTEST_FUNC(CDSLConstraintTest::EresUnittest_NotNullReject),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_ExactBindingEquality()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	BOOL ok = true;
	CTableDescriptor *table = fix.PtabdescCreate("capture_identity", 2);
	CColRefArray *left_columns = nullptr, *right_columns = nullptr;
	CExpression *left = fix.PexprLogicalGet(table, "left", &left_columns);
	table->AddRef();
	CExpression *right = fix.PexprLogicalGet(table, "right", &right_columns);
	// Usage pruning is not a change of base relation or declared column identity.
	(*right_columns)[1]->MarkAsUnused();
	CColRefArray *compared_columns = GPOS_NEW(mp) CColRefArray(mp);
	compared_columns->Append((*left_columns)[0]);
	CExpression *one = fix.PexprEqConst((*left_columns)[0], 1);
	CExpression *two = fix.PexprEqConst((*left_columns)[0], 2);
	CExpression *first_filter = fix.PexprLogicalSelect(left, one);
	CExpression *same_filter = fix.PexprLogicalSelect(left, one);
	CExpression *different_filter = fix.PexprLogicalSelect(left, two);
	CExpression *right_one = fix.PexprEqConst((*right_columns)[0], 1);
	CExpression *right_two = fix.PexprEqConst((*right_columns)[0], 2);
	CExpression *alias_filter = fix.PexprLogicalSelect(right, right_one);
	CExpression *different_alias_filter = fix.PexprLogicalSelect(right, right_two);
	CExpression *correlated_filter = fix.PexprLogicalSelect(right, one);
	CExpression *relations[] = {first_filter, same_filter, different_filter,
		alias_filter, different_alias_filter, left, correlated_filter};
	for (BOOL exact : {false, true})
	{
		for (const CHAR *equality : {"Eq(t0,t1)", "Eq(a0,a1)", "Eq(s0,s1)"})
		{
			const BOOL relation = 't' == equality[3];
			const BOOL schema = 's' == equality[3];
			// These rules isolate premise checking, not certified rewrites.
			CDSLRule *rule = PdslruleParseLocal(mp, (std::string(
				"InnerJoin<p0 a0 a1>(Proj<a2 s0>(Input<t0>),Proj<a3 s1>(Input<t1>))|Input<t2>|") +
				(exact ? "t2 := t0;" : "TableEq(t2,t0);") + equality).c_str());
			if (nullptr == rule) return GPOS_FAILED;
			for (ULONG shape = 0; shape < (relation ? GPOS_ARRAY_SIZE(relations) : 3); ++shape)
			{
				CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
				model->FBind(PsymByName(rule, "t0"), relation ? first_filter : left);
				model->FBind(PsymByName(rule, "t1"), relation
					? relations[shape]
					: (2 == shape ? right : left));
				if (!relation)
				{
					CColRefArray *other = GPOS_NEW(mp) CColRefArray(mp);
					other->Append(2 == shape ? (*right_columns)[0] : (*left_columns)[0]);
					model->FBind(PsymByName(rule, schema ? "s0" : "a0"), compared_columns);
					model->FBind(PsymByName(rule, schema ? "s1" : "a1"),
						0 == shape ? compared_columns : other);
					other->Release();
				}
				const BOOL admitted = CDSLConstraintChecker(mp).FCheck(rule, model);
				const BOOL expected = relation ? shape < 2 || (!exact && 3 == shape)
					: !exact || 2 != shape;
				if (admitted != expected)
					GPOS_TRACE_FORMAT("capture %s exact=%d shape=%lu admitted=%d",
						equality, exact, shape, admitted);
				ok &= admitted == expected;
				model->Release();
			}
			rule->Release();
		}
	}
	first_filter->Release(); same_filter->Release(); different_filter->Release();
	alias_filter->Release(); different_alias_filter->Release();
	correlated_filter->Release();
	compared_columns->Release();
	right_one->Release(); right_two->Release();
	one->Release(); two->Release(); left->Release(); right->Release();
	return ok ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_SliceCompose()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *rule = PdslruleParseLocal(mp,
		"Limit<n0 n1>(Limit<n2 n3>(Input<t0>))|Limit<n4 n5>(Input<t1>)|"
		"TableEq(t1,t0);SliceCompose(n0,n1,n2,n3,n4,n5)");
	if (nullptr == rule)
	{
		return GPOS_FAILED;
	}
	CDSLSymbolArray *syms = (*rule->Pdrgpcon())[1]->Pdrgpsym();
	CColRefArray *cols = nullptr;
	CExpression *input = fix.PexprLogicalGet("slice_input", 1, &cols);
	CDSLConstraintChecker checker(mp);
	GPOS_RESULT status = GPOS_OK;
	// Exhaustive small slices, including zero counts and offsets past the
	// inner end. Compare membership directly, not just the arithmetic formula.
	for (ULONG code = 0; code < 256; code++)
	{
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		model->FBind(PsymByName(rule, "t0"), input);
		LINT values[4];
		for (ULONG i = 0; i < 4; i++)
		{
			values[i] = (code >> (2 * i)) & 3;
			CExpression *value = CUtils::PexprScalarConstInt8(mp, values[i]);
			model->FBind((*syms)[i], value);
			value->Release();
		}
		if (!checker.FCheck(rule, model) || !checker.FCheck(rule, model))
		{
			model->Release();
			status = GPOS_FAILED;
			break;
		}
		CDSLInstantiator instantiator(mp);
		CExpression *target = instantiator.PexprInstantiate(rule, model);
		if (nullptr == target || COperator::EopLogicalLimit != target->Pop()->Eopid())
		{
			CRefCount::SafeRelease(target);
			model->Release();
			status = GPOS_FAILED;
			break;
		}
		auto number = [](CExpression *expr) {
			return dynamic_cast<gpnaucrates::IDatumInt8 *>(
				CScalarConst::PopConvert(expr->Pop())->GetDatum())->Value();
		};
		const LINT offset = number((*target)[1]);
		const LINT count = number((*target)[2]);
		for (LINT row = 0; row < 12; row++)
		{
			const BOOL nested = row >= values[3] && row < values[3] + values[2] &&
				row >= values[3] + values[1] && row < values[3] + values[1] + values[0];
			if (nested != (row >= offset && row < offset + count))
			{
				status = GPOS_FAILED;
			}
		}
		target->Release();
		model->Release();
	}
	// Invalid inputs never manufacture a result. Validate already-bound
	// outputs too: a constructive constraint cannot overwrite a source fact.
	for (ULONG kind = 0; kind < 8; kind++)
	{
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		for (ULONG i = 0; i < 4; i++)
		{
			if (kind == 4 && i == 0)
			{
				continue;
			}
			LINT value = 2;
			if (kind == 0 && i == 0) value = -1;
			if (kind == 3 && i == 3) value = gpos::lint_max;
			CExpression *expr = kind == 2 && i == 0
				? CUtils::PexprScalarIdent(mp, (*cols)[0])
				: (kind == 7 && i == 0
					? CUtils::PexprScalarConstInt4(mp, 2)
					: CUtils::PexprScalarConstInt8(mp, value, kind == 1 && i == 0));
			model->FBind((*syms)[i], expr);
			expr->Release();
		}
		if (kind == 5 || kind == 6)
		{
			CExpression *wrong = CUtils::PexprScalarConstInt8(mp, 99);
			model->FBind((*syms)[kind - 1], wrong);
			wrong->Release();
		}
		if (checker.FCheck(rule, model)) status = GPOS_FAILED;
		model->Release();
	}
	const LINT boundaries[][6] = {
		{gpos::lint_max, 0, gpos::lint_max, 0, gpos::lint_max, 0},
		{3, 5, 10, gpos::lint_max - 5, 3, gpos::lint_max},
		{4, gpos::lint_max, 3, 0, 0, gpos::lint_max},
	};
	for (const auto &values : boundaries)
	{
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		for (ULONG i = 0; i < 6; i++)
		{
			CExpression *value = CUtils::PexprScalarConstInt8(mp, values[i]);
			model->FBind((*syms)[i], value);
			value->Release();
		}
		if (!checker.FCheck(rule, model)) status = GPOS_FAILED;
		model->Release();
	}
	input->Release();
	rule->Release();
	return status;
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_DeterministicSubqueryBoundary()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	// Checker fixtures, not asserted rewrite equivalences. Property annotations
	// precede constructors so the target predicate is not already materialized.
	const CHAR *rules[] = {
		"Filter<p0 a0>(Input<t0>)|Input<t1>|TableEq(t1,t0);Deterministic(p0)",
		"Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
		"Deterministic(p1);TableEq(t1,t0);PredicateEq(p1,p0);AttrsEq(a1,a0)",
		"Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
		"Deterministic(p1);TableEq(t1,t0);PredicateNotTrue(p2,p0);"
		"PredicateAnd(p1,p2,p0);AttrsEq(a1,a0)",
		"Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
		"ErrorFree(p1);TableEq(t1,t0);PredicateNotTrue(p2,p0);"
		"PredicateAnd(p1,p2,p0);AttrsEq(a1,a0)",
		"Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
		"Deterministic(p1);ErrorFree(p1);TableEq(t1,t0);"
		"PredicateAnd(p2,p0,p0);PredicateNotTrue(p1,p2);AttrsEq(a1,a0)",
	};
	CColRefArray *pdrgpcr = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("deterministic_input", 1, &pdrgpcr);
	CDSLConstraintChecker checker(mp);
	GPOS_RESULT eres = GPOS_OK;
	for (ULONG has_subquery = 0; has_subquery < 2; ++has_subquery)
	{
		CExpression *pexprPredicate = nullptr;
		if (has_subquery)
		{
			pexprGet->AddRef();
			CExpression *pexprLimit = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CLogicalLimit(mp, GPOS_NEW(mp) COrderSpec(mp),
					true, true, false), pexprGet,
				CUtils::PexprScalarConstInt8(mp, 0), CUtils::PexprScalarConstInt8(mp, 1));
			CExpression *pexprSubquery = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CScalarSubquery(mp, (*pdrgpcr)[0], false, false), pexprLimit);
			pexprPredicate = GPOS_NEW(mp) CExpression(mp,
				GPOS_NEW(mp) CScalarNullTest(mp), pexprSubquery);
		}
		else
		{
			pexprPredicate = fix.PexprEqPred((*pdrgpcr)[0], (*pdrgpcr)[0]);
		}
		// Both trees pass the former function-only guard. Only the second
		// can pick a different row on a repeated unordered subquery scan.
		GPOS_ASSERT(!pexprPredicate->DeriveHasNonScalarFunction());
		GPOS_ASSERT(IMDFunction::EfsImmutable ==
			pexprPredicate->DeriveScalarFunctionProperties()->Efs());
		for (const CHAR *rule : rules)
		{
			CDSLRule *prule = PdslruleParseLocal(mp, rule);
			GPOS_ASSERT(nullptr != prule);
			for (ULONG bound = 0; bound < 2; ++bound)
			{
				CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
				BindTableAndAttr(pmodel, PsymByName(prule, "t0"), pexprGet,
					PsymByName(prule, "a0"), (*pdrgpcr)[0], mp);
				if (bound)
				{
					pmodel->FBind(PsymByName(prule, "p0"), pexprPredicate);
				}
				if (checker.FCheck(prule, pmodel) != (bound && !has_subquery))
				{
					eres = GPOS_FAILED;
				}
				pmodel->Release();
			}
			prule->Release();
		}
		pexprPredicate->Release();
	}
	pexprGet->Release();
	return eres;
}

static GPOS_RESULT
EresOutputAttrs(BOOL fHasKey)
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp, "Input<t0>|Filter<p0 a0>(Input<t1>)|"
			"OutputAttrs(a0,t0);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fHasKey
		? fix.PexprLogicalGet("keyed_output", 3, &pdrgpcrOut, 0 /*key*/)
		: fix.PexprLogicalGet("unkeyed_output", 3, &pdrgpcrOut);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	pmodel->FBind(PsymByName(prule, "t0"), pexprGet);

	CDSLConstraintChecker checker(mp);
	const BOOL fHolds = checker.FCheck(prule, pmodel);
	const GPOS_RESULT eres = fHolds ? GPOS_OK : GPOS_FAILED;
	pmodel->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_OutputAttrsAdmit()
{
	return EresOutputAttrs(true);
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_OutputAttrsAdmitWithoutKey()
{
	return EresOutputAttrs(false);
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_UniqueAdmitThroughJoin()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp, "Proj*<a0 s0>(Input<t0>)|Input<t1>|Unique(t0,a0);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrLeft = nullptr;
	CExpression *pexprLeft =
		fix.PexprLogicalGet("left_t", 2, &pdrgpcrLeft, 0 /*key*/);
	CTableDescriptor *ptabdescRight = fix.PtabdescCreate("right_t", 2);
	CBitSet *pbsRightKey = GPOS_NEW(mp) CBitSet(mp);
	(void) pbsRightKey->ExchangeSet(0);
	(void) pbsRightKey->ExchangeSet(1);
	(void) ptabdescRight->FAddKeySet(pbsRightKey);
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprRightGet =
		fix.PexprLogicalGet(ptabdescRight, "right_t", &pdrgpcrRight);
	CExpression *pexprFixed = fix.PexprEqConst((*pdrgpcrRight)[1], 7);
	CExpression *pexprRight =
		fix.PexprLogicalSelect(pexprRightGet, pexprFixed);
	CExpression *pexprJoinPred =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprJoin =
		fix.PexprLogicalInnerJoin(pexprLeft, pexprRight, pexprJoinPred);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	BindTableAndAttr(pmodel, PsymByName(prule, "t0"), pexprJoin,
					 PsymByName(prule, "a0"), (*pdrgpcrLeft)[0], mp);
	CDSLConstraintChecker checker(mp);
	GPOS_RESULT eres = checker.FCheck(prule, pmodel) ? GPOS_OK : GPOS_FAILED;

	// Adding a column from the other input does not invalidate the anchor-key
	// proof. The left key still identifies at most one left row and the filtered
	// composite right key permits at most one match for its join column.
	CDSLModel *pmodelCombined = GPOS_NEW(mp) CDSLModel(mp);
	pmodelCombined->FBind(PsymByName(prule, "t0"), pexprJoin);
	CColRefArray *pdrgpcrCombined = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrCombined->Append((*pdrgpcrLeft)[0]);
	pdrgpcrCombined->Append((*pdrgpcrRight)[0]);
	pmodelCombined->FBind(PsymByName(prule, "a0"), pdrgpcrCombined);
	pdrgpcrCombined->Release();
	if (!checker.FCheck(prule, pmodelCombined))
	{
		eres = GPOS_FAILED;
	}

	pmodelCombined->Release();
	pmodel->Release();
	pexprJoin->Release();
	pexprJoinPred->Release();
	pexprRight->Release();
	pexprFixed->Release();
	pexprRightGet->Release();
	pexprLeft->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_UniqueAdmitOnFixedKey()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);
	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|Unique(t0,a0);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CTableDescriptor *ptabdesc = fix.PtabdescCreate("t0", 3);
	CBitSet *pbsKey = GPOS_NEW(mp) CBitSet(mp);
	(void) pbsKey->ExchangeSet(0);
	(void) pbsKey->ExchangeSet(1);
	(void) ptabdesc->FAddKeySet(pbsKey);
	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet(ptabdesc, "t0", &pdrgpcrOut);
	CExpression *pexprPred = fix.PexprEqConst((*pdrgpcrOut)[0], 10);
	CExpression *pexprSelect = fix.PexprLogicalSelect(pexprGet, pexprPred);
	pexprPred->Release();

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	// c1 is not a key by itself, but together with fixed c0 it covers the
	// composite key (c0,c1), so c1 is unique within the selected rows.
	BindTableAndAttr(pmodel, PsymByName(prule, "t0"), pexprSelect,
					 PsymByName(prule, "a0"), (*pdrgpcrOut)[1], mp);
	CDSLConstraintChecker checker(mp);
	GPOS_RESULT eres = checker.FCheck(prule, pmodel) ? GPOS_OK : GPOS_FAILED;

	pmodel->Release();
	pexprSelect->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	AttrsSub(a0,t0)
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLConstraintTest::EresUnittest_AttrsSubAdmit()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|AttrsSub(a0,t0);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 3, &pdrgpcrOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	// a0 bound to c0 (which IS in t0's output) => subset holds
	BindTableAndAttr(pmodel, PsymByName(prule, "t0"), pexprGet,
					 PsymByName(prule, "a0"), (*pdrgpcrOut)[0], mp);

	CDSLConstraintChecker checker(mp);
	GPOS_RESULT eres = checker.FCheck(prule, pmodel) ? GPOS_OK : GPOS_FAILED;

	pmodel->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_AttrsSubReject()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|AttrsSub(a0,t0);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	// t0 is a 2-column Get; a0 bound to a column from a DIFFERENT table => not a
	// subset of t0's output.
	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 2, &pdrgpcrOut);
	CColRefArray *pdrgpcrOther = nullptr;
	CExpression *pexprOther = fix.PexprLogicalGet("tX", 2, &pdrgpcrOther);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	BindTableAndAttr(pmodel, PsymByName(prule, "t0"), pexprGet,
					 PsymByName(prule, "a0"), (*pdrgpcrOther)[0], mp);

	CDSLConstraintChecker checker(mp);
	// must REJECT: foreign column is not in t0's output
	GPOS_RESULT eres = checker.FCheck(prule, pmodel) ? GPOS_FAILED : GPOS_OK;

	pmodel->Release();
	pexprGet->Release();
	pexprOther->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	AttrsSub(a0,a1)
//---------------------------------------------------------------------------
static GPOS_RESULT
EresAttrsSubAttrs(BOOL fReverse)
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp,
		"Filter<p1 a1>(Filter<p0 a0>(Input<t0>))|Input<t1>|"
		"AttrsSub(a0,a1);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 3, &pdrgpcrOut);
	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	pmodel->FBind(PsymByName(prule, "t0"), pexprGet);

	CColRefArray *pdrgpcrNarrow = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrNarrow->Append((*pdrgpcrOut)[0]);
	CColRefArray *pdrgpcrWide = GPOS_NEW(mp) CColRefArray(mp);
	pdrgpcrWide->Append((*pdrgpcrOut)[0]);
	pdrgpcrWide->Append((*pdrgpcrOut)[1]);
	pmodel->FBind(PsymByName(prule, "a0"),
				  fReverse ? pdrgpcrWide : pdrgpcrNarrow);
	pmodel->FBind(PsymByName(prule, "a1"),
				  fReverse ? pdrgpcrNarrow : pdrgpcrWide);
	pdrgpcrWide->Release();
	pdrgpcrNarrow->Release();

	CDSLConstraintChecker checker(mp);
	const BOOL fHolds = checker.FCheck(prule, pmodel);
	const GPOS_RESULT eres = fReverse == fHolds ? GPOS_FAILED : GPOS_OK;

	pmodel->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_AttrsSubAttrsAdmit()
{
	return EresAttrsSubAttrs(false /*fReverse*/);
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_AttrsSubAttrsReject()
{
	return EresAttrsSubAttrs(true /*fReverse*/);
}

//---------------------------------------------------------------------------
//	Unique(t0,a0)
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLConstraintTest::EresUnittest_UniqueAdmit()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|Unique(t0,a0);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	// t0 with column 0 registered as a unique key
	CTableDescriptor *ptabdesc = fix.PtabdescCreate("t0", 3, 0 /*ulKeyCol*/);
	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet(ptabdesc, "t0", &pdrgpcrOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	// a0 bound to the key column
	BindTableAndAttr(pmodel, PsymByName(prule, "t0"), pexprGet,
					 PsymByName(prule, "a0"), (*pdrgpcrOut)[0], mp);

	CDSLConstraintChecker checker(mp);
	GPOS_RESULT eres = checker.FCheck(prule, pmodel) ? GPOS_OK : GPOS_FAILED;

	pmodel->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_UniqueReject()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|Unique(t0,a0);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	// t0 with NO key registered
	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 3, &pdrgpcrOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	BindTableAndAttr(pmodel, PsymByName(prule, "t0"), pexprGet,
					 PsymByName(prule, "a0"), (*pdrgpcrOut)[0], mp);

	CDSLConstraintChecker checker(mp);
	// must REJECT: no key collection => uniqueness cannot be confirmed
	GPOS_RESULT eres = checker.FCheck(prule, pmodel) ? GPOS_FAILED : GPOS_OK;

	pmodel->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	NotNull(t0,a0)
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLConstraintTest::EresUnittest_NotNullAdmit()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|NotNull(t0,a0);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	// default fixture columns are non-nullable
	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet("t0", 3, &pdrgpcrOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	BindTableAndAttr(pmodel, PsymByName(prule, "t0"), pexprGet,
					 PsymByName(prule, "a0"), (*pdrgpcrOut)[0], mp);

	CDSLConstraintChecker checker(mp);
	GPOS_RESULT eres = checker.FCheck(prule, pmodel) ? GPOS_OK : GPOS_FAILED;

	pmodel->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_NotNullThroughLeftJoin()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|NotNull(t0,a0);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CColRefArray *pdrgpcrLeft = nullptr;
	CColRefArray *pdrgpcrRight = nullptr;
	CExpression *pexprLeft =
		fix.PexprLogicalGet("left_t", 2, &pdrgpcrLeft);
	CExpression *pexprRight =
		fix.PexprLogicalGet("right_t", 2, &pdrgpcrRight);
	CExpression *pexprPred =
		fix.PexprEqPred((*pdrgpcrLeft)[0], (*pdrgpcrRight)[0]);
	CExpression *pexprJoin =
		fix.PexprLogicalLeftOuterJoin(pexprLeft, pexprRight, pexprPred);

	CDSLConstraintChecker checker(mp);
	CDSLModel *pmodelLeft = GPOS_NEW(mp) CDSLModel(mp);
	BindTableAndAttr(pmodelLeft, PsymByName(prule, "t0"), pexprJoin,
					 PsymByName(prule, "a0"), (*pdrgpcrLeft)[0], mp);
	BOOL fLeftAdmitted = checker.FCheck(prule, pmodelLeft);
	pmodelLeft->Release();

	CDSLModel *pmodelRight = GPOS_NEW(mp) CDSLModel(mp);
	BindTableAndAttr(pmodelRight, PsymByName(prule, "t0"), pexprJoin,
					 PsymByName(prule, "a0"), (*pdrgpcrRight)[0], mp);
	BOOL fRightRejected = !checker.FCheck(prule, pmodelRight);
	pmodelRight->Release();

	pexprJoin->Release();
	pexprPred->Release();
	pexprRight->Release();
	pexprLeft->Release();
	prule->Release();
	return fLeftAdmitted && fRightRejected ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLConstraintTest::EresUnittest_NotNullReject()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fix(mp);

	CDSLRule *prule = PdslruleParseLocal(
		mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|NotNull(t0,a0);TableEq(t1,t0)");
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	// t0 with NULLABLE columns
	CTableDescriptor *ptabdesc =
		fix.PtabdescCreate("t0", 3, gpos::ulong_max /*no key*/, true /*nullable*/);
	CColRefArray *pdrgpcrOut = nullptr;
	CExpression *pexprGet = fix.PexprLogicalGet(ptabdesc, "t0", &pdrgpcrOut);

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	BindTableAndAttr(pmodel, PsymByName(prule, "t0"), pexprGet,
					 PsymByName(prule, "a0"), (*pdrgpcrOut)[0], mp);

	CDSLConstraintChecker checker(mp);
	// must REJECT: bound column is nullable
	GPOS_RESULT eres = checker.FCheck(prule, pmodel) ? GPOS_FAILED : GPOS_OK;

	pmodel->Release();
	pexprGet->Release();
	prule->Release();
	return eres;
}

// EOF
