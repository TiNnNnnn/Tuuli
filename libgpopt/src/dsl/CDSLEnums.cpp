//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLEnums.cpp
//
//	@doc:
//		Static lookup tables for the DSL operator/constraint enums. Every
//		The shared prefix is transcribed from WeTune's Java source. Compute, Expr,
//		and ExprListEq extend the shared DSL for explicit ComputeScalar/LET
//		semantics and are appended so existing enum identities remain stable.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLEnums.h"

#include "gpos/common/clibwrapper.h"

using namespace gpopt;

namespace
{
// Per-operator descriptor. Order MUST match EDslOpKind.
struct SDslOpDesc
{
	EDslOpKind edslop;
	const CHAR *sz_name;   // canonical printed name
	ULONG ul_children;	   // WeTune OpKind.numPredecessors
	ULONG ul_syms;		   // count of positional <...> symbols
	EDslSymbolKind rgesymk[GPOPT_DSL_MAX_OP_SYMS];	// positional symbol kinds
};

// The authoritative operator table. Sources:
//   - name + child arity  : OpKind.java  (numPredecessors, text)
//   - positional symbols   : SymbolsImpl.bindSymbol() + FragmentUtils.bindNames()
// WeTune positional order (INSIDE <...>), verified against fewshot rules:
//   Input      <t>                        Filter<p0 a1> => [pred, attrs]
//   InnerJoin  <[a a] [a s] p a a> (optional keys/output, predicate and its
//                                     per-child dependencies)
//   LeftJoin   <[a a] [a s] p a a>
//   Filter     <p a>   (predicate, attrs)      <-- pred FIRST
//   InSubFilter<a [a p a a]> (outer key, optional inner key, residual predicate
//                              and its per-child dependencies)
//   Exists     <[p a a]> (optional predicate and per-child dependencies)
//   NotExists  <>      (no symbols)
//   Any        <p a>   (comparison predicate, outer dependencies)
//   All        <p a>   (comparison predicate, outer dependencies)
//   Proj       <a s>   (attrs, schema)
//   Agg        <a a a f s p> (groupBy, aggAttrs, aggOutAttrs, func, schema, havingPred)
//   Sort       <a>     (attrs)
//   Limit      <n n>   (limit, offset)
//   Union      <a s>   (optional ordered full-row output attrs/schema)
//   Compute    <e a s> (exact expression list, dependencies, defined columns)
//   SemiJoin   <p a a> (complete predicate, left/right dependencies)
const SDslOpDesc rg_op_desc[] = {
	{EdslopInput, "Input", 0, 1, {EdslsymTable}},
	{EdslopInnerJoin, "InnerJoin", 2, 7,
	 {EdslsymAttrs, EdslsymAttrs, EdslsymAttrs, EdslsymSchema,
	  EdslsymPred, EdslsymAttrs, EdslsymAttrs}},
	{EdslopLeftJoin, "LeftJoin", 2, 7,
	 {EdslsymAttrs, EdslsymAttrs, EdslsymAttrs, EdslsymSchema,
	  EdslsymPred, EdslsymAttrs, EdslsymAttrs}},
	{EdslopFilter, "Filter", 1, 3,
	 {EdslsymPred, EdslsymAttrs, EdslsymAttrs}},
	{EdslopInSubFilter, "InSubFilter", 2, 5,
	 {EdslsymAttrs, EdslsymAttrs, EdslsymPred, EdslsymAttrs,
	  EdslsymAttrs}},
	{EdslopExists, "Exists", 2, 0, {}},
	{EdslopNotExists, "NotExists", 2, 0, {}},
	{EdslopProj, "Proj", 1, 2, {EdslsymAttrs, EdslsymSchema}},
	{EdslopAgg, "Agg", 1, 6,
	 {EdslsymAttrs, EdslsymAttrs, EdslsymAttrs, EdslsymFunc, EdslsymSchema,
	  EdslsymPred}},
	{EdslopSort, "Sort", 1, 1, {EdslsymAttrs}},
	{EdslopLimit, "Limit", 1, 2, {EdslsymScalar, EdslsymScalar}},
	{EdslopUnion, "Union", 2, 2, {EdslsymAttrs, EdslsymSchema}},
	{EdslopCompute, "Compute", 1, 3,
	 {EdslsymExpr, EdslsymAttrs, EdslsymSchema}},
	{EdslopAny, "Any", 2, 2, {EdslsymPred, EdslsymAttrs}},
	{EdslopAll, "All", 2, 2, {EdslsymPred, EdslsymAttrs}},
	{EdslopEmpty, "Empty", 0, 1, {EdslsymTable}},
	{EdslopSemiJoin, "SemiJoin", 2, 3,
	 {EdslsymPred, EdslsymAttrs, EdslsymAttrs}},
	{EdslopSemiApply, "SemiApply", 2, 4,
	 {EdslsymPred, EdslsymAttrs, EdslsymAttrs, EdslsymAttrs}},
	{EdslopAntiJoin, "AntiJoin", 2, 3,
	 {EdslsymPred, EdslsymAttrs, EdslsymAttrs}},
	{EdslopAntiApply, "AntiApply", 2, 4,
	 {EdslsymPred, EdslsymAttrs, EdslsymAttrs, EdslsymAttrs}},
	{EdslopInnerApply, "InnerApply", 2, 4,
	 {EdslsymPred, EdslsymAttrs, EdslsymAttrs, EdslsymAttrs}},
	{EdslopLeftOuterApply, "LeftApply", 2, 4,
	 {EdslsymPred, EdslsymAttrs, EdslsymAttrs, EdslsymAttrs}},
	{EdslopAntiJoinNotIn, "AntiJoinNotIn", 2, 6,
	 {EdslsymPred, EdslsymAttrs, EdslsymAttrs, EdslsymPred,
	  EdslsymAttrs, EdslsymAttrs}},
	{EdslopAntiApplyNotIn, "AntiApplyNotIn", 2, 4,
	 {EdslsymPred, EdslsymAttrs, EdslsymAttrs, EdslsymAttrs}},
	{EdslopWindowRows, "WindowRows", 1, 3,
	 {EdslsymAttrs, EdslsymOrder, EdslsymWindow}},
	{EdslopWindowFrame, "Window", 1, 4,
	 {EdslsymAttrs, EdslsymOrder, EdslsymFrame, EdslsymWindow}},
	{EdslopMaxOneRow, "MaxOneRow", 1, 0, {}},
	{EdslopAssertMaxOneRow, "AssertMaxOneRow", 1, 0, {}},
	{EdslopAssert, "Assert", 1, 2, {EdslsymPred, EdslsymAttrs}},
};

const ULONG ul_num_ops = GPOS_ARRAY_SIZE(rg_op_desc);

const SDslOpDesc *
PdescOp(EDslOpKind edslop)
{
	GPOS_ASSERT(EdslopSentinel != edslop);
	GPOS_ASSERT(rg_op_desc[edslop].edslop == edslop &&
				"EDslOpKind order out of sync with rg_op_desc");
	return &rg_op_desc[edslop];
}

// Constraint descriptor. Order MUST match EDslConstraintKind.
struct SDslConDesc
{
	EDslConstraintKind edslcon;
	const CHAR *sz_name;
	ULONG ul_arity;
};

// Source: Constraint.java Kind(numSyms).
const SDslConDesc rg_con_desc[] = {
	{EdslconTableEq, "TableEq", 2},		{EdslconAttrsEq, "AttrsEq", 2},
	{EdslconPredicateEq, "PredicateEq", 2}, {EdslconSchemaEq, "SchemaEq", 2},
	{EdslconFuncEq, "FuncEq", 2},		{EdslconScalarEq, "ScalarEq", 2},
	{EdslconAttrsSub, "AttrsSub", 2},	{EdslconUnique, "Unique", 2},
	{EdslconNotNull, "NotNull", 2},		{EdslconReference, "Reference", 4},
	{EdslconErrorFree, "ErrorFree", 1},
	{EdslconDeterministic, "Deterministic", 1},
	{EdslconExprListEq, "ExprListEq", 2},
	{EdslconExprConcat, "ExprConcat", 3},
	{EdslconExprDepsDisjoint, "ExprDepsDisjoint", 2},
	{EdslconExprSplit, "ExprSplit", 4},
	{EdslconAttrsIntersect, "AttrsIntersect", 3},
	{EdslconPredicateFalse, "PredicateFalse", 1},
	{EdslconScalarOne, "ScalarOne", 1},
	{EdslconScalarZero, "ScalarZero", 1},
	{EdslconAttrsEmpty, "AttrsEmpty", 1},
	{EdslconPredicateAnd, "PredicateAnd", 3},
	{EdslconAttrsUnion, "AttrsUnion", 3},
	{EdslconExprFilterCommute, "ExprFilterCommute", 3},
	{EdslconAggFilterCommute, "AggFilterCommute", 7},
	{EdslconSchemaUnion, "SchemaUnion", 3},
	{EdslconAggCorrelationPullup, "AggCorrelationPullup", 12},
	{EdslconMinimalGrouping, "MinimalGrouping", 2},
	{EdslconCorrelationEquality, "CorrelationEquality", 3},
	{EdslconAggCorrelationGrouping, "AggCorrelationGrouping", 10},
	{EdslconQuantifiedPredicateEq, "QuantifiedPredicateEq", 2},
	{EdslconOrderEq, "OrderEq", 2},
	{EdslconWindowEq, "WindowEq", 2},
	{EdslconFrameEq, "FrameEq", 2},
	{EdslconWindowCorrelationPartition, "WindowCorrelationPartition", 6},
	{EdslconWindowFrameCorrelationPartition,
	 "WindowFrameCorrelationPartition", 7},
};

const ULONG ul_num_cons = GPOS_ARRAY_SIZE(rg_con_desc);

// symbol-prefix letters, indexed by EDslSymbolKind
const CHAR rg_sym_prefix[] = {'t', 'a', 'p', 's', 'f', 'n', 'e', 'o', 'w', 'm'};
}  // namespace

