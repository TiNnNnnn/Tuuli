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

#include <vector>

#include "gpos/base.h"

#include "gpopt/base/CCastUtils.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarIdent.h"

using namespace gpopt;

namespace
{
CColRef *
PcrJoinKeyOperand(CExpression *pexpr)
{
	if (COperator::EopScalarIdent == pexpr->Pop()->Eopid())
	{
		return const_cast<CColRef *>(
			CScalarIdent::PopConvert(pexpr->Pop())->Pcr());
	}
	if (CCastUtils::FBinaryCoercibleCastedScId(pexpr))
	{
		return const_cast<CColRef *>(
			CScalarIdent::PopConvert((*pexpr)[0]->Pop())->Pcr());
	}
	return nullptr;
}

// Return the already-bound source attrs connected to psymJoin by a direct
// AttrsEq declaration. Target symbols and not-yet-bound source symbols do not
// constrain this match. Multiple declarations are accepted only when they
// agree on the same ordered column vector.
const CColRefArray *
PdrgpcrEqualityPeer(const CDSLRule *prule, const CDSLSymbol *psymJoin,
					const CDSLModel *pmodel)
{
	if (nullptr == prule)
	{
		return nullptr;
	}

	const CColRefArray *pdrgpcrExpected = nullptr;
	CDSLConstraintArray *pdrgpcon = prule->Pdrgpcon();
	for (ULONG ul = 0; ul < pdrgpcon->Size(); ul++)
	{
		const CDSLConstraint *pcon = (*pdrgpcon)[ul];
		if (EdslconAttrsEq != pcon->Edslcon() ||
			2 != pcon->Pdrgpsym()->Size())
		{
			continue;
		}
		const CDSLSymbol *psymFirst = (*pcon->Pdrgpsym())[0];
		const CDSLSymbol *psymSecond = (*pcon->Pdrgpsym())[1];
		const CDSLSymbol *psymPeer = nullptr;
		if (psymJoin == psymFirst)
		{
			psymPeer = psymSecond;
		}
		else if (psymJoin == psymSecond)
		{
			psymPeer = psymFirst;
		}
		if (nullptr == psymPeer || EdslsideSource != psymPeer->Eside())
		{
			continue;
		}

		CColRefArray *pdrgpcrPeer = pmodel->PdrgpcrAttrs(psymPeer);
		if (nullptr == pdrgpcrPeer)
		{
			continue;
		}
		if (nullptr == pdrgpcrExpected)
		{
			pdrgpcrExpected = pdrgpcrPeer;
			continue;
		}
		if (pdrgpcrExpected->Size() != pdrgpcrPeer->Size())
		{
			return nullptr;
		}
		for (ULONG col = 0; col < pdrgpcrExpected->Size(); col++)
		{
			if ((*pdrgpcrExpected)[col] != (*pdrgpcrPeer)[col])
			{
				return nullptr;
			}
		}
	}
	return pdrgpcrExpected;
}

// WeTune may match one equality edge of a join whose physical predicate has
// several cross-child equalities. If a surrounding source operator has already
// bound an AttrsEq peer, select the unique ordered edge subset compatible with
// that peer. The complete predicate is still retained on the model, so
// unselected equalities are never dropped by target construction.
void
NarrowJoinKeys(CMemoryPool *mp, const CDSLRule *prule,
			   const CDSLSymbol *psymLeft, const CDSLSymbol *psymRight,
			   const CDSLModel *pmodel, CColRefArray **ppdrgpcrLeft,
			   CColRefArray **ppdrgpcrRight)
{
	const CColRefArray *pdrgpcrExpectedLeft =
		PdrgpcrEqualityPeer(prule, psymLeft, pmodel);
	const CColRefArray *pdrgpcrExpectedRight =
		PdrgpcrEqualityPeer(prule, psymRight, pmodel);
	if (nullptr == pdrgpcrExpectedLeft && nullptr == pdrgpcrExpectedRight)
	{
		return;
	}
	if (nullptr != pdrgpcrExpectedLeft && nullptr != pdrgpcrExpectedRight &&
		pdrgpcrExpectedLeft->Size() != pdrgpcrExpectedRight->Size())
	{
		return;
	}

	const CColRefArray *pdrgpcrSelector = nullptr != pdrgpcrExpectedLeft
		? pdrgpcrExpectedLeft
		: pdrgpcrExpectedRight;
	CColRefArray *pdrgpcrNarrowLeft = GPOS_NEW(mp) CColRefArray(mp);
	CColRefArray *pdrgpcrNarrowRight = GPOS_NEW(mp) CColRefArray(mp);
	std::vector<BOOL> used((*ppdrgpcrLeft)->Size(), false);
	for (ULONG expected = 0; expected < pdrgpcrSelector->Size(); expected++)
	{
		ULONG match = gpos::ulong_max;
		ULONG matches = 0;
		for (ULONG candidate = 0; candidate < (*ppdrgpcrLeft)->Size();
			 candidate++)
		{
			if (used[candidate])
			{
				continue;
			}
			BOOL fMatches = nullptr != pdrgpcrExpectedLeft
				? (*(*ppdrgpcrLeft))[candidate] ==
					  (*pdrgpcrExpectedLeft)[expected]
				: (*(*ppdrgpcrRight))[candidate] ==
					  (*pdrgpcrExpectedRight)[expected];
			if (fMatches && nullptr != pdrgpcrExpectedLeft &&
				nullptr != pdrgpcrExpectedRight)
			{
				fMatches = (*(*ppdrgpcrRight))[candidate] ==
					(*pdrgpcrExpectedRight)[expected];
			}
			if (fMatches)
			{
				match = candidate;
				matches++;
			}
		}
		// An ambiguous subset must be left for the ordinary constraint checker;
		// choosing an arbitrary equality would make rule order affect semantics.
		if (1 != matches)
		{
			pdrgpcrNarrowLeft->Release();
			pdrgpcrNarrowRight->Release();
			return;
		}
		used[match] = true;
		pdrgpcrNarrowLeft->Append((*(*ppdrgpcrLeft))[match]);
		pdrgpcrNarrowRight->Append((*(*ppdrgpcrRight))[match]);
	}

	(*ppdrgpcrLeft)->Release();
	(*ppdrgpcrRight)->Release();
	*ppdrgpcrLeft = pdrgpcrNarrowLeft;
	*ppdrgpcrRight = pdrgpcrNarrowRight;
}
}  // namespace

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
			!CPredicateUtils::IsEqualityOp(pexprConj))
		{
			// non-equi conjunct: preserve as residual (AddRef — the array owns it).
			pexprConj->AddRef();
			pdrgpexprResidual->Append(pexprConj);
			continue;
		}

		// Equality over identifiers, optionally hidden below binary-coercible
		// casts, is still a positional join-key equality. Non-binary casts remain
		// residual because they need not preserve the FK/uniqueness semantics.
		CColRef *pcr0 = PcrJoinKeyOperand((*pexprConj)[0]);
		CColRef *pcr1 = PcrJoinKeyOperand((*pexprConj)[1]);
		if (nullptr == pcr0 || nullptr == pcr1)
		{
			pexprConj->AddRef();
			pdrgpexprResidual->Append(pexprConj);
			continue;
		}

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
	NarrowJoinKeys(m_mp, m_prule, psymLeft, psymRight, pmodel,
				   &pdrgpcrLeft, &pdrgpcrRight);

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

	// The complete predicate already retains every residual conjunct. Do not put
	// Join residuals in the Filter-global residual slot: nested joins/filters must
	// not overwrite or inherit one another's predicates.
	pdrgpexprResidual->Release();

	// Record by this Join node's attrs pair, so nested joins retain independent
	// predicates and target-side AttrsEq aliases can find the right one.
	CExpression *pexprPred = (*pexprJoin)[2];
	return pmodel->FSetJoinPred(psymLeft, psymRight, pexprPred);
}

// EOF
