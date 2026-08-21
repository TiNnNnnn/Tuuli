//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLParserTest.h
//
//	@doc:
//		Unit tests for the DSL rule parser (CDSLRuleParser) + loader
//		(CDSLRuleLoader): round-trip, symbol arity, operator aliases, shared
//		symbol namespace, constraint arity, whitespace robustness, EQ-only
//		admission, and error reporting.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLParserTest_H
#define GPOPT_CDSLParserTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CDSLParserTest
//
//	@doc:
//		Unittests for the DSL rule parser and loader.
//---------------------------------------------------------------------------
class CDSLParserTest
{
public:
	static GPOS_RESULT EresUnittest();

	static GPOS_RESULT EresUnittest_RoundTrip();
	static GPOS_RESULT EresUnittest_SymbolArity();
	static GPOS_RESULT EresUnittest_Aliases();
	static GPOS_RESULT EresUnittest_AggFunctions();
	static GPOS_RESULT EresUnittest_SymbolNamespace();
	static GPOS_RESULT EresUnittest_Constraints();
	static GPOS_RESULT EresUnittest_Whitespace();
	static GPOS_RESULT EresUnittest_Loader();
	static GPOS_RESULT EresUnittest_Errors();
};	// class CDSLParserTest
}  // namespace gpopt

#endif	// !GPOPT_CDSLParserTest_H