// ---------------------------------------------------------------------------
// CDSLOpKindTable
// ---------------------------------------------------------------------------

const CHAR *
CDSLOpKindTable::SzName(EDslOpKind edslop)
{
	return PdescOp(edslop)->sz_name;
}

ULONG
CDSLOpKindTable::UlChildren(EDslOpKind edslop)
{
	return PdescOp(edslop)->ul_children;
}

ULONG
CDSLOpKindTable::UlSyms(EDslOpKind edslop)
{
	return PdescOp(edslop)->ul_syms;
}

EDslSymbolKind
CDSLOpKindTable::EsymkindAt(EDslOpKind edslop, ULONG ul)
{
	const SDslOpDesc *pdesc = PdescOp(edslop);
	GPOS_ASSERT(ul < pdesc->ul_syms);
	return pdesc->rgesymk[ul];
}

COperator::EOperatorId
CDSLOpKindTable::Eopid(EDslOpKind edslop, BOOL fDistinct)
{
	switch (edslop)
	{
		case EdslopFilter:
			return COperator::EopLogicalSelect;
		case EdslopProj:
			// Proj* (dedup projection) => ORCA SELECT DISTINCT => CLogicalGbAgg
			// (grouping = projected cols, empty agg list); plain Proj => Project.
			return fDistinct ? COperator::EopLogicalGbAgg
							 : COperator::EopLogicalProject;
		case EdslopCompute:
			return COperator::EopLogicalProject;
		case EdslopInnerJoin:
			return COperator::EopLogicalInnerJoin;
		case EdslopLeftJoin:
			return COperator::EopLogicalLeftOuterJoin;
		case EdslopSemiJoin:
			return COperator::EopLogicalLeftSemiJoin;
		case EdslopSemiApply:
			return COperator::EopLogicalLeftSemiApply;
		case EdslopAntiJoin:
			return COperator::EopLogicalLeftAntiSemiJoin;
		case EdslopAntiApply:
			return COperator::EopLogicalLeftAntiSemiApply;
		case EdslopAntiJoinNotIn:
			return COperator::EopLogicalLeftAntiSemiJoinNotIn;
		case EdslopAntiApplyNotIn:
			return COperator::EopLogicalLeftAntiSemiApplyNotIn;
		case EdslopInnerApply:
			return COperator::EopLogicalInnerApply;
		case EdslopLeftOuterApply:
			return COperator::EopLogicalLeftOuterApply;
		case EdslopAgg:
			return COperator::EopLogicalGbAgg;
		case EdslopExists:
			return COperator::EopLogicalLeftSemiApply;
		case EdslopNotExists:
			return COperator::EopLogicalLeftAntiSemiApply;
		case EdslopInSubFilter:
			return COperator::EopLogicalLeftSemiApplyIn;
		case EdslopAny:
			return COperator::EopLogicalLeftSemiApplyIn;
		case EdslopAll:
			return COperator::EopLogicalLeftAntiSemiApplyNotIn;
		case EdslopUnion:
			// Union* (dedup) => UNION => set semantics; Union => UNION ALL.
			return fDistinct ? COperator::EopLogicalUnion
							 : COperator::EopLogicalUnionAll;
		case EdslopSort:
		case EdslopLimit:
			// ORCA fuses ORDER BY and LIMIT/OFFSET in CLogicalLimit. The DSL
			// matcher exposes the fused node as the virtual shape
			// Limit(Sort(child)); both DSL roots therefore share one shell.
			return COperator::EopLogicalLimit;
		case EdslopWindowRows:
		case EdslopWindowFrame:
			return COperator::EopLogicalSequenceProject;
		case EdslopMaxOneRow:
			return COperator::EopLogicalMaxOneRow;
		case EdslopAssertMaxOneRow:
			// This is a semantic DSL operator whose target builder expands ORCA's
			// canonical SequenceProject + Assert implementation.
			return COperator::EopLogicalAssert;
		case EdslopAssert:
			return COperator::EopLogicalAssert;
		case EdslopEmpty:
			return COperator::EopLogicalConstTableGet;
		case EdslopInput:
			// base-relation placeholder; no logical op — matched as a subtree.
			return COperator::EopSentinel;
		default:
			return COperator::EopSentinel;
	}
}

