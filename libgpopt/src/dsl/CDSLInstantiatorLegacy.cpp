//---------------------------------------------------------------------------
// MONSOON DSL rule engine — CDSLInstantiatorLegacy.cpp
// Constraint-defined expression construction retained for unmigrated rule libraries.
// Calls back into the shared resolvers for operands. Remove only after those constructors
// and their rule/test consumers are migrated; relational compatibility paths still live
// beside their operator builders.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLColumnDomain.h"
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpos/common/CAutoRef.h"
#include "gpopt/dsl/CDSLExpressionDefinitions.h"
#include "CDSLInstantiatorUtils.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLExprListUtils.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "naucrates/traceflags/traceflags.h"

using namespace gpopt;
using namespace gpopt::dslinstantiator;

namespace
{
void
TracePredicateDomainSplitFailure(const CHAR *szReason, ULONG ulConjuncts = 0,
								 ULONG ulResidual = 0,
								 ULONG ulExternal = 0,
								 ULONG ulExternalOuter = 0)
{
	if (GPOS_FTRACE(EopttracePrintDSLRule))
	{
		GPOS_TRACE_FORMAT(
			"DSL_CONSTRAINT_TRACE kind=PredicateDomainSplit stage=instantiate "
			"status=rejected reason=%s conjuncts=%d residual=%d external=%d "
			"external_outer=%d",
			szReason, ulConjuncts, ulResidual, ulExternal, ulExternalOuter);
	}
}
}  // namespace

namespace gpopt
{
namespace dslinstantiator
{
// Find the only source InSub node whose predicate was bound by the matcher.
// A direct target-to-source AttrsEq lookup is preferred by the caller; this
// conservative fallback supports proven rewrites which move IN across an
// equality join and therefore name the other join-key attrs on the target.
const CDSLOp *
PopOnlyBoundInSub(const CDSLOp *pop, const CDSLModel *pmodel,
				  ULONG *pulMatches)
{
	const CDSLOp *popFound = nullptr;
	if (EdslopInSubFilter == pop->Edslop() && nullptr != pop->Pdrgpsym() &&
		(1 == pop->Pdrgpsym()->Size() || 5 == pop->Pdrgpsym()->Size()) &&
		nullptr != pmodel->PexprInSubPred((*pop->Pdrgpsym())[0]))
	{
		(*pulMatches)++;
		popFound = pop;
	}
	for (ULONG ul = 0; ul < pop->UlChildren(); ul++)
	{
		const CDSLOp *popChild =
			PopOnlyBoundInSub((*pop)[ul], pmodel, pulMatches);
		if (nullptr != popChild)
		{
			popFound = popChild;
		}
	}
	return popFound;
}
}  // namespace dslinstantiator
}  // namespace gpopt

