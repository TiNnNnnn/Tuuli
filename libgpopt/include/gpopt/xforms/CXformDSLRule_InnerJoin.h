//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CXformDSLRule_InnerJoin.h
//
//	@doc:
//		Thin exploration-xform shell whose SOURCE root operator is
//		CLogicalInnerJoin. It owns no rewrite logic of its own: on Transform it
//		hands the matched expression to the shared CDSLRuleEngine, which applies
//		every loaded DSL rule bucketed under EopLogicalInnerJoin (see
//		docs/DSL_XFORM_ENGINE_DESIGN.md §五, docs/DSL_WETUNE_ALIGNMENT.md M2).
//
//		Pattern: CLogicalInnerJoin(CPatternLeaf, CPatternLeaf, CPatternLeaf) — two
//		relational children and a scalar join predicate, all loose; the engine
//		(CDSLJoinMatcher) narrows per rule.
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDSLRule_InnerJoin_H
#define GPOPT_CXformDSLRule_InnerJoin_H

#include "gpos/base.h"

#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CXformDSLRule_InnerJoin
//
//	@doc:
//		DSL-rule shell rooted at CLogicalInnerJoin.
//---------------------------------------------------------------------------
class CXformDSLRule_InnerJoin : public CXformExploration
{
private:
public:
	CXformDSLRule_InnerJoin(const CXformDSLRule_InnerJoin &) = delete;

	// ctor
	explicit CXformDSLRule_InnerJoin(CMemoryPool *mp);

	// dtor
	~CXformDSLRule_InnerJoin() override = default;

	// ident accessors
	EXformId
	Exfid() const override
	{
		return ExfDSLRuleInnerJoin;
	}

	const CHAR *
	SzId() const override
	{
		return "CXformDSLRule_InnerJoin";
	}

	// compute xform promise for a given expression handle
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;

	// actual transform
	void Transform(CXformContext *, CXformResult *,
				   CExpression *) const override;

};	// class CXformDSLRule_InnerJoin

}  // namespace gpopt

#endif	// !GPOPT_CXformDSLRule_InnerJoin_H

// EOF
