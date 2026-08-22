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
CDSLModel::CDSLModel(CMemoryPool *mp)
	: m_mp(mp),
	  m_pdrgpexprResidual(nullptr),
	  m_pdrgpexprExistsResidual(nullptr),
	  m_pdrgpexprInSubResidual(nullptr),
	  m_fDedupDrop(false),
	  m_pexprDistinctAgg(nullptr)
{
	GPOS_ASSERT(nullptr != mp);
	m_phmSymToRef = GPOS_NEW(mp) CDSLSymbolToRefMap(mp);
	m_phmInSubPred = GPOS_NEW(mp) CDSLSymbolToExpressionMap(mp);
	m_phmProjList = GPOS_NEW(mp) CDSLSymbolToExpressionMap(mp);
	m_phmJoinPred = GPOS_NEW(mp) CDSLSymbolToExpressionMap(mp);
	m_pdrgpexprUnionBindings = GPOS_NEW(mp) CExpressionArray(mp);
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
	m_phmInSubPred->Release();
	m_phmProjList->Release();
	m_phmJoinPred->Release();
	m_pdrgpexprUnionBindings->Release();
	CRefCount::SafeRelease(m_pdrgpexprResidual);
	CRefCount::SafeRelease(m_pdrgpexprExistsResidual);
	CRefCount::SafeRelease(m_pdrgpexprInSubResidual);
	CRefCount::SafeRelease(m_pexprDistinctAgg);
}

void
CDSLModel::SetExistsResidualConjuncts(CExpressionArray *pdrgpexpr)
{
	CRefCount::SafeRelease(m_pdrgpexprExistsResidual);
	m_pdrgpexprExistsResidual = pdrgpexpr;
}

BOOL
CDSLModel::FSetInSubPred(const CDSLSymbol *psymAttrs, CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != psymAttrs);
	GPOS_ASSERT(EdslsymAttrs == psymAttrs->Esymkind());
	GPOS_ASSERT(nullptr != pexpr);

	CExpression *pexprExisting = m_phmInSubPred->Find(psymAttrs);
	if (nullptr != pexprExisting)
	{
		BOOL fCompatible = pexprExisting->Matches(pexpr);
		pexpr->Release();
		return fCompatible;
	}
	BOOL fInserted = m_phmInSubPred->Insert(
		const_cast<CDSLSymbol *>(psymAttrs), pexpr);
	GPOS_ASSERT(fInserted);
	return fInserted;
}

CExpression *
CDSLModel::PexprInSubPred(const CDSLSymbol *psymAttrs) const
{
	GPOS_ASSERT(nullptr != psymAttrs);
	GPOS_ASSERT(EdslsymAttrs == psymAttrs->Esymkind());
	return m_phmInSubPred->Find(psymAttrs);
}

void
CDSLModel::SetInSubResidualConjuncts(CExpressionArray *pdrgpexpr)
{
	CRefCount::SafeRelease(m_pdrgpexprInSubResidual);
	m_pdrgpexprInSubResidual = pdrgpexpr;
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
//		CDSLModel::FSetProjList
//
//	@doc:
//		Record one matched CLogicalProject's project list by schema symbol. A
//		rebind is accepted only when the scalar structures match.
//---------------------------------------------------------------------------
BOOL
CDSLModel::FSetProjList(const CDSLSymbol *psymSchema, CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != psymSchema);
	GPOS_ASSERT(EdslsymSchema == psymSchema->Esymkind());
	GPOS_ASSERT(nullptr != pexpr);

	CExpression *pexprExisting = m_phmProjList->Find(psymSchema);
	if (nullptr != pexprExisting)
	{
		BOOL fCompatible = pexprExisting->Matches(pexpr);
		pexpr->Release();
		return fCompatible;
	}
	BOOL fInserted = m_phmProjList->Insert(
		const_cast<CDSLSymbol *>(psymSchema), pexpr);
	GPOS_ASSERT(fInserted);
	return fInserted;
}