BOOL
CDSLOpKindTable::FHasPreUnnestRepresentation(EDslOpKind edslop)
{
	return EdslopExists == edslop || EdslopNotExists == edslop ||
		   EdslopInSubFilter == edslop || EdslopAny == edslop ||
		   EdslopAll == edslop;
}

BOOL
CDSLOpKindTable::FMatcherSupported(EDslOpKind edslop)
{
	switch (edslop)
	{
		case EdslopInput:
		case EdslopInnerJoin:
		case EdslopLeftJoin:
		case EdslopFilter:
		case EdslopInSubFilter:
		case EdslopExists:
		case EdslopNotExists:
		case EdslopAny:
		case EdslopAll:
		case EdslopProj:
		case EdslopAgg:
		case EdslopUnion:
		case EdslopSort:
		case EdslopLimit:
		case EdslopCompute:
		case EdslopEmpty:
		case EdslopSemiJoin:
		case EdslopSemiApply:
		case EdslopAntiJoin:
		case EdslopAntiApply:
		case EdslopAntiJoinNotIn:
		case EdslopAntiApplyNotIn:
		case EdslopInnerApply:
		case EdslopLeftOuterApply:
		case EdslopWindowRows:
		case EdslopWindowFrame:
		case EdslopMaxOneRow:
		case EdslopAssert:
			return true;
		case EdslopAssertMaxOneRow:
		case EdslopSentinel:
			return false;
	}
	return false;
}

