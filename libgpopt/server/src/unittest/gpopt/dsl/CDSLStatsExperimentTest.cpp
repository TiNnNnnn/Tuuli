//---------------------------------------------------------------------------
// Cardinality experiment tests.
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLStatsExperimentTest.h"

#include <sstream>

#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/dsl/CDSLStatsExperiment.h"
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

using namespace gpopt;

GPOS_RESULT
CDSLStatsExperimentTest::EresUnittest()
{
	CUnittest tests[] = {
		GPOS_UNITTEST_FUNC(
			CDSLStatsExperimentTest::EresUnittest_ResolveSPJBoundaries),
		GPOS_UNITTEST_FUNC(
			CDSLStatsExperimentTest::EresUnittest_ExpressionFingerprintRoundTrip),
		GPOS_UNITTEST_FUNC(CDSLStatsExperimentTest::EresUnittest_StrictInput),
	};
	return CUnittest::EresExecute(tests, GPOS_ARRAY_SIZE(tests));
}

GPOS_RESULT
CDSLStatsExperimentTest::EresUnittest_ResolveSPJBoundaries()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fixture(mp);
	CColRefArray *a_cols = nullptr;
	CColRefArray *b_cols = nullptr;
	CExpression *a = fixture.PexprLogicalGet("a", 2, &a_cols);
	CExpression *predicate = fixture.PexprPredAtom((*a_cols)[0]);
	CExpression *inner_select = fixture.PexprLogicalSelect(a, predicate);
	predicate->Release();
	predicate = fixture.PexprPredAtom((*a_cols)[1]);
	CExpression *outer_select =
		fixture.PexprLogicalSelect(inner_select, predicate);
	predicate->Release();
	CExpression *b = fixture.PexprLogicalGet("b", 1, &b_cols);
	CExpression *join_predicate =
		fixture.PexprEqPred((*a_cols)[0], (*b_cols)[0]);
	CExpression *join = fixture.PexprLogicalInnerJoin(
		outer_select, b, join_predicate);
	join_predicate->Release();

	CWStringDynamic errors(mp);
	const CHAR *config =
		"experiment: boundary-test\n"
		"cardinalities:\n"
		"  - relations: [a]\n"
		"    rows: 7\n"
		"  - relations: [b, a]\n"
		"    rows: 11.5\n";
	CDSLStatsExperimentSnapshot *snapshot =
		CDSLStatsExperimentSnapshot::PsnapshotLoadBuffer(
			mp, config, join, &errors);
	BOOL valid = nullptr != snapshot;
	if (valid)
	{
		const auto *base = snapshot->Ptarget(outer_select);
		const auto *joined = snapshot->Ptarget(join);
		valid = nullptr != base && 7.0 == base->m_rows && nullptr != joined &&
			11.5 == joined->m_rows && nullptr == snapshot->Ptarget(inner_select) &&
			nullptr == snapshot->Ptarget(a);
	}

	GPOS_DELETE(snapshot);
	errors.Reset();
	snapshot = CDSLStatsExperimentSnapshot::PsnapshotLoadBuffer(
		mp,
		"experiment: discover-test\n"
		"discover: true\n"
		"cardinalities:\n",
		join, &errors);
	valid = valid && nullptr != snapshot && 5 == snapshot->UlTargets() &&
		nullptr != snapshot->Ptarget(outer_select) &&
		nullptr != snapshot->Ptarget(b) && nullptr != snapshot->Ptarget(join) &&
		nullptr != snapshot->Ptarget(inner_select) &&
		nullptr != snapshot->Ptarget(a);
	GPOS_DELETE(snapshot);
	join->Release();
	b->Release();
	outer_select->Release();
	inner_select->Release();
	a->Release();
	return valid ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLStatsExperimentTest::EresUnittest_ExpressionFingerprintRoundTrip()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fixture(mp);
	CColRefArray *cols = nullptr;
	CExpression *get = fixture.PexprLogicalGet("a", 1, &cols);
	CExpression *agg = fixture.PexprLogicalGbAgg(get, cols);
	CWStringDynamic errors(mp);
	CDSLStatsExperimentSnapshot *snapshot =
		CDSLStatsExperimentSnapshot::PsnapshotLoadBuffer(
			mp,
			"experiment: discover-expression\n"
			"discover: true\n"
			"cardinalities:\n",
			agg, &errors);
	const SDSLStatsExperimentTarget *discovered =
		nullptr == snapshot ? nullptr : snapshot->Ptarget(agg);
	BOOL valid = nullptr != discovered && discovered->m_relations.empty() &&
		"CLogicalGbAgg" == discovered->m_operator &&
		16 == discovered->m_fingerprint.size();
	std::string fingerprint =
		nullptr == discovered ? "" : discovered->m_fingerprint;
	GPOS_DELETE(snapshot);

	std::ostringstream config;
	config << "experiment: inject-expression\n"
			   << "cardinalities:\n"
			   << "  - expression: " << fingerprint << "\n"
			   << "    operator: CLogicalGbAgg\n"
			   << "    rows: 13\n";
	errors.Reset();
	snapshot = CDSLStatsExperimentSnapshot::PsnapshotLoadBuffer(
		mp, config.str().c_str(), agg, &errors);
	const SDSLStatsExperimentTarget *injected =
		nullptr == snapshot ? nullptr : snapshot->Ptarget(agg);
	valid = valid && nullptr != injected && injected->m_inject &&
		13.0 == injected->m_rows && nullptr == snapshot->Ptarget(get);

	GPOS_DELETE(snapshot);
	agg->Release();
	get->Release();
	return valid ? GPOS_OK : GPOS_FAILED;
}

GPOS_RESULT
CDSLStatsExperimentTest::EresUnittest_StrictInput()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();
	CDSLTestFixture fixture(mp);
	CExpression *get = fixture.PexprLogicalGet("a", 1);
	CWStringDynamic errors(mp);
	const CHAR *duplicate =
		"experiment: invalid\n"
		"cardinalities:\n"
		"  - relations: [a]\n"
		"    rows: 1\n"
		"  - relations: [a]\n"
		"    rows: 2\n";
	CDSLStatsExperimentSnapshot *snapshot =
		CDSLStatsExperimentSnapshot::PsnapshotLoadBuffer(
			mp, duplicate, get, &errors);
	BOOL valid = nullptr == snapshot && 0 < errors.Length();
	GPOS_DELETE(snapshot);

	CColRefArray *cols = nullptr;
	CExpression *repeated = fixture.PexprLogicalGet("repeated", 1, &cols);
	CExpression *predicate = fixture.PexprEqPred((*cols)[0], (*cols)[0]);
	CExpression *join =
		fixture.PexprLogicalInnerJoin(repeated, repeated, predicate);
	predicate->Release();
	std::ostringstream ambiguous;
	ambiguous << "experiment: ambiguous\n"
			  << "cardinalities:\n"
			  << "  - expression: "
			  << CDSLStatsExperimentSnapshot::Fingerprint(mp, repeated) << "\n"
			  << "    operator: CLogicalGet\n"
			  << "    rows: 3\n";
	errors.Reset();
	snapshot = CDSLStatsExperimentSnapshot::PsnapshotLoadBuffer(
		mp, ambiguous.str().c_str(), join, &errors);
	valid = valid && nullptr == snapshot && 0 < errors.Length();
	GPOS_DELETE(snapshot);
	join->Release();
	repeated->Release();
	get->Release();
	return valid ? GPOS_OK : GPOS_FAILED;
}