CExpression *
CDSLModel::PexprProjList(const CDSLSymbol *psymSchema) const
{
	GPOS_ASSERT(nullptr != psymSchema);
	GPOS_ASSERT(EdslsymSchema == psymSchema->Esymkind());
	return m_phmProjList->Find(psymSchema);
}

void
CDSLModel::AddUnionBinding(CExpression *pexprUnion)
{
	GPOS_ASSERT(nullptr != pexprUnion);
	GPOS_ASSERT(COperator::EopLogicalUnion == pexprUnion->Pop()->Eopid() ||
				COperator::EopLogicalUnionAll == pexprUnion->Pop()->Eopid());
	pexprUnion->AddRef();
	m_pdrgpexprUnionBindings->Append(pexprUnion);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLModel::FSetJoinPred
//
//	@doc:
//		Index one source Join predicate by both attrs symbols. Existing bindings are
//		accepted only when they carry the same scalar tree.
//---------------------------------------------------------------------------
BOOL
CDSLModel::FSetJoinPred(const CDSLSymbol *psymLeftAttrs,
						 const CDSLSymbol *psymRightAttrs,
						 CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != psymLeftAttrs);
	GPOS_ASSERT(nullptr != psymRightAttrs);
	GPOS_ASSERT(nullptr != pexpr);

	CExpression *pexprLeft = m_phmJoinPred->Find(psymLeftAttrs);
	CExpression *pexprRight = m_phmJoinPred->Find(psymRightAttrs);
	if ((nullptr != pexprLeft && !pexprLeft->Matches(pexpr)) ||
		(nullptr != pexprRight && !pexprRight->Matches(pexpr)))
	{
		return false;
	}
	if (nullptr == pexprLeft)
	{
		pexpr->AddRef();
		BOOL fInserted GPOS_ASSERTS_ONLY = m_phmJoinPred->Insert(
			const_cast<CDSLSymbol *>(psymLeftAttrs), pexpr);
		GPOS_ASSERT(fInserted);
	}
	if (psymRightAttrs != psymLeftAttrs && nullptr == pexprRight)
	{
		pexpr->AddRef();
		BOOL fInserted GPOS_ASSERTS_ONLY = m_phmJoinPred->Insert(
			const_cast<CDSLSymbol *>(psymRightAttrs), pexpr);
		GPOS_ASSERT(fInserted);
	}
	return true;
}

CExpression *
CDSLModel::PexprJoinPred(const CDSLSymbol *psymLeftAttrs,
						 const CDSLSymbol *psymRightAttrs) const
{
	CExpression *pexprLeft = m_phmJoinPred->Find(psymLeftAttrs);
	CExpression *pexprRight = m_phmJoinPred->Find(psymRightAttrs);
	return nullptr != pexprLeft && nullptr != pexprRight &&
			   pexprLeft->Matches(pexprRight)
		   ? pexprLeft
		   : nullptr;
}

BOOL
CDSLModel::FSetDistinctAgg(CExpression *pexprAgg)
{
	GPOS_ASSERT(nullptr != pexprAgg);
	if (nullptr != m_pexprDistinctAgg)
	{
		return m_pexprDistinctAgg == pexprAgg;
	}
	pexprAgg->AddRef();
	m_pexprDistinctAgg = pexprAgg;
	return true;
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
//		CDSLModel typed accessors
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

CExpression *
CDSLModel::PexprScalar(const CDSLSymbol *psym) const
{
	GPOS_ASSERT(EdslsymScalar == psym->Esymkind());
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

CExpressionArray *
CDSLModel::PdrgpexprFunc(const CDSLSymbol *psym) const
{
	GPOS_ASSERT(EdslsymFunc == psym->Esymkind());
	return static_cast<CExpressionArray *>(PvalLookup(psym));
}

// EOF
