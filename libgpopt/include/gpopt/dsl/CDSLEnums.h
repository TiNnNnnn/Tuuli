//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLEnums.h
//
//	@doc:
//		Enumerations and static lookup tables for the WeTune rewrite-rule DSL.
//
//		This header is the SINGLE SOURCE OF TRUTH mirroring WeTune's Java model:
//		  - EDslOpKind          <-> wtune.superopt.fragment.OpKind
//		  - EDslSymbolKind       <-> wtune.superopt.fragment.Symbol.Kind
//		  - EDslConstraintKind   <-> wtune.superopt.constraint.Constraint.Kind
//		and the per-operator metadata that WeTune spreads across OpKind.java,
//		SymbolsImpl.bindSymbol() and FragmentUtils.bindNames():
//		  - child arity           (OpKind.numPredecessors)
//		  - positional symbol kinds inside <...>  (bindSymbol / bindNames order)
//		  - name aliases accepted on input        (OpKind.parse)
//		plus the mapping each DSL operator -> ORCA COperator::EOperatorId used to
//		build the template tree out of ORCA's logical-operator vocabulary.
//
//		Keeping every WeTune-derived constant here (rather than scattered) means
//		the parser, the listener/builder, the round-trip printer and the xform
//		engine all agree by construction.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLEnums_H
#define GPOPT_CDSLEnums_H

#include "gpos/base.h"

#include "gpopt/operators/COperator.h"

namespace gpopt
{
using namespace gpos;

// Kind of a DSL symbol. The first six entries mirror the legacy WeTune prefix;
// Expr is the shared EXPR_LIST kind for an exact scalar expression list.
// DSL prefix letters (t/a/p/s/f/n/e) are only a naming CONVENTION; the
// authoritative kind of a symbol is decided POSITIONALLY by the operator that
// declares it (see rgul_dsl_op_sym_schema below), exactly as WeTune does in
// SymbolsImpl.bindSymbol().
enum EDslSymbolKind
{
	EdslsymTable = 0,	 // t
	EdslsymAttrs,		 // a
	EdslsymPred,		 // p
	EdslsymSchema,		 // s
	EdslsymFunc,		 // f
	EdslsymScalar,		 // n
	EdslsymExpr,		 // e (exact CScalarProjectList / expression list)
	EdslsymSentinel
};

// Kind of a DSL operator. The first 11 entries mirror WeTune OpKind. Compute is
// MONSOON's explicit ORCA ComputeScalar/LET operator; unlike WeTune Proj it does
// not prune the relational child's columns.
enum EDslOpKind
{
	EdslopInput = 0,
	EdslopInnerJoin,
	EdslopLeftJoin,
	EdslopFilter,		 // WeTune SIMPLE_FILTER
	EdslopInSubFilter,
	EdslopExists,		 // WeTune EXISTS_FILTER
	EdslopNotExists,	 // negated existential subquery filter
	EdslopProj,
	EdslopAgg,
	EdslopSort,
	EdslopLimit,
	EdslopUnion,		 // WeTune SET_OP
	EdslopCompute,		 // ORCA CLogicalProject / ComputeScalar
	EdslopAny,			 // quantified comparison: predicate is TRUE for some row
	EdslopAll,			 // quantified comparison: predicate is TRUE for every row
	EdslopSentinel
};

// Sort direction carried by a Sort operator (WeTune SortAsc / SortDesc / Sort).
enum EDslSortDir
{
	EdslsortNone = 0,	 // bare "Sort"
	EdslsortAsc,
	EdslsortDesc
};

// Optional aggregate-function kind encoded by newer SQLSolver spellings such
// as Agg_count. MONSOON's established rules use bare Agg, which leaves the kind
// unknown and matches the bound aggregate expression(s) without name filtering.
enum EDslAggFuncKind
{
	EdslaggfuncUnknown = 0,
	EdslaggfuncSum,
	EdslaggfuncAverage,
	EdslaggfuncCount,
	EdslaggfuncMax,
	EdslaggfuncMin,
	EdslaggfuncSentinel
};

// Which side of the rule a symbol was first declared on. WeTune shares ONE
// SymbolNaming across source and target, and reusing a name on both sides is a
// parse error ("value already present"); we record the side to reproduce that.
enum EDslSide
{
	EdslsideSource = 0,
	EdslsideTarget
};

// Kind of a DSL constraint. The through-Reference prefix mirrors WeTune
// Constraint.Kind in the same order. Scalar-safety and expression-list
// constraints preserve the established pgORCA append-only identities; names
// and arities are shared with WeTune even where later enum ordinals differ.
enum EDslConstraintKind
{
	EdslconTableEq = 0,
	EdslconAttrsEq,
	EdslconPredicateEq,
	EdslconSchemaEq,
	EdslconFuncEq,
	EdslconScalarEq,
	EdslconAttrsSub,
	EdslconUnique,
	EdslconNotNull,
	EdslconReference,
	EdslconErrorFree,
	EdslconDeterministic,
	EdslconExprListEq,
	EdslconExprConcat,
	EdslconExprDepsDisjoint,
	EdslconExprSplit,
	EdslconSentinel
};

// Max number of positional symbols any operator declares inside <...> (Agg = 6).
#define GPOPT_DSL_MAX_OP_SYMS 6

//---------------------------------------------------------------------------
//	@class:
//		CDSLOpKindTable
//
//	@doc:
//		Static, allocation-free lookup for per-operator metadata. All methods
//		are pure functions of the enum; no state, safe to call anywhere.
//---------------------------------------------------------------------------
class CDSLOpKindTable
{
public:
	// canonical printed name, e.g. EdslopFilter -> "Filter" (no '*', no dir)
	static const CHAR *SzName(EDslOpKind edslop);

