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
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/dsl/CDSLExprListUtils.h"
#include "gpos/common/CHashMapIter.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/search/CGroupExpression.h"
#include "naucrates/md/IMDTypeBool.h"

using namespace gpopt;

CDSLFrameBound::CDSLFrameBound(CWindowFrame::EFrameBoundary efb,
							   CExpression *pexprOffset)
	: m_efb(efb), m_pexprOffset(pexprOffset)
{
	if (nullptr != m_pexprOffset)
	{
		m_pexprOffset->AddRef();
	}
}

CDSLFrameBound::~CDSLFrameBound()
{
	CRefCount::SafeRelease(m_pexprOffset);
}

BOOL
CDSLFrameBound::Matches(const CDSLFrameBound *other) const
{
	return nullptr != other && m_efb == other->m_efb &&
		   (m_pexprOffset == other->m_pexprOffset ||
			(nullptr != m_pexprOffset && nullptr != other->m_pexprOffset &&
			 CDSLMatchView::FSameCapturedExpression(m_pexprOffset, other->m_pexprOffset)));
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLModel::CDSLModel
//---------------------------------------------------------------------------
CDSLModel::CDSLModel(CMemoryPool *mp, CGroupExpression *source)
	: m_mp(mp),
	  m_pgexprSource(source),
	  m_phmSubqueryMarkers(nullptr),
	  m_pdrgpexprResidual(nullptr),
	  m_fDedupDrop(false),
	  m_pexprDistinctAgg(nullptr)
{
	GPOS_ASSERT(nullptr != mp);
	m_phmSymToRef = GPOS_NEW(mp) CDSLSymbolToRefMap(mp);
	m_phmScalarContexts = nullptr;
	m_pdrgpsymDerived = GPOS_NEW(mp) CDSLSymbolArray(mp);
	// Most match attempts never use these operator-specific bindings. Allocate
	// each map on its first write, as with the subquery-marker map.
	m_phmInSubPred = nullptr;
	m_phmInSubCarrier = nullptr;
	m_phmFilterCarrier = nullptr;
	m_phmComputeCarrier = nullptr;
	m_phmApplyCarrier = nullptr;
	m_phmProjList = nullptr;
	m_phmProjLimitShell = nullptr;
	m_phmProjAggShell = nullptr;
	m_phmAggBinding = nullptr;
	m_phmVirtualIdentityProj = nullptr;
	m_phmJoinPred = nullptr;
	m_phmJoinEqualities = nullptr;
	m_phmWindowCarrier = nullptr;
	m_pdrgpexprUnionBindings = GPOS_NEW(mp) CExpressionArray(mp);
	m_pdrgpexprNaryUnionTails = GPOS_NEW(mp) CExpressionArray(mp);
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
	CRefCount::SafeRelease(m_phmScalarContexts);
	m_pdrgpsymDerived->Release();
	CRefCount::SafeRelease(m_phmSubqueryMarkers);
	CRefCount::SafeRelease(m_phmInSubPred);
	CRefCount::SafeRelease(m_phmInSubCarrier);
	CRefCount::SafeRelease(m_phmFilterCarrier);
	CRefCount::SafeRelease(m_phmComputeCarrier);
	CRefCount::SafeRelease(m_phmApplyCarrier);
	CRefCount::SafeRelease(m_phmProjList);
	CRefCount::SafeRelease(m_phmProjLimitShell);
	CRefCount::SafeRelease(m_phmProjAggShell);
	CRefCount::SafeRelease(m_phmAggBinding);
	CRefCount::SafeRelease(m_phmVirtualIdentityProj);
	CRefCount::SafeRelease(m_phmJoinPred);
	CRefCount::SafeRelease(m_phmJoinEqualities);
	CRefCount::SafeRelease(m_phmWindowCarrier);
	m_pdrgpexprUnionBindings->Release();
	m_pdrgpexprNaryUnionTails->Release();
	CRefCount::SafeRelease(m_pdrgpexprResidual);
	CRefCount::SafeRelease(m_pexprDistinctAgg);
}

BOOL
CDSLModel::FSetInSubPred(const CDSLSymbol *psymAttrs, CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != psymAttrs);
	GPOS_ASSERT(EdslsymAttrs == psymAttrs->Esymkind());
	GPOS_ASSERT(nullptr != pexpr);

	if (nullptr == m_phmInSubPred)
	{
		m_phmInSubPred = GPOS_NEW(m_mp) CDSLSymbolToExpressionMap(m_mp);
	}
	CExpression *pexprExisting = m_phmInSubPred->Find(psymAttrs);
	if (nullptr != pexprExisting)
	{
		BOOL fCompatible = CDSLMatchView::FSameCapturedExpression(pexprExisting, pexpr);
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
	return nullptr == m_phmInSubPred ? nullptr : m_phmInSubPred->Find(psymAttrs);
}

BOOL
CDSLModel::FSetInSubCarrier(const CDSLSymbol *psymAttrs,
							 CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != psymAttrs);
	GPOS_ASSERT(EdslsymAttrs == psymAttrs->Esymkind());
	GPOS_ASSERT(nullptr != pexpr);

	if (nullptr == m_phmInSubCarrier)
	{
		m_phmInSubCarrier = GPOS_NEW(m_mp) CDSLSymbolToExpressionMap(m_mp);
	}
	CExpression *pexprExisting = m_phmInSubCarrier->Find(psymAttrs);
	if (nullptr != pexprExisting)
	{
		BOOL fCompatible = CDSLMatchView::FSameCapturedExpression(pexprExisting, pexpr);
		pexpr->Release();
		return fCompatible;
	}
	BOOL fInserted = m_phmInSubCarrier->Insert(
		const_cast<CDSLSymbol *>(psymAttrs), pexpr);
	GPOS_ASSERT(fInserted);
	return fInserted;
}

