//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CXformDSLRule_Select.h
//
//	@doc:
//		Thin exploration-xform shell whose SOURCE root operator is
//		CLogicalSelect. It owns no rewrite logic of its own: on Transform it
//		hands the matched expression to the shared CDSLRuleEngine, which applies
//		every loaded DSL rule bucketed under EopLogicalSelect.
//
//		This is one of the ~6-8 shells (one per WeTune source-root operator kind)
//		that, together with the engine, replace what would otherwise be many
//		hand-written C++ xforms. See docs/DSL_XFORM_ENGINE_DESIGN.md §五.
//
//		Pattern: CLogicalSelect(CPatternLeaf, CPatternLeaf) — a relational child
//		and a scalar predicate child, both loose; the engine narrows per rule.
//
//		PHASE 1: the shell + its 3 compile-time wirings are real, so ORCA
//		dispatches Transform() after CREATE EXTENSION. The engine call is made,
//		but the engine's match/check/instantiate are stubs (no rewrite yet).
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDSLRule_Select_H
#define GPOPT_CXformDSLRule_Select_H

#include "gpos/base.h"

#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CXformDSLRule_Select
//
//	@doc:
//		DSL-rule shell rooted at CLogicalSelect.
//---------------------------------------------------------------------------
class CXformDSLRule_Select : public CXformExploration
{
private:
public:
	CXformDSLRule_Select(const CXformDSLRule_Select &) = delete;

	// ctor
	explicit CXformDSLRule_Select(CMemoryPool *mp);

	// dtor
	~CXformDSLRule_Select() override = default;

	// ident accessors
	EXformId
	Exfid() const override
	{
		return ExfDSLRuleSelect;
	}

	const CHAR *
	SzId() const override
	{
		return "CXformDSLRule_Select";
	}

	// compute xform promise for a given expression handle
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;

	// actual transform
	void Transform(CXformContext *, CXformResult *,
				   CExpression *) const override;

};	// class CXformDSLRule_Select

}  // namespace gpopt

#endif	// !GPOPT_CXformDSLRule_Select_H

// EOF
