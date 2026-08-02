//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLJoinMatcher.cpp
//
//	@doc:
//		Implementation of the join symbol binder (see CDSLJoinMatcher.h). Migrates
//		the SEMANTICS of WeTune's join match: bind the equi-join key columns to the
//		two <a> symbols, keep non-equi conjuncts as residual, recurse both children.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLJoinMatcher.h"

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarIdent.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinMatcher::FSplitPredicate
//---------------------------------------------------------------------------
BOOL
CDSLJoinMatcher::FSplitPredicate(CExpression *pexprPred,
								 CExpression *pexprLeftRel,
								 CColRefArray *pdrgpcrLeft,
								 CColRefArray *pdrgpcrRight,
								 CExpressionArray *pdrgpexprResidual) const
{
	CColRefSet *pcrsLeft = pexprLeftRel->DeriveOutputColumns();

	CExpressionArray *pdrgpexprConj =
		CPredicateUtils::PdrgpexprConjuncts(m_mp, pexprPred);
	const ULONG ulConj = pdrgpexprConj->Size();
	BOOL fOk = true;
	for (ULONG ul = 0; ul < ulConj && fOk; ul++)
	{
		CExpression *pexprConj = (*pdrgpexprConj)[ul];

		// FPlainEquality does unchecked operator[]; a non-materialized conjunct
		// (arity 0) would abort. With CPatternTree the predicate tree is fully
		// materialized, but guard defensively anyway.
		if (2 != pexprConj->Arity() ||
			!CPredicateUtils::FPlainEquality(pexprConj))
		{
			// non-equi conjunct: preserve as residual (AddRef — the array owns it).
			pexprConj->AddRef();
			pdrgpexprResidual->Append(pexprConj);
			continue;
		}

		// plain equality over two CScalarIdents: recover both columns.
		CColRef *pcr0 = const_cast<CColRef *>(
			CScalarIdent::PopConvert((*pexprConj)[0]->Pop())->Pcr());
		CColRef *pcr1 = const_cast<CColRef *>(
			CScalarIdent::PopConvert((*pexprConj)[1]->Pop())->Pcr());

		// orient by which column belongs to the left relation's output.
		BOOL f0Left = pcrsLeft->FMember(pcr0);
		BOOL f1Left = pcrsLeft->FMember(pcr1);
		if (f0Left && !f1Left)
		{
			pdrgpcrLeft->Append(pcr0);
			pdrgpcrRight->Append(pcr1);
		}
		else if (f1Left && !f0Left)
		{
			pdrgpcrLeft->Append(pcr1);
			pdrgpcrRight->Append(pcr0);
		}
		else
		{
			// both or neither on the left: not a cross-child join key we can
			// orient (e.g. a same-side equality). Keep it as residual so nothing
			// is dropped, but it does not contribute to <a>/<a> key binding.
			pexprConj->AddRef();
			pdrgpexprResidual->Append(pexprConj);
		}
	}
	pdrgpexprConj->Release();
	return fOk;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLJoinMatcher::FMatch
//---------------------------------------------------------------------------
BOOL
CDSLJoinMatcher::FMatch(const CDSLOp *popJoin, CExpression *pexprJoin,
						CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != popJoin);
	GPOS_ASSERT(EdslopInnerJoin == popJoin->Edslop() ||
				EdslopLeftJoin == popJoin->Edslop());
	GPOS_ASSERT(nullptr != pexprJoin);

	// identity + arity gate: the live node must be the matching join carrying
	// (left rel, right rel, scalar predicate).
	const COperator::EOperatorId eopid = pexprJoin->Pop()->Eopid();
	const BOOL fInner = (EdslopInnerJoin == popJoin->Edslop());
	const COperator::EOperatorId eopidExpected =
		fInner ? COperator::EopLogicalInnerJoin
			   : COperator::EopLogicalLeftOuterJoin;
	if (eopid != eopidExpected || 3 != pexprJoin->Arity())
	{
		return false;
	}

	// join schema is <a a> — left keys then right keys (validated at parse).
	CDSLSymbolArray *pdrgpsym = popJoin->Pdrgpsym();
	if (nullptr == pdrgpsym || 2 != pdrgpsym->Size() ||
		2 != popJoin->UlChildren())
	{
		return false;
	}
	const CDSLSymbol *psymLeft = (*pdrgpsym)[0];
	const CDSLSymbol *psymRight = (*pdrgpsym)[1];

	// split the predicate into left/right equi-key columns + residual.
	CColRefArray *pdrgpcrLeft = GPOS_NEW(m_mp) CColRefArray(m_mp);
	CColRefArray *pdrgpcrRight = GPOS_NEW(m_mp) CColRefArray(m_mp);
	CExpressionArray *pdrgpexprResidual = GPOS_NEW(m_mp) CExpressionArray(m_mp);

	if (!FSplitPredicate((*pexprJoin)[2], (*pexprJoin)[0], pdrgpcrLeft,
						 pdrgpcrRight, pdrgpexprResidual))
	{
		pdrgpcrLeft->Release();
		pdrgpcrRight->Release();
		pdrgpexprResidual->Release();
		return false;
	}

	// bind the two <a> symbols (FBind AddRefs; release our local refs after).
	BOOL fBound = pmodel->FBind(psymLeft, pdrgpcrLeft) &&
				  pmodel->FBind(psymRight, pdrgpcrRight);
	pdrgpcrLeft->Release();
	pdrgpcrRight->Release();
	if (!fBound)
	{
		pdrgpexprResidual->Release();
		return false;
	}

	// recurse both relational children through the generic matcher.
	if (!m_pmatcher->FMatch((*popJoin)[0], (*pexprJoin)[0], pmodel) ||
		!m_pmatcher->FMatch((*popJoin)[1], (*pexprJoin)[1], pmodel))
	{
		pdrgpexprResidual->Release();
		return false;
	}

	// record residual (non-equi / un-orientable) conjuncts (ownership transferred).
	pmodel->SetResidualConjuncts(pdrgpexprResidual);

	// record the whole predicate subtree so the instantiator grafts the exact
	// equi + non-equi predicate back (AddRef — the model keeps its own ref).
	CExpression *pexprPred = (*pexprJoin)[2];
	pexprPred->AddRef();
	pmodel->SetJoinPred(pexprPred);
	return true;
}

// EOF
