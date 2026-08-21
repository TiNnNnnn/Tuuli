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
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CLogicalUnion.h"
#include "gpopt/operators/CLogicalUnionAll.h"
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
		GPOS_UNITTEST_FUNC(
			CDSLEngineTest::EresUnittest_SubqueryRepresentationCapability),
		GPOS_UNITTEST_FUNC(CDSLEngineTest::EresUnittest_CapabilityMetadata),
		GPOS_UNITTEST_FUNC(CDSLEngineTest::EresUnittest_StubsCallable),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
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
