//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLTestFixture.h
//
//	@doc:
//		Phase-2 unit-test fixture (base "A" in docs/WETUNE_TEST_MIGRATION.md §1).
//
//		pgorca does NOT vendor GPDB's CTestUtils, so there is no ready-made way
//		to stand up a live optimizer context + build logical CExpression trees in
//		a test. This fixture supplies exactly that, and nothing more:
//
//		  * builds a tiny in-memory metadata source PROGRAMMATICALLY (the built-in
//		    scalar types int4/int8/bool/oid via CMDType*GPDB) — no external DXL
//		    file to author/ship — wraps it in a CMDProviderMemory + CMDAccessor,
//		    and installs a COptCtxt in TLS (RAII, via CAutoOptCtxt).
//		  * hands out placeholder CColRefs (CColumnFactory::PcrCreate) and small
//		    logical subtrees (Get / Select / InnerJoin / Project) so a test only
//		    writes "this rule + this input shape + expected".
//
//		Predicate model: we build OPAQUE predicates from CScalarIdent columns and
//		CScalarBoolOp(And) — this needs no comparison-operator metadata and mirrors
//		WeTune's opaque-atom predicate model (p0/p1 are unanalyzed), which is
//		exactly what the DSL engine matches on.
//
//		Lifetime: construct one CDSLTestFixture per test on the test's own pool;
//		it owns the metadata objects + accessor + opt-ctxt for the test's scope.
//		Expressions handed back are owned by the CALLER (Release them).
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLTestFixture_H
#define GPOPT_CDSLTestFixture_H

#include "gpos/base.h"
#include "gpos/common/CAutoP.h"

#include "gpopt/base/CAutoOptCtxt.h"
#include "gpopt/base/CColRef.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CExpression.h"
#include "naucrates/md/CMDProviderMemory.h"
#include "naucrates/md/IMDType.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CDSLTestFixture
//
//	@doc:
//		RAII harness giving a test a live optimizer context + expression builders.
//---------------------------------------------------------------------------
class CDSLTestFixture
{
private:
	CMemoryPool *m_mp;

	// in-memory metadata: built-in types, held until teardown
	IMDCacheObjectArray *m_pdrgpmdobj;
	CMDProviderMemory *m_pmdp;
	CMDAccessor *m_pmda;

	// installs / removes the COptCtxt in TLS for this fixture's scope
	CAutoOptCtxt *m_paoc;

	// cached int4 type (the default column type)
	const IMDType *m_pmdtypeInt4;

	// monotonically increasing fake relation oid, so each table descriptor gets
	// a distinct mdid
	ULONG m_ulNextRelOid;

public:
	CDSLTestFixture(const CDSLTestFixture &) = delete;

	explicit CDSLTestFixture(CMemoryPool *mp);

	~CDSLTestFixture();

	CMemoryPool *
	Pmp() const
	{
		return m_mp;
	}
	CMDAccessor *
	Pmda() const
	{
		return m_pmda;
	}

	//------------------------------------------------------------------
	// columns
	//------------------------------------------------------------------

	// a fresh int4 placeholder column named szName (owned by the column factory)
	CColRef *PcrCreateInt4(const CHAR *szName);

	//------------------------------------------------------------------
	// relations / logical leaves
	//------------------------------------------------------------------

	// build a heap table descriptor named szTable with ulCols int4 columns
	// c0..c(ulCols-1). If ulKeyCol < ulCols, that column is registered as a
	// unique key (for Unique-constraint tests). If fNullable, the columns are
	// created nullable (for NotNull-constraint rejection tests); the default is
	// non-nullable.
	CTableDescriptor *PtabdescCreate(const CHAR *szTable, ULONG ulCols,
									 ULONG ulKeyCol = gpos::ulong_max,
									 BOOL fNullable = false);

	// CLogicalGet over ptabdesc; fills *ppdrgpcrOut with the output colrefs when
	// non-NULL (caller does NOT own that array's ref — it lives on the Get).
	CExpression *PexprLogicalGet(CTableDescriptor *ptabdesc,
								 const CHAR *szAlias,
								 CColRefArray **ppdrgpcrOut = nullptr);

	// convenience: a Get over a fresh ulCols-column table in one call
	CExpression *PexprLogicalGet(const CHAR *szTable, ULONG ulCols,
								 CColRefArray **ppdrgpcrOut = nullptr,
								 ULONG ulKeyCol = gpos::ulong_max);

	//------------------------------------------------------------------
	// scalar predicates (opaque, WeTune-style)
	//------------------------------------------------------------------

	// an opaque single-column predicate leaf: BoolTest-like wrapper we treat as
	// an atomic conjunct. Implemented as "pcr" identity wrapped so it derives
	// used-columns = {pcr}. Used to stand in for p0/p1 atoms.
	CExpression *PexprPredAtom(CColRef *pcr);

	// AND together the given atoms into one CScalarBoolOp(And) (>=2 atoms) or
	// return the single atom unchanged. Consumes one ref of each (AddRef inside).
	CExpression *PexprConjunctionOfAtoms(CColRef **rgpcr, ULONG ulAtoms);

	// an equi-join key predicate: pcrLeft = pcrRight (CScalarCmp over two
	// CScalarIdents, int4 '='). Unlike PexprPredAtom this is a plain-equality the
	// join matcher can split into left/right keys. Caller owns the result.
	CExpression *PexprEqPred(CColRef *pcrLeft, CColRef *pcrRight);

	//------------------------------------------------------------------
	// logical nodes
	//------------------------------------------------------------------

	// Select(child, predicate). AddRefs both; caller owns the result.
	CExpression *PexprLogicalSelect(CExpression *pexprChild,
									CExpression *pexprPred);

	// InnerJoin(left, right, predicate). AddRefs all; caller owns result.
	CExpression *PexprLogicalInnerJoin(CExpression *pexprLeft,
									   CExpression *pexprRight,
									   CExpression *pexprPred);

	// LeftOuterJoin(left, right, predicate). AddRefs all; caller owns result.
	// Used by the join-elimination LeftJoin rule (rules.txt line 205).
	CExpression *PexprLogicalLeftOuterJoin(CExpression *pexprLeft,
										   CExpression *pexprRight,
										   CExpression *pexprPred);

	// Project(child, project-list) projecting the given columns as pass-through
	// CScalarIdent elements (one CScalarProjectElement per column, redefining the
	// same CColRef). AddRefs the child; caller owns the result. Models a plain
	// column projection (SELECT c0, c1, ... FROM ...).
	CExpression *PexprLogicalProject(CExpression *pexprChild,
									 CColRefArray *pdrgpcrProj);

	// GbAgg(child, empty-project-list) grouping by the given columns with NO
	// aggregate functions — ORCA's representation of SELECT DISTINCT cols. AddRefs
	// the child and the grouping array; caller owns the result. Used by the dedup
	// (DISTINCT) elimination rule (the DSL analogue of CXformSimplifyGbAgg).
	//
	// If pcrAgg is non-NULL, a genuine MAX(pcrAggInput) aggregate project element
	// defining pcrAgg is added (falling back to grouping[0]).
	CExpression *PexprLogicalGbAgg(CExpression *pexprChild,
								   CColRefArray *pdrgpcrGrouping,
								   CColRef *pcrAgg = nullptr,
								   CColRef *pcrAggInput = nullptr);
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLTestFixture_H
