//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CXformDSLRule_Project.h
//
//	@doc:
//		Thin exploration-xform shell whose SOURCE root operator is
//		CLogicalProject. Like CXformDSLRule_Select it owns no rewrite logic: on
//		Transform it hands the matched expression to the shared CDSLRuleEngine,
//		which applies every loaded DSL rule bucketed under EopLogicalProject.
//
//		One of the ~6-8 shells (one per WeTune source-root operator kind) that,
//		together with the engine, replace hand-written C++ xforms. See
//		docs/DSL_XFORM_ENGINE_DESIGN.md §五 and docs/DSL_WETUNE_ALIGNMENT.md M1.
//
//		Pattern: CLogicalProject(CPatternLeaf, CPatternLeaf) — a relational child
//		and a scalar project-list child, both loose; the engine narrows per rule.
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDSLRule_Project_H
#define GPOPT_CXformDSLRule_Project_H

#include "gpos/base.h"

#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CXformDSLRule_Project
//
//	@doc:
//		DSL-rule shell rooted at CLogicalProject.
//---------------------------------------------------------------------------
class CXformDSLRule_Project : public CXformExploration
{
private:
public:
	CXformDSLRule_Project(const CXformDSLRule_Project &) = delete;

	// ctor
	explicit CXformDSLRule_Project(CMemoryPool *mp);

	// dtor
	~CXformDSLRule_Project() override = default;

	// ident accessors
	EXformId
	Exfid() const override
	{
		return ExfDSLRuleProject;
	}

	const CHAR *
	SzId() const override
	{
		return "CXformDSLRule_Project";
	}

	// compute xform promise for a given expression handle
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;

	// actual transform
	void Transform(CXformContext *, CXformResult *,
				   CExpression *) const override;

};	// class CXformDSLRule_Project

}  // namespace gpopt

#endif	// !GPOPT_CXformDSLRule_Project_H

// EOF
