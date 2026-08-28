//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLExistsMatcher.h
//
//	@doc:
//		Match Exists/NotExists both before and after subquery unnesting. Positive
//		EXISTS maps to LeftSemiApply and may carry the native LIMIT 1 shell;
//		NOT EXISTS maps to LeftAntiSemiApply. Translator-generated sibling
//		conjuncts are preserved as dedicated residuals.
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
