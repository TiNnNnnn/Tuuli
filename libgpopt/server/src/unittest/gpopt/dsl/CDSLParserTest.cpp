//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLParserTest.cpp
//
//	@doc:
//		Unit tests for the DSL rule parser and loader. Uses only public entry
//		points (CDSLRuleParser::PdslruleParse, CDSLRuleLoader::*). Rules used
//		here are real, WeTune-proven rules taken from the MONSOON corpus, one
//		per operator, so the tests double as a coverage check over the operator
//		vocabulary.
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLParserTest.h"

#include "gpos/io/COstreamString.h"
#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/dsl/CDSLRuleLoader.h"
#include "gpopt/dsl/CDSLRuleParser.h"

using namespace gpopt;

namespace
{
// Parse sz_dsl and return the rule (or NULL). Caller releases.
CDSLRule *
Parse(CMemoryPool *mp, const CHAR *sz_dsl)
{
	CWStringDynamic strErr(mp);
	return CDSLRuleParser::PdslruleParse(mp, sz_dsl, nullptr, &strErr);
}

// Render a rule to its canonical DSL text into a UTF-8-ish CHAR comparison
// via a wide dynamic string, then compare to expected (wide).
BOOL
FRoundTrips(CMemoryPool *mp, const CHAR *sz_dsl)
{
	CDSLRule *rule = Parse(mp, sz_dsl);
	if (nullptr == rule)
	{
		return false;
	}
	CWStringDynamic str(mp);
	COstreamString oss(&str);
	IOstream &os = oss;
	rule->OsPrint(os);
	rule->Release();

	// build expected wide string from sz_dsl
	CWStringConst expected(mp, sz_dsl);
	return str.Equals(&expected);
}
}  // namespace

GPOS_RESULT
CDSLParserTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CDSLParserTest::EresUnittest_RoundTrip),
		GPOS_UNITTEST_FUNC(CDSLParserTest::EresUnittest_SymbolArity),
		GPOS_UNITTEST_FUNC(CDSLParserTest::EresUnittest_Aliases),
		GPOS_UNITTEST_FUNC(CDSLParserTest::EresUnittest_AggFunctions),
		GPOS_UNITTEST_FUNC(CDSLParserTest::EresUnittest_SymbolNamespace),
		GPOS_UNITTEST_FUNC(CDSLParserTest::EresUnittest_Constraints),
		GPOS_UNITTEST_FUNC(CDSLParserTest::EresUnittest_Whitespace),
		GPOS_UNITTEST_FUNC(CDSLParserTest::EresUnittest_Loader),
		GPOS_UNITTEST_FUNC(CDSLParserTest::EresUnittest_Errors),
	};
	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

//---------------------------------------------------------------------------
// Aggregate operator suffixes carry the WeTune function kind. COUNT alone may
// carry '*', which maps to COUNT(DISTINCT ...).
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLParserTest::EresUnittest_AggFunctions()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	struct
	{
		const CHAR *dsl;
		EDslAggFuncKind kind;
		BOOL distinct;
	} rg[] = {
		{"Agg_sum<a0 a1 a2 f0 s0 p0>(Input<t0>)|Input<t1>|TableEq(t1,t0)",
		 EdslaggfuncSum, false},
		{"Agg_average<a0 a1 a2 f0 s0 p0>(Input<t0>)|Input<t1>|TableEq(t1,t0)",
		 EdslaggfuncAverage, false},
		{"Agg_count*<a0 a1 a2 f0 s0 p0>(Input<t0>)|Input<t1>|TableEq(t1,t0)",
		 EdslaggfuncCount, true},
		{"Agg_max<a0 a1 a2 f0 s0 p0>(Input<t0>)|Input<t1>|TableEq(t1,t0)",
		 EdslaggfuncMax, false},
		{"Agg_min<a0 a1 a2 f0 s0 p0>(Input<t0>)|Input<t1>|TableEq(t1,t0)",
		 EdslaggfuncMin, false},
	};

	for (ULONG ul = 0; ul < GPOS_ARRAY_SIZE(rg); ul++)
	{
		CDSLRule *prule = Parse(mp, rg[ul].dsl);
		if (nullptr == prule)
		{
			return GPOS_FAILED;
		}
		CDSLOp *pop = prule->PfragSrc()->PopRoot();
		BOOL fOk = EdslopAgg == pop->Edslop() &&
				   rg[ul].kind == pop->Edslaggfunc() &&
				   rg[ul].distinct == pop->FDistinct();
		prule->Release();
		if (!fOk || !FRoundTrips(mp, rg[ul].dsl))
		{
			return GPOS_FAILED;
		}
	}

	CDSLRule *bad = Parse(
		mp,
		"Agg_sum*<a0 a1 a2 f0 s0 p0>(Input<t0>)|Input<t1>|TableEq(t1,t0)");
	if (nullptr != bad)
	{
		bad->Release();
		return GPOS_FAILED;
	}
	return GPOS_OK;
}