BOOL
CDSLOpKindTable::FInstantiatorSupported(EDslOpKind edslop)
{
	switch (edslop)
	{
		case EdslopInput:
		case EdslopInnerJoin:
		case EdslopLeftJoin:
		case EdslopFilter:
		case EdslopInSubFilter:
		case EdslopExists:
		case EdslopNotExists:
		case EdslopAny:
		case EdslopAll:
		case EdslopProj:
		case EdslopAgg:
		case EdslopUnion:
		case EdslopSort:
		case EdslopLimit:
		case EdslopCompute:
		case EdslopEmpty:
		case EdslopSemiJoin:
		case EdslopSemiApply:
		case EdslopAntiJoin:
		case EdslopAntiApply:
		case EdslopAntiJoinNotIn:
		case EdslopAntiApplyNotIn:
		case EdslopInnerApply:
		case EdslopLeftOuterApply:
		case EdslopWindowRows:
		case EdslopWindowFrame:
		case EdslopAssertMaxOneRow:
		case EdslopAssert:
			return true;
		case EdslopMaxOneRow:
		case EdslopSentinel:
			return false;
	}
	return false;
}

BOOL
CDSLOpKindTable::FSourceRootDispatchSupported(EDslOpKind edslop,
										   BOOL fDistinct)
{
	// SequenceProject is currently supported as a nested template node.  A
	// standalone Window-root rule needs its own Cascade shell before the audit
	// may advertise it as dispatchable.
	if (EdslopWindowRows == edslop || EdslopWindowFrame == edslop)
	{
		return false;
	}
	return EdslopInput != edslop && FMatcherSupported(edslop) &&
		   COperator::EopSentinel != Eopid(edslop, fDistinct);
}

