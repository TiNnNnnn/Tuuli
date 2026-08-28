//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLEngineTest.cpp
//
//	@doc:
//		Phase-1 tests: shell registration + engine bucketing key + stub safety.
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLEngineTest.h"

#include "gpos/base.h"
#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/dsl/CDSLRuleLoader.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/dsl/CDSLRulePrefixIndex.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CLogicalUnion.h"
#include "gpopt/operators/CLogicalUnionAll.h"
#include "gpopt/operators/CPatternTree.h"
#include "gpopt/search/CGroup.h"
#include "gpopt/search/CGroupExpression.h"
#include "gpopt/search/CGroupProxy.h"
#include "gpopt/search/CJobJoinEnumeration.h"
#include "gpopt/xforms/CXform.h"
#include "gpopt/xforms/CXformDSLRule_Select.h"
#include "gpopt/xforms/CXformFactory.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CDSLEngineTest::EresUnittest
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLEngineTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CDSLEngineTest::EresUnittest_ShellRegistered),
		GPOS_UNITTEST_FUNC(CDSLEngineTest::EresUnittest_SelectDispatches),
		GPOS_UNITTEST_FUNC(CDSLEngineTest::EresUnittest_UnionShellsDispatch),
		GPOS_UNITTEST_FUNC(CDSLEngineTest::EresUnittest_LimitShellDispatch),
		GPOS_UNITTEST_FUNC(CDSLEngineTest::EresUnittest_Bucketing),
		GPOS_UNITTEST_FUNC(CDSLEngineTest::EresUnittest_PrefixIndex),
		GPOS_UNITTEST_FUNC(
			CDSLEngineTest::EresUnittest_SubqueryRepresentationCapability),
		GPOS_UNITTEST_FUNC(CDSLEngineTest::EresUnittest_CapabilityMetadata),
		GPOS_UNITTEST_FUNC(CDSLEngineTest::EresUnittest_DSLProvenance),
		GPOS_UNITTEST_FUNC(
			CDSLEngineTest::EresUnittest_DPHyperNativeOwnership),
		GPOS_UNITTEST_FUNC(
			CDSLEngineTest::EresUnittest_ReachableTransientEmptyGroup),
		GPOS_UNITTEST_FUNC(CDSLEngineTest::EresUnittest_StubsCallable),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

GPOS_RESULT
CDSLEngineTest::EresUnittest_DPHyperNativeOwnership()
{
	if (!CXform::FDSLShell(CXform::ExfDSLRuleLimit) ||
		CXform::FDSLShell(CXform::ExfDPHyperJoinRegion) ||
		!CXform::FPGORCAExploration(CXform::ExfDPHyperJoinRegion) ||
		CXform::FPGORCAExploration(CXform::ExfSimplifyGbAgg))
	{
		return GPOS_FAILED;
	}
	CXformFactory *factory = CXformFactory::Pxff();
	if (nullptr == factory)
	{
		return GPOS_FAILED;
	}

	ULONG join_enumeration = 0;
	ULONG owned = 0;
	for (ULONG ul = 0; ul < CXform::ExfDSLRuleSelect; ul++)
	{
		const CXform::EXformId exfid = static_cast<CXform::EXformId>(ul);
		if (!CJobJoinEnumeration::FNativeJoinEnumerationXform(exfid))
		{
			continue;
		}
		if (!factory->IsXformIdUsed(exfid) ||
			!factory->Pxf(exfid)->FExploration())
		{
			return GPOS_FAILED;
		}
		join_enumeration++;
		owned += CJobJoinEnumeration::FReplacesNativeXform(exfid) ? 1 : 0;
	}

	return 19 == join_enumeration && 15 == owned &&
			!CJobJoinEnumeration::FReplacesNativeXform(
				CXform::ExfSimplifyGbAgg) &&
			!CJobJoinEnumeration::FReplacesNativeXform(
				CXform::ExfImplementFullOuterMergeJoin)
		? GPOS_OK
		: GPOS_FAILED;
}

namespace
{
CExpression *
PexprPrefixLeaf(CMemoryPool *mp)
{
	return GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp));
}

CExpression *
PexprPrefixJoin(CMemoryPool *mp, CExpression *pexprLeft,
				CExpression *pexprRight)
{
	return GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalInnerJoin(mp), pexprLeft, pexprRight,
		PexprPrefixLeaf(mp));
}

