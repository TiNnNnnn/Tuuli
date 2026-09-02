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
	EdslsymOrder,		 // o (complete window order-spec array)
	EdslsymWindow,		 // w (exact window project list)
	EdslsymFrame,		 // m (complete window-frame array)
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
	EdslopEmpty,		 // zero-row relation carrying a bound table schema
	EdslopSemiJoin,		 // explicit relational left semi join
	EdslopSemiApply,	 // dependent left semi apply with explicit correlations
	EdslopAntiJoin,		 // explicit relational left anti semi join
	EdslopAntiApply,	 // dependent left anti semi apply with explicit correlations
	EdslopInnerApply,	 // dependent inner apply with explicit correlations
	EdslopLeftOuterApply,  // dependent left outer apply with explicit correlations
	EdslopAntiJoinNotIn,   // NULL-aware relational left anti semi join
	EdslopAntiApplyNotIn,  // dependent NULL-aware anti apply
	EdslopWindowRows,	   // cumulative/default-frame window
	EdslopWindowFrame,	   // window with explicit frame metadata
	EdslopMaxOneRow,		   // scalar-subquery cardinality contract
	EdslopAssertMaxOneRow,  // executable assertion for the same contract
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
	// DepsDisjoint(left,right): dependency-bearing scalar/window metadata and
	// column vectors have disjoint column domains. This is the shared scope
	// primitive for movement.
	EdslconDepsDisjoint,
	EdslconExprSplit,
	// Ordered column-vector intersection. The first symbol is derived from the
	// second by retaining only columns exposed by the third symbol (a relational
	// subtree or another column vector). Attrs and Schema vectors are both
	// supported, which keeps the operation independent of any one DSL operator.
	EdslconAttrsIntersect,
	EdslconPredicateFalse,
	// Fixed scalar constructors used by target-side Limit/Top-N templates.
	// They are semantic predicates over any scalar symbol, not xform-specific
	// flags, and can also validate a source-side bound constant.
	EdslconScalarOne,
	EdslconScalarZero,
	// Empty ordered column vector. Source bindings are applicability guards;
	// target-only bindings are materialized by the instantiator.
	EdslconAttrsEmpty,
	// Non-empty ordered column vector, independent of any consuming operator.
	EdslconAttrsNonEmpty,
	// PredicateAnd(out,left,right): out is the canonical conjunction of two
	// predicate symbols. Target-only outputs are materialized lazily.
	EdslconPredicateAnd,
	// AttrsUnion(out,left,right): stable, duplicate-free union of two attribute
	// vectors. Target-only outputs are materialized lazily.
	EdslconAttrsUnion,
	// SchemaUnion(out,input,extra): stable, duplicate-free extension of an
	// aggregate output schema with relational attributes.
	EdslconSchemaUnion,
	// MinimalGrouping(group,schema): derive ORCA's child-dependent minimal
	// grouping metadata for the aggregate identified by its full grouping and
	// output schema.  This property does not change relational semantics.
	EdslconMinimalGrouping,
	// CorrelationEquality(pred,local,outer): pred is a non-empty conjunction of
	// cross-domain column equalities and covers exactly both dependency vectors.
	// This is operator-independent decorrelation evidence; aggregate grouping,
	// join construction, and other consumers remain separate constraints.
	EdslconCorrelationEquality,
	EdslconOrderEq,
	EdslconWindowEq,
	EdslconFrameEq,
	// OutputAttrs(out,relation): out is the complete ordered logical output
	// vector of relation. Target-only out symbols are materialized lazily.
	// Uniqueness is deliberately expressed by the independent Unique constraint.
	EdslconOutputAttrs,
	// SchemaFromAttrs(schema,attrs): schema is the same ordered concrete column
	// vector as attrs, crossing the DSL's distinct symbol namespaces explicitly.
	EdslconSchemaFromAttrs,
	// PredicateDomainSplit(source,residual,external,
	// residual_outer,residual_inner,external_local,external_outer,outer,inner):
	// partition one complete source predicate by the two current relational
	// domains. Predicate composition is expressed independently by PredicateAnd.
	// A conjunct mixing both current domains with an external domain is rejected.
	EdslconPredicateDomainSplit,
	// PredicateExists(out,input): out is EXISTS(input).
	EdslconPredicateExists,
	// PredicateNotExists(out,input): out is NOT EXISTS(input).
	EdslconPredicateNotExists,
	// PredicateAny(out,comparison,outer_attrs,input): out is comparison ANY(input).
	EdslconPredicateAny,
	// PredicateAll(out,comparison,outer_attrs,input): out is comparison ALL(input).
	EdslconPredicateAll,
	// PredicateScalarSubquery(out,lowered,left,right,correlation,input).
	EdslconPredicateScalarSubquery,
	// ExprListScalarSubquery(source,lowered,predicate,left,right,correlation,
	// output,input): replace the sole scalar subquery in an expression list.
	EdslconExprListScalarSubquery,
	// ExprListExists(source,lowered,marker_expr,marker_attrs,marker_schema,
	// predicate,left,right,correlation,required_inner,input).
	EdslconExprListExists,
	EdslconExprListNotExists,
	EdslconSentinel
};

// Max number of positional symbols any operator declares inside <...> (Agg = 6).
#define GPOPT_DSL_MAX_OP_SYMS 7

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

	// Fixed type of a constructively defined output position. Sentinel means
	// the position is an input/premise and cannot introduce a local symbol.
	static EDslSymbolKind EsymkindDerivedOutput(EDslConstraintKind edslcon,
											 ULONG ulPosition);

	// resolve a constraint name to its kind; also accepts the legacy "Pick*"
	// spelling WeTune rewrites to "Attrs*". Returns EdslconSentinel if unknown.
	static EDslConstraintKind Parse(const CHAR *sz_name);

	// Whether CDSLConstraintChecker implements this constraint kind.
	static BOOL FCheckerSupported(EDslConstraintKind edslcon);
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLEnums_H