//---------------------------------------------------------------------------
// Round-trip: parse -> OsPrint reproduces the canonical input, for one real
// proven rule per operator family. These are already canonical (no aliases, no
// stray spaces) so print == input.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLParserTest::EresUnittest_RoundTrip()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	const CHAR *rgsz[] = {
		// Proj*/Unique
		"Proj*<a0 s0>(Input<t0>)|Proj<a1 s1>(Input<t1>)|AttrsSub(a0,t0);"
		"Unique(t0,a0);TableEq(t1,t0);AttrsEq(a1,a0);SchemaEq(s1,s0)",
		// Filter (pred first) + Sort
		"Filter<p0 a1>(SortAsc<a0>(Input<t0>))|SortAsc<a3>(Filter<p1 a2>"
		"(Input<t1>))|AttrsSub(a0,t0);TableEq(t1,t0);AttrsEq(a2,a1);"
		"AttrsEq(a3,a0);PredicateEq(p1,p0)",
		// LeftJoin/InnerJoin + Reference
		"LeftJoin<a0 a1>(Input<t0>,Input<t1>)|InnerJoin<a2 a3>(Input<t2>,"
		"Input<t3>)|AttrsSub(a0,t0);Reference(t0,a0,t1,a1);TableEq(t2,t0);"
		"TableEq(t3,t1);AttrsEq(a2,a0);AttrsEq(a3,a1)",
		// Extended Join output binding remains optional and round-trips exactly.
		"InnerJoin<a0 a1 a2 s0>(Input<t0>,Input<t1>)|InnerJoin<a3 a4 a5 "
		"s1>(Input<t2>,Input<t3>)|TableEq(t2,t1);TableEq(t3,t0);"
		"AttrsEq(a3,a1);AttrsEq(a4,a0);AttrsEq(a5,a2);SchemaEq(s1,s0)",
		// Join residual predicates bind the expression and dependencies by side.
		"InnerJoin<a0 a1 p0 a2 a3>(Input<t0>,Input<t1>)|InnerJoin<a4 a5 "
		"p1 a6 a7>(Input<t2>,Input<t3>)|AttrsSub(a0,t0);AttrsSub(a1,t1);"
		"AttrsSub(a2,t0);AttrsSub(a3,t1);TableEq(t2,t0);TableEq(t3,t1);"
		"AttrsEq(a4,a0);AttrsEq(a5,a1);PredicateEq(p1,p0);AttrsEq(a6,a2);"
		"AttrsEq(a7,a3)",
		// Output and residual bindings compose without changing their order.
		"InnerJoin<a0 a1 a2 s0 p0 a3 a4>(Input<t0>,Input<t1>)|"
		"InnerJoin<a5 a6 a7 s1 p1 a8 a9>(Input<t2>,Input<t3>)|"
		"AttrsSub(a0,t0);AttrsSub(a1,t1);AttrsSub(a3,t0);AttrsSub(a4,t1);"
		"TableEq(t2,t0);TableEq(t3,t1);AttrsEq(a5,a0);AttrsEq(a6,a1);"
		"AttrsEq(a7,a2);SchemaEq(s1,s0);PredicateEq(p1,p0);AttrsEq(a8,a3);"
		"AttrsEq(a9,a4)",
		// Explicit SemiJoin binds its complete predicate and exact dependencies.
		"SemiJoin<p0 a0 a1>(Input<t0>,Input<t1>)|SemiJoin<p1 a2 a3>"
		"(Input<t2>,Input<t3>)|TableEq(t2,t0);TableEq(t3,t1);"
		"PredicateEq(p1,p0);AttrsEq(a2,a0);AttrsEq(a3,a1)",
		// Existing MONSOON corpus form (fewshot_curated.txt): bare Agg with five
		// symbols groupBy, aggAttrs, func, schema, having.
		"Exists(Proj<a0 s0>(Input<t0>),Agg<a1 a2 f0 s1 p0>(Input<t1>))|"
		"Exists(Proj<a3 s2>(Input<t2>),Agg<a4 a5 f1 s3 p1>(Input<t3>))|"
		"AttrsSub(a0,t0);AttrsSub(a1,t1);AttrsSub(a2,t1);TableEq(t2,t0);"
		"TableEq(t3,t1);AttrsEq(a3,a0);AttrsEq(a4,a1);AttrsEq(a5,a2);"
		"PredicateEq(p1,p0);SchemaEq(s2,s0);SchemaEq(s3,s1);FuncEq(f1,f0)",
		// Negated existential subquery remains distinct from positive Exists.
		"NotExists(Input<t0>,Proj*<a0 s0>(Input<t1>))|NotExists(Input<t2>,"
		"Proj<a1 s1>(Input<t3>))|AttrsSub(a0,t1);TableEq(t2,t0);"
		"TableEq(t3,t1);AttrsEq(a1,a0);SchemaEq(s1,s0)",
		// Quantified predicates are first-class and are not equality-specific.
		"Any<p0 a0>(Input<t0>,Input<t1>)|All<p1 a1>(Input<t2>,Input<t3>)|"
		"TableEq(t2,t0);TableEq(t3,t1);PredicateEq(p1,p0);AttrsEq(a1,a0)",
		// InSubFilter + Limit + Input-only target
		"InSubFilter<a1>(Input<t0>,Proj<a0 s0>(Input<t1>))|Limit<n0 n1>"
		"(Input<t2>)|TableEq(t2,t0);ScalarEq(n0,n1)",
		// Extended InSub binds both equality keys and an exact residual predicate.
		"InSubFilter<a0 a1 p0 a2 a3>(Input<t0>,Input<t1>)|InnerJoin<a4 "
		"a5 p1 a6 a7>(Input<t2>,Input<t3>)|TableEq(t2,t0);TableEq(t3,t1);"
		"AttrsEq(a4,a0);AttrsEq(a5,a1);PredicateEq(p1,p0);AttrsEq(a6,a2);"
		"AttrsEq(a7,a3)",
		// Extended Union output binding remains optional and round-trips exactly.
		"Union*<a0 s0>(Input<t0>,Input<t1>)|Union<a1 s1>(Input<t2>,"
		"Input<t3>)|TableEq(t2,t0);TableEq(t3,t1);AttrsEq(a1,a0);"
		"SchemaEq(s1,s0)",
		// Expression-safety constraints used by proved scalar substitutions.
		"Proj<a0 s0>(Input<t0>)|Proj<a1 s1>(Input<t1>)|TableEq(t1,t0);"
		"AttrsEq(a1,a0);SchemaEq(s1,s0);ErrorFree(a0);Deterministic(a0)",
		// Explicit ORCA ComputeScalar/LET expression-list capture.
		"Compute<e0 a0 s0>(Input<t0>)|Compute<e1 a1 s1>(Input<t1>)|"
		"TableEq(t1,t0);ExprListEq(e1,e0);AttrsEq(a1,a0);SchemaEq(s1,s0);"
		"ErrorFree(e0);Deterministic(e0)",
		// Generic expression-list composition for independent LET layers.
		"Compute<e0 a0 s0>(Compute<e1 a1 s1>(Input<t0>))|Compute<e2 a2 "
		"s2>(Input<t1>)|TableEq(t1,t0);ExprDepsDisjoint(e0,s1);"
		"ExprConcat(e2,e0,e1)",
		// Partial layer normalization keeps dependent expressions as a residual.
		"Compute<e0 a0 s0>(Compute<e1 a1 s1>(Input<t0>))|Compute<e3 a3 "
		"s3>(Compute<e2 a2 s2>(Input<t1>))|TableEq(t1,t0);"
		"ExprSplit(e2,e3,e0,e1)",
		// Predicate composition is explicit and target predicates can be derived.
		"SemiApply<p0 a0 a1 a2>(Input<t0>,Filter<p1 a3>(Input<t1>))|"
		"SemiJoin<p2 a4 a5>(Input<t2>,Input<t3>)|TableEq(t2,t0);"
		"TableEq(t3,t1);PredicateAnd(p2,p0,p1);AttrsEq(a4,a0);"
		"AttrsEq(a5,a1)",
		// Aggregate decorrelation declares target grouping/schema constructors
		// and a reusable equality-only pullup contract.
		"SemiApply<p0 a0 a1 a2>(Input<t0>,Agg<a3 a4 f0 s0 p1>(Filter<p2 "
		"a5 a6>(Input<t1>)))|SemiJoin<p3 a7 a8>(Input<t2>,Agg<a9 a10 f1 "
		"s1 p4>(Input<t3>))|TableEq(t2,t0);TableEq(t3,t1);"
		"PredicateAnd(p3,p0,p2);AttrsEq(a2,a6);AttrsUnion(a7,a0,a6);"
		"AttrsUnion(a8,a1,a5);AttrsUnion(a9,a3,a5);AttrsEq(a10,a4);"
		"FuncEq(f1,f0);SchemaUnion(s1,s0,a5);PredicateEq(p4,p1);"
		"AggCorrelationPullup(p0,p2,p3,a3,a9,a4,f0,s0,s1,p1,a5,a6);"
		"CorrelationEquality(p2,a5,a6);"
		"AggCorrelationGrouping(p2,a3,a9,a4,f0,s0,s1,p1,a5,a6)",
		// Window metadata uses the same positional vocabulary as WeTune. The
		// default-frame and explicit-frame spellings remain distinct in the IR.
		"WindowRows<a0 o0 w0>(Input<t0>)|WindowRows<a1 o1 w1>(Input<t1>)|"
		"TableEq(t1,t0);AttrsEq(a1,a0);OrderEq(o1,o0);WindowEq(w1,w0);"
		"ErrorFree(w0)",
		"Window<a0 o0 m0 w0>(Input<t0>)|Window<a1 o1 m1 w1>(Input<t1>)|"
		"TableEq(t1,t0);AttrsEq(a1,a0);OrderEq(o1,o0);FrameEq(m1,m0);"
		"WindowEq(w1,w0);ErrorFree(w0)",
		"MaxOneRow(Input<t0>)|AssertMaxOneRow(Input<t1>)|TableEq(t1,t0)",
		"Assert<p0 a0>(Input<t0>)|Assert<p1 a1>(Input<t1>)|TableEq(t1,t0);"
		"PredicateEq(p1,p0);AttrsEq(a1,a0)",
	};

	for (ULONG ul = 0; ul < GPOS_ARRAY_SIZE(rgsz); ul++)
	{
		if (!FRoundTrips(mp, rgsz[ul]))
		{
			return GPOS_FAILED;
		}
	}
	return GPOS_OK;
}