CExpression *
CDSLModel::PexprInSubCarrier(const CDSLSymbol *psymAttrs) const
{
	GPOS_ASSERT(nullptr != psymAttrs);
	GPOS_ASSERT(EdslsymAttrs == psymAttrs->Esymkind());
	return nullptr == m_phmInSubCarrier ? nullptr : m_phmInSubCarrier->Find(psymAttrs);
}

BOOL
CDSLModel::FSetFilterCarrier(const CDSLSymbol *psymPred,
							 CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != psymPred);
	GPOS_ASSERT(EdslsymPred == psymPred->Esymkind());
	GPOS_ASSERT(nullptr != pexpr);

	if (nullptr == m_phmFilterCarrier)
	{
		m_phmFilterCarrier = GPOS_NEW(m_mp) CDSLSymbolToExpressionMap(m_mp);
	}
	CExpression *pexprExisting = m_phmFilterCarrier->Find(psymPred);
	if (nullptr != pexprExisting)
	{
		BOOL fCompatible = CDSLMatchView::FSameCapturedExpression(pexprExisting, pexpr);
		pexpr->Release();
		return fCompatible;
	}
	BOOL fInserted = m_phmFilterCarrier->Insert(
		const_cast<CDSLSymbol *>(psymPred), pexpr);
	GPOS_ASSERT(fInserted);
	return fInserted;
}

CExpression *
CDSLModel::PexprFilterCarrier(const CDSLSymbol *psymPred) const
{
	GPOS_ASSERT(nullptr != psymPred);
	GPOS_ASSERT(EdslsymPred == psymPred->Esymkind());
	return nullptr == m_phmFilterCarrier ? nullptr : m_phmFilterCarrier->Find(psymPred);
}

