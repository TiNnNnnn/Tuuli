//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLUnionMatcher.h
//
//	@doc:
//		Stage-1 matcher for Union / Union*. WeTune's binary rule shape is matched
//		against an exact associative view when ORCA has flattened a set-op chain;
//		the ordered set-op column maps are retained for target instantiation.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLUnionMatcher_H
#define GPOPT_CDSLUnionMatcher_H

#include "gpos/base.h"

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

class CDSLMatcher;

class CDSLUnionMatcher
{
private:
	CMemoryPool *m_mp;
	const CDSLMatcher *m_pmatcher;

public:
	CDSLUnionMatcher(const CDSLUnionMatcher &) = delete;

	CDSLUnionMatcher(CMemoryPool *mp, const CDSLMatcher *pmatcher)
		: m_mp(mp), m_pmatcher(pmatcher)
	{
		GPOS_ASSERT(nullptr != mp);
		GPOS_ASSERT(nullptr != pmatcher);
	}

	BOOL FMatch(const CDSLOp *popUnion, CExpression *pexprUnion,
				CDSLModel *pmodel) const;
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLUnionMatcher_H
