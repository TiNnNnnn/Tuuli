//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLRuleLoader.h
//
//	@doc:
//		Loads a rule library — a text file (or in-memory buffer) with one rule
//		DSL per line — into an array of validated CDSLRule objects.
//
//		Line format:  <source>|<target>|<constraints>?  [<TAB> verdict]
//		  * blank lines and lines starting with '#' are ignored
//		  * an optional TAB-separated trailing field carries the proof verdict
//		    (e.g. "EQ"); when fEqOnly is set, only verdict=="EQ" rules (or lines
//		    with no verdict field, treated as trusted) are admitted — mirroring
//		    the MONSOON trust chain: ORCA applies rules without re-checking
//		    equivalence, so only formally-proven rules should be loaded.
//
//		A malformed line does not abort the load: it is counted and its error
//		recorded (up to a cap), so one bad rule can't sink the whole library.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLRuleLoader_H
#define GPOPT_CDSLRuleLoader_H

#include "gpos/base.h"
#include "gpos/string/CWStringDynamic.h"

#include "gpopt/dsl/CDSLRule.h"

namespace gpopt
{
using namespace gpos;

using CDSLRuleArray = CDynamicPtrArray<CDSLRule, CleanupRelease>;

//---------------------------------------------------------------------------
//	@class:
//		CDSLRuleLoader
//
//	@doc:
//		Stateless loader. Both entry points return a newly created, ref-counted
//		CDSLRuleArray (caller owns one ref). Statistics (admitted / skipped /
//		failed) and the first few error messages are reported through the
//		out-params when non-NULL.
//---------------------------------------------------------------------------
class CDSLRuleLoader
{
public:
	struct SLoadStats
	{
		ULONG ul_admitted = 0;	// parsed OK and admitted
		ULONG ul_skipped = 0;	// valid but filtered out (non-EQ under fEqOnly)
		ULONG ul_failed = 0;	// parse/semantic error
	};

	// Parse every rule line in an in-memory buffer.
	static CDSLRuleArray *PdrgpdslruleLoadBuffer(CMemoryPool *mp,
												 const CHAR *sz_content,
												 BOOL fEqOnly,
												 SLoadStats *pstats,
												 CWStringDynamic *pstrErrs);

	// Read a file and delegate to PdrgpdslruleLoadBuffer. Returns NULL if the
	// file cannot be opened (pstrErrs describes the failure).
	static CDSLRuleArray *PdrgpdslruleLoadFile(CMemoryPool *mp,
											   const CHAR *sz_path,
											   BOOL fEqOnly, SLoadStats *pstats,
											   CWStringDynamic *pstrErrs);
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLRuleLoader_H