BOOL
CDSLModel::FSetComputeCarrier(const CDSLSymbol *psymExpr, CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != psymExpr);
	GPOS_ASSERT(EdslsymExpr == psymExpr->Esymkind());
	GPOS_ASSERT(nullptr != pexpr);
	if (nullptr == m_phmComputeCarrier)
	{
		m_phmComputeCarrier = GPOS_NEW(m_mp) CDSLSymbolToExpressionMap(m_mp);
	}
	CExpression *existing = m_phmComputeCarrier->Find(psymExpr);
	if (nullptr != existing)
	{
		const BOOL compatible =
			CDSLMatchView::FSameCapturedExpression(existing, pexpr);
		pexpr->Release();
		return compatible;
	}
	return m_phmComputeCarrier->Insert(
		const_cast<CDSLSymbol *>(psymExpr), pexpr);
}

CExpression *
CDSLModel::PexprComputeCarrier(const CDSLSymbol *psymExpr) const
{
	GPOS_ASSERT(nullptr != psymExpr);
	GPOS_ASSERT(EdslsymExpr == psymExpr->Esymkind());
	return nullptr == m_phmComputeCarrier ? nullptr : m_phmComputeCarrier->Find(psymExpr);
}

BOOL
CDSLModel::FSetApplyCarrier(const CDSLSymbol *psymPred,
							 CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != psymPred);
	GPOS_ASSERT(EdslsymPred == psymPred->Esymkind());
	GPOS_ASSERT(nullptr != pexpr);

	if (nullptr == m_phmApplyCarrier)
	{
		m_phmApplyCarrier = GPOS_NEW(m_mp) CDSLSymbolToExpressionMap(m_mp);
	}
	CExpression *pexprExisting = m_phmApplyCarrier->Find(psymPred);
	if (nullptr != pexprExisting)
	{
		BOOL fCompatible = CDSLMatchView::FSameCapturedExpression(pexprExisting, pexpr);
		pexpr->Release();
		return fCompatible;
	}
	BOOL fInserted = m_phmApplyCarrier->Insert(
		const_cast<CDSLSymbol *>(psymPred), pexpr);
	GPOS_ASSERT(fInserted);
	return fInserted;
}

CExpression *
CDSLModel::PexprApplyCarrier(const CDSLSymbol *psymPred) const
{
	GPOS_ASSERT(nullptr != psymPred);
	GPOS_ASSERT(EdslsymPred == psymPred->Esymkind());
	return nullptr == m_phmApplyCarrier ? nullptr : m_phmApplyCarrier->Find(psymPred);
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

	if (nullptr == m_phmProjList)
	{
		m_phmProjList = GPOS_NEW(m_mp) CDSLSymbolToExpressionMap(m_mp);
	}
	CExpression *pexprExisting = m_phmProjList->Find(psymSchema);
	if (nullptr != pexprExisting)
	{
		BOOL fCompatible = CDSLMatchView::FSameCapturedExpression(pexprExisting, pexpr);
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
	return nullptr == m_phmProjList ? nullptr : m_phmProjList->Find(psymSchema);
}

BOOL
CDSLModel::FSetVirtualIdentityProj(const CDSLSymbol *psymSchema,
								   CExpression *pexprCarrier)
{
	GPOS_ASSERT(nullptr != psymSchema);
	GPOS_ASSERT(EdslsymSchema == psymSchema->Esymkind());
	GPOS_ASSERT(nullptr != pexprCarrier);

	if (nullptr == m_phmVirtualIdentityProj)
	{
		m_phmVirtualIdentityProj = GPOS_NEW(m_mp) CDSLSymbolToExpressionMap(m_mp);
	}
	CExpression *pexprExisting = m_phmVirtualIdentityProj->Find(psymSchema);
	if (nullptr != pexprExisting)
	{
		BOOL fCompatible = CDSLMatchView::FSameCapturedExpression(pexprExisting, pexprCarrier);
		pexprCarrier->Release();
		return fCompatible;
	}
	BOOL fInserted = m_phmVirtualIdentityProj->Insert(
		const_cast<CDSLSymbol *>(psymSchema), pexprCarrier);
	GPOS_ASSERT(fInserted);
	return fInserted;
}

BOOL
CDSLModel::FVirtualIdentityProj(const CDSLSymbol *psymSchema) const
{
	GPOS_ASSERT(nullptr != psymSchema);
	GPOS_ASSERT(EdslsymSchema == psymSchema->Esymkind());
	return nullptr != m_phmVirtualIdentityProj &&
		   nullptr != m_phmVirtualIdentityProj->Find(psymSchema);
}

BOOL
CDSLModel::FSetProjLimitShell(const CDSLSymbol *psymSchema,
							 CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != psymSchema);
	GPOS_ASSERT(EdslsymSchema == psymSchema->Esymkind());
	GPOS_ASSERT(nullptr != pexpr);

	if (nullptr == m_phmProjLimitShell)
	{
		m_phmProjLimitShell = GPOS_NEW(m_mp) CDSLSymbolToExpressionMap(m_mp);
	}
	CExpression *pexprExisting = m_phmProjLimitShell->Find(psymSchema);
	if (nullptr != pexprExisting)
	{
		BOOL fCompatible = CDSLMatchView::FSameCapturedExpression(pexprExisting, pexpr);
		pexpr->Release();
		return fCompatible;
	}
	BOOL fInserted = m_phmProjLimitShell->Insert(
		const_cast<CDSLSymbol *>(psymSchema), pexpr);
	GPOS_ASSERT(fInserted);
	return fInserted;
}