	// number of relational children (WeTune OpKind.numPredecessors)
	static ULONG UlChildren(EDslOpKind edslop);

	// number of positional symbols inside <...>
	static ULONG UlSyms(EDslOpKind edslop);

	// kind of the i-th positional symbol (i < UlSyms)
	static EDslSymbolKind EsymkindAt(EDslOpKind edslop, ULONG ul);

	// ORCA logical operator this DSL operator maps to; returns
	// COperator::EopSentinel for operators with no direct logical counterpart
	// (Input placeholder; Sort/Limit which ORCA has no independent logical op
	// for). fDistinct selects Union vs UnionAll and Proj vs dedup GbAgg.
	static COperator::EOperatorId Eopid(EDslOpKind edslop, BOOL fDistinct);

	// True when the DSL operator is a SQL subquery filter whose translated
	// expression has two supported matching representations: a Select carrying
	// a scalar subquery before unnesting, and an Apply operator afterwards.
	// Dispatch uses this operator capability (not a particular rule shape) to
	// choose exactly one representation in an optimization run.
	static BOOL FHasPreUnnestRepresentation(EDslOpKind edslop);

	// Static execution capabilities used by both diagnostics and corpus audit.
	// These describe whether the current generic engine has a matcher/target
	// builder for an operator kind; they do not claim that a particular live
	// SQL expression will satisfy a rule's bindings or metadata constraints.
	static BOOL FMatcherSupported(EDslOpKind edslop);
	static BOOL FInstantiatorSupported(EDslOpKind edslop);
	static BOOL FSourceRootDispatchSupported(EDslOpKind edslop,
										 BOOL fDistinct);

	// resolve an input operator token (with aliases + '*'/dir suffixes stripped)
	// to a kind. Mirrors WeTune OpKind.parse. Returns EdslopSentinel if unknown.
	// On return, *pfStar / *pedslsort report the parsed '*' and Sort direction.
	static EDslOpKind Parse(const CHAR *sz_token, BOOL *pfStar,
							EDslSortDir *pedslsort,
							EDslAggFuncKind *pedslaggfunc);

	// canonical suffix used after "Agg_"; NULL for Unknown/Sentinel.
	static const CHAR *SzAggFuncName(EDslAggFuncKind edslaggfunc);

	// letter conventionally used to print a symbol of this kind (t/a/p/s/f/n)
	static CHAR WcSymPrefix(EDslSymbolKind esymk);
};

//---------------------------------------------------------------------------
//	@class:
//		CDSLConstraintKindTable
//
//	@doc:
//		Static lookup for constraint metadata (name + arity). Mirrors WeTune
//		Constraint.Kind.
//---------------------------------------------------------------------------
class CDSLConstraintKindTable
{
public:
	// printed name, e.g. EdslconAttrsSub -> "AttrsSub"
	static const CHAR *SzName(EDslConstraintKind edslcon);

	// number of symbol arguments (TableEq..NotNull = 2, Reference = 4)
	static ULONG UlArity(EDslConstraintKind edslcon);

	// resolve a constraint name to its kind; also accepts the legacy "Pick*"
	// spelling WeTune rewrites to "Attrs*". Returns EdslconSentinel if unknown.
	static EDslConstraintKind Parse(const CHAR *sz_name);

	// Whether CDSLConstraintChecker implements this constraint kind.
	static BOOL FCheckerSupported(EDslConstraintKind edslcon);
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLEnums_H
