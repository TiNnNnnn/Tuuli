//---------------------------------------------------------------------------
// Generic matcher for quantified comparison filters (ANY / ALL).
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLQuantifiedMatcher_H
#define GPOPT_CDSLQuantifiedMatcher_H

#include "gpos/base.h"

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

class CDSLMatcher;

// Any<p a>(outer, inner) and All<p a>(outer, inner) bind the complete binary
// comparison to p and its outer dependencies to a. The matcher deliberately
// does not constrain p to equality, so the same framework represents every
// SQL quantified comparison supported by ORCA metadata.
class CDSLQuantifiedMatcher
{
private:
	CMemoryPool *m_mp;
	const CDSLMatcher *m_pmatcher;

	BOOL FMatchInner(const CDSLOp *popInner, CExpression *pexprInner,
					 CColRefArray *pdrgpcrProjected,
					 CDSLModel *pmodel) const;
	CExpression *PexprComparison(CExpression *pexprSubquery) const;

public:
	CDSLQuantifiedMatcher(const CDSLQuantifiedMatcher &) = delete;

	CDSLQuantifiedMatcher(CMemoryPool *mp, const CDSLMatcher *pmatcher)
		: m_mp(mp), m_pmatcher(pmatcher)
	{
		GPOS_ASSERT(nullptr != mp);
		GPOS_ASSERT(nullptr != pmatcher);
	}

	BOOL FMatch(const CDSLOp *pop, CExpression *pexpr,
				CDSLModel *pmodel) const;
};
}  // namespace gpopt

#endif  // !GPOPT_CDSLQuantifiedMatcher_H