BOOL
CDSLInstantiator::FMaterializePredicateDomainSplit(
	const CDSLConstraint *pcon, const CDSLModel *pmodel, ULONG ulDepth) const
{
	if (nullptr == pcon || 9 != pcon->Pdrgpsym()->Size() ||
		nullptr == m_prule || ulDepth > m_prule->Pdrgpcon()->Size())
	{
		TracePredicateDomainSplitFailure("invalid_arguments");
		return false;
	}
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	const CDSLSymbol *psymResidual = PsymResolve((*pdrgpsym)[1]);
	const CDSLSymbol *psymExternal = PsymResolve((*pdrgpsym)[2]);
	const CDSLSymbol *psymResidualOuter = PsymResolve((*pdrgpsym)[3]);
	const CDSLSymbol *psymResidualInner = PsymResolve((*pdrgpsym)[4]);
	const CDSLSymbol *psymExternalLocal = PsymResolve((*pdrgpsym)[5]);
	const CDSLSymbol *psymExternalOuter = PsymResolve((*pdrgpsym)[6]);
	const BOOL fAllCached =
		nullptr != m_phmDerivedPreds->Find(psymResidual) &&
		nullptr != m_phmDerivedPreds->Find(psymExternal) &&
		nullptr != m_phmDerivedCols->Find(psymResidualOuter) &&
		nullptr != m_phmDerivedCols->Find(psymResidualInner) &&
		nullptr != m_phmDerivedCols->Find(psymExternalLocal) &&
		nullptr != m_phmDerivedCols->Find(psymExternalOuter);
	if (fAllCached)
	{
		return true;
	}
	if (nullptr != m_phmDerivedPreds->Find(psymResidual) ||
		nullptr != m_phmDerivedPreds->Find(psymExternal) ||
		nullptr != m_phmDerivedCols->Find(psymResidualOuter) ||
		nullptr != m_phmDerivedCols->Find(psymResidualInner) ||
		nullptr != m_phmDerivedCols->Find(psymExternalLocal) ||
		nullptr != m_phmDerivedCols->Find(psymExternalOuter))
	{
		TracePredicateDomainSplitFailure("partial_cached_outputs");
		return false;
	}

	CExpression *pexprSource = PexprResolvePredicate(
		(*pdrgpsym)[0], pmodel, ulDepth + 1);
	const auto domain = [&](ULONG slot) {
		const auto *symbol = PsymResolve((*pdrgpsym)[slot]);
		CRefCount *binding = pmodel->PvalLookup(symbol);
		if (nullptr == binding && EdslsymTable != symbol->Esymkind())
			binding = PdrgpcrResolveCols(symbol, pmodel, ulDepth + 1);
		return CDSLColumnDomain::PcrsBinding(m_mp, binding);
	};
	CAutoRef<CColRefSet> outer(domain(7)), inner(domain(8));
	CColRefSet *pcrsOuter = outer.Value();
	CColRefSet *pcrsInner = inner.Value();
	if (nullptr == pexprSource || nullptr == pcrsOuter || nullptr == pcrsInner)
	{
		TracePredicateDomainSplitFailure("unbound_input");
		CRefCount::SafeRelease(pexprSource);
		return false;
	}

	if (!pcrsOuter->IsDisjoint(pcrsInner))
	{
		TracePredicateDomainSplitFailure("overlapping_table_domains");
		pexprSource->Release();
		return false;
	}
	CColRefSet *pcrsChildren = GPOS_NEW(m_mp) CColRefSet(m_mp, *pcrsOuter);
	pcrsChildren->Union(pcrsInner);
	CColRefSet *pcrsResidualOuter = GPOS_NEW(m_mp) CColRefSet(m_mp);
	CColRefSet *pcrsResidualInner = GPOS_NEW(m_mp) CColRefSet(m_mp);
	CColRefSet *pcrsExternalLocal = GPOS_NEW(m_mp) CColRefSet(m_mp);
	CColRefSet *pcrsExternalOuter = GPOS_NEW(m_mp) CColRefSet(m_mp);
	CExpressionArray *pdrgpexprResidual = GPOS_NEW(m_mp) CExpressionArray(m_mp);
	CExpressionArray *pdrgpexprExternal = GPOS_NEW(m_mp) CExpressionArray(m_mp);
	CExpressionArray *pdrgpexprConjuncts =
		CPredicateUtils::PdrgpexprConjuncts(m_mp, pexprSource);
	const ULONG ulConjuncts = pdrgpexprConjuncts->Size();
	BOOL fValid = true;
	for (ULONG ul = 0; fValid && ul < pdrgpexprConjuncts->Size(); ul++)
	{
		CExpression *pexprConjunct = (*pdrgpexprConjuncts)[ul];
		CColRefSet *pcrsUsed = GPOS_NEW(m_mp)
			CColRefSet(m_mp, *pexprConjunct->DeriveUsedColumns());
		// Anything wholly owned by the two current inputs remains a residual,
		// including a predicate local to only one side. Check this before the
		// external-domain case: a one-sided local predicate is necessarily
		// disjoint from the other side, but it is not a correlation.
		if (pcrsChildren->ContainsAll(pcrsUsed))
		{
			pexprConjunct->AddRef();
			pdrgpexprResidual->Append(pexprConjunct);
			CColRefSet *pcrsCurrent =
				GPOS_NEW(m_mp) CColRefSet(m_mp, *pcrsUsed);
			pcrsCurrent->Intersection(pcrsOuter);
			pcrsResidualOuter->Union(pcrsCurrent);
			pcrsCurrent->Release();
			pcrsCurrent = GPOS_NEW(m_mp) CColRefSet(m_mp, *pcrsUsed);
			pcrsCurrent->Intersection(pcrsInner);
			pcrsResidualInner->Union(pcrsCurrent);
			pcrsCurrent->Release();
		}
		else if (pcrsUsed->IsDisjoint(pcrsOuter) ||
				 pcrsUsed->IsDisjoint(pcrsInner))
		{
			pexprConjunct->AddRef();
			pdrgpexprExternal->Append(pexprConjunct);
			CColRefSet *pcrsLocal =
				GPOS_NEW(m_mp) CColRefSet(m_mp, *pcrsUsed);
			pcrsLocal->Intersection(pcrsChildren);
			pcrsExternalLocal->Union(pcrsLocal);
			pcrsLocal->Release();
			pcrsUsed->Difference(pcrsChildren);
			pcrsExternalOuter->Union(pcrsUsed);
		}
		else
		{
			fValid = false;
		}
		pcrsUsed->Release();
	}
	pdrgpexprConjuncts->Release();
	pexprSource->Release();
	fValid = fValid && 0 < pdrgpexprExternal->Size() &&
		0 < pcrsExternalOuter->Size();
	if (!fValid)
	{
		TracePredicateDomainSplitFailure(
			0 == pdrgpexprExternal->Size()
				? "missing_external_conjunct"
				: (0 == pcrsExternalOuter->Size()
					   ? "missing_external_reference"
					   : "mixed_domain_conjunct"),
			ulConjuncts, pdrgpexprResidual->Size(),
			pdrgpexprExternal->Size(), pcrsExternalOuter->Size());
		pdrgpexprResidual->Release();
		pdrgpexprExternal->Release();
		pcrsResidualOuter->Release();
		pcrsResidualInner->Release();
		pcrsExternalLocal->Release();
		pcrsExternalOuter->Release();
		pcrsChildren->Release();
		return false;
	}
	pcrsChildren->Release();

	CExpression *pexprResidual =
		CPredicateUtils::PexprConjunction(m_mp, pdrgpexprResidual);
	CExpression *pexprExternal =
		CPredicateUtils::PexprConjunction(m_mp, pdrgpexprExternal);
	CColRefArray *pdrgpcrResidualOuter = pcrsResidualOuter->Pdrgpcr(m_mp);
	CColRefArray *pdrgpcrResidualInner = pcrsResidualInner->Pdrgpcr(m_mp);
	CColRefArray *pdrgpcrExternalLocal = pcrsExternalLocal->Pdrgpcr(m_mp);
	CColRefArray *pdrgpcrExternalOuter = pcrsExternalOuter->Pdrgpcr(m_mp);
	pcrsResidualOuter->Release();
	pcrsResidualInner->Release();
	pcrsExternalLocal->Release();
	pcrsExternalOuter->Release();

	{
		BOOL fInserted GPOS_ASSERTS_ONLY = m_phmDerivedPreds->Insert(
			const_cast<CDSLSymbol *>(psymResidual), pexprResidual);
		GPOS_ASSERT(fInserted);
	}
	{
		BOOL fInserted GPOS_ASSERTS_ONLY = m_phmDerivedPreds->Insert(
			const_cast<CDSLSymbol *>(psymExternal), pexprExternal);
		GPOS_ASSERT(fInserted);
	}
	{
		BOOL fInserted GPOS_ASSERTS_ONLY = m_phmDerivedCols->Insert(
			const_cast<CDSLSymbol *>(psymResidualOuter),
			pdrgpcrResidualOuter);
		GPOS_ASSERT(fInserted);
	}
	{
		BOOL fInserted GPOS_ASSERTS_ONLY = m_phmDerivedCols->Insert(
			const_cast<CDSLSymbol *>(psymResidualInner),
			pdrgpcrResidualInner);
		GPOS_ASSERT(fInserted);
	}
	{
		BOOL fInserted GPOS_ASSERTS_ONLY = m_phmDerivedCols->Insert(
			const_cast<CDSLSymbol *>(psymExternalLocal), pdrgpcrExternalLocal);
		GPOS_ASSERT(fInserted);
	}
	{
		BOOL fInserted GPOS_ASSERTS_ONLY = m_phmDerivedCols->Insert(
			const_cast<CDSLSymbol *>(psymExternalOuter), pdrgpcrExternalOuter);
		GPOS_ASSERT(fInserted);
	}
	return true;
}

