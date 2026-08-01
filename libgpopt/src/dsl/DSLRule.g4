/*
 * DSLRule.g4 — ANTLR4 grammar for the WeTune rewrite-rule DSL.
 *
 * A rule is three '|'-separated segments:  <source> | <target> | <constraints>?
 * where each fragment is a prefix-functional operator tree, e.g.
 *   Filter<p0 a1>(SortAsc<a0>(Input<t0>))
 * and constraints are ';'-separated, e.g.
 *   AttrsSub(a0,t0);TableEq(t1,t0);AttrsEq(a2,a1)
 *
 * This grammar recognises STRUCTURE ONLY. All semantic validation — operator
 * name legality + aliases (ExistsFilter->Exists, SimpleFilter->Filter, ...),
 * the number/kind of symbols each operator declares inside <...> (Filter=<pred
 * attrs>, Agg=<a a f s p>, ...), and constraint arity (TableEq=2, Reference=4)
 * — is performed in the C++ listener (CDSLRuleBuilder), mirroring WeTune's
 * OpKind.parse / SymbolsImpl.bindSymbol / Constraint.parse. Keeping the grammar
 * purely structural lets the builder emit precise error messages and stay a
 * byte-for-byte behavioural match with WeTune's hand-written parser.
 *
 * Symbols always carry a numeric index (t0, a1, p0, s0, f0, n0) — WeTune never
 * emits a bare letter — so SYMBOL is [a-z][0-9]+, which cannot collide with the
 * capitalised operator/constraint identifiers matched by ID.
 */
grammar DSLRule;

// Generated C++ classes are emitted into the `dsl` namespace via the ANTLR
// `-package dsl` codegen flag (see libgpopt/src/dsl/CMakeLists.txt), keeping
// them out of gpopt's global scope. CDSLRuleParser.cpp uses dsl::DSLRuleParser.

// ---- parser rules ----------------------------------------------------------
// 'fragment' is a reserved word in ANTLR (fragment lexer rules), so the source
// and target trees are named 'frag'.
rule_       : frag BAR frag ( BAR constraints? )? EOF ;
frag        : op ;
op          : ID STAR? symlist? ( LP op ( COMMA op )* RP )? ;
symlist     : LT SYMBOL+ GT ;
constraints : constraint ( SEMI constraint )* ;
constraint  : ID LP SYMBOL ( COMMA SYMBOL )* RP ;

// ---- lexer rules -----------------------------------------------------------
BAR    : '|' ;
STAR   : '*' ;
LP     : '(' ;
RP     : ')' ;
LT     : '<' ;
GT     : '>' ;
COMMA  : ',' ;
SEMI   : ';' ;

// SYMBOL must precede ID; both are tried but ANTLR takes the longest match,
// and 't0' (len 2) beats 't' (len 1). Capitalised names ('Input') fail SYMBOL
// at char 0 and fall through to ID.
SYMBOL : [a-z] [0-9]+ ;
ID     : [A-Za-z]+ ;

WS     : [ \t\r\n]+ -> skip ;
