//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CXformDSLRule_LeftJoin.h
//
//	@doc:
//		Thin exploration-xform shell whose SOURCE root operator is
//		CLogicalLeftOuterJoin. It owns no rewrite logic of its own: on Transform it
//		hands the matched expression to the shared CDSLRuleEngine, which applies
//		every loaded DSL rule bucketed under EopLogicalLeftOuterJoin (see
//		docs/DSL_WETUNE_ALIGNMENT.md M2 — this is the shell for the flagship
//		LeftJoin->InnerJoin rewrite guarded by Reference + NotNull).
//
//		Pattern: CLogicalLeftOuterJoin(CPatternLeaf, CPatternLeaf, CPatternLeaf) —
//		two relational children and a scalar join predicate, all loose; the engine
//		(CDSLJoinMatcher) narrows per rule.
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDSLRule_LeftJoin_H
#define GPOPT_CXformDSLRule_LeftJoin_H

#include "gpos/base.h"

#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CXformDSLRule_LeftJoin
//
//	@doc:
//		DSL-rule shell rooted at CLogicalLeftOuterJoin.
//---------------------------------------------------------------------------
class CXformDSLRule_LeftJoin : public CXformExploration
{
private:
public:
	CXformDSLRule_LeftJoin(const CXformDSLRule_LeftJoin &) = delete;

	// ctor
	explicit CXformDSLRule_LeftJoin(CMemoryPool *mp);

	// dtor
	~CXformDSLRule_LeftJoin() override = default;

	// ident accessors
	EXformId
	Exfid() const override
	{
		return ExfDSLRuleLeftJoin;
	}

	const CHAR *
	SzId() const override
	{
		return "CXformDSLRule_LeftJoin";
	}

	// compute xform promise for a given expression handle
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;

	// actual transform
	void Transform(CXformContext *, CXformResult *,
				   CExpression *) const override;

};	// class CXformDSLRule_LeftJoin

}  // namespace gpopt

#endif	// !GPOPT_CXformDSLRule_LeftJoin_H

// EOF