//---------------------------------------------------------------------------
// Symbol arity: wrong number of symbols inside <...> is rejected; correct
// number accepted.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLParserTest::EresUnittest_SymbolArity()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// Filter needs 2 (<pred attrs>); one is an error.
	CDSLRule *bad = Parse(mp, "Filter<p0>(Input<t0>)|Input<t1>|TableEq(t1,t0)");
	if (nullptr != bad)
	{
		bad->Release();
		return GPOS_FAILED;
	}
	// Agg accepts the corpus's 5-symbol form and current SQLSolver's 6-symbol
	// extension, but neither 4 nor 7 symbols.
	bad = Parse(mp, "Agg<a0 a1 f0 s0>(Input<t0>)|Input<t1>|TableEq(t1,t0)");
	if (nullptr != bad)
	{
		bad->Release();
		return GPOS_FAILED;
	}
	bad = Parse(
		mp,
		"Agg<a0 a1 a2 a3 f0 s0 p0>(Input<t0>)|Input<t1>|TableEq(t1,t0)");
	if (nullptr != bad)
	{
		bad->Release();
		return GPOS_FAILED;
	}
	CDSLRule *agg5 = Parse(
		mp, "Agg<a0 a1 f0 s0 p0>(Input<t0>)|Input<t1>|TableEq(t1,t0)");
	CDSLRule *agg6 = Parse(
		mp,
		"Agg_max<a0 a1 a2 f0 s0 p0>(Input<t0>)|Input<t1>|TableEq(t1,t0)");
	if (nullptr == agg5 || nullptr == agg6)
	{
		CRefCount::SafeRelease(agg5);
		CRefCount::SafeRelease(agg6);
		return GPOS_FAILED;
	}
	agg5->Release();
	agg6->Release();
	// Union keeps the legacy zero-symbol form and accepts exactly two output
	// symbols; a partial output declaration is ambiguous and rejected.
	bad = Parse(
		mp, "Union<a0>(Input<t0>,Input<t1>)|Input<t2>|TableEq(t2,t0)");
	if (nullptr != bad)
	{
		bad->Release();
		return GPOS_FAILED;
	}
	CDSLRule *union0 = Parse(
		mp, "Union(Input<t0>,Input<t1>)|Input<t2>|TableEq(t2,t0)");
	CDSLRule *union2 = Parse(
		mp, "Union<a0 s0>(Input<t0>,Input<t1>)|Input<t2>|TableEq(t2,t0)");
	if (nullptr == union0 || nullptr == union2)
	{
		CRefCount::SafeRelease(union0);
		CRefCount::SafeRelease(union2);
		return GPOS_FAILED;
	}
	union0->Release();
	union2->Release();
	// Join keeps the legacy two-key form and independently accepts a complete
	// predicate/dependency triple, output pair, and keyed residual triple.
	// A three-attrs declaration is not the predicate-only <p a a> form.
	bad = Parse(
		mp,
		"InnerJoin<a0 a1 a2>(Input<t0>,Input<t1>)|Input<t2>|TableEq(t2,t0)");
	if (nullptr != bad)
	{
		bad->Release();
		return GPOS_FAILED;
	}
	CDSLRule *join2 = Parse(
		mp,
		"InnerJoin<a0 a1>(Input<t0>,Input<t1>)|Input<t2>|TableEq(t2,t0)");
	CDSLRule *join3 = Parse(
		mp,
		"InnerJoin<p0 a0 a1>(Input<t0>,Input<t1>)|Input<t2>|TableEq(t2,t0)");
	CDSLRule *join4 = Parse(
		mp,
		"InnerJoin<a0 a1 a2 s0>(Input<t0>,Input<t1>)|Input<t2>|TableEq(t2,t0)");
	CDSLRule *join5 = Parse(
		mp,
		"InnerJoin<a0 a1 p0 a2 a3>(Input<t0>,Input<t1>)|Input<t2>|TableEq(t2,t0)");
	CDSLRule *join7 = Parse(
		mp,
		"InnerJoin<a0 a1 a2 s0 p0 a3 a4>(Input<t0>,Input<t1>)|Input<t2>|TableEq(t2,t0)");
	bad = Parse(
		mp,
		"InnerJoin<a0 a1 a2 s0 p0 a3>(Input<t0>,Input<t1>)|Input<t2>|TableEq(t2,t0)");
	if (nullptr == join2 || nullptr == join3 || nullptr == join4 ||
		nullptr == join5 ||
		nullptr == join7 || nullptr != bad)
	{
		CRefCount::SafeRelease(join2);
		CRefCount::SafeRelease(join3);
		CRefCount::SafeRelease(join4);
		CRefCount::SafeRelease(join5);
		CRefCount::SafeRelease(join7);
		CRefCount::SafeRelease(bad);
		return GPOS_FAILED;
	}
	join2->Release();
	join3->Release();
	join4->Release();
	join5->Release();
	join7->Release();
	// SemiJoin always binds one complete predicate and its exact dependencies.
	bad = Parse(
		mp,
		"SemiJoin<a0 a1 a2>(Input<t0>,Input<t1>)|Input<t2>|TableEq(t2,t0)");
	CDSLRule *semiJoin = Parse(
		mp,
		"SemiJoin<p0 a0 a1>(Input<t0>,Input<t1>)|Input<t2>|TableEq(t2,t0)");
	if (nullptr != bad || nullptr == semiJoin)
	{
		CRefCount::SafeRelease(bad);
		CRefCount::SafeRelease(semiJoin);
		return GPOS_FAILED;
	}
	semiJoin->Release();
	// InSub keeps its equality-only one-symbol corpus form and accepts the
	// complete key/predicate/dependency extension, but no partial declaration.
	bad = Parse(
		mp,
		"InSubFilter<a0 a1>(Input<t0>,Input<t1>)|Input<t2>|TableEq(t2,t0)");
	CDSLRule *insub1 = Parse(
		mp, "InSubFilter<a0>(Input<t0>,Input<t1>)|Input<t2>|TableEq(t2,t0)");
	CDSLRule *insub5 = Parse(
		mp,
		"InSubFilter<a0 a1 p0 a2 a3>(Input<t0>,Input<t1>)|Input<t2>|"
		"TableEq(t2,t0)");
	if (nullptr != bad || nullptr == insub1 || nullptr == insub5)
	{
		CRefCount::SafeRelease(bad);
		CRefCount::SafeRelease(insub1);
		CRefCount::SafeRelease(insub5);
		return GPOS_FAILED;
	}
	insub1->Release();
	insub5->Release();
	// Correct arities parse.
	CDSLRule *ok =
		Parse(mp, "Filter<p0 a0>(Input<t0>)|Input<t1>|TableEq(t1,t0)");
	if (nullptr == ok)
	{
		return GPOS_FAILED;
	}
	ok->Release();
	return GPOS_OK;
}

