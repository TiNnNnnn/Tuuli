//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLJoinElimTest.h
//
//	@doc:
//		Join-elimination three-stage tests (base "A").
//
//		Migrates the SEMANTICS of WeTune's canonical join-elimination rules
//		(sqlsolver_data/prepared/rules.txt), which collapse a projection over a
//		two-table join into a projection over a SINGLE table when the dropped
//		side contributes no rows and no duplication:
//
//		  line 205 (LeftJoin elimination — Unique-guarded, base-A verifiable):
//		    Proj<a2 s0>(LeftJoin<a0 a1>(Input<t0>,Input<t1>))
//		      | Proj<a3 s1>(Input<t2>)
//		      | AttrsSub(a0,t0);AttrsSub(a1,t1);AttrsSub(a2,t0);
//		        Unique(t1,a1);
//		        TableEq(t2,t0);AttrsEq(a3,a2);SchemaEq(s1,s0)
//		    (a LEFT join never drops left rows; Unique(t1,a1) forbids duplication;
//		     the projection reads only t0 cols, so t1 is dead — drop the join.)
//
//		  line 180 (InnerJoin elimination — FK-guarded, needs a live relcache):
//		    Proj<a2 s0>(InnerJoin<a0 a1>(Input<t0>,Input<t1>))
//		      | Proj<a3 s1>(Input<t2>)
//		      | ...NotNull(t0,a0);Reference(t0,a0,t1,a1);Unique(t1,a1);...
//		    (NotNull + FK + Unique => every t0 row matches exactly one t1 row, so
//		     the inner join is row-preserving over t0 — drop it.)
//
//		These rules compose only ALREADY-implemented building blocks (Proj match
//		M1, Join match M2, Input, Unique/NotNull/Reference checks, alias-driven
//		instantiation). No new operator, matcher, or shell is required: the source
//		root Proj recurses into the Join through the generic matcher dispatch, and
//		the target Proj-over-Input reuses the source projlist over t0's Get while
//		the alias map (TableEq/AttrsEq/SchemaEq) drops the join and t1.
//
//		This is the first rule where the TARGET structure differs from the SOURCE
//		(Proj-over-Join -> Proj-over-Input): it strictly REMOVES an operator, so —
//		unlike the equivalent LeftJoin->InnerJoin replacement — a live cost model
//		must prefer it (see docs/ORCA_TRACE_GUIDE.md §5).
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLJoinElimTest_H
#define GPOPT_CDSLJoinElimTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CDSLJoinElimTest
//
//	@doc:
//		Unit tests for join elimination (Proj-over-Join -> Proj-over-Input).
//---------------------------------------------------------------------------
class CDSLJoinElimTest
{
public:
	static GPOS_RESULT EresUnittest();

	// LeftJoin elimination (rules.txt line 205) fires end-to-end under Unique:
	// match + check + instantiate produce Proj(Get t0) with the join and t1 gone.
	static GPOS_RESULT EresUnittest_LeftJoinElimFires();
	static GPOS_RESULT EresUnittest_LeftJoinElimBelowAgg();

	// the same rule must NOT fire when t1's join key is not unique (the Unique
	// guard would let the join duplicate left rows).
	static GPOS_RESULT EresUnittest_LeftJoinElimRejectsWithoutUnique();

	// InnerJoin elimination (rules.txt line 180) must NOT fire on the FK-less
	// programmatic fixture (Reference cannot be confirmed) — live-FK is base C.
	static GPOS_RESULT EresUnittest_InnerJoinElimRejectsWithoutFK();
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLJoinElimTest_H

// EOF