CExpression *
CDSLModel::PexprProjLimitShell(const CDSLSymbol *psymSchema) const
{
	GPOS_ASSERT(nullptr != psymSchema);
	GPOS_ASSERT(EdslsymSchema == psymSchema->Esymkind());
	return nullptr == m_phmProjLimitShell ? nullptr : m_phmProjLimitShell->Find(psymSchema);
}

BOOL
CDSLModel::FSetProjAggShell(const CDSLSymbol *psymSchema,
						   CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != psymSchema);
	GPOS_ASSERT(EdslsymSchema == psymSchema->Esymkind());
	GPOS_ASSERT(nullptr != pexpr);

	if (nullptr == m_phmProjAggShell)
	{
		m_phmProjAggShell = GPOS_NEW(m_mp) CDSLSymbolToExpressionMap(m_mp);
	}
	CExpression *pexprExisting = m_phmProjAggShell->Find(psymSchema);
	if (nullptr != pexprExisting)
	{
		BOOL fCompatible = CDSLMatchView::FSameCapturedExpression(pexprExisting, pexpr);
		pexpr->Release();
		return fCompatible;
	}
	BOOL fInserted = m_phmProjAggShell->Insert(
		const_cast<CDSLSymbol *>(psymSchema), pexpr);
	GPOS_ASSERT(fInserted);
	return fInserted;
}

CExpression *
CDSLModel::PexprProjAggShell(const CDSLSymbol *psymSchema) const
{
	GPOS_ASSERT(nullptr != psymSchema);
	GPOS_ASSERT(EdslsymSchema == psymSchema->Esymkind());
	return nullptr == m_phmProjAggShell ? nullptr : m_phmProjAggShell->Find(psymSchema);
}

BOOL
CDSLModel::FSetAggBinding(const CDSLSymbol *psymSchema,
						  CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != psymSchema);
	GPOS_ASSERT(EdslsymSchema == psymSchema->Esymkind());
	GPOS_ASSERT(nullptr != pexpr);

	if (nullptr == m_phmAggBinding)
	{
		m_phmAggBinding = GPOS_NEW(m_mp) CDSLSymbolToExpressionMap(m_mp);
	}
	CExpression *pexprExisting = m_phmAggBinding->Find(psymSchema);
	if (nullptr != pexprExisting)
	{
		BOOL fCompatible = CDSLMatchView::FSameCapturedExpression(pexprExisting, pexpr);
		pexpr->Release();
		return fCompatible;
	}
	BOOL fInserted = m_phmAggBinding->Insert(
		const_cast<CDSLSymbol *>(psymSchema), pexpr);
	GPOS_ASSERT(fInserted);
	return fInserted;
}