//---------------------------------------------------------------------------
// Aliases: ExistsFilter->Exists, SimpleFilter/PlainFilter->Filter; Proj*
// distinct; SortAsc/SortDesc direction; Union* distinct. Verified via the
// canonical round-trip: an aliased input prints as its canonical form.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLParserTest::EresUnittest_Aliases()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	struct
	{
		const CHAR *in;
		const CHAR *canonical;
	} rg[] = {
		{"ExistsFilter(Input<t0>,Input<t1>)|Input<t2>|TableEq(t2,t0)",
		 "Exists(Input<t0>,Input<t1>)|Input<t2>|TableEq(t2,t0)"},
		{"NotExistsFilter(Input<t0>,Input<t1>)|Input<t2>|TableEq(t2,t0)",
		 "NotExists(Input<t0>,Input<t1>)|Input<t2>|TableEq(t2,t0)"},
		{"LeftSemiJoin<p0 a0 a1>(Input<t0>,Input<t1>)|Input<t2>|"
		 "TableEq(t2,t0)",
		 "SemiJoin<p0 a0 a1>(Input<t0>,Input<t1>)|Input<t2>|"
		 "TableEq(t2,t0)"},
		{"LeftSemiApply<p0 a0 a1 a2>(Input<t0>,Input<t1>)|Input<t2>|"
		 "TableEq(t2,t0)",
		 "SemiApply<p0 a0 a1 a2>(Input<t0>,Input<t1>)|Input<t2>|"
		 "TableEq(t2,t0)"},
		{"SimpleFilter<p0 a0>(Input<t0>)|Input<t1>|TableEq(t1,t0)",
		 "Filter<p0 a0>(Input<t0>)|Input<t1>|TableEq(t1,t0)"},
		{"PlainFilter<p0 a0>(Input<t0>)|Input<t1>|TableEq(t1,t0)",
		 "Filter<p0 a0>(Input<t0>)|Input<t1>|TableEq(t1,t0)"},
		{"Compute<e0 a0 s0>(Input<t0>)|Compute<e1 a1 s1>(Input<t1>)|"
		 "TableEq(t1,t0);ExprEq(e1,e0)",
		 "Compute<e0 a0 s0>(Input<t0>)|Compute<e1 a1 s1>(Input<t1>)|"
		 "TableEq(t1,t0);ExprListEq(e1,e0)"},
		{"SortDesc<a0>(Input<t0>)|SortDesc<a1>(Input<t1>)|TableEq(t1,t0);"
		 "AttrsEq(a1,a0)",
		 "SortDesc<a0>(Input<t0>)|SortDesc<a1>(Input<t1>)|TableEq(t1,t0);"
		 "AttrsEq(a1,a0)"},
	};

	for (ULONG ul = 0; ul < GPOS_ARRAY_SIZE(rg); ul++)
	{
		CDSLRule *rule = Parse(mp, rg[ul].in);
		if (nullptr == rule)
		{
			return GPOS_FAILED;
		}
		CWStringDynamic str(mp);
		COstreamString oss(&str);
		IOstream &os = oss;
		rule->OsPrint(os);
		rule->Release();
		CWStringConst expected(mp, rg[ul].canonical);
		if (!str.Equals(&expected))
		{
			return GPOS_FAILED;
		}
	}
	return GPOS_OK;
}