EDslOpKind
CDSLOpKindTable::Parse(const CHAR *sz_token, BOOL *pfStar,
					   EDslSortDir *pedslsort,
					   EDslAggFuncKind *pedslaggfunc)
{
	GPOS_ASSERT(nullptr != sz_token);
	*pfStar = false;
	*pedslsort = EdslsortNone;
	*pedslaggfunc = EdslaggfuncUnknown;

	// Strip a trailing '*' (Proj* / Union*). WeTune sets a dedup flag for it.
	CHAR rgch[64];
	clib::Strncpy(rgch, sz_token, GPOS_ARRAY_SIZE(rgch) - 1);
	rgch[GPOS_ARRAY_SIZE(rgch) - 1] = '\0';
	ULONG ul_len = clib::Strlen(rgch);
	if (0 < ul_len && '*' == rgch[ul_len - 1])
	{
		*pfStar = true;
		rgch[ul_len - 1] = '\0';
	}

	// Alias resolution, mirroring WeTune OpKind.parse (case labels).
	struct SAlias
	{
		const CHAR *sz;
		EDslOpKind edslop;
		EDslSortDir edslsort;
	};
	static const SAlias rg_alias[] = {
		{"Input", EdslopInput, EdslsortNone},
		{"Empty", EdslopEmpty, EdslsortNone},
		{"InnerJoin", EdslopInnerJoin, EdslsortNone},
		{"LeftJoin", EdslopLeftJoin, EdslsortNone},
		{"SemiJoin", EdslopSemiJoin, EdslsortNone},
		{"LeftSemiJoin", EdslopSemiJoin, EdslsortNone},
		{"SemiApply", EdslopSemiApply, EdslsortNone},
		{"LeftSemiApply", EdslopSemiApply, EdslsortNone},
		{"AntiJoin", EdslopAntiJoin, EdslsortNone},
		{"LeftAntiSemiJoin", EdslopAntiJoin, EdslsortNone},
		{"AntiApply", EdslopAntiApply, EdslsortNone},
		{"LeftAntiSemiApply", EdslopAntiApply, EdslsortNone},
		{"AntiJoinNotIn", EdslopAntiJoinNotIn, EdslsortNone},
		{"LeftAntiSemiJoinNotIn", EdslopAntiJoinNotIn, EdslsortNone},
		{"AntiApplyNotIn", EdslopAntiApplyNotIn, EdslsortNone},
		{"LeftAntiSemiApplyNotIn", EdslopAntiApplyNotIn, EdslsortNone},
		{"InnerApply", EdslopInnerApply, EdslsortNone},
		{"LeftApply", EdslopLeftOuterApply, EdslsortNone},
		{"LeftOuterApply", EdslopLeftOuterApply, EdslsortNone},
		{"Filter", EdslopFilter, EdslsortNone},
		{"PlainFilter", EdslopFilter, EdslsortNone},
		{"SimpleFilter", EdslopFilter, EdslsortNone},
		{"InSubFilter", EdslopInSubFilter, EdslsortNone},
		{"SubqueryFilter", EdslopInSubFilter, EdslsortNone},
		{"InSub", EdslopInSubFilter, EdslsortNone},
		{"Exists", EdslopExists, EdslsortNone},
		{"ExistsFilter", EdslopExists, EdslsortNone},
		{"NotExists", EdslopNotExists, EdslsortNone},
		{"NotExistsFilter", EdslopNotExists, EdslsortNone},
		{"Any", EdslopAny, EdslsortNone},
		{"AnyFilter", EdslopAny, EdslsortNone},
		{"All", EdslopAll, EdslsortNone},
		{"AllFilter", EdslopAll, EdslsortNone},
		{"Proj", EdslopProj, EdslsortNone},
		{"Compute", EdslopCompute, EdslsortNone},
		{"Agg", EdslopAgg, EdslsortNone},
		{"Agg_sum", EdslopAgg, EdslsortNone},
		{"Agg_average", EdslopAgg, EdslsortNone},
		{"Agg_count", EdslopAgg, EdslsortNone},
		{"Agg_max", EdslopAgg, EdslsortNone},
		{"Agg_min", EdslopAgg, EdslsortNone},
		{"Union", EdslopUnion, EdslsortNone},
		{"Limit", EdslopLimit, EdslsortNone},
		{"Sort", EdslopSort, EdslsortNone},
		{"SortAsc", EdslopSort, EdslsortAsc},
		{"SortDesc", EdslopSort, EdslsortDesc},
		{"WindowRows", EdslopWindowRows, EdslsortNone},
		{"Window", EdslopWindowFrame, EdslsortNone},
		{"MaxOneRow", EdslopMaxOneRow, EdslsortNone},
		{"AssertMaxOneRow", EdslopAssertMaxOneRow, EdslsortNone},
		{"Assert", EdslopAssert, EdslsortNone},
	};
	for (ULONG ul = 0; ul < GPOS_ARRAY_SIZE(rg_alias); ul++)
	{
		if (0 == clib::Strcmp(rgch, rg_alias[ul].sz))
		{
			if (0 == clib::Strcmp(rgch, "Agg_sum"))
			{
				*pedslaggfunc = EdslaggfuncSum;
			}
			else if (0 == clib::Strcmp(rgch, "Agg_average"))
			{
				*pedslaggfunc = EdslaggfuncAverage;
			}
			else if (0 == clib::Strcmp(rgch, "Agg_count"))
			{
				*pedslaggfunc = EdslaggfuncCount;
			}
			else if (0 == clib::Strcmp(rgch, "Agg_max"))
			{
				*pedslaggfunc = EdslaggfuncMax;
			}
			else if (0 == clib::Strcmp(rgch, "Agg_min"))
			{
				*pedslaggfunc = EdslaggfuncMin;
			}

			// '*' is meaningful only for Proj, Union, and COUNT(DISTINCT ...).
			if (*pfStar && EdslopProj != rg_alias[ul].edslop &&
				EdslopUnion != rg_alias[ul].edslop &&
				!(EdslopAgg == rg_alias[ul].edslop &&
				  EdslaggfuncCount == *pedslaggfunc))
			{
				return EdslopSentinel;
			}
			*pedslsort = rg_alias[ul].edslsort;
			return rg_alias[ul].edslop;
		}
	}
	return EdslopSentinel;
}

