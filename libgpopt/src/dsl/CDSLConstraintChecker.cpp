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

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CKeyCollection.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/operators/CExpression.h"

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
//		AttrsSub(a,t): the columns bound to the attrs symbol must be a subset of
//		the output columns of the subtree bound to the table symbol. Symbols are
//		distinguished by their (declaration-time) kind, not argument position.
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

	// find the attrs and table symbols by kind (order is a/t but be robust)
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

	CColRefSet *pcrsOutput = pexprTable->DeriveOutputColumns();
	BOOL fHolds = pcrsOutput->ContainsAll(pcrsAttrs);
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
//		Reference(t0,a0,t1,a1): a0 references a1 via a foreign key. In this ORCA
//		build the FK metadata (IMDRelation::ForeignKeyAt) is populated only by
//		CMDRelationGPDB from a live relcache; the programmatic test fixture
//		carries no FK info, so this is implemented conservatively — with no FK
//		metadata reachable we currently REJECT (a rule guarded by Reference will
//		simply not fire on synthetic relations). Full FK verification is enabled
//		once base B / live PG metadata is wired (doc §1); the test migration marks
//		Reference as pending accordingly.
//---------------------------------------------------------------------------
BOOL
CDSLConstraintChecker::FCheckReference(const CDSLConstraint *,  // pcon
									   const CDSLModel *		   // pmodel
) const
{
	// conservative default: cannot confirm the FK => do not fire.
	return false;
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