CExpression *
CDSLModel::PexprAggBinding(const CDSLSymbol *psymSchema) const
{
	GPOS_ASSERT(nullptr != psymSchema);
	GPOS_ASSERT(EdslsymSchema == psymSchema->Esymkind());
	return nullptr == m_phmAggBinding ? nullptr : m_phmAggBinding->Find(psymSchema);
}

void
CDSLModel::AddUnionBinding(CExpression *pexprUnion)
{
	GPOS_ASSERT(nullptr != pexprUnion);
	const COperator::EOperatorId eopid = pexprUnion->Pop()->Eopid();
	GPOS_ASSERT(COperator::EopLogicalUnion == eopid ||
				COperator::EopLogicalUnionAll == eopid ||
				COperator::EopLogicalIntersect == eopid ||
				COperator::EopLogicalIntersectAll == eopid ||
				COperator::EopLogicalDifference == eopid ||
				COperator::EopLogicalDifferenceAll == eopid);
	pexprUnion->AddRef();
	m_pdrgpexprUnionBindings->Append(pexprUnion);
}

void
CDSLModel::AddNaryUnionTail(CExpression *pexprUnionAll)
{
	GPOS_ASSERT(nullptr != pexprUnionAll);
	GPOS_ASSERT(COperator::EopLogicalUnionAll ==
				pexprUnionAll->Pop()->Eopid());
	pexprUnionAll->AddRef();
	m_pdrgpexprNaryUnionTails->Append(pexprUnionAll);
}

BOOL
CDSLModel::FIsNaryUnionTail(CExpression *pexpr) const
{
	for (ULONG ul = 0; ul < m_pdrgpexprNaryUnionTails->Size(); ul++)
	{
		if ((*m_pdrgpexprNaryUnionTails)[ul] == pexpr)
		{
			return true;
		}
	}
	return false;
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
						 CExpression *pexpr, BOOL equalitiesOnly)
{
	GPOS_ASSERT(nullptr != psymLeftAttrs);
	GPOS_ASSERT(nullptr != psymRightAttrs);
	GPOS_ASSERT(nullptr != pexpr);

	auto *&predicates = equalitiesOnly ? m_phmJoinEqualities : m_phmJoinPred;
	if (nullptr == predicates)
	{
		predicates = GPOS_NEW(m_mp) CDSLSymbolToExpressionMap(m_mp);
	}
	CExpression *pexprLeft = predicates->Find(psymLeftAttrs);
	CExpression *pexprRight = predicates->Find(psymRightAttrs);
	if ((nullptr != pexprLeft && !CDSLMatchView::FSameCapturedExpression(pexprLeft, pexpr)) ||
		(nullptr != pexprRight && !CDSLMatchView::FSameCapturedExpression(pexprRight, pexpr)))
	{
		return false;
	}
	if (nullptr == pexprLeft)
	{
		pexpr->AddRef();
		BOOL fInserted GPOS_ASSERTS_ONLY = predicates->Insert(
			const_cast<CDSLSymbol *>(psymLeftAttrs), pexpr);
		GPOS_ASSERT(fInserted);
	}
	if (psymRightAttrs != psymLeftAttrs && nullptr == pexprRight)
	{
		pexpr->AddRef();
		BOOL fInserted GPOS_ASSERTS_ONLY = predicates->Insert(
			const_cast<CDSLSymbol *>(psymRightAttrs), pexpr);
		GPOS_ASSERT(fInserted);
	}
	return true;
}

