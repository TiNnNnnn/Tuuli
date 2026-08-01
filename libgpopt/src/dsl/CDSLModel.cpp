//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLModel.cpp
//
//	@doc:
//		Implementation of the symbol-binding model (see CDSLModel.h).
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLModel.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CDSLModel::CDSLModel
//---------------------------------------------------------------------------
CDSLModel::CDSLModel(CMemoryPool *mp) : m_mp(mp), m_pdrgpexprResidual(nullptr)
{
	GPOS_ASSERT(nullptr != mp);
	m_phmSymToRef = GPOS_NEW(mp) CDSLSymbolToRefMap(mp);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLModel::~CDSLModel
//---------------------------------------------------------------------------
CDSLModel::~CDSLModel()
{
	// releasing the map releases every stored value (CleanupRelease); keys are
	// unowned (CleanupNULL).
	m_phmSymToRef->Release();
	CRefCount::SafeRelease(m_pdrgpexprResidual);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLModel::SetResidualConjuncts
//
//	@doc:
//		Take ownership of the unconsumed-conjunct list (filter split, #25). A
//		second call replaces the previous set (the earlier one is released).
//---------------------------------------------------------------------------
void
CDSLModel::SetResidualConjuncts(CExpressionArray *pdrgpexpr)
{
	CRefCount::SafeRelease(m_pdrgpexprResidual);
	m_pdrgpexprResidual = pdrgpexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLModel::FBind
//
//	@doc:
//		Bind a symbol to an artifact. Rebinding to the same value is a no-op
//		success; rebinding to a different value fails (WeTune incompatible
//		reassignment). AddRef's the value on first insert.
//---------------------------------------------------------------------------
BOOL
CDSLModel::FBind(const CDSLSymbol *psym, CRefCount *pval)
{
	GPOS_ASSERT(nullptr != psym);
	GPOS_ASSERT(nullptr != pval);

	CRefCount *pvalExisting = m_phmSymToRef->Find(psym);
	if (nullptr != pvalExisting)
	{
		// already bound: only compatible if it is the SAME artifact
		return pvalExisting == pval;
	}

	pval->AddRef();
	// key is const in the map's eyes; CHashMap takes non-const K*, and the map
	// never mutates or owns the key (CleanupNULL).
	BOOL fInserted =
		m_phmSymToRef->Insert(const_cast<CDSLSymbol *>(psym), pval);
	GPOS_ASSERT(fInserted);
	return fInserted;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLModel::PvalLookup
//---------------------------------------------------------------------------
CRefCount *
CDSLModel::PvalLookup(const CDSLSymbol *psym) const
{
	GPOS_ASSERT(nullptr != psym);
	return m_phmSymToRef->Find(psym);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLModel::PexprTable / PexprPred / PdrgpcrAttrs / PdrgpcrSchema
//
//	@doc:
//		Typed views over the stored CRefCount*. The stored dynamic type is
//		guaranteed by the binder (Match) matching the symbol kind; here we just
//		static_cast. NULL when unbound.
//---------------------------------------------------------------------------
CExpression *
CDSLModel::PexprTable(const CDSLSymbol *psym) const
{
	GPOS_ASSERT(EdslsymTable == psym->Esymkind());
	return dynamic_cast<CExpression *>(PvalLookup(psym));
}

CExpression *
CDSLModel::PexprPred(const CDSLSymbol *psym) const
{
	GPOS_ASSERT(EdslsymPred == psym->Esymkind());
	return dynamic_cast<CExpression *>(PvalLookup(psym));
}

CColRefArray *
CDSLModel::PdrgpcrAttrs(const CDSLSymbol *psym) const
{
	GPOS_ASSERT(EdslsymAttrs == psym->Esymkind());
	// CColRefArray is CDynamicPtrArray<CColRef, CleanupNULL>, itself a
	// CRefCount; recover via static_cast (dynamic_cast on template arrays is
	// unreliable).
	return static_cast<CColRefArray *>(PvalLookup(psym));
}

CColRefArray *
CDSLModel::PdrgpcrSchema(const CDSLSymbol *psym) const
{
	GPOS_ASSERT(EdslsymSchema == psym->Esymkind());
	return static_cast<CColRefArray *>(PvalLookup(psym));
}

// EOF