CExpression *
CDSLInstantiator::PexprResolveLegacyPredicate(const CDSLSymbol *psym, const CDSLModel *pmodel, ULONG ulDepth) const
{
	const auto *pdef = m_prule->Pexprdefs()->Pdef(psym);
	const CDSLConstraint *pconSplit = nullptr;
	CDSLConstraintArray *pdrgpcon = m_prule->Pdrgpcon();
	for (ULONG ul = 0; ul < pdrgpcon->Size(); ul++)
	{
		const CDSLConstraint *pcon = (*pdrgpcon)[ul];
		if (EdslconPredicateDomainSplit == pcon->Edslcon() &&
			9 == pcon->Pdrgpsym()->Size() &&
			((*pcon->Pdrgpsym())[1] == psym ||
			 (*pcon->Pdrgpsym())[2] == psym))
		{
			if (nullptr != pconSplit)
			{
				return nullptr;
			}
			pconSplit = pcon;
			continue;
		}
	}
	if (nullptr != pconSplit)
	{
		if (nullptr != pdef ||
			!FMaterializePredicateDomainSplit(pconSplit, pmodel, ulDepth + 1))
		{
			return nullptr;
		}
		CExpression *pexprSplit = m_phmDerivedPreds->Find(psym);
		if (nullptr != pexprSplit)
		{
			pexprSplit->AddRef();
		}
		return pexprSplit;
	}
	if (nullptr != pdef && EdslexprNullSafeEq == pdef->Edslexpr())
	{
		CColRefArray *pdrgpcrLeft =
			PdrgpcrResolveCols(pdef->PsymOperand(0), pmodel);
		CColRefArray *pdrgpcrRight =
			PdrgpcrResolveCols(pdef->PsymOperand(1), pmodel);
		if (!CDSLMatchView::FNullSafeEqComparableColumns(pdrgpcrLeft, pdrgpcrRight))
		{
			return nullptr;
		}
		return CPredicateUtils::PexprINDFConjunction(
			m_mp, pdrgpcrLeft, pdrgpcrRight);
	}
	if (nullptr != pdef &&
		(EdslexprNotTrue == pdef->Edslexpr() || EdslexprNot == pdef->Edslexpr()))
	{
		CExpression *pexprInput = PexprResolvePredicate(
			pdef->PsymOperand(0), pmodel, ulDepth + 1);
		if (nullptr == pexprInput)
		{
			return nullptr;
		}
		return PexprBuildNegation(pexprInput, EdslexprNotTrue == pdef->Edslexpr());
	}
	if (nullptr == pdef || EdslexprAnd != pdef->Edslexpr())
	{
		return nullptr;
	}

	CExpression *pexprLeft = PexprResolvePredicate(
		pdef->PsymOperand(0), pmodel, ulDepth + 1);
	CExpression *pexprRight = PexprResolvePredicate(
		pdef->PsymOperand(1), pmodel, ulDepth + 1);
	if (nullptr == pexprLeft || nullptr == pexprRight)
	{
		CRefCount::SafeRelease(pexprLeft);
		CRefCount::SafeRelease(pexprRight);
		return nullptr;
	}
	CExpression *pexprResult =
		CPredicateUtils::PexprConjunction(m_mp, pexprLeft, pexprRight);
	pexprLeft->Release();
	pexprRight->Release();
	return pexprResult;
}