const CHAR *
CDSLOpKindTable::SzAggFuncName(EDslAggFuncKind edslaggfunc)
{
	switch (edslaggfunc)
	{
		case EdslaggfuncSum:
			return "sum";
		case EdslaggfuncAverage:
			return "average";
		case EdslaggfuncCount:
			return "count";
		case EdslaggfuncMax:
			return "max";
		case EdslaggfuncMin:
			return "min";
		default:
			return nullptr;
	}
}

CHAR
CDSLOpKindTable::WcSymPrefix(EDslSymbolKind esymk)
{
	GPOS_ASSERT(EdslsymSentinel != esymk);
	return rg_sym_prefix[esymk];
}

// ---------------------------------------------------------------------------
// CDSLConstraintKindTable
// ---------------------------------------------------------------------------

const CHAR *
CDSLConstraintKindTable::SzName(EDslConstraintKind edslcon)
{
	GPOS_ASSERT(EdslconSentinel != edslcon);
	GPOS_ASSERT(rg_con_desc[edslcon].edslcon == edslcon &&
				"EDslConstraintKind order out of sync with rg_con_desc");
	return rg_con_desc[edslcon].sz_name;
}

ULONG
CDSLConstraintKindTable::UlArity(EDslConstraintKind edslcon)
{
	GPOS_ASSERT(EdslconSentinel != edslcon);
	return rg_con_desc[edslcon].ul_arity;
}

