//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLInstantiator.cpp
//
//	@doc:
//		Implementation of the target builder (see CDSLInstantiator.h). Migrates
//		the SEMANTICS of WeTune Instantiation for structural operators.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLInstantiator.h"

#include "gpos/base.h"

#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CPredicateUtils.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::CDSLInstantiator
//---------------------------------------------------------------------------
CDSLInstantiator::CDSLInstantiator(CMemoryPool *mp) : m_mp(mp), m_phmAlias(nullptr)
{
	GPOS_ASSERT(nullptr != mp);
	m_phmAlias = GPOS_NEW(mp) CDSLSymbolAliasMap(mp);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::~CDSLInstantiator
//---------------------------------------------------------------------------
CDSLInstantiator::~CDSLInstantiator()
{
	m_phmAlias->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::BuildAliasMap
//
//	@doc:
//		From each equality constraint *Eq(x,y), link the target-side symbol to the
//		source-side symbol whose binding it reuses. Both symbols share a kind
//		(the DSL guarantees *Eq relates same-kind symbols), so we only need the
//		side to orient the alias.
//---------------------------------------------------------------------------
void
CDSLInstantiator::BuildAliasMap(const CDSLRule *prule)
{
	CDSLConstraintArray *pdrgpcon = prule->Pdrgpcon();
	const ULONG ulCon = pdrgpcon->Size();
	for (ULONG ul = 0; ul < ulCon; ul++)
	{
		const CDSLConstraint *pcon = (*pdrgpcon)[ul];
		switch (pcon->Edslcon())
		{
			case EdslconTableEq:
			case EdslconAttrsEq:
			case EdslconPredicateEq:
			case EdslconSchemaEq:
			case EdslconFuncEq:
			case EdslconScalarEq:
				break;	// an aliasing equality
			default:
				continue;  // structural constraint: not an alias
		}

		CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
		if (2 != pdrgpsym->Size())
		{
			continue;
		}
		CDSLSymbol *psym0 = (*pdrgpsym)[0];
		CDSLSymbol *psym1 = (*pdrgpsym)[1];

		// orient: target-side symbol aliases the source-side symbol.
		CDSLSymbol *psymTgt = nullptr;
		CDSLSymbol *psymSrc = nullptr;
		if (EdslsideTarget == psym0->Eside() &&
			EdslsideSource == psym1->Eside())
		{
			psymTgt = psym0;
			psymSrc = psym1;
		}
		else if (EdslsideSource == psym0->Eside() &&
				 EdslsideTarget == psym1->Eside())
		{
			psymTgt = psym1;
			psymSrc = psym0;
		}
		else
		{
			// both same side (e.g. two source symbols in one class): no target
			// alias to record here.
			continue;
		}

		// first alias wins (a target symbol may appear in several *Eq; any source
		// representative of its class is fine since they are all co-bound).
		if (nullptr == m_phmAlias->Find(psymTgt))
		{
			BOOL fOk = m_phmAlias->Insert(psymTgt, psymSrc);
			GPOS_ASSERT(fOk);
			(void) fOk;
		}
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PsymResolve
//---------------------------------------------------------------------------
const CDSLSymbol *
CDSLInstantiator::PsymResolve(const CDSLSymbol *psym) const
{
	CDSLSymbol *psymSrc = m_phmAlias->Find(psym);
	return (nullptr != psymSrc) ? psymSrc : psym;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildInput
//
//	@doc:
//		Input<t>: reuse the relational subtree bound to t (resolved through the
//		alias map). AddRef-graft it into the target.
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildInput(const CDSLOp *pop,
								  const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	if (nullptr == pdrgpsym || 1 != pdrgpsym->Size())
	{
		return nullptr;
	}
	const CDSLSymbol *psymTable = PsymResolve((*pdrgpsym)[0]);
	CExpression *pexpr = pmodel->PexprTable(psymTable);
	if (nullptr == pexpr)
	{
		return nullptr;
	}
	pexpr->AddRef();
	return pexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildFilter
//
//	@doc:
//		Filter<p a>: Select(child, predicate). The predicate is the conjunct
//		bound to p (resolved through the alias map) CONJOINED with every residual
//		conjunct the matcher preserved, so no predicate is dropped.
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildFilter(const CDSLOp *pop,
								   const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	if (nullptr == pdrgpsym || 2 != pdrgpsym->Size() || 1 != pop->UlChildren())
	{
		return nullptr;
	}
	const CDSLSymbol *psymPred = PsymResolve((*pdrgpsym)[0]);

	CExpression *pexprChild = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprChild)
	{
		return nullptr;
	}

	CExpression *pexprPredBound = pmodel->PexprPred(psymPred);
	if (nullptr == pexprPredBound)
	{
		pexprChild->Release();
		return nullptr;
	}

	// collect the target predicate's conjuncts: the bound conjunct + residuals.
	CExpressionArray *pdrgpexpr = GPOS_NEW(m_mp) CExpressionArray(m_mp);
	pexprPredBound->AddRef();
	pdrgpexpr->Append(pexprPredBound);

	CExpressionArray *pdrgpexprResidual = pmodel->PdrgpexprResidual();
	if (nullptr != pdrgpexprResidual)
	{
		const ULONG ulResidual = pdrgpexprResidual->Size();
		for (ULONG ul = 0; ul < ulResidual; ul++)
		{
			CExpression *pexprR = (*pdrgpexprResidual)[ul];
			pexprR->AddRef();
			pdrgpexpr->Append(pexprR);
		}
	}

	// build one conjunctive predicate (single conjunct -> itself; many -> And).
	CExpression *pexprPred = CPredicateUtils::PexprConjunction(m_mp, pdrgpexpr);

	return GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprChild, pexprPred);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildJoin
//
//	@doc:
//		InnerJoin/LeftJoin: rebuild both children and attach the join predicate.
//		The generic matcher (#24) matches a join's two relational children but
//		does not yet bind its scalar predicate / keys, so a structural join
//		rule cannot recover the predicate here. Until join-key binding lands, a
//		join target is not instantiable — return NULL (the rule simply does not
//		fire), which is safe. Children are still built to validate the recursion.
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildJoin(const CDSLOp *pop,
								 const CDSLModel *pmodel) const
{
	if (2 != pop->UlChildren())
	{
		return nullptr;
	}
	CExpression *pexprLeft = PexprBuild((*pop)[0], pmodel);
	CExpression *pexprRight =
		(nullptr != pexprLeft) ? PexprBuild((*pop)[1], pmodel) : nullptr;

	// join predicate / keys not yet recovered by the matcher — see doc.
	CRefCount::SafeRelease(pexprLeft);
	CRefCount::SafeRelease(pexprRight);
	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuild
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuild(const CDSLOp *pop, const CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != pop);

	switch (pop->Edslop())
	{
		case EdslopInput:
			return PexprBuildInput(pop, pmodel);
		case EdslopFilter:
			return PexprBuildFilter(pop, pmodel);
		case EdslopInnerJoin:
		case EdslopLeftJoin:
			return PexprBuildJoin(pop, pmodel);
		default:
			// Proj / Agg / Union / Sort / Limit: not yet instantiable (future
			// work). The rule does not fire.
			return nullptr;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprInstantiate
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprInstantiate(const CDSLRule *prule,
								   const CDSLModel *pmodel)
{
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(nullptr != pmodel);

	BuildAliasMap(prule);
	return PexprBuild(prule->PfragTgt()->PopRoot(), pmodel);
}

// EOF
