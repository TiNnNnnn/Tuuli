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
		GPOS_UNITTEST_FUNC(CDSLEngineTest::EresUnittest_Bucketing),
		GPOS_UNITTEST_FUNC(CDSLEngineTest::EresUnittest_StubsCallable),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
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

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLEngineTest::EresUnittest_StubsCallable
//
//	@doc:
//		The global engine exists after gpopt_init and its dispatch + remaining
//		three-stage entry points are safe to call. RulesForRoot never returns
//		NULL; the still-stubbed Check/Instantiate return false/NULL, so a shell's
//		Transform loop is a safe no-op with no rules loaded. (FMatch is now real —
//		task #24 — and is covered by CDSLMatchTest, which feeds it live
//		expressions; it asserts non-null args, so it is not exercised here.)
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

	// remaining stubs: parse one rule, confirm Check/Instantiate are callable and
	// behave as documented (no rewrite until #26/#27 land).
	const CHAR *szRule =
		"Filter<p0 a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|"
		"TableEq(t1,t0);AttrsEq(a1,a0);PredicateEq(p1,p0)";
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