CExpression *
PexprPrefixGbAgg(CMemoryPool *mp, COperator::EGbAggType egbaggtype,
				 CExpression *pexprRel)
{
	return GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp)
			CLogicalGbAgg(mp, GPOS_NEW(mp) CColRefArray(mp), egbaggtype),
		pexprRel, PexprPrefixLeaf(mp));
}

CDSLRule *
PrulePrefix(CMemoryPool *mp, const CHAR *szRule)
{
	return CDSLRuleParser::PdslruleParse(mp, szRule, "EQ", nullptr);
}
}  // namespace

GPOS_RESULT
CDSLEngineTest::EresUnittest_PrefixIndex()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	CDSLRule *pruleAny = PrulePrefix(
		mp,
		"InnerJoin<a0 a1>(Input<t0>,Input<t1>)|Input<t2>|TableEq(t2,t0)");
	CDSLRule *pruleLeft = PrulePrefix(
		mp,
		"InnerJoin<a0 a1>(InnerJoin<a2 a3>(Input<t0>,Input<t1>),Input<t2>)|"
		"Input<t3>|TableEq(t3,t0)");
	CDSLRule *pruleRight = PrulePrefix(
		mp,
		"InnerJoin<a0 a1>(Input<t0>,InnerJoin<a2 a3>(Input<t1>,Input<t2>))|"
		"Input<t3>|TableEq(t3,t0)");
	CDSLRule *pruleFallback = PrulePrefix(
		mp,
		"Filter<p0 a0>(Input<t0>)|Input<t1>|TableEq(t1,t0)");
	if (nullptr == pruleAny || nullptr == pruleLeft ||
		nullptr == pruleRight || nullptr == pruleFallback)
	{
		CRefCount::SafeRelease(pruleAny);
		CRefCount::SafeRelease(pruleLeft);
		CRefCount::SafeRelease(pruleRight);
		CRefCount::SafeRelease(pruleFallback);
		return GPOS_FAILED;
	}

	CDSLRulePrefixIndex *pindex = GPOS_NEW(mp) CDSLRulePrefixIndex(mp);
	// Deliberately insert out of order: returned candidates must follow ordinal.
	pindex->Insert(pruleRight, 2, COperator::EopLogicalInnerJoin);
	pindex->Insert(pruleFallback, 3, COperator::EopLogicalInnerJoin);
	pindex->Insert(pruleAny, 0, COperator::EopLogicalInnerJoin);
	pindex->Insert(pruleLeft, 1, COperator::EopLogicalInnerJoin);

	CExpression *pexpr = PexprPrefixJoin(
		mp,
		PexprPrefixJoin(mp, PexprPrefixLeaf(mp), PexprPrefixLeaf(mp)),
		PexprPrefixLeaf(mp));
	CDSLRuleArray *pdrgprule = pindex->PdrgpruleCandidates(mp, pexpr);
	BOOL fValid =
		1 == pindex->UlFallbackRules() && 3 == pdrgprule->Size() &&
		pruleAny == (*pdrgprule)[0] && pruleLeft == (*pdrgprule)[1] &&
		pruleFallback == (*pdrgprule)[2];

	pdrgprule->Release();
	pexpr->Release();
	GPOS_DELETE(pindex);
	pruleFallback->Release();
	pruleRight->Release();
	pruleLeft->Release();
	pruleAny->Release();

	// Filter chains normalize to one Select. The trie indexes the stable Select
	// prefix and then distinguishes the exposed base relation. An Input base is
	// still the general wildcard candidate.
	CDSLRule *pruleFilterLeft = PrulePrefix(
		mp,
		"Filter<p0 a2>(LeftJoin<a0 a1>(Input<t0>,Input<t1>))|Input<t2>|"
		"TableEq(t2,t0)");
	CDSLRule *pruleFilterInner = PrulePrefix(
		mp,
		"Filter<p0 a2>(InnerJoin<a0 a1>(Input<t0>,Input<t1>))|Input<t2>|"
		"TableEq(t2,t0)");
	CDSLRule *pruleFilterAny = PrulePrefix(
		mp,
		"Filter<p1 a1>(Filter<p0 a0>(Input<t0>))|Input<t1>|"
		"TableEq(t1,t0)");
	if (nullptr == pruleFilterLeft || nullptr == pruleFilterInner ||
		nullptr == pruleFilterAny)
	{
		CRefCount::SafeRelease(pruleFilterLeft);
		CRefCount::SafeRelease(pruleFilterInner);
		CRefCount::SafeRelease(pruleFilterAny);
		return GPOS_FAILED;
	}
	pindex = GPOS_NEW(mp) CDSLRulePrefixIndex(mp);
	pindex->Insert(pruleFilterLeft, 0, COperator::EopLogicalSelect);
	pindex->Insert(pruleFilterInner, 1, COperator::EopLogicalSelect);
	pindex->Insert(pruleFilterAny, 2, COperator::EopLogicalSelect);
	pexpr = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp),
		PexprPrefixJoin(mp, PexprPrefixLeaf(mp), PexprPrefixLeaf(mp)),
		PexprPrefixLeaf(mp));
	pdrgprule = pindex->PdrgpruleCandidates(mp, pexpr);
	fValid = fValid && 2 == pdrgprule->Size() &&
			 pruleFilterInner == (*pdrgprule)[0] &&
			 pruleFilterAny == (*pdrgprule)[1];
	pdrgprule->Release();
	pexpr->Release();
	GPOS_DELETE(pindex);
	pruleFilterAny->Release();
	pruleFilterInner->Release();
	pruleFilterLeft->Release();

	// Ordinary root Proj has a stable Project prefix even though its matcher may
	// peel representation shells below it. The direct child path remains fully
	// discriminated for the common case.
	CDSLRule *pruleProjLeft = PrulePrefix(
		mp,
		"Proj<a2 s0>(LeftJoin<a0 a1>(Input<t0>,Input<t1>))|Input<t2>|"
		"TableEq(t2,t0)");
	CDSLRule *pruleProjInner = PrulePrefix(
		mp,
		"Proj<a2 s0>(InnerJoin<a0 a1>(Input<t0>,Input<t1>))|Input<t2>|"
		"TableEq(t2,t0)");
	CDSLRule *pruleProjAny = PrulePrefix(
		mp,
		"Proj<a0 s0>(Input<t0>)|Input<t1>|TableEq(t1,t0)");
	if (nullptr == pruleProjLeft || nullptr == pruleProjInner ||
		nullptr == pruleProjAny)
	{
		CRefCount::SafeRelease(pruleProjLeft);
		CRefCount::SafeRelease(pruleProjInner);
		CRefCount::SafeRelease(pruleProjAny);
		return GPOS_FAILED;
	}
	pindex = GPOS_NEW(mp) CDSLRulePrefixIndex(mp);
	pindex->Insert(pruleProjLeft, 0, COperator::EopLogicalProject);
	pindex->Insert(pruleProjInner, 1, COperator::EopLogicalProject);
	pindex->Insert(pruleProjAny, 2, COperator::EopLogicalProject);
	pexpr = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalProject(mp),
		PexprPrefixJoin(mp, PexprPrefixLeaf(mp), PexprPrefixLeaf(mp)),
		PexprPrefixLeaf(mp));
	pdrgprule = pindex->PdrgpruleCandidates(mp, pexpr);
	fValid = fValid && 2 == pdrgprule->Size() &&
			 pruleProjInner == (*pdrgprule)[0] &&
			 pruleProjAny == (*pdrgprule)[1];
	pdrgprule->Release();
	pexpr->Release();
	GPOS_DELETE(pindex);
	pruleProjAny->Release();
	pruleProjInner->Release();
	pruleProjLeft->Release();

	// Proj* and Agg source matchers accept only canonical Global GbAgg roots.
	// The trie token must reject Local split alternatives before full matching.
	CDSLRule *pruleDistinct = PrulePrefix(
		mp,
		"Proj*<a2 s0>(InnerJoin<a0 a1>(Input<t0>,Input<t1>))|Input<t2>|"
		"TableEq(t2,t0)");
	if (nullptr == pruleDistinct)
	{
		return GPOS_FAILED;
	}
	pindex = GPOS_NEW(mp) CDSLRulePrefixIndex(mp);
	pindex->Insert(pruleDistinct, 0, COperator::EopLogicalGbAgg);
	pexpr = PexprPrefixGbAgg(
		mp, COperator::EgbaggtypeGlobal,
		PexprPrefixJoin(mp, PexprPrefixLeaf(mp), PexprPrefixLeaf(mp)));
	pdrgprule = pindex->PdrgpruleCandidates(mp, pexpr);
	fValid = fValid && 1 == pdrgprule->Size() &&
			 pruleDistinct == (*pdrgprule)[0];
	pdrgprule->Release();
	pexpr->Release();

	pexpr = PexprPrefixGbAgg(
		mp, COperator::EgbaggtypeLocal,
		PexprPrefixJoin(mp, PexprPrefixLeaf(mp), PexprPrefixLeaf(mp)));
	pdrgprule = pindex->PdrgpruleCandidates(mp, pexpr);
	fValid = fValid && 0 == pdrgprule->Size();
	pdrgprule->Release();
	pexpr->Release();
	GPOS_DELETE(pindex);
	pruleDistinct->Release();

	return fValid ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLEngineTest::EresUnittest_DSLProvenance()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	CGroupExpression *pgexprBase = GPOS_NEW(mp) CGroupExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), GPOS_NEW(mp) CGroupArray(mp),
		CXform::ExfInvalid, nullptr, false /*fIntermediate*/);
	CGroupExpression *pgexprNative = GPOS_NEW(mp) CGroupExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), GPOS_NEW(mp) CGroupArray(mp),
		CXform::ExfSelect2Filter, pgexprBase, false /*fIntermediate*/);
	CGroupExpression *pgexprDSL = GPOS_NEW(mp) CGroupExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), GPOS_NEW(mp) CGroupArray(mp),
		CXform::ExfDSLRuleSelect, pgexprNative, false /*fIntermediate*/);
	CGroupExpression *pgexprNativeAfterDSL = GPOS_NEW(mp) CGroupExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), GPOS_NEW(mp) CGroupArray(mp),
		CXform::ExfSelect2Filter, pgexprDSL, false /*fIntermediate*/);

	const BOOL fValid =
		!CGroupExpression::FDSLRuleXform(CXform::ExfSelect2Filter) &&
		CGroupExpression::FDSLRuleXform(CXform::ExfDSLRuleSelect) &&
		CGroupExpression::FDSLRuleXform(CXform::ExfDSLRuleLimit) &&
		!pgexprBase->FHasDSLProvenance() &&
		!pgexprNative->FHasDSLProvenance() &&
		pgexprDSL->FHasDSLProvenance() &&
		pgexprNativeAfterDSL->FHasDSLProvenance();

	pgexprNativeAfterDSL->Release();
	pgexprDSL->Release();
	pgexprNative->Release();
	pgexprBase->Release();
	return fValid ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLEngineTest::EresUnittest_ReachableTransientEmptyGroup()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	CGroup *pgroupParent = GPOS_NEW(mp) CGroup(mp);
	CGroup *pgroupEmpty = GPOS_NEW(mp) CGroup(mp);
	CGroup *pgroupOther = GPOS_NEW(mp) CGroup(mp);
	{
		CGroupProxy gp(pgroupParent);
		gp.SetId(0);
	}
	{
		CGroupProxy gp(pgroupEmpty);
		gp.SetId(1);
	}
	{
		CGroupProxy gp(pgroupOther);
		gp.SetId(2);
	}

	CGroupArray *pdrgpgroup = GPOS_NEW(mp) CGroupArray(mp);
	pdrgpgroup->Append(pgroupEmpty);
	CGroupExpression *pgexpr = GPOS_NEW(mp) CGroupExpression(
		mp, GPOS_NEW(mp) CLogicalSelect(mp), pdrgpgroup,
		CXform::ExfInvalid, nullptr, false /*fIntermediate*/);
	{
		CGroupProxy gp(pgroupParent);
		gp.Insert(pgexpr);
	}

	const BOOL fValid = CGroup::FReachable(mp, pgroupParent, pgroupEmpty) &&
						!CGroup::FReachable(mp, pgroupParent, pgroupOther);

	pgroupOther->Release();
	pgroupEmpty->Release();
	pgroupParent->Release();
	return fValid ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLEngineTest::EresUnittest_CapabilityMetadata()
{
	for (ULONG ul = 0; ul < EdslopSentinel; ul++)
	{
		const EDslOpKind edslop = static_cast<EDslOpKind>(ul);
		const BOOL fExpected = true;
		if (fExpected != CDSLOpKindTable::FMatcherSupported(edslop) ||
			fExpected != CDSLOpKindTable::FInstantiatorSupported(edslop))
		{
			return GPOS_FAILED;
		}
	}

	if (CDSLOpKindTable::FSourceRootDispatchSupported(EdslopInput, false) ||
		!CDSLOpKindTable::FSourceRootDispatchSupported(EdslopFilter, false) ||
		!CDSLOpKindTable::FSourceRootDispatchSupported(EdslopProj, true) ||
		!CDSLOpKindTable::FSourceRootDispatchSupported(EdslopSort, false) ||
		!CDSLOpKindTable::FSourceRootDispatchSupported(EdslopLimit, false))
	{
		return GPOS_FAILED;
	}

	for (ULONG ul = 0; ul < EdslconSentinel; ul++)
	{
		if (!CDSLConstraintKindTable::FCheckerSupported(
				static_cast<EDslConstraintKind>(ul)))
		{
			return GPOS_FAILED;
		}
	}
	return GPOS_OK;
}

