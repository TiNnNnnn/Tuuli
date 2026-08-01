//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLTestFixture.cpp
//
//	@doc:
//		Implementation of the phase-2 unit-test fixture (see CDSLTestFixture.h).
//---------------------------------------------------------------------------
#include "unittest/gpopt/dsl/CDSLTestFixture.h"

#include "gpos/common/CAutoRef.h"

#include "gpopt/base/CColumnFactory.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/mdcache/CMDCache.h"
#include "gpopt/metadata/CColumnDescriptor.h"
#include "gpopt/metadata/CName.h"
#include "gpopt/optimizer/COptimizerConfig.h"
#include "gpopt/operators/CLogicalGet.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CScalarBoolOp.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarNullTest.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"  // UNASSIGNED_QUERYID
#include "naucrates/md/CMDIdGPDB.h"
#include "naucrates/md/CMDProviderGeneric.h"
#include "naucrates/md/CMDProviderMemory.h"
#include "naucrates/md/CMDTypeBoolGPDB.h"
#include "naucrates/md/CMDTypeInt4GPDB.h"
#include "naucrates/md/CMDTypeInt8GPDB.h"
#include "naucrates/md/CMDTypeOidGPDB.h"
#include "naucrates/md/CSystemId.h"
#include "naucrates/md/IMDTypeBool.h"

using namespace gpopt;
using namespace gpmd;

// arbitrary starting oid for synthetic relations (well clear of built-in oids)
#define GPOPT_DSL_TEST_REL_OID_BASE 100000

