//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLRuleParser.h
//
//	@doc:
//		Front door of the DSL parser: turns one rule-DSL string into a validated
//		template-IR CDSLRule. Wraps the ANTLR4-generated lexer/parser and the
//		semantic checks that mirror WeTune (operator aliases, per-operator symbol
//		count/kind, shared source/target symbol namespace, constraint arity).
//
//		This header deliberately exposes NO ANTLR types — all ANTLR4 machinery,
//		std::string and exception handling are confined to CDSLRuleParser.cpp so
//		they never leak into the rest of gpopt.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLRuleParser_H
#define GPOPT_CDSLRuleParser_H

#include "gpos/base.h"
#include "gpos/string/CWStringDynamic.h"

#include "gpopt/dsl/CDSLRule.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CDSLRuleParser
//
//	@doc:
//		Stateless facade. Parse one rule at a time.
//---------------------------------------------------------------------------
class CDSLRuleParser
{
public:
	// Parse a single rule DSL string of the form
	//   <source>|<target>|<constraints>?
	// Returns a newly created, ref-counted CDSLRule (caller owns one ref and
	// must Release it), or NULL on any syntax/semantic error. When it returns
	// NULL and pstrErr != NULL, pstrErr is filled with a human-readable message.
	// sz_verdict (may be NULL) is stored as proof metadata on the rule.
	static CDSLRule *PdslruleParse(CMemoryPool *mp, const CHAR *sz_dsl,
								   const CHAR *sz_verdict,
								   CWStringDynamic *pstrErr);
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLRuleParser_H
