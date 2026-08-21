//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLExistsMatcher.h
//
//	@doc:
//		Match Exists(left,right) both before subquery unnesting
//		(Select(left, ScalarSubqueryExists(right))) and after it
//		(CLogicalLeftSemiApply(left, LIMIT 1 right, TRUE)). Translator-generated
//		conjuncts beside EXISTS are preserved as dedicated residuals; LIMIT 1 is
//		an internal uncorrelated-subquery optimization transparent to the DSL.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLExistsMatcher_H
#define GPOPT_CDSLExistsMatcher_H

#include "gpos/base.h"

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

class CDSLMatcher;

class CDSLExistsMatcher
{
private:
	CMemoryPool *m_mp;
	const CDSLMatcher *m_pmatcher;

	// True only for the exact LIMIT 1/OFFSET 0 wrapper inserted by
	// CSubqueryHandler::FRemoveExistentialSubquery.
	BOOL FExistsLimitOne(CExpression *pexpr) const;

public:
	CDSLExistsMatcher(const CDSLExistsMatcher &) = delete;

	CDSLExistsMatcher(CMemoryPool *mp, const CDSLMatcher *pmatcher)
		: m_mp(mp), m_pmatcher(pmatcher)
	{
		GPOS_ASSERT(nullptr != mp);
		GPOS_ASSERT(nullptr != pmatcher);
	}

	BOOL FMatch(const CDSLOp *pop, CExpression *pexpr,
				CDSLModel *pmodel) const;
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLExistsMatcher_H
