//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLConstraintChecker.cpp
//
//	@doc:
//		Implementation of the constraint checker (see CDSLConstraintChecker.h).
//		Migrates the SEMANTICS of WeTune Model.checkConstraints for the four
//		structural constraints (AttrsSub / Unique / NotNull / Reference).
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLConstraintChecker.h"

#include "gpos/base.h"
#include "gpos/error/CException.h"

#include "naucrates/exception.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefTable.h"
#include "gpopt/base/CKeyCollection.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CLogicalGet.h"
#include "naucrates/md/CMDForeignKey.h"
#include "naucrates/md/IMDRelation.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintChecker::PcrsFromAttrsSym
//
//	@doc:
//		Materialize the columns bound to an attrs symbol as a CColRefSet. NULL if
//		the symbol is unbound (a constraint over an unbound symbol cannot hold).
//---------------------------------------------------------------------------
CColRefSet *
CDSLConstraintChecker::PcrsFromAttrsSym(const CDSLSymbol *psymAttrs,
										const CDSLModel *pmodel) const
{
	CColRefArray *pdrgpcr = pmodel->PdrgpcrAttrs(psymAttrs);
	if (nullptr == pdrgpcr)
	{
		return nullptr;
	}
	CColRefSet *pcrs = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrs->Include(pdrgpcr);
	return pcrs;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintChecker::FCheckAttrsSub
//
//	@doc:
//		AttrsSub(a,x): x is either a table/subtree symbol or a schema symbol.
//---------------------------------------------------------------------------
BOOL
CDSLConstraintChecker::FCheckAttrsSub(const CDSLConstraint *pcon,
									  const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (2 != pdrgpsym->Size())
	{
		return false;
	}

	const CDSLSymbol *psymAttrs = (*pdrgpsym)[0];
	const CDSLSymbol *psymSource = (*pdrgpsym)[1];
	if (EdslsymAttrs != psymAttrs->Esymkind() ||
		(EdslsymTable != psymSource->Esymkind() &&
		 EdslsymSchema != psymSource->Esymkind()))
	{
		return false;
	}

	CColRefSet *pcrsAttrs = PcrsFromAttrsSym(psymAttrs, pmodel);
	if (nullptr == pcrsAttrs)
	{
		return false;
	}

	BOOL fHolds = false;
	if (EdslsymTable == psymSource->Esymkind())
	{
		CExpression *pexprTable = pmodel->PexprTable(psymSource);
		fHolds = nullptr != pexprTable &&
				 pexprTable->DeriveOutputColumns()->ContainsAll(pcrsAttrs);
	}
	else
	{
		CColRefArray *pdrgpcrSchema = pmodel->PdrgpcrSchema(psymSource);
		if (nullptr != pdrgpcrSchema)
		{
			CColRefSet *pcrsSchema = GPOS_NEW(m_mp) CColRefSet(m_mp);
			pcrsSchema->Include(pdrgpcrSchema);
			fHolds = pcrsSchema->ContainsAll(pcrsAttrs);
			pcrsSchema->Release();
		}
	}
	pcrsAttrs->Release();
	return fHolds;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintChecker::FCheckUnique
//
//	@doc:
//		Unique(t,a): the columns bound to <a> form a key of the subtree bound to
//		<t>. Uses the derived key collection (a superset key is acceptable — a
//		non-exact match, since the DSL asserts "these columns are unique", not
//		"these are exactly the key").
//---------------------------------------------------------------------------
BOOL
CDSLConstraintChecker::FCheckUnique(const CDSLConstraint *pcon,
									const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (2 != pdrgpsym->Size())
	{
		return false;
	}

	const CDSLSymbol *psymAttrs = nullptr;
	const CDSLSymbol *psymTable = nullptr;
	for (ULONG ul = 0; ul < 2; ul++)
	{
		const CDSLSymbol *psym = (*pdrgpsym)[ul];
		if (EdslsymAttrs == psym->Esymkind())
		{
			psymAttrs = psym;
		}
		else if (EdslsymTable == psym->Esymkind())
		{
			psymTable = psym;
		}
	}
	if (nullptr == psymAttrs || nullptr == psymTable)
	{
		return false;
	}

	CExpression *pexprTable = pmodel->PexprTable(psymTable);
	CColRefSet *pcrsAttrs = PcrsFromAttrsSym(psymAttrs, pmodel);
	if (nullptr == pexprTable || nullptr == pcrsAttrs)
	{
		CRefCount::SafeRelease(pcrsAttrs);
		return false;
	}

	CKeyCollection *pkc = pexprTable->DeriveKeyCollection();
	// no keys derived => the uniqueness assertion cannot be confirmed => reject.
	BOOL fHolds =
		(nullptr != pkc) && pkc->FKey(pcrsAttrs, false /*fExactMatch*/);
	pcrsAttrs->Release();
	return fHolds;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintChecker::FCheckNotNull
//
//	@doc:
//		NotNull(t,a): every column bound to <a> is non-nullable in the subtree
//		bound to <t> (derived not-null columns contain all of <a>).
//---------------------------------------------------------------------------
BOOL
CDSLConstraintChecker::FCheckNotNull(const CDSLConstraint *pcon,
									 const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (2 != pdrgpsym->Size())
	{
		return false;
	}

	const CDSLSymbol *psymAttrs = nullptr;
	const CDSLSymbol *psymTable = nullptr;
	for (ULONG ul = 0; ul < 2; ul++)
	{
		const CDSLSymbol *psym = (*pdrgpsym)[ul];
		if (EdslsymAttrs == psym->Esymkind())
		{
			psymAttrs = psym;
		}
		else if (EdslsymTable == psym->Esymkind())
		{
			psymTable = psym;
		}
	}
	if (nullptr == psymAttrs || nullptr == psymTable)
	{
		return false;
	}

	CExpression *pexprTable = pmodel->PexprTable(psymTable);
	CColRefSet *pcrsAttrs = PcrsFromAttrsSym(psymAttrs, pmodel);
	if (nullptr == pexprTable || nullptr == pcrsAttrs)
	{
		CRefCount::SafeRelease(pcrsAttrs);
		return false;
	}

	CColRefSet *pcrsNotNull = pexprTable->DeriveNotNullColumns();
	BOOL fHolds = pcrsNotNull->ContainsAll(pcrsAttrs);
	pcrsAttrs->Release();
	return fHolds;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintChecker::FCheckReference
//
//	@doc:
//		Reference(t0,a0,t1,a1): the columns bound to a0 (in the relation bound to
//		t0) reference the columns bound to a1 (in the relation bound to t1) via a
//		foreign key. Symbols are positional: [t0, a0, t1, a1].
//
//		Live-metadata path (M2): resolve t0's bound subtree to its CLogicalGet ->
//		CTableDescriptor -> relation MDId -> IMDRelation, then look for a foreign
//		key whose referenced relation is t1's MDId and whose local/referenced attno
//		sets equal the attnos of a0 / a1. FK metadata is populated only by
//		CMDRelationGPDB from a live relcache; the programmatic test fixture carries
//		none, so on a synthetic relation ForeignKeyCount()==0 and this returns false
//		(a Reference-guarded rule then simply does not fire — no regression).
//---------------------------------------------------------------------------

// collect the (table) attnos of the columns bound to an attrs symbol into pais.
// Returns false if the symbol is unbound or any bound column is not a table
// column (a Reference over computed columns cannot be an FK).
static BOOL
FCollectAttnos(CMemoryPool *mp, const CDSLSymbol *psymAttrs,
			   const CDSLModel *pmodel, IntPtrArray *pais)
{
	CColRefArray *pdrgpcr = pmodel->PdrgpcrAttrs(psymAttrs);
	if (nullptr == pdrgpcr || 0 == pdrgpcr->Size())
	{
		return false;
	}
	const ULONG ulCols = pdrgpcr->Size();
	for (ULONG ul = 0; ul < ulCols; ul++)
	{
		CColRef *pcr = (*pdrgpcr)[ul];
		if (CColRef::EcrtTable != pcr->Ecrt())
		{
			return false;
		}
		pais->Append(GPOS_NEW(mp) INT(CColRefTable::PcrConvert(pcr)->AttrNum()));
	}
	return true;
}

// true iff the two attno arrays hold the same SET of attnos (order-independent).
static BOOL
FSameAttnoSet(const IntPtrArray *paisFst, const IntPtrArray *paisSnd)
{
	const ULONG ulFst = paisFst->Size();
	if (ulFst != paisSnd->Size())
	{
		return false;
	}
	for (ULONG ul = 0; ul < ulFst; ul++)
	{
		const INT iTarget = *(*paisFst)[ul];
		BOOL fFound = false;
		for (ULONG ulS = 0; ulS < paisSnd->Size() && !fFound; ulS++)
		{
			fFound = (iTarget == *(*paisSnd)[ulS]);
		}
		if (!fFound)
		{
			return false;
		}
	}
	return true;
}

// resolve the relation MDId of the subtree bound to a table symbol; NULL if the
// subtree is not a plain CLogicalGet (conservative — cannot confirm the FK).
static IMDId *
PmdidRelFromTableSym(const CDSLSymbol *psymTable, const CDSLModel *pmodel)
{
	CExpression *pexpr = pmodel->PexprTable(psymTable);
	if (nullptr == pexpr ||
		COperator::EopLogicalGet != pexpr->Pop()->Eopid())
	{
		return nullptr;
	}
	return CLogicalGet::PopConvert(pexpr->Pop())->Ptabdesc()->MDId();
}

BOOL
CDSLConstraintChecker::FCheckReference(const CDSLConstraint *pcon,
									   const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (4 != pdrgpsym->Size())
	{
		return false;
	}
	// positional schema: Reference(t0, a0, t1, a1)
	const CDSLSymbol *psymTab0 = (*pdrgpsym)[0];
	const CDSLSymbol *psymAttr0 = (*pdrgpsym)[1];
	const CDSLSymbol *psymTab1 = (*pdrgpsym)[2];
	const CDSLSymbol *psymAttr1 = (*pdrgpsym)[3];
	if (EdslsymTable != psymTab0->Esymkind() ||
		EdslsymAttrs != psymAttr0->Esymkind() ||
		EdslsymTable != psymTab1->Esymkind() ||
		EdslsymAttrs != psymAttr1->Esymkind())
	{
		return false;
	}

	IMDId *pmdidRel0 = PmdidRelFromTableSym(psymTab0, pmodel);
	IMDId *pmdidRel1 = PmdidRelFromTableSym(psymTab1, pmodel);
	if (nullptr == pmdidRel0 || nullptr == pmdidRel1)
	{
		return false;
	}

	IntPtrArray *paisLocal = GPOS_NEW(m_mp) IntPtrArray(m_mp);
	IntPtrArray *paisRef = GPOS_NEW(m_mp) IntPtrArray(m_mp);
	if (!FCollectAttnos(m_mp, psymAttr0, pmodel, paisLocal) ||
		!FCollectAttnos(m_mp, psymAttr1, pmodel, paisRef))
	{
		paisLocal->Release();
		paisRef->Release();
		return false;
	}

	CMDAccessor *pmda = COptCtxt::PoctxtFromTLS()->Pmda();

	BOOL fHolds = false;
	// RetrieveRel raises ExmiMDCacheEntryNotFound when the relation isn't cached
	// (e.g. the synthetic programmatic-test fixture registers only scalar types).
	// A best-effort FK check must never abort optimization, so swallow that one
	// exception and treat it as "cannot confirm the FK" => reject.
	GPOS_TRY
	{
		const IMDRelation *prel = pmda->RetrieveRel(pmdidRel0);
		const ULONG ulFK = prel->ForeignKeyCount();
		for (ULONG ul = 0; ul < ulFK && !fHolds; ul++)
		{
			const CMDForeignKey *pfk = prel->ForeignKeyAt(ul);
			if (pfk->RefMdid()->Equals(pmdidRel1) &&
				FSameAttnoSet(pfk->LocalAttnos(), paisLocal) &&
				FSameAttnoSet(pfk->RefAttnos(), paisRef))
			{
				fHolds = true;
			}
		}
	}
	GPOS_CATCH_EX(ex)
	{
		if (GPOS_MATCH_EX(ex, gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryNotFound))
		{
			GPOS_RESET_EX;
			fHolds = false;
		}
		else
		{
			paisLocal->Release();
			paisRef->Release();
			GPOS_RETHROW(ex);
		}
	}
	GPOS_CATCH_END;

	paisLocal->Release();
	paisRef->Release();
	return fHolds;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintChecker::FCheckOne
//---------------------------------------------------------------------------
BOOL
CDSLConstraintChecker::FCheckOne(const CDSLConstraint *pcon,
								 const CDSLModel *pmodel) const
{
	switch (pcon->Edslcon())
	{
		case EdslconAttrsSub:
			return FCheckAttrsSub(pcon, pmodel);
		case EdslconUnique:
			return FCheckUnique(pcon, pmodel);
		case EdslconNotNull:
			return FCheckNotNull(pcon, pmodel);
		case EdslconReference:
			return FCheckReference(pcon, pmodel);

		// equality-class constraints are not run-time data checks; they govern
		// how the target reuses source bindings (consumed by the instantiator,
		// #27). FBind already enforced same-class-same-artifact during match, so
		// they hold by construction here.
		case EdslconTableEq:
		case EdslconAttrsEq:
		case EdslconPredicateEq:
		case EdslconSchemaEq:
		case EdslconFuncEq:
		case EdslconScalarEq:
			return true;

		default:
			// unknown constraint kind: be safe, do not fire.
			return false;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintChecker::FCheck
//
//	@doc:
//		All constraints must hold. Early-abort on the first failure (WeTune
//		Model.checkConstraints behaviour).
//---------------------------------------------------------------------------
BOOL
CDSLConstraintChecker::FCheck(const CDSLRule *prule,
							  const CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(nullptr != pmodel);

	CDSLConstraintArray *pdrgpcon = prule->Pdrgpcon();
	if (nullptr == pdrgpcon)
	{
		return true;  // no constraints => trivially satisfied
	}

	const ULONG ulCon = pdrgpcon->Size();
	for (ULONG ul = 0; ul < ulCon; ul++)
	{
		if (!FCheckOne((*pdrgpcon)[ul], pmodel))
		{
			return false;
		}
	}
	return true;
}

// EOF