//---------------------------------------------------------------------------
// Shared symbol namespace: reusing a name on BOTH sides is a "value already
// present" error (WeTune BiMap); constraints correctly reference symbols
// across sides.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLParserTest::EresUnittest_SymbolNamespace()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// t0 declared on both sides -> redeclaration error.
	CDSLRule *bad = Parse(mp, "Input<t0>|Input<t0>|");
	if (nullptr != bad)
	{
		bad->Release();
		return GPOS_FAILED;
	}
	// disjoint names + cross-side constraint -> OK.
	CDSLRule *ok = Parse(mp, "Input<t0>|Input<t1>|TableEq(t1,t0)");
	if (nullptr == ok)
	{
		return GPOS_FAILED;
	}
	ok->Release();
	return GPOS_OK;
}

//---------------------------------------------------------------------------
// Constraints: arity enforced (Reference=4, Unique=2); unknown constraint
// rejected; constraint referencing an undeclared symbol rejected.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLParserTest::EresUnittest_Constraints()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// Reference with 2 args (needs 4) -> error.
	CDSLRule *bad = Parse(mp, "Input<t0>|Input<t1>|Reference(t1,t0)");
	if (nullptr != bad)
	{
		bad->Release();
		return GPOS_FAILED;
	}
	// unknown constraint name -> error.
	bad = Parse(mp, "Input<t0>|Input<t1>|Frobnicate(t1,t0)");
	if (nullptr != bad)
	{
		bad->Release();
		return GPOS_FAILED;
	}
	// constraint referencing undeclared symbol -> error.
	bad = Parse(mp, "Input<t0>|Input<t1>|TableEq(t1,t9)");
	if (nullptr != bad)
	{
		bad->Release();
		return GPOS_FAILED;
	}
	// well-formed Reference(4) + Unique(2) -> OK.
	CDSLRule *ok = Parse(
		mp,
		"LeftJoin<a0 a1>(Input<t0>,Input<t1>)|InnerJoin<a2 a3>(Input<t2>,"
		"Input<t3>)|Reference(t0,a0,t1,a1);Unique(t0,a0);TableEq(t2,t0);"
		"TableEq(t3,t1);AttrsEq(a2,a0);AttrsEq(a3,a1)");
	if (nullptr == ok)
	{
		return GPOS_FAILED;
	}
	ok->Release();
	return GPOS_OK;
}

