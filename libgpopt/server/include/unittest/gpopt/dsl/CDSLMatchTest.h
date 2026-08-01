//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLMatchTest.h
//
//	@doc:
//		Migrated WeTune Match tests (the generic skeleton slice, task #24):
//		Input opaque-subtree binding, operator-identity gate, and relational-child
//		recursion. Filter-chain / join-key / attrs binding are covered by their
//		own components' tests (#25 / #27).
//
//		WeTune source basis: optimizer/Match.java (matchOne INPUT / operator
//		dispatch), see docs/WETUNE_TEST_MIGRATION.md §7.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLMatchTest_H
#define GPOPT_CDSLMatchTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

class CDSLMatchTest
{
public:
	static GPOS_RESULT EresUnittest();

	// Input<t0> matches ANY relational subtree and binds t0 to it
	static GPOS_RESULT EresUnittest_InputBindsAnySubtree();

	// Filter<..>(Input) matches a live Select(Get), binding the Input child
	static GPOS_RESULT EresUnittest_SelectRootMatchesAndRecurses();

	// InnerJoin<..>(Input,Input) matches a live InnerJoin, binding both children
	static GPOS_RESULT EresUnittest_JoinRootMatchesBothChildren();

	// operator-identity gate: a Select-rooted template rejects a bare Get
	static GPOS_RESULT EresUnittest_IdentityGateRejects();
};	// class CDSLMatchTest
}  // namespace gpopt

#endif	// !GPOPT_CDSLMatchTest_H
