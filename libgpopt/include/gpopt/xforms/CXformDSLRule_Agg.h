//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CXformDSLRule_Agg.h
//
//	@doc:
//		Thin exploration-xform shell whose SOURCE root operator is an ORCA
//		aggregate/dedup shell. Like the other DSL shells it owns no rewrite logic: on
//		Transform it hands the matched expression to the shared CDSLRuleEngine,
//		which applies every loaded DSL rule bucketed under EopLogicalGbAgg.
//
//		This shell is what a Proj* (deduplicated projection) rule fires on, since
//		SELECT DISTINCT is a CLogicalGbAgg in ORCA. The same matcher also supports
//		five/six-symbol real aggregate rules; HAVING rules are additionally routed
//		through the Select shell. See docs/DSL_WETUNE_ALIGNMENT.md.
//
//		Pattern: CPatternTree. Registration limits the xform to CLogicalGbAgg and
//		CLogicalGbAggDeduplicate, while the complete-tree binding lets their shared
//		matcher inspect both relational and scalar children.
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDSLRule_Agg_H
#define GPOPT_CXformDSLRule_Agg_H

#include "gpos/base.h"

#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CXformDSLRule_Agg
//
//	@doc:
//		DSL-rule shell rooted at CLogicalGbAgg.
//---------------------------------------------------------------------------
class CXformDSLRule_Agg : public CXformExploration
{
private:
public:
	CXformDSLRule_Agg(const CXformDSLRule_Agg &) = delete;

	// ctor
	explicit CXformDSLRule_Agg(CMemoryPool *mp);

	// dtor
	~CXformDSLRule_Agg() override = default;

	// ident accessors
	EXformId
	Exfid() const override
	{
		return ExfDSLRuleAgg;
	}

	const CHAR *
	SzId() const override
	{
		return "CXformDSLRule_Agg";
	}

	// compute xform promise for a given expression handle
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;

	// actual transform
	void Transform(CXformContext *, CXformResult *,
				   CExpression *) const override;

};	// class CXformDSLRule_Agg

}  // namespace gpopt

#endif	// !GPOPT_CXformDSLRule_Agg_H

// EOF