//---------------------------------------------------------------------------
// Whitespace robustness: spaces around '|' ';' ',' are tolerated (WS skipped),
// while spaces INSIDE <...> remain the real symbol separator.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLParserTest::EresUnittest_Whitespace()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// spaces around bars and commas
	CDSLRule *ok = Parse(
		mp, "Filter<p0 a0>(Input<t0>) | Input<t1> | TableEq(t1, t0)");
	if (nullptr == ok)
	{
		return GPOS_FAILED;
	}
	ok->Release();
	return GPOS_OK;
}

//---------------------------------------------------------------------------
// Loader: EQ-only admission. A buffer with an EQ rule, a NEQ rule and a
// comment/blank line yields exactly one admitted rule and one skipped.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLParserTest::EresUnittest_Loader()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	const CHAR *sz_buf =
		"# a comment\n"
		"\n"
		"Input<t0>|Input<t1>|TableEq(t1,t0)\tEQ\n"
		"Proj<a0 s0>(Input<t0>)|Proj<a1 s1>(Input<t1>)|TableEq(t1,t0);"
		"AttrsEq(a1,a0);SchemaEq(s1,s0)\tNEQ\n";

	CDSLRuleLoader::SLoadStats stats;
	CWStringDynamic strErrs(mp);
	CDSLRuleArray *pdrg = CDSLRuleLoader::PdrgpdslruleLoadBuffer(
		mp, sz_buf, true /*fEqOnly*/, &stats, &strErrs);

	BOOL ok = (nullptr != pdrg) && (1 == pdrg->Size()) &&
			  (3 == (*pdrg)[0]->UlSourceLine()) &&
			  (1 == stats.ul_admitted) && (1 == stats.ul_skipped) &&
			  (0 == stats.ul_failed);
	if (nullptr != pdrg)
	{
		pdrg->Release();
	}
	return ok ? GPOS_OK : GPOS_FAILED;
}

//---------------------------------------------------------------------------
// Errors: a syntactically broken rule returns NULL and a non-empty message,
// and does not crash.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLParserTest::EresUnittest_Errors()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	CWStringDynamic strErr(mp);
	// missing target segment
	CDSLRule *bad =
		CDSLRuleParser::PdslruleParse(mp, "Filter<p0 a0>(Input<t0>)", nullptr,
									  &strErr);
	if (nullptr != bad)
	{
		bad->Release();
		return GPOS_FAILED;
	}
	if (0 == strErr.Length())
	{
		return GPOS_FAILED;
	}
	return GPOS_OK;
}