//---------------------------------------------------------------------------
//	@function:
//		CDSLTestFixture::CDSLTestFixture
//
//	@doc:
//		Build built-in scalar type metadata programmatically, wrap in an
//		in-memory provider + accessor, and install a COptCtxt in TLS.
//---------------------------------------------------------------------------
CDSLTestFixture::CDSLTestFixture(CMemoryPool *mp)
	: m_mp(mp),
	  m_pdrgpmdobj(nullptr),
	  m_pmdp(nullptr),
	  m_pmda(nullptr),
	  m_paoc(nullptr),
	  m_pmdtypeInt4(nullptr),
	  m_ulNextRelOid(GPOPT_DSL_TEST_REL_OID_BASE)
{
	// the built-in scalar types the fixture (and CScalarNullTest, which needs
	// bool) can reference. mdids are EmdidGeneral/<oid>, matching what
	// CMDProviderGeneric hands the accessor for these type-infos.
	m_pdrgpmdobj = GPOS_NEW(mp) IMDCacheObjectArray(mp);
	m_pdrgpmdobj->Append(GPOS_NEW(mp) CMDTypeInt4GPDB(mp));
	m_pdrgpmdobj->Append(GPOS_NEW(mp) CMDTypeInt8GPDB(mp));
	m_pdrgpmdobj->Append(GPOS_NEW(mp) CMDTypeBoolGPDB(mp));
	m_pdrgpmdobj->Append(GPOS_NEW(mp) CMDTypeOidGPDB(mp));

	m_pmdp = GPOS_NEW(mp) CMDProviderMemory(mp, m_pdrgpmdobj);
	m_pmdp->AddRef();

	// register under the same system id the generic provider uses for built-in
	// types, so type-info lookups resolve against our in-memory objects.
	CSystemId sysid = CMDProviderGeneric::SysidDefault();
	m_pmda = GPOS_NEW(mp) CMDAccessor(mp, CMDCache::Pcache(), sysid, m_pmdp);

	// install COptCtxt (RAII). Pass an explicit default config to disambiguate
	// the two CAutoOptCtxt overloads (COptimizerConfig* vs ICostModel*).
	m_paoc = GPOS_NEW(mp) CAutoOptCtxt(mp, m_pmda, nullptr /*pceeval*/,
									   COptimizerConfig::PoconfDefault(mp));

	m_pmdtypeInt4 = m_pmda->PtMDType<IMDTypeInt4>();
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLTestFixture::~CDSLTestFixture
//---------------------------------------------------------------------------
CDSLTestFixture::~CDSLTestFixture()
{
	// tear down in reverse order: COptCtxt (removes TLS) first, then accessor,
	// then provider + md objects.
	GPOS_DELETE(m_paoc);
	GPOS_DELETE(m_pmda);
	m_pmdp->Release();
	m_pdrgpmdobj->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLTestFixture::PcrCreateInt4
//---------------------------------------------------------------------------
CColRef *
CDSLTestFixture::PcrCreateInt4(const CHAR *szName)
{
	CColumnFactory *pcf = COptCtxt::PoctxtFromTLS()->Pcf();
	CWStringDynamic strName(m_mp);
	strName.AppendFormat(GPOS_WSZ_LIT("%s"), szName);
	CName name(m_mp, &strName);
	return pcf->PcrCreate(m_pmdtypeInt4, default_type_modifier, name);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLTestFixture::PtabdescCreate
//
//	@doc:
//		Heap table with ulCols int4 columns c0..c(ulCols-1); optional unique key.
//---------------------------------------------------------------------------
CTableDescriptor *
CDSLTestFixture::PtabdescCreate(const CHAR *szTable, ULONG ulCols,
								ULONG ulKeyCol, BOOL fNullable)
{
	CWStringDynamic strTable(m_mp);
	strTable.AppendFormat(GPOS_WSZ_LIT("%s"), szTable);
	CName nameTable(m_mp, &strTable);

	IMDId *pmdidRel =
		GPOS_NEW(m_mp) CMDIdGPDB(IMDId::EmdidRel, m_ulNextRelOid++, 1, 0);

	CTableDescriptor *ptabdesc = GPOS_NEW(m_mp) CTableDescriptor(
		m_mp, pmdidRel, nameTable, false /*convert_hash_to_random*/,
		IMDRelation::EreldistrRandom, IMDRelation::ErelstorageHeap,
		0 /*ulExecuteAsUser*/, -1 /*lockmode*/, 0 /*acl_mode*/,
		UNASSIGNED_QUERYID);

	for (ULONG ul = 0; ul < ulCols; ul++)
	{
		CWStringDynamic strCol(m_mp);
		strCol.AppendFormat(GPOS_WSZ_LIT("c%d"), ul);
		CName nameCol(m_mp, &strCol);
		CColumnDescriptor *pcoldesc = GPOS_NEW(m_mp) CColumnDescriptor(
			m_mp, m_pmdtypeInt4, default_type_modifier, nameCol,
			(INT) (ul + 1) /*attno, 1-based*/, fNullable /*is_nullable*/,
			gpos::ulong_max /*width*/);
		ptabdesc->AddColumn(pcoldesc);
	}

	if (ulKeyCol < ulCols)
	{
		CBitSet *pbsKey = GPOS_NEW(m_mp) CBitSet(m_mp);
		(void) pbsKey->ExchangeSet(ulKeyCol);
		(void) ptabdesc->FAddKeySet(pbsKey);
	}

	return ptabdesc;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLTestFixture::PexprLogicalGet
//---------------------------------------------------------------------------
CExpression *
CDSLTestFixture::PexprLogicalGet(CTableDescriptor *ptabdesc,
								 const CHAR *szAlias,
								 CColRefArray **ppdrgpcrOut)
{
	CWStringDynamic strAlias(m_mp);
	strAlias.AppendFormat(GPOS_WSZ_LIT("%s"), szAlias);
	CName *pnameAlias = GPOS_NEW(m_mp) CName(m_mp, &strAlias);

	CLogicalGet *popGet = GPOS_NEW(m_mp)
		CLogicalGet(m_mp, pnameAlias, ptabdesc, false /*hasSecurityQuals*/);
	CExpression *pexprGet = GPOS_NEW(m_mp) CExpression(m_mp, popGet);

	if (nullptr != ppdrgpcrOut)
	{
		*ppdrgpcrOut = popGet->PdrgpcrOutput();
	}
	return pexprGet;
}

CExpression *
CDSLTestFixture::PexprLogicalGet(const CHAR *szTable, ULONG ulCols,
								 CColRefArray **ppdrgpcrOut, ULONG ulKeyCol)
{
	CTableDescriptor *ptabdesc = PtabdescCreate(szTable, ulCols, ulKeyCol);
	return PexprLogicalGet(ptabdesc, szTable, ppdrgpcrOut);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLTestFixture::PexprPredAtom
//
//	@doc:
//		Opaque atomic conjunct: IsNull(pcr). Boolean-typed (valid AND child),
//		derives used-columns = {pcr}, needs no comparison-operator metadata, and
//		PdrgpexprConjuncts treats it as one indivisible conjunct — matching
//		WeTune's opaque p0/p1 atoms.
//---------------------------------------------------------------------------
CExpression *
CDSLTestFixture::PexprPredAtom(CColRef *pcr)
{
	CExpression *pexprIdent = GPOS_NEW(m_mp)
		CExpression(m_mp, GPOS_NEW(m_mp) CScalarIdent(m_mp, pcr));
	return GPOS_NEW(m_mp)
		CExpression(m_mp, GPOS_NEW(m_mp) CScalarNullTest(m_mp), pexprIdent);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLTestFixture::PexprConjunctionOfAtoms
//---------------------------------------------------------------------------
CExpression *
CDSLTestFixture::PexprConjunctionOfAtoms(CColRef **rgpcr, ULONG ulAtoms)
{
	GPOS_ASSERT(0 < ulAtoms);

	if (1 == ulAtoms)
	{
		return PexprPredAtom(rgpcr[0]);
	}

	CExpressionArray *pdrgpexpr = GPOS_NEW(m_mp) CExpressionArray(m_mp);
	for (ULONG ul = 0; ul < ulAtoms; ul++)
	{
		pdrgpexpr->Append(PexprPredAtom(rgpcr[ul]));
	}
	return GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CScalarBoolOp(m_mp, CScalarBoolOp::EboolopAnd),
		pdrgpexpr);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLTestFixture::PexprLogicalSelect
//---------------------------------------------------------------------------
CExpression *
CDSLTestFixture::PexprLogicalSelect(CExpression *pexprChild,
									CExpression *pexprPred)
{
	pexprChild->AddRef();
	pexprPred->AddRef();
	return GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprChild, pexprPred);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLTestFixture::PexprLogicalInnerJoin
//---------------------------------------------------------------------------
CExpression *
CDSLTestFixture::PexprLogicalInnerJoin(CExpression *pexprLeft,
									   CExpression *pexprRight,
									   CExpression *pexprPred)
{
	pexprLeft->AddRef();
	pexprRight->AddRef();
	pexprPred->AddRef();
	return GPOS_NEW(m_mp)
		CExpression(m_mp, GPOS_NEW(m_mp) CLogicalInnerJoin(m_mp), pexprLeft,
					pexprRight, pexprPred);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLTestFixture::PexprLogicalProject
//
//	@doc:
//		Project(child, CScalarProjectList(prEl0, prEl1, ...)) where each element
//		re-defines one of pdrgpcrProj as a pass-through CScalarIdent of itself.
//		This mirrors a plain column projection; the defined columns ARE the input
//		columns (so DeriveOutputColumns is stable under identity rewrites).
//---------------------------------------------------------------------------
CExpression *
CDSLTestFixture::PexprLogicalProject(CExpression *pexprChild,
									 CColRefArray *pdrgpcrProj)
{
	CExpressionArray *pdrgpexprPrEl = GPOS_NEW(m_mp) CExpressionArray(m_mp);
	const ULONG ulProj = pdrgpcrProj->Size();
	for (ULONG ul = 0; ul < ulProj; ul++)
	{
		CColRef *pcr = (*pdrgpcrProj)[ul];
		CExpression *pexprIdent = GPOS_NEW(m_mp)
			CExpression(m_mp, GPOS_NEW(m_mp) CScalarIdent(m_mp, pcr));
		CExpression *pexprPrEl = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CScalarProjectElement(m_mp, pcr), pexprIdent);
		pdrgpexprPrEl->Append(pexprPrEl);
	}
	CExpression *pexprPrjList = GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CScalarProjectList(m_mp), pdrgpexprPrEl);

	pexprChild->AddRef();
	return GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CLogicalProject(m_mp), pexprChild, pexprPrjList);
}

// EOF