CExpression *
CDSLModel::PexprJoinPred(const CDSLSymbol *psymLeftAttrs,
						 const CDSLSymbol *psymRightAttrs, BOOL equalitiesOnly) const
{
	auto *predicates = equalitiesOnly ? m_phmJoinEqualities : m_phmJoinPred;
	if (nullptr == predicates)
	{
		return nullptr;
	}
	CExpression *pexprLeft = predicates->Find(psymLeftAttrs);
	CExpression *pexprRight = predicates->Find(psymRightAttrs);
	return nullptr != pexprLeft && nullptr != pexprRight &&
			   CDSLMatchView::FSameCapturedExpression(pexprLeft, pexprRight)
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
		if (EdslsymFrameBound == psym->Esymkind())
		{
			return static_cast<CDSLFrameBound *>(pvalExisting)->Matches(
				static_cast<CDSLFrameBound *>(pval));
		}
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

CDSLScalarContext *
CDSLModel::PcontextScalar(const CDSLSymbol *symbol) const
{
	return nullptr == m_phmScalarContexts ? nullptr :
		static_cast<CDSLScalarContext *>(m_phmScalarContexts->Find(symbol));
}

BOOL
CDSLModel::FBindScalarContext(const CDSLSymbol *symbol, CDSLScalarContext *context)
{
	const auto *existing = PcontextScalar(symbol);
	if (nullptr != existing) return existing->Matches(context);
	if (nullptr == m_phmScalarContexts)
		m_phmScalarContexts = GPOS_NEW(m_mp) CDSLSymbolToRefMap(m_mp);
	context->AddRef();
	return m_phmScalarContexts->Insert(const_cast<CDSLSymbol *>(symbol), context);
}

BOOL
CDSLModel::FCopyExpressionBindingsTo(CDSLModel *target) const
{
	using Iterator = CHashMapIter<CDSLSymbol, CRefCount, gpos::HashPtr<CDSLSymbol>,
		gpos::EqualPtr<CDSLSymbol>, CleanupNULL<CDSLSymbol>, CleanupRelease<CRefCount>>;
	CDSLSymbolToRefMap *sources[] = {m_phmSymToRef, m_phmScalarContexts};
	CDSLSymbolToRefMap **destinations[] = {&target->m_phmSymToRef, &target->m_phmScalarContexts};
	for (BOOL copying : {false, true})
		for (ULONG i = 0; i < 2; ++i)
		{
			if (nullptr == sources[i]) continue;
			if (copying && nullptr == *destinations[i])
				*destinations[i] = GPOS_NEW(target->m_mp) CDSLSymbolToRefMap(target->m_mp);
			Iterator iterator(sources[i]);
			while (iterator.Advance())
			{
				const auto *key = iterator.Key();
				auto *value = const_cast<CRefCount *>(iterator.Value());
				auto *existing = nullptr == *destinations[i] ? nullptr : (*destinations[i])->Find(key);
				if (nullptr != existing && existing != value) return false;
				if (copying && nullptr == existing)
				{
					value->AddRef();
					(*destinations[i])->Insert(const_cast<CDSLSymbol *>(key), value);
				}
			}
		}
	return true;
}

BOOL
CDSLModel::FBindDerived(const CDSLSymbol *psym, CRefCount *pval)
{
	if (!FBind(psym, pval))
	{
		return false;
	}
	if (FDerivedBinding(psym))
	{
		return true;
	}
	const_cast<CDSLSymbol *>(psym)->AddRef();
	m_pdrgpsymDerived->Append(const_cast<CDSLSymbol *>(psym));
	return true;
}

BOOL
CDSLModel::FDerivedBinding(const CDSLSymbol *psym) const
{
	for (ULONG ul = 0; ul < m_pdrgpsymDerived->Size(); ul++)
	{
		if ((*m_pdrgpsymDerived)[ul] == psym)
		{
			return true;
		}
	}
	return false;
}

CColRef *
CDSLModel::PcrSubqueryMarker(const CDSLConstraint *constraint) const
{
	return nullptr == m_phmSubqueryMarkers ? nullptr : m_phmSubqueryMarkers->Find(constraint);
}

CColRef *
CDSLModel::PcrCreateSubqueryMarker(const CDSLConstraint *constraint,
	CExpression *subquery) const
{
	CColRef *marker = PcrSubqueryMarker(constraint);
	if (nullptr != marker) return marker;
	// Only repeated construction of the same Memo occurrence shares identity.
	// Standalone/RBO trees, synthetic views and distinct constructor slots stay
	// fresh. Output binding/provenance is still validated on every invocation.
	if (nullptr != m_pgexprSource && nullptr != subquery->Pgexpr())
		return m_pgexprSource->PcrDSLSubqueryMarker(constraint, subquery->Pgexpr());
	COptCtxt *context = COptCtxt::PoctxtFromTLS();
	return context->Pcf()->PcrCreate(
		context->Pmda()->PtMDType<gpmd::IMDTypeBool>(), default_type_modifier);
}

BOOL
CDSLModel::FRecordSubqueryMarker(const CDSLConstraint *constraint, CColRef *marker)
{
	GPOS_ASSERT(nullptr != constraint && nullptr != marker);
	CColRef *existing = PcrSubqueryMarker(constraint);
	if (nullptr != existing) return existing == marker;
	if (nullptr == m_phmSubqueryMarkers)
		m_phmSubqueryMarkers = GPOS_NEW(m_mp) ConstraintToMarkerMap(m_mp);
	return m_phmSubqueryMarkers->Insert(const_cast<CDSLConstraint *>(constraint), marker);
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

CExpression *
CDSLModel::PexprExpr(const CDSLSymbol *psym) const
{
	GPOS_ASSERT(EdslsymExpr == psym->Esymkind());
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

COrderSpecArray *
CDSLModel::PdrgposOrder(const CDSLSymbol *psym) const
{
	GPOS_ASSERT(EdslsymOrder == psym->Esymkind());
	return static_cast<COrderSpecArray *>(PvalLookup(psym));
}

CExpression *
CDSLModel::PexprWindow(const CDSLSymbol *psym) const
{
	GPOS_ASSERT(EdslsymWindow == psym->Esymkind());
	return dynamic_cast<CExpression *>(PvalLookup(psym));
}

CColRefArray *
CDSLModel::PdrgpcrRank(const CDSLSymbol *psym) const
{
	GPOS_ASSERT(EdslsymRank == psym->Esymkind());
	return static_cast<CColRefArray *>(PvalLookup(psym));
}

CWindowFrameArray *
CDSLModel::PdrgpwfFrame(const CDSLSymbol *psym) const
{
	GPOS_ASSERT(EdslsymFrame == psym->Esymkind());
	return static_cast<CWindowFrameArray *>(PvalLookup(psym));
}

BOOL
CDSLModel::FSetWindowCarrier(const CDSLSymbol *psymWindow,
							 CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != psymWindow);
	GPOS_ASSERT(EdslsymWindow == psymWindow->Esymkind());
	GPOS_ASSERT(nullptr != pexpr);
	if (nullptr == m_phmWindowCarrier)
	{
		m_phmWindowCarrier = GPOS_NEW(m_mp) CDSLSymbolToExpressionMap(m_mp);
	}
	CExpression *pexprExisting = m_phmWindowCarrier->Find(psymWindow);
	if (nullptr != pexprExisting)
	{
		BOOL fCompatible = pexprExisting == pexpr;
		pexpr->Release();
		return fCompatible;
	}
	BOOL fInserted = m_phmWindowCarrier->Insert(
		const_cast<CDSLSymbol *>(psymWindow), pexpr);
	GPOS_ASSERT(fInserted);
	return fInserted;
}

CExpression *
CDSLModel::PexprWindowCarrier(const CDSLSymbol *psymWindow) const
{
	GPOS_ASSERT(nullptr != psymWindow);
	GPOS_ASSERT(EdslsymWindow == psymWindow->Esymkind());
	return nullptr == m_phmWindowCarrier ? nullptr : m_phmWindowCarrier->Find(psymWindow);
}

// EOF
