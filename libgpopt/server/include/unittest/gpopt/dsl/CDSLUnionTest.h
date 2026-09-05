//---------------------------------------------------------------------------
//	MONSOON DSL Union tests
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLUnionTest_H
#define GPOPT_CDSLUnionTest_H

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

class CDSLUnionTest
{
public:
	static GPOS_RESULT EresUnittest();
	static GPOS_RESULT EresUnittest_MatchAndDistinctGate();
	static GPOS_RESULT EresUnittest_SetOpKindsMatchAndInstantiate();
	static GPOS_RESULT EresUnittest_IntersectInputBindingsBuildJoin();
	static GPOS_RESULT EresUnittest_NarySetOpUsesAssociativeView();
	static GPOS_RESULT EresUnittest_InstantiatePreservesColumnMaps();
	static GPOS_RESULT EresUnittest_OutputBindingBuildsFullRowDedup();
	static GPOS_RESULT EresUnittest_DistinctUnionViewMatchesFullRowDedup();
	static GPOS_RESULT EresUnittest_GroupingSubsetPushesBelowDistinctUnion();
	static GPOS_RESULT EresUnittest_GroupingSubsetPushesBelowUnionAll();
	static GPOS_RESULT EresUnittest_SwapsBranchesByConstraints();
	static GPOS_RESULT EresUnittest_RejectsRemapAcrossOptimizerGbAgg();
	static GPOS_RESULT EresUnittest_CorpusTwoProjects();
	static GPOS_RESULT EresUnittest_CorpusNestedDistinctProjects();
	static GPOS_RESULT EresUnittest_JoinDistributionBuildsFreshBranches();
	static GPOS_RESULT EresUnittest_LeftJoinDistributionBuildsFreshBranches();
	static GPOS_RESULT EresUnittest_JoinDistributionRejectsDistinctUnion();
	static GPOS_RESULT EresUnittest_SharedBranchesUseCTE();
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLUnionTest_H