EDslConstraintKind
CDSLConstraintKindTable::Parse(const CHAR *sz_name)
{
	GPOS_ASSERT(nullptr != sz_name);
	// WeTune rewrites the legacy "Pick" prefix to "Attrs" for backward compat
	// (ConstraintImpl.parse: fields[0].replace("Pick","Attrs")). Handle
	// "PickSub"->"AttrsSub" and "PickEq"->"AttrsEq" by canonicalising first.
	CHAR rgch[64];
	if (0 == clib::Strncmp(sz_name, "Pick", 4))
	{
		clib::Strncpy(rgch, "Attrs", 6);
		clib::Strncpy(rgch + 5, sz_name + 4, GPOS_ARRAY_SIZE(rgch) - 6);
		rgch[GPOS_ARRAY_SIZE(rgch) - 1] = '\0';
		sz_name = rgch;
	}
	// Pre-canonical Compute prototypes used ExprEq. Accept it as an input alias,
	// but always stringify the shared WeTune name ExprListEq.
	if (0 == clib::Strcmp(sz_name, "ExprEq"))
	{
		sz_name = "ExprListEq";
	}
	for (ULONG ul = 0; ul < ul_num_cons; ul++)
	{
		if (0 == clib::Strcmp(sz_name, rg_con_desc[ul].sz_name))
		{
			return rg_con_desc[ul].edslcon;
		}
	}
	return EdslconSentinel;
}

BOOL
CDSLConstraintKindTable::FCheckerSupported(EDslConstraintKind edslcon)
{
	switch (edslcon)
	{
		case EdslconTableEq:
		case EdslconAttrsEq:
		case EdslconPredicateEq:
		case EdslconSchemaEq:
		case EdslconFuncEq:
		case EdslconScalarEq:
		case EdslconAttrsSub:
		case EdslconUnique:
		case EdslconNotNull:
		case EdslconReference:
		case EdslconErrorFree:
		case EdslconDeterministic:
		case EdslconExprListEq:
		case EdslconExprConcat:
		case EdslconExprDepsDisjoint:
		case EdslconExprSplit:
		case EdslconAttrsIntersect:
		case EdslconPredicateFalse:
		case EdslconScalarOne:
		case EdslconScalarZero:
		case EdslconAttrsEmpty:
		case EdslconPredicateAnd:
		case EdslconAttrsUnion:
		case EdslconExprFilterCommute:
		case EdslconAggFilterCommute:
		case EdslconSchemaUnion:
		case EdslconAggCorrelationPullup:
		case EdslconMinimalGrouping:
		case EdslconCorrelationEquality:
		case EdslconAggCorrelationGrouping:
		case EdslconQuantifiedPredicateEq:
		case EdslconOrderEq:
		case EdslconWindowEq:
		case EdslconFrameEq:
		case EdslconWindowCorrelationPartition:
		case EdslconWindowFrameCorrelationPartition:
			return true;
		case EdslconSentinel:
			return false;
	}
	return false;
}