CColRefArray *
CDSLInstantiator::PdrgpcrFromLegacyEquality(const CDSLSymbol *psym,
	const CDSLModel *pmodel, ULONG ulDepth) const
{
	CDSLConstraintArray *pdrgpcon = m_prule->Pdrgpcon();
	// An equality may name another target-side value that is itself derived
	// (for example, a Union input row equal to its derived output row).
	// Source aliases are handled by BuildAliasMap; follow only the remaining
	// target equality here, with the existing depth bound preventing cycles.
	const EDslConstraintKind edslconEq =
		EdslsymAttrs == psym->Esymkind() ? EdslconAttrsEq
										  : EdslconSchemaEq;
	for (ULONG ul = 0; ul < pdrgpcon->Size(); ul++)
	{
		const CDSLConstraint *pcon = (*pdrgpcon)[ul];
		if (edslconEq != pcon->Edslcon() ||
			2 != pcon->Pdrgpsym()->Size())
		{
			continue;
		}
		for (ULONG side = 0; side < 2; side++)
		{
			if ((*pcon->Pdrgpsym())[side] == psym)
			{
				CColRefArray *pdrgpcrPeer = PdrgpcrResolveCols(
					(*pcon->Pdrgpsym())[1 - side], pmodel, ulDepth + 1);
				if (nullptr != pdrgpcrPeer)
				{
					return pdrgpcrPeer;
				}
			}
		}
	}
	return nullptr;
}