GPOS_RESULT
CDSLEngineTest::EresUnittest_LimitShellDispatch()
{
	CXformFactory *pxff = CXformFactory::Pxff();
	CXform *pxform =
		(nullptr == pxff) ? nullptr : pxff->Pxf("CXformDSLRule_Limit");
	if (nullptr == pxform || CXform::ExfDSLRuleLimit != pxform->Exfid() ||
		!pxform->FExploration() || pxform->FImplementation())
	{
		return GPOS_FAILED;
	}

	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CLogicalLimit *popLimit = GPOS_NEW(mp) CLogicalLimit(mp);
	CXformSet *pxfs = popLimit->PxfsCandidates(mp);
	GPOS_RESULT eres = pxfs->Get(CXform::ExfDSLRuleLimit) ? GPOS_OK
													 : GPOS_FAILED;
	pxfs->Release();
	popLimit->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLEngineTest::EresUnittest_UnionShellsDispatch
//
//	@doc:
//		Both set-op shells are registered as exploration xforms and both logical
//		operators advertise the corresponding id to the scheduler.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLEngineTest::EresUnittest_UnionShellsDispatch()
{
	CXformFactory *pxff = CXformFactory::Pxff();
	if (nullptr == pxff)
	{
		return GPOS_FAILED;
	}

	CXform *pxformUnion = pxff->Pxf("CXformDSLRule_Union");
	CXform *pxformUnionAll = pxff->Pxf("CXformDSLRule_UnionAll");
	if (nullptr == pxformUnion || nullptr == pxformUnionAll ||
		CXform::ExfDSLRuleUnion != pxformUnion->Exfid() ||
		CXform::ExfDSLRuleUnionAll != pxformUnionAll->Exfid() ||
		!pxformUnion->FExploration() || pxformUnion->FImplementation() ||
		!pxformUnionAll->FExploration() || pxformUnionAll->FImplementation() ||
		pxformUnion != pxff->Pxf(CXform::ExfDSLRuleUnion) ||
		pxformUnionAll != pxff->Pxf(CXform::ExfDSLRuleUnionAll))
	{
		return GPOS_FAILED;
	}

	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CLogicalUnion *popUnion = GPOS_NEW(mp) CLogicalUnion(mp);
	CLogicalUnionAll *popUnionAll = GPOS_NEW(mp) CLogicalUnionAll(mp);
	CXformSet *pxfsUnion = popUnion->PxfsCandidates(mp);
	CXformSet *pxfsUnionAll = popUnionAll->PxfsCandidates(mp);

	GPOS_RESULT eres =
		pxfsUnion->Get(CXform::ExfDSLRuleUnion) &&
		pxfsUnionAll->Get(CXform::ExfDSLRuleUnionAll)
			? GPOS_OK
			: GPOS_FAILED;

	pxfsUnion->Release();
	pxfsUnionAll->Release();
	popUnion->Release();
	popUnionAll->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLEngineTest::EresUnittest_SubqueryRepresentationCapability
//
//	@doc:
//		Select-stage routing is classified by DSL operator semantics. Both a
//		single and a nested InSub source have the same capability; Exists shares
//		it, while ordinary Select/Agg operators do not. This prevents dispatch
//		from growing rule-shape checks as more corpus rules are admitted.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLEngineTest::EresUnittest_SubqueryRepresentationCapability()
{
	if (!CDSLOpKindTable::FHasPreUnnestRepresentation(EdslopInSubFilter) ||
		!CDSLOpKindTable::FHasPreUnnestRepresentation(EdslopExists) ||
		CDSLOpKindTable::FHasPreUnnestRepresentation(EdslopFilter) ||
		CDSLOpKindTable::FHasPreUnnestRepresentation(EdslopAgg))
	{
		return GPOS_FAILED;
	}

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLEngineTest::EresUnittest_ShellRegistered
//
//	@doc:
//		The Select shell is in the xform factory, has the expected id/name, and
//		is classified as an exploration xform (design §八.5).
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLEngineTest::EresUnittest_ShellRegistered()
{
	CXformFactory *pxff = CXformFactory::Pxff();
	if (nullptr == pxff)
	{
		return GPOS_FAILED;
	}

	CXform *pxform = pxff->Pxf("CXformDSLRule_Select");
	if (nullptr == pxform ||
		CXform::ExfDSLRuleSelect != pxform->Exfid() ||
		!pxform->FExploration() || pxform->FImplementation())
	{
		return GPOS_FAILED;
	}

	// same instance is reachable by id
	if (pxform != pxff->Pxf(CXform::ExfDSLRuleSelect))
	{
		return GPOS_FAILED;
	}

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLEngineTest::EresUnittest_SelectDispatches
//
//	@doc:
//		The decisive dispatch proof (design §二): the scheduler routes an
//		expression to an xform iff the xform's id is in the operator's
//		PxfsCandidates() set. We build a CLogicalSelect, query its candidate
//		set, and assert ExfDSLRuleSelect is a member — so ORCA WILL hand every
//		Select group expression to our shell's Transform().
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLEngineTest::EresUnittest_SelectDispatches()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	CLogicalSelect *popSelect = GPOS_NEW(mp) CLogicalSelect(mp);
	CXformSet *pxfs = popSelect->PxfsCandidates(mp);

	GPOS_RESULT eres =
		pxfs->Get(CXform::ExfDSLRuleSelect) ? GPOS_OK : GPOS_FAILED;

	pxfs->Release();
	popSelect->Release();
	return eres;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLEngineTest::EresUnittest_Bucketing
//
//	@doc:
//		The dispatch key a shell buckets on — CDSLRule::EopidSrcRoot() — is the
//		ORCA logical op of the source root. A Filter-rooted rule must bucket
//		under EopLogicalSelect (so the Select shell owns it); an InnerJoin-rooted
//		rule under EopLogicalInnerJoin.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLEngineTest::EresUnittest_Bucketing()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// a real proven predicate-preserving rule rooted at Filter -> Select bucket
	const CHAR *szFilterRule =
		"Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
		"TableEq(t1,t0);AttrsEq(a1,a0);PredicateEq(p1,p0)";
	CDSLRule *pruleFilter =
		CDSLRuleParser::PdslruleParse(mp, szFilterRule, "EQ", nullptr);
	if (nullptr == pruleFilter ||
		COperator::EopLogicalSelect != pruleFilter->EopidSrcRoot())
	{
		CRefCount::SafeRelease(pruleFilter);
		return GPOS_FAILED;
	}
	pruleFilter->Release();

	// an InnerJoin-rooted rule -> InnerJoin bucket (NOT the Select shell)
	const CHAR *szJoinRule =
		"InnerJoin<a0 a1>(Input<t0>,Input<t1>)|"
		"InnerJoin<a3 a2>(Input<t3>,Input<t2>)|"
		"TableEq(t2,t0);TableEq(t3,t1);AttrsEq(a2,a0);AttrsEq(a3,a1)";
	CDSLRule *pruleJoin =
		CDSLRuleParser::PdslruleParse(mp, szJoinRule, "EQ", nullptr);
	if (nullptr == pruleJoin ||
		COperator::EopLogicalInnerJoin != pruleJoin->EopidSrcRoot() ||
		COperator::EopLogicalSelect == pruleJoin->EopidSrcRoot())
	{
		CRefCount::SafeRelease(pruleJoin);
		return GPOS_FAILED;
	}
	pruleJoin->Release();

	// In the WeTune vocabulary bare Union means bag union (ORCA UnionAll),
	// while Union* means duplicate-eliminating union (ORCA Union).
	const CHAR *szUnionAllRule =
		"Union(Input<t0>,Input<t1>)|Union(Input<t2>,Input<t3>)|"
		"TableEq(t2,t0);TableEq(t3,t1)";
	CDSLRule *pruleUnionAll =
		CDSLRuleParser::PdslruleParse(mp, szUnionAllRule, "EQ", nullptr);
	if (nullptr == pruleUnionAll ||
		COperator::EopLogicalUnionAll != pruleUnionAll->EopidSrcRoot())
	{
		CRefCount::SafeRelease(pruleUnionAll);
		return GPOS_FAILED;
	}
	pruleUnionAll->Release();

	const CHAR *szUnionRule =
		"Union*(Input<t0>,Input<t1>)|Union*(Input<t2>,Input<t3>)|"
		"TableEq(t2,t0);TableEq(t3,t1)";
	CDSLRule *pruleUnion =
		CDSLRuleParser::PdslruleParse(mp, szUnionRule, "EQ", nullptr);
	if (nullptr == pruleUnion ||
		COperator::EopLogicalUnion != pruleUnion->EopidSrcRoot())
	{
		CRefCount::SafeRelease(pruleUnion);
		return GPOS_FAILED;
	}
	pruleUnion->Release();

	const CHAR *szLimitRule =
		"Limit<n0 n1>(SortAsc<a0>(Input<t0>))|"
		"Limit<n2 n3>(SortAsc<a1>(Input<t1>))|"
		"TableEq(t1,t0);AttrsEq(a1,a0);ScalarEq(n2,n0);"
		"ScalarEq(n3,n1)";
	CDSLRule *pruleLimit =
		CDSLRuleParser::PdslruleParse(mp, szLimitRule, "EQ", nullptr);
	if (nullptr == pruleLimit ||
		COperator::EopLogicalLimit != pruleLimit->EopidSrcRoot())
	{
		CRefCount::SafeRelease(pruleLimit);
		return GPOS_FAILED;
	}
	pruleLimit->Release();

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLEngineTest::EresUnittest_StubsCallable
//
//	@doc:
//		The global engine exists after gpopt_init and its dispatch + three-stage
//		entry points are safe to call. RulesForRoot never returns NULL. FMatch
//		(#24) and FCheckConstraints (#26) are real and covered by their own suites
//		(they assert non-null model / feed live expressions); here we confirm that
//		with an EMPTY model a structural-constraint rule is safely REJECTED by
//		Check (a subset constraint over an unbound symbol cannot hold) and that
//		the still-stubbed Instantiate returns NULL — so a shell's Transform loop
//		is a safe no-op with no rules loaded.
//---------------------------------------------------------------------------
GPOS_RESULT
CDSLEngineTest::EresUnittest_StubsCallable()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	CDSLRuleEngine *peng = CDSLRuleEngine::Instance();
	if (nullptr == peng)  // created in gpopt_init
	{
		return GPOS_FAILED;
	}

	// dispatch buckets are never NULL, even for a root with no rules
	if (nullptr == peng->PdrgpruleForRoot(COperator::EopLogicalSelect) ||
		nullptr == peng->PdrgpruleForRoot(COperator::EopLogicalGet))
	{
		return GPOS_FAILED;
	}

	// a rule carrying a STRUCTURAL constraint (AttrsSub) — against an empty model
	// Check must reject (unbound symbols), and Instantiate (still a stub) returns
	// NULL. Confirms the entry points are callable and fail safe.
	const CHAR *szRule =
		"Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
		"AttrsSub(a0,t0);TableEq(t1,t0);AttrsEq(a1,a0);PredicateEq(p1,p0)";
	CDSLRule *prule = CDSLRuleParser::PdslruleParse(mp, szRule, "EQ", nullptr);
	if (nullptr == prule)
	{
		return GPOS_FAILED;
	}

	CDSLModel *pmodel = GPOS_NEW(mp) CDSLModel(mp);
	GPOS_RESULT eres = GPOS_OK;

	if (0 != pmodel->Size() ||
		peng->FCheckConstraints(prule, pmodel, nullptr /*pexpr*/) ||
		nullptr != peng->PexprInstantiate(mp, prule, pmodel))
	{
		eres = GPOS_FAILED;
	}

	pmodel->Release();
	prule->Release();

	return eres;
}

// EOF
