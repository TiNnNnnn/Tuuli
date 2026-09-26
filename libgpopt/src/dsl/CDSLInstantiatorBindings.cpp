//---------------------------------------------------------------------------
// MONSOON DSL rule engine — CDSLInstantiatorBindings.cpp
// Alias resolution and typed expression construction. Only absent/legacy definitions
// reach the explicit compatibility resolvers; a failed typed construction never falls back.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLInstantiator.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLConstraintChecker.h"
#include "gpopt/dsl/CDSLExpressionDefinitions.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarBooleanTest.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CScalarConst.h"
#include "gpopt/operators/CScalarIf.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarSubqueryExists.h"
#include "gpopt/operators/CScalarSubquery.h"
#include "gpopt/operators/CScalarSubqueryAll.h"
#include "gpopt/operators/CScalarSubqueryAny.h"
#include "naucrates/base/IDatumInt8.h"
#include "gpopt/xforms/CXformUtils.h"

using namespace gpopt;

BOOL
CDSLInstantiator::FMaterializeConstraintOutputs(
	const CDSLRule *prule, const CDSLConstraint *pcon, CDSLModel *pmodel)
{
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(nullptr != pcon);
	GPOS_ASSERT(nullptr != pmodel);
	if (nullptr == m_prule)
	{
		m_prule = prule;
		BuildAliasMap(prule);
	}
	else if (m_prule != prule)
	{
		return false;
	}
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (EdslconSliceCompose == pcon->Edslcon())
	{
		if (6 != pdrgpsym->Size())
		{
			return false;
		}
		LINT values[6] = {};
		BOOL bound[6] = {};
		for (ULONG ul = 0; ul < 6; ul++)
		{
			const CDSLSymbol *psym = (*pdrgpsym)[ul];
			if (EdslsymScalar != psym->Esymkind())
			{
				return false;
			}
			CExpression *pexpr = PexprResolveScalar(psym, pmodel);
			if (nullptr == pexpr)
			{
				if (ul < 4 || EdslsideTarget != psym->Eside())
				{
					return false;
				}
				continue;
			}
			// PostgreSQL LIMIT/OFFSET are int8. NULL is unbounded/default,
			// not a natural-number slice; dynamic or negative values may err.
			gpnaucrates::IDatumInt8 *datum =
				COperator::EopScalarConst == pexpr->Pop()->Eopid()
					? dynamic_cast<gpnaucrates::IDatumInt8 *>(
						  CScalarConst::PopConvert(pexpr->Pop())->GetDatum())
					: nullptr;
			const BOOL valid = nullptr != datum && !datum->IsNull() &&
				0 <= datum->Value();
			if (valid)
			{
				values[ul] = datum->Value();
				bound[ul] = true;
			}
			pexpr->Release();
			if (!valid)
			{
				return false;
			}
		}
		if (values[3] > gpos::lint_max - values[1])
		{
			return false;
		}
		const LINT remaining = values[2] > values[1] ? values[2] - values[1] : 0;
		const LINT result[2] = {
			values[0] < remaining ? values[0] : remaining, values[3] + values[1]};
		// Validate both outputs before adding either constructive binding.
		for (ULONG ul = 0; ul < 2; ul++)
		{
			if (bound[ul + 4] && values[ul + 4] != result[ul])
			{
				return false;
			}
		}
		for (ULONG ul = 0; ul < 2; ul++)
		{
			CExpression *pexpr = PexprResolveScalar((*pdrgpsym)[ul + 4], pmodel);
			if (nullptr == pexpr)
			{
				pexpr = CUtils::PexprScalarConstInt8(m_mp, result[ul]);
			}
			else if (dynamic_cast<gpnaucrates::IDatumInt8 *>(
				CScalarConst::PopConvert(pexpr->Pop())->GetDatum())->Value() != result[ul])
			{
				pexpr->Release();
				return false;
			}
			const BOOL ok = pmodel->FBindDerived((*pdrgpsym)[ul + 4], pexpr);
			pexpr->Release();
			if (!ok)
			{
				return false;
			}
		}
		return true;
	}
	if (EdslconOrderEmpty == pcon->Edslcon())
	{
		const CDSLSymbol *psymOrder = (*pdrgpsym)[0];
		COrderSpecArray *pdrgpos = pmodel->PdrgposOrder(psymOrder);
		if (nullptr != pdrgpos)
		{
			return 0 == pdrgpos->Size();
		}
		pdrgpos = GPOS_NEW(m_mp) COrderSpecArray(m_mp);
		const BOOL fBound = pmodel->FBindDerived(psymOrder, pdrgpos);
		pdrgpos->Release();
		return fBound;
	}
	if (EdslconRankAttrs == pcon->Edslcon())
	{
		const CDSLSymbol *psymAttrs = (*pdrgpsym)[0];
		const CDSLSymbol *psymRank = (*pdrgpsym)[1];
		CColRefArray *pdrgpcrAttrs = pmodel->PdrgpcrAttrs(psymAttrs);
		CColRefArray *pdrgpcrRank = pmodel->PdrgpcrRank(psymRank);
		if (nullptr == pdrgpcrAttrs && nullptr == pdrgpcrRank)
		{
			pdrgpcrRank = GPOS_NEW(m_mp) CColRefArray(m_mp);
			pdrgpcrRank->Append(CXformUtils::PcrCreateRowNumber(m_mp));
			const BOOL fRank = pmodel->FBindDerived(psymRank, pdrgpcrRank);
			const BOOL fAttrs = fRank &&
				pmodel->FBindDerived(psymAttrs, pdrgpcrRank);
			pdrgpcrRank->Release();
			return fAttrs;
		}
		CColRefArray *pdrgpcr = nullptr == pdrgpcrAttrs
			? pdrgpcrRank
			: pdrgpcrAttrs;
		if (nullptr == pdrgpcr || 1 != pdrgpcr->Size() ||
			(nullptr != pdrgpcrAttrs && nullptr != pdrgpcrRank &&
			 !CColRef::Equals(pdrgpcrAttrs, pdrgpcrRank)))
		{
			return false;
		}
		return nullptr == pdrgpcrAttrs
			? pmodel->FBindDerived(psymAttrs, pdrgpcr)
			: (nullptr == pdrgpcrRank
				   ? pmodel->FBindDerived(psymRank, pdrgpcr)
				   : true);
	}

	for (ULONG ul = 0; ul < pdrgpsym->Size(); ul++)
	{
		EDslSymbolKind esymkind =
			CDSLConstraintKindTable::EsymkindDerivedOutput(pcon->Edslcon(), ul);
		if (EdslconAttrsIntersect == pcon->Edslcon() && 0 == ul)
		{
			esymkind = (*pdrgpsym)[0]->Esymkind();
		}
		const CDSLSymbol *psym = (*pdrgpsym)[ul];
		if (EdslsymSentinel == esymkind && EdslsideTarget == psym->Eside() &&
			(EdslsymAttrs == psym->Esymkind() ||
			 EdslsymSchema == psym->Esymkind()) &&
			PsymResolve(psym) != psym)
		{
			esymkind = psym->Esymkind();
		}
		if ((EdslconExprListScalarSubquery == pcon->Edslcon() ||
			 EdslconExprListExists == pcon->Edslcon() ||
			 EdslconExprListNotExists == pcon->Edslcon() ||
			 EdslconExprListAny == pcon->Edslcon() ||
			 EdslconExprListAll == pcon->Edslcon()) &&
			1 == ul &&
			0 < pdrgpsym->Size())
		{
			esymkind = (*pdrgpsym)[0]->Esymkind();
		}
		if (EdslsymSentinel == esymkind)
		{
			continue;
		}

		if (esymkind != psym->Esymkind())
		{
			return false;
		}
		if (nullptr != pmodel->PvalLookup(psym))
		{
			continue;
		}

		CRefCount *pval = nullptr;
		BOOL fOwned = false;
		switch (esymkind)
		{
			case EdslsymPred:
				pval = PexprResolvePredicate(psym, pmodel);
				fOwned = true;
				break;
			case EdslsymAttrs:
			case EdslsymSchema:
				pval = PdrgpcrResolveCols(psym, pmodel);
				break;
			case EdslsymScalar:
				pval = PexprResolveScalar(psym, pmodel);
				fOwned = true;
				break;
			case EdslsymExpr:
				pval = PexprResolveExpr(psym, pmodel);
				fOwned = true;
				break;
			default:
				return false;
		}

		const BOOL fBound =
			nullptr != pval && pmodel->FBindDerived(psym, pval);
		if (fOwned)
		{
			CRefCount::SafeRelease(pval);
		}
		if (!fBound)
		{
			return false;
		}
	}
	return true;
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
//		Typed metadata references also resolve to their original source capture.
//---------------------------------------------------------------------------
void
CDSLInstantiator::BuildAliasMap(const CDSLRule *prule)
{
	CDSLConstraintArray *pdrgpcon = prule->Pdrgpcon();
	const ULONG ulCon = pdrgpcon->Size();
	BOOL fChanged = true;
	while (fChanged)
	{
		fChanged = false;
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
				case EdslconExprListEq:
				case EdslconOrderEq:
				case EdslconWindowEq:
				case EdslconFrameEq:
				case EdslconRankEq:
					break;
				default:
					continue;
			}

			CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
			if (2 != pdrgpsym->Size())
			{
				continue;
			}
			for (ULONG side = 0; side < 2; side++)
			{
				CDSLSymbol *psymTgt = (*pdrgpsym)[side];
				CDSLSymbol *psymPeer = (*pdrgpsym)[1 - side];
				if (EdslsideTarget != psymTgt->Eside() ||
					nullptr != m_phmAlias->Find(psymTgt))
				{
					continue;
				}
				CDSLSymbol *psymSrc = EdslsideSource == psymPeer->Eside()
					? psymPeer
					: m_phmAlias->Find(psymPeer);
				if (nullptr != psymSrc && EdslsideSource == psymSrc->Eside())
				{
					BOOL fOk = m_phmAlias->Insert(psymTgt, psymSrc);
					GPOS_ASSERT(fOk);
					(void) fOk;
					fChanged = true;
				}
			}
		}
		// Metadata references select the original capture, including the full
		// projection artifact keyed by its schema. Resolve chains here so every
		// consumer sees the same source; do not turn constructed predicates into aliases.
		const auto *definitions = prule->Pexprdefs();
		for (ULONG i = 0; i < definitions->UlDefinitions(); ++i)
		{
			const auto *def = definitions->PdefAt(i);
			if (CDSLExpressionDefinitions::EBuild != def->Binding() ||
				EdslexprRef != def->Edslexpr() ||
				EdslsymPred == def->PsymOutput()->Esymkind())
			{
				continue;
			}
			CDSLSymbol *target = const_cast<CDSLSymbol *>(def->PsymOutput());
			CDSLSymbol *input = const_cast<CDSLSymbol *>(def->PsymOperand(0));
			if (nullptr != m_phmAlias->Find(target))
			{
				continue;
			}
			CDSLSymbol *source = EdslsideSource == input->Eside()
				? input : m_phmAlias->Find(input);
			if (nullptr != source && EdslsideSource == source->Eside())
			{
				BOOL inserted GPOS_ASSERTS_ONLY = m_phmAlias->Insert(target, source);
				GPOS_ASSERT(inserted);
				fChanged = true;
			}
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

CExpressionArray *
CDSLInstantiator::PdrgpexprResolveArguments(const CDSLSymbol *symbol,
	const CDSLModel *model, ULONG depth) const
{
	GPOS_CHECK_STACK_SIZE;
	if (nullptr == m_prule || EdslsymValueList != symbol->Esymkind() ||
		depth > m_prule->Pexprdefs()->UlDefinitions()) return nullptr;
	symbol = PsymResolve(symbol);
	auto *bound = static_cast<CExpressionArray *>(model->PvalLookup(symbol));
	if (nullptr != bound) { bound->AddRef(); return bound; }
	const auto *def = m_prule->Pexprdefs()->Pdef(symbol);
	if (nullptr == def || CDSLExpressionDefinitions::EBuild != def->Binding()) return nullptr;
	if (EdslexprRef == def->Edslexpr())
		return PdrgpexprResolveArguments(def->PsymOperand(0), model, depth + 1);
	if (EdslexprArgs != def->Edslexpr()) return nullptr;
	CExpressionArray *arguments = GPOS_NEW(m_mp) CExpressionArray(m_mp);
	if (0 == def->Arity()) return arguments;
	CExpression *value = PexprResolveScalar(def->PsymOperand(0), model, depth + 1);
	CExpressionArray *tail = PdrgpexprResolveArguments(def->PsymOperand(1), model, depth + 1);
	if (nullptr == value || nullptr == tail)
	{
		CRefCount::SafeRelease(value);
		CRefCount::SafeRelease(tail);
		arguments->Release();
		return nullptr;
	}
	arguments->Append(value);
	for (ULONG i = 0; i < tail->Size(); ++i)
	{
		(*tail)[i]->AddRef();
		arguments->Append((*tail)[i]);
	}
	tail->Release();
	return arguments;
}

CExpression *
CDSLInstantiator::PexprResolveScalar(const CDSLSymbol *psym,
									const CDSLModel *pmodel, ULONG depth) const
{
	if (nullptr == psym || EdslsymScalar != psym->Esymkind() ||
		(nullptr != m_prule && depth > m_prule->Pexprdefs()->UlDefinitions()))
	{
		return nullptr;
	}
	const CDSLSymbol *psymResolved = PsymResolve(psym);
	CExpression *pexpr = pmodel->PexprScalar(psymResolved);
	if (nullptr != pexpr)
	{
		pexpr->AddRef();
		return pexpr;
	}
	if (nullptr == m_prule)
	{
		return nullptr;
	}
	const auto *binding = m_prule->Pexprdefs()->Pdef(psymResolved);
	if (nullptr != binding && CDSLExpressionDefinitions::ELegacy != binding->Binding())
	{
		if (CDSLExpressionDefinitions::EBuild != binding->Binding())
			return nullptr;
		if (EdslexprRef == binding->Edslexpr())
			return PexprResolveScalar(binding->PsymOperand(0), pmodel, depth + 1);
		if (EdslexprColumn == binding->Edslexpr())
		{
			const auto *columns = PdrgpcrResolveCols(binding->PsymOperand(0), pmodel, depth + 1);
			return nullptr != columns && 1 == columns->Size()
				? CUtils::PexprScalarIdent(m_mp, (*columns)[0]) : nullptr;
		}
		if (EdslexprScalarSubquery == binding->Edslexpr())
		{
			const auto *outputs = PdrgpcrResolveCols(binding->PsymOperand(0), pmodel, depth + 1);
			CExpression *query = pmodel->PexprTable(PsymResolve(binding->PsymOperand(1)));
			if (nullptr == outputs || 1 != outputs->Size() ||
				!CDSLMatchView::FSelectedSubqueryInput(query, (*outputs)[0])) return nullptr;
			query->AddRef();
			return GPOS_NEW(m_mp) CExpression(m_mp,
				GPOS_NEW(m_mp) CScalarSubquery(m_mp, (*outputs)[0], false, false), query);
		}
		if (EdslexprCall == binding->Edslexpr())
		{
			auto *head = static_cast<CExpression *>(pmodel->PvalLookup(PsymResolve(binding->PsymOperand(0))));
			CExpressionArray *arguments = PdrgpexprResolveArguments(binding->PsymOperand(1), pmodel, depth + 1);
			if (nullptr == head || !CDSLMatchView::FCallArgumentTypes(head, arguments))
			{
				CRefCount::SafeRelease(arguments);
				return nullptr;
			}
			head->Pop()->AddRef();
			CExpression *call = GPOS_NEW(m_mp) CExpression(m_mp, head->Pop(), arguments);
			if (!CDSLMatchView::FScalarCall(call)) { call->Release(); return nullptr; }
			return call;
		}
		if (EdslexprCase == binding->Edslexpr())
		{
			CExpression *condition = PexprResolvePredicate(binding->PsymOperand(0), pmodel);
			CExpression *yes = PexprResolveScalar(binding->PsymOperand(1), pmodel, depth + 1);
			CExpression *no = PexprResolveScalar(binding->PsymOperand(2), pmodel, depth + 1);
			if (nullptr == condition || nullptr == yes || nullptr == no ||
				IMDType::EtiBool != COptCtxt::PoctxtFromTLS()->Pmda()->RetrieveType(
					CScalar::PopConvert(condition->Pop())->MdidType())->GetDatumType() ||
				!CScalar::PopConvert(yes->Pop())->MdidType()->Equals(CScalar::PopConvert(no->Pop())->MdidType()))
			{
				CRefCount::SafeRelease(condition);
				CRefCount::SafeRelease(yes);
				CRefCount::SafeRelease(no);
				return nullptr;
			}
			// Keep native CASE laziness; never lower the arms into eager calls.
			IMDId *type = CScalar::PopConvert(yes->Pop())->MdidType();
			type->AddRef();
			return GPOS_NEW(m_mp) CExpression(m_mp, GPOS_NEW(m_mp) CScalarIf(m_mp, type), condition, yes, no);
		}
		if (EdslexprBoolValue != binding->Edslexpr())
			return nullptr;
		CExpression *value = PexprResolvePredicate(binding->PsymOperand(0), pmodel);
		if (nullptr != value && IMDType::EtiBool !=
			COptCtxt::PoctxtFromTLS()->Pmda()->RetrieveType(
				CScalar::PopConvert(value->Pop())->MdidType())->GetDatumType())
		{
			value->Release();
			return nullptr;
		}
		return value;
	}

	return PexprResolveLegacyScalar(psym);
}

CExpression *
CDSLInstantiator::PexprInstantiatePredicate(const CDSLRule *prule,
										  const CDSLSymbol *psym,
										  const CDSLModel *pmodel)
{
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(nullptr != pmodel);
	if (nullptr == m_prule)
	{
		m_prule = prule;
		BuildAliasMap(prule);
	}
	else if (m_prule != prule)
	{
		return nullptr;
	}
	return PexprResolvePredicate(psym, pmodel);
}

CExpression *
CDSLInstantiator::PexprBuildNegation(CExpression *input, BOOL fNotTrue) const
{
	GPOS_ASSERT(nullptr != input);
	if (fNotTrue)
	{
		return GPOS_NEW(m_mp) CExpression(m_mp,
			GPOS_NEW(m_mp) CScalarBooleanTest(m_mp, CScalarBooleanTest::EbtIsNotTrue),
			input);
	}
	// The native constructor preserves its operand, even for constants or NOT.
	return CUtils::PexprNegate(m_mp, input);
}

CExpression *
CDSLInstantiator::PexprResolvePredicate(const CDSLSymbol *psym,
									   const CDSLModel *pmodel,
									   ULONG ulDepth) const
{
	if (nullptr == psym || nullptr == m_prule ||
		EdslsymPred != psym->Esymkind() ||
		ulDepth >
			m_prule->Pdrgpcon()->Size() + m_prule->Pexprdefs()->UlDefinitions())
	{
		return nullptr;
	}
	psym = PsymResolve(psym);
	CExpression *pexprBound = pmodel->PexprPred(psym);
	if (nullptr != pexprBound)
	{
		pexprBound->AddRef();
		return pexprBound;
	}
	CExpression *pexprDerived = m_phmDerivedPreds->Find(psym);
	if (nullptr != pexprDerived)
	{
		pexprDerived->AddRef();
		return pexprDerived;
	}

	const CDSLExpressionDefinitions::CDefinition *pdef =
		m_prule->Pexprdefs()->Pdef(psym);
	if (nullptr != pdef &&
		CDSLExpressionDefinitions::ELegacy != pdef->Binding())
	{
		// A source pattern is a test/capture, never a recipe for
		// manufacturing a missing source value. Only target definitions may
		// construct values.
		if (CDSLExpressionDefinitions::EBuild != pdef->Binding())
		{
			return nullptr;
		}
		if (EdslexprExists == pdef->Edslexpr())
		{
			CExpression *query = pmodel->PexprTable(PsymResolve(pdef->PsymOperand(0)));
			if (nullptr == query || !query->Pop()->FLogical()) return nullptr;
			query->AddRef();
			return GPOS_NEW(m_mp) CExpression(m_mp,
				GPOS_NEW(m_mp) CScalarSubqueryExists(m_mp), query);
		}
		if (EdslexprAny == pdef->Edslexpr() || EdslexprAll == pdef->Edslexpr())
		{
			CExpression *head = static_cast<CExpression *>(pmodel->PvalLookup(PsymResolve(pdef->PsymOperand(0))));
			CExpression *query = pmodel->PexprTable(PsymResolve(pdef->PsymOperand(3)));
			CColRefArray *outputs = PdrgpcrResolveCols(pdef->PsymOperand(2), pmodel);
			if (nullptr == outputs || 1 != outputs->Size()) return nullptr;
			const CColRef *output = (*outputs)[0];
			CExpressionArray *arguments = PdrgpexprResolveArguments(pdef->PsymOperand(1), pmodel, ulDepth + 1);
			// Every c capture is a native binary comparison, independent of the
			// quantifier and selected column specified by this target template.
			if (nullptr == head || COperator::EopScalarCmp != head->Pop()->Eopid() ||
				!CDSLMatchView::FQuantifiedInputs(head, query, arguments, output))
			{
				CRefCount::SafeRelease(arguments);
				return nullptr;
			}
			const auto *comparison = CScalarCmp::PopConvert(head->Pop());
			IMDId *id = comparison->MdIdOp();
			id->AddRef();
			auto *name = GPOS_NEW(m_mp) CWStringConst(m_mp, comparison->Pstr()->GetBuffer());
			COperator *op = EdslexprAny == pdef->Edslexpr()
				? static_cast<COperator *>(GPOS_NEW(m_mp) CScalarSubqueryAny(m_mp, id, name, output))
				: static_cast<COperator *>(GPOS_NEW(m_mp) CScalarSubqueryAll(m_mp, id, name, output));
			query->AddRef();
			(*arguments)[0]->AddRef();
			CExpression *result = GPOS_NEW(m_mp) CExpression(m_mp, op, query, (*arguments)[0]);
			arguments->Release();
			return result;
		}
		if (EdslexprCompare == pdef->Edslexpr())
		{
			CExpression *head = static_cast<CExpression *>(pmodel->PvalLookup(PsymResolve(pdef->PsymOperand(0))));
			CExpressionArray *arguments = PdrgpexprResolveArguments(pdef->PsymOperand(1), pmodel, ulDepth + 1);
			if (nullptr == head || nullptr == arguments || 2 != arguments->Size())
			{
				CRefCount::SafeRelease(arguments);
				return nullptr;
			}
			if (COperator::EopScalarCmp != head->Pop()->Eopid() ||
				!CDSLMatchView::FScalarCall(head) ||
				!CDSLMatchView::FCallArgumentTypes(head, arguments))
			{
				arguments->Release();
				return nullptr;
			}
			head->Pop()->AddRef();
			CExpression *result = GPOS_NEW(m_mp) CExpression(m_mp, head->Pop(), arguments);
			if (!CDSLMatchView::FScalarCall(result)) { result->Release(); return nullptr; }
			return result;
		}

		if (EdslexprValueBool == pdef->Edslexpr())
		{
			CExpression *value = PexprResolveScalar(pdef->PsymOperand(0), pmodel, ulDepth + 1);
			if (nullptr != value && IMDType::EtiBool !=
				COptCtxt::PoctxtFromTLS()->Pmda()->RetrieveType(
					CScalar::PopConvert(value->Pop())->MdidType())->GetDatumType())
			{
				value->Release();
				return nullptr;
			}
			// Boolean value and predicate share the same native scalar; no cast.
			return value;
		}
		if (EdslexprNullSafeEq == pdef->Edslexpr())
		{
			CColRefArray *left = PdrgpcrResolveCols(pdef->PsymOperand(0), pmodel);
			CColRefArray *right = PdrgpcrResolveCols(pdef->PsymOperand(1), pmodel);
			if (nullptr == left || nullptr == right || 0 == left->Size() ||
				left->Size() != right->Size())
				return nullptr;
			for (ULONG i = 0; i < left->Size(); ++i)
			{
				if (!(*left)[i]->RetrieveType()->MDId()->Equals((*right)[i]->RetrieveType()->MDId()) ||
					!IMDId::IsValid((*left)[i]->RetrieveType()->GetMdidForCmpType(IMDType::EcmptEq)))
				{
					return nullptr;
				}
			}
			// The existing constructor retains these non-constant comparison
			// leaves in order, including duplicates; no conjunct extraction.
			return CPredicateUtils::PexprINDFConjunction(m_mp, left, right);
		}
		CExpression *input =
			PexprResolvePredicate(pdef->PsymOperand(0), pmodel, ulDepth + 1);
		if (nullptr == input || EdslexprRef == pdef->Edslexpr())
		{
			return input;
		}
		if (EdslexprAnd == pdef->Edslexpr() || EdslexprOr == pdef->Edslexpr())
		{
			CExpression *right = PexprResolvePredicate(
				pdef->PsymOperand(1), pmodel, ulDepth + 1);
			if (nullptr == right)
			{
				input->Release();
				return nullptr;
			}
			// Explicit bindings preserve operand order, duplicates and nesting.
			return GPOS_NEW(m_mp) CExpression(
				m_mp, GPOS_NEW(m_mp) CScalarBoolOp(m_mp,
					EdslexprAnd == pdef->Edslexpr() ? CScalarBoolOp::EboolopAnd
						: CScalarBoolOp::EboolopOr),
				input, right);
		}
		return PexprBuildNegation(input, EdslexprNotTrue == pdef->Edslexpr());
	}
	return PexprResolveLegacyPredicate(psym, pmodel, ulDepth);
}

CColRefArray *
CDSLInstantiator::PdrgpcrResolveCols(const CDSLSymbol *psym,
									const CDSLModel *pmodel,
									ULONG ulDepth) const
{
	if (nullptr == psym || nullptr == m_prule ||
		ulDepth > m_prule->Pdrgpcon()->Size() + m_prule->Pexprdefs()->UlDefinitions())
	{
		return nullptr;
	}
	psym = PsymResolve(psym);
	if (EdslsymAttrs != psym->Esymkind() &&
		EdslsymSchema != psym->Esymkind())
	{
		return nullptr;
	}

	CRefCount *pval = pmodel->PvalLookup(psym);
	if (nullptr != pval)
	{
		return dynamic_cast<CColRefArray *>(pval);
	}
	CRefCount *pvalDerived = m_phmDerivedCols->Find(psym);
	if (nullptr != pvalDerived)
	{
		return dynamic_cast<CColRefArray *>(pvalDerived);
	}
	const auto *binding = m_prule->Pexprdefs()->Pdef(psym);
	if (nullptr != binding && CDSLExpressionDefinitions::ELegacy != binding->Binding())
	{
		if (CDSLExpressionDefinitions::EBuild != binding->Binding())
			return nullptr;
		if (EdslexprRef == binding->Edslexpr())
			return PdrgpcrResolveCols(binding->PsymOperand(0), pmodel, ulDepth + 1);
		if (EdslexprScalarDeps == binding->Edslexpr())
		{
			CExpression *pexpr = PexprResolveScalar(binding->PsymOperand(0), pmodel, ulDepth + 1);
			if (nullptr == pexpr) return nullptr;
			CColRefArray *columns = pexpr->DeriveUsedColumns()->Pdrgpcr(m_mp);
			pexpr->Release();
			if (!m_phmDerivedCols->Insert(const_cast<CDSLSymbol *>(psym), columns))
			{
				columns->Release();
				return nullptr;
			}
			return columns;
		}
		return nullptr;
	}

	return PdrgpcrResolveLegacyCols(psym, pmodel, ulDepth);
}

CExpression *
CDSLInstantiator::PexprResolveExpr(const CDSLSymbol *psym,
								   const CDSLModel *pmodel,
								   ULONG ulDepth) const
{
	if (nullptr == psym || EdslsymExpr != psym->Esymkind() ||
		ulDepth > m_prule->Pdrgpcon()->Size() + m_prule->Pexprdefs()->UlDefinitions())
	{
		return nullptr;
	}
	psym = PsymResolve(psym);
	CExpression *pexprBound = pmodel->PexprExpr(psym);
	if (nullptr != pexprBound)
	{
		pexprBound->AddRef();
		return pexprBound;
	}
	const auto *binding = m_prule->Pexprdefs()->Pdef(psym);
	if (nullptr != binding && CDSLExpressionDefinitions::ELegacy != binding->Binding())
	{
		if (CDSLExpressionDefinitions::EBuild != binding->Binding())
			return nullptr;
		if (EdslexprRef == binding->Edslexpr())
			return PexprResolveExpr(binding->PsymOperand(0), pmodel, ulDepth + 1);
		if (EdslexprItem != binding->Edslexpr())
			return nullptr;
		if (0 == binding->Arity())
			return GPOS_NEW(m_mp) CExpression(m_mp, GPOS_NEW(m_mp) CScalarProjectList(m_mp));
		CExpression *value = PexprResolveScalar(binding->PsymOperand(0), pmodel);
		CColRefArray *output = PdrgpcrResolveCols(binding->PsymOperand(1), pmodel);
		CExpression *tail = PexprResolveExpr(binding->PsymOperand(2), pmodel, ulDepth + 1);
		if (nullptr == value || nullptr == output || 1 != output->Size() ||
			nullptr == tail || COperator::EopScalarProjectList != tail->Pop()->Eopid() ||
			!(*output)[0]->RetrieveType()->MDId()->Equals(CScalar::PopConvert(value->Pop())->MdidType()) ||
			(*output)[0]->TypeModifier() != CScalar::PopConvert(value->Pop())->TypeModifier())
		{
			CRefCount::SafeRelease(value);
			CRefCount::SafeRelease(tail);
			return nullptr;
		}
		CExpressionArray *items = GPOS_NEW(m_mp) CExpressionArray(m_mp);
		items->Append(GPOS_NEW(m_mp) CExpression(m_mp,
			GPOS_NEW(m_mp) CScalarProjectElement(m_mp, (*output)[0]), value));
		for (ULONG i = 0; i < tail->Arity(); ++i)
		{
			(*tail)[i]->AddRef();
			items->Append((*tail)[i]);
		}
		tail->Release();
		return GPOS_NEW(m_mp) CExpression(m_mp, GPOS_NEW(m_mp) CScalarProjectList(m_mp), items);
	}

	return PexprResolveLegacyExpr(psym, pmodel, ulDepth);
}