CExpression *
CDSLInstantiator::PexprResolveLegacyExpr(const CDSLSymbol *psym, const CDSLModel *pmodel, ULONG ulDepth) const
{
	const CDSLConstraint *pconDef = nullptr;
	ULONG ulDefOutput = 0;
	CDSLConstraintArray *pdrgpcon = m_prule->Pdrgpcon();
	for (ULONG ul = 0; ul < pdrgpcon->Size(); ul++)
	{
		const CDSLConstraint *pcon = (*pdrgpcon)[ul];
		ULONG ulOutput = gpos::ulong_max;
		if (EdslconExprConcat == pcon->Edslcon() &&
			3 == pcon->Pdrgpsym()->Size() &&
			(*pcon->Pdrgpsym())[0] == psym)
		{
			ulOutput = 0;
		}
		else if (EdslconExprNulls == pcon->Edslcon() &&
				 3 == pcon->Pdrgpsym()->Size() &&
				 (*pcon->Pdrgpsym())[0] == psym)
		{
			ulOutput = 0;
		}
		else if (EdslconExprSplit == pcon->Edslcon() &&
				 4 == pcon->Pdrgpsym()->Size())
		{
			if ((*pcon->Pdrgpsym())[0] == psym)
			{
				ulOutput = 0;
			}
			else if ((*pcon->Pdrgpsym())[1] == psym)
			{
				ulOutput = 1;
			}
		}
		if (gpos::ulong_max == ulOutput)
		{
			continue;
		}
		if (nullptr != pconDef)
		{
			return nullptr;  // ambiguous derived definition
		}
		pconDef = pcon;
		ulDefOutput = ulOutput;
	}
	if (nullptr == pconDef)
	{
		return nullptr;
	}
	if (EdslconExprNulls == pconDef->Edslcon())
	{
		CColRefArray *pdrgpcrTemplate = PdrgpcrResolveCols(
			(*pconDef->Pdrgpsym())[1], pmodel, ulDepth + 1);
		if (nullptr == pdrgpcrTemplate)
		{
			return nullptr;
		}
		CExpression *pexprResult = CDSLExprListUtils::PexprNulls(m_mp, pdrgpcrTemplate);
		CColRefArray *pdrgpcrOutput = CDSLExprListUtils::PdrgpcrOutput(m_mp, pexprResult);
		const CDSLSymbol *psymOutputAttrs = (*pconDef->Pdrgpsym())[2];
		if (!m_phmDerivedCols->Insert(
				const_cast<CDSLSymbol *>(psymOutputAttrs), pdrgpcrOutput))
		{
			pdrgpcrOutput->Release();
			pexprResult->Release();
			return nullptr;
		}
		return pexprResult;
	}

	if (EdslconExprSplit == pconDef->Edslcon())
	{
		CExpression *pexprUpper = PexprResolveExpr(
			(*pconDef->Pdrgpsym())[2], pmodel, ulDepth + 1);
		CExpression *pexprLower = PexprResolveExpr(
			(*pconDef->Pdrgpsym())[3], pmodel, ulDepth + 1);
		CExpression *pexprMerged = nullptr;
		CExpression *pexprResidual = nullptr;
		const BOOL fSplit = CDSLExprListUtils::FSplit(
			m_mp, pexprUpper, pexprLower, &pexprMerged, &pexprResidual);
		CRefCount::SafeRelease(pexprUpper);
		CRefCount::SafeRelease(pexprLower);
		if (!fSplit)
		{
			return nullptr;
		}
		CExpression *pexprResult =
			0 == ulDefOutput ? pexprMerged : pexprResidual;
		(0 == ulDefOutput ? pexprResidual : pexprMerged)->Release();
		return pexprResult;
	}

	CExpression *pexprLeft = PexprResolveExpr(
		(*pconDef->Pdrgpsym())[1], pmodel, ulDepth + 1);
	CExpression *pexprRight = PexprResolveExpr(
		(*pconDef->Pdrgpsym())[2], pmodel, ulDepth + 1);
	CExpression *pexprResult = CDSLExprListUtils::PexprConcat(
		m_mp, pexprLeft, pexprRight);
	CRefCount::SafeRelease(pexprLeft);
	CRefCount::SafeRelease(pexprRight);
	return pexprResult;
}
