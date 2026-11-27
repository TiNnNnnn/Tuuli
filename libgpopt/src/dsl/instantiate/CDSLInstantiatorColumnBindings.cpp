// Resolve column captures, typed references, metadata and fresh Bind outputs.
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLColumnDomain.h"
#include "gpopt/dsl/CDSLExpressionDefinitions.h"
#include "gpopt/dsl/CDSLExprListUtils.h"
#include "gpos/common/CAutoRef.h"
#include "gpopt/operators/CScalarSubquery.h"

using namespace gpopt;

CColRefArray *
CDSLInstantiator::PdrgpcrCacheColumns(const CDSLSymbol *psym, CColRefArray *columns) const
{
	if (nullptr != columns && !m_phmDerivedCols->Insert(const_cast<CDSLSymbol *>(psym), columns))
	{
		columns->Release();
		return nullptr;
	}
	return columns;
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
	// Relational domains retain physical row-identity columns; OutputAttrs
	// separately selects the observable output vector.
	if (EdslsymTable == psym->Esymkind())
	{
		auto *cached = dynamic_cast<CColRefArray *>(m_phmDerivedCols->Find(psym));
		if (nullptr != cached) return cached;
		CExpression *input = pmodel->PexprTable(psym);
		if (nullptr == input) return nullptr;
		CColRefArray *columns = input->DeriveOutputColumns()->Pdrgpcr(m_mp);
		return PdrgpcrCacheColumns(psym, columns);
	}
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
			const CDSLSymbol *input = binding->PsymOperand(0);
			CExpression *pexpr = EdslsymExpr == input->Esymkind()
				? PexprResolveExpr(input, pmodel, ulDepth + 1)
				: EdslsymPred == input->Esymkind()
					? PexprResolvePredicate(input, pmodel, ulDepth + 1)
					: PexprResolveScalar(input, pmodel, ulDepth + 1);
			if (nullptr == pexpr) return nullptr;
			CColRefArray *columns = pexpr->DeriveUsedColumns()->Pdrgpcr(m_mp);
			pexpr->Release();
			return PdrgpcrCacheColumns(psym, columns);
		}
		return nullptr;
	}

	// A constraint may check the result of Bind. Only genuine output slots
	// take precedence over the column that Bind allocates.
	const CDSLConstraint *definition = nullptr;
	BOOL rankOutput = false;
	for (ULONG i = 0; i < m_prule->Pdrgpcon()->Size(); ++i)
	{
		const auto *con = (*m_prule->Pdrgpcon())[i];
		const auto kind = con->Edslcon();
		const auto *symbols = con->Pdrgpsym();
		if (symbols->Size() != CDSLConstraintKindTable::UlArity(kind)) continue;
		for (ULONG j = 0; j < symbols->Size(); ++j)
		{
			const auto role = CDSLConstraintKindTable::EsymkindDerivedOutput(kind, j);
			if ((*symbols)[j] != psym || (role != EdslsymAttrs && role != EdslsymSchema))
				continue;
			if (EdslconRankAttrs == kind)
				rankOutput = true;  // RankAttrs binds both columns in the model.
			else
			{
				if (nullptr != definition ||
					(EdslconAttrsIntersect != kind &&
					 role != psym->Esymkind()))
					return nullptr;
				definition = con;
			}
			break;
		}
	}
	if (nullptr != definition)
		return PdrgpcrFromColumnConstraint(definition, psym, pmodel, ulDepth);
	if (rankOutput || nullptr != binding)
		return PdrgpcrFromLegacyEquality(psym, pmodel, ulDepth);
	const auto findBind = [&](const auto &self, const CDSLOp *op) -> const CDSLOp * {
		if (EdslopBind == op->Edslop() && (*op->Pdrgpsym())[1] == psym) return op;
		for (ULONG i = 0; i < op->UlChildren(); ++i)
			if (const auto *found = self(self, (*op)[i])) return found;
		return nullptr;
	};
	const auto *producer = findBind(findBind, m_prule->PfragTgt()->PopRoot());
	if (nullptr != producer)
	{
		CExpression *value = PexprResolveScalar((*producer->Pdrgpsym())[0], pmodel, ulDepth + 1);
		if (nullptr == value) return nullptr;
		CColRef *column = nullptr;
		if (COperator::EopScalarSubquery == value->Pop()->Eopid())
			column = const_cast<CColRef *>(CScalarSubquery::PopConvert(value->Pop())->Pcr());
		else if (COperator::EopScalarSubqueryExists == value->Pop()->Eopid() ||
				 COperator::EopScalarSubqueryNotExists == value->Pop()->Eopid() ||
				 COperator::EopScalarSubqueryAny == value->Pop()->Eopid() ||
				 COperator::EopScalarSubqueryAll == value->Pop()->Eopid())
			column = pmodel->PcrCreateSubqueryResult(producer, value);
		value->Release();
		if (nullptr == column) return nullptr;
		auto *columns = GPOS_NEW(m_mp) CColRefArray(m_mp);
		columns->Append(column);
		return PdrgpcrCacheColumns(psym, columns);
	}
	return PdrgpcrFromLegacyEquality(psym, pmodel, ulDepth);
}

CColRefArray *
CDSLInstantiator::PdrgpcrFromColumnConstraint(const CDSLConstraint *pconDef,
	const CDSLSymbol *psym, const CDSLModel *pmodel, ULONG ulDepth) const
{
	if (EdslconAttrsEmpty == pconDef->Edslcon())
		return PdrgpcrCacheColumns(psym, GPOS_NEW(m_mp) CColRefArray(m_mp));
	if (EdslconExprNulls == pconDef->Edslcon())
	{
		CExpression *pexprNulls = PexprResolveExpr(
			(*pconDef->Pdrgpsym())[0], pmodel, ulDepth + 1);
		CRefCount::SafeRelease(pexprNulls);
		return dynamic_cast<CColRefArray *>(m_phmDerivedCols->Find(psym));
	}
	if (EdslconPredicateDomainSplit == pconDef->Edslcon())
	{
		if (!FMaterializePredicateDomainSplit(pconDef, pmodel, ulDepth + 1))
		{
			return nullptr;
		}
		return dynamic_cast<CColRefArray *>(m_phmDerivedCols->Find(psym));
	}
	if (EdslconOutputAttrs == pconDef->Edslcon())
	{
		const CDSLSymbol *psymTable =
			PsymResolve((*pconDef->Pdrgpsym())[1]);
		CColRefArray *pdrgpcrResult = nullptr;
		if (EdslsymExpr == psymTable->Esymkind())
		{
			CExpression *list = PexprResolveExpr(psymTable, pmodel, ulDepth + 1);
			pdrgpcrResult = CDSLExprListUtils::PdrgpcrOutput(m_mp, list);
			CRefCount::SafeRelease(list);
		}
		else if (EdslsymTable == psymTable->Esymkind())
		{
			CExpression *pexprTable = pmodel->PexprTable(psymTable);
			// OutputAttrs uses the same observable row domain as its checker.
			// List outputs instead retain their exact order.
			if (nullptr != pexprTable)
				pdrgpcrResult = CDSLColumnDomain::PdrgpcrOutput(m_mp, pexprTable->DeriveOutputColumns());
		}
		return PdrgpcrCacheColumns(psym, pdrgpcrResult);
	}
	if (EdslconSchemaFromAttrs == pconDef->Edslcon())
	{
		const CDSLSymbol *psymAttrs =
			PsymResolve((*pconDef->Pdrgpsym())[1]);
		if (EdslsymAttrs != psymAttrs->Esymkind())
		{
			return nullptr;
		}
		CColRefArray *pdrgpcrAttrs =
			PdrgpcrResolveCols(psymAttrs, pmodel, ulDepth + 1);
		if (nullptr == pdrgpcrAttrs)
		{
			return nullptr;
		}
		pdrgpcrAttrs->AddRef();
		return PdrgpcrCacheColumns(psym, pdrgpcrAttrs);
	}
	if (EdslconFuncAttrs == pconDef->Edslcon())
	{
		const CDSLSymbol *psymFuncs =
			PsymResolve((*pconDef->Pdrgpsym())[1]);
		CAutoRef<CExpressionArray> functions(PdrgpexprResolveFunctions(psymFuncs, pmodel));
		CExpressionArray *pdrgpexprFuncs = functions.Value();
		if (nullptr == pdrgpexprFuncs)
		{
			return nullptr;
		}
		CColRefSet *pcrsAttrs = GPOS_NEW(m_mp) CColRefSet(m_mp);
		for (ULONG ul = 0; ul < pdrgpexprFuncs->Size(); ul++)
		{
			pcrsAttrs->Include((*pdrgpexprFuncs)[ul]->DeriveUsedColumns());
		}
		CColRefArray *pdrgpcrAttrs = pcrsAttrs->Pdrgpcr(m_mp);
		pcrsAttrs->Release();
		return PdrgpcrCacheColumns(psym, pdrgpcrAttrs);
	}
	if (EdslconAttrsUnion == pconDef->Edslcon() ||
		EdslconSchemaUnion == pconDef->Edslcon())
	{
		const CDSLSymbol *psymLeft =
			PsymResolve((*pconDef->Pdrgpsym())[1]);
		const CDSLSymbol *psymRight =
			PsymResolve((*pconDef->Pdrgpsym())[2]);
		const BOOL fAttrsUnion = EdslconAttrsUnion == pconDef->Edslcon();
		if ((fAttrsUnion &&
			 (EdslsymAttrs != psym->Esymkind() ||
			  EdslsymAttrs != psymLeft->Esymkind() ||
			  EdslsymAttrs != psymRight->Esymkind())) ||
			(!fAttrsUnion &&
			 (EdslsymSchema != psym->Esymkind() ||
			  EdslsymSchema != psymLeft->Esymkind() ||
			  (EdslsymAttrs != psymRight->Esymkind() &&
			   EdslsymSchema != psymRight->Esymkind()))))
		{
			return nullptr;
		}
		CColRefArray *pdrgpcrLeft =
			PdrgpcrResolveCols(psymLeft, pmodel, ulDepth + 1);
		CColRefArray *pdrgpcrRight =
			PdrgpcrResolveCols(psymRight, pmodel, ulDepth + 1);
		if (nullptr == pdrgpcrLeft || nullptr == pdrgpcrRight)
		{
			return nullptr;
		}

		CColRefArray *pdrgpcrResult = GPOS_NEW(m_mp) CColRefArray(m_mp);
		CColRefSet *pcrsSeen = GPOS_NEW(m_mp) CColRefSet(m_mp);
		CColRefArray *rgpdrgpcr[] = {pdrgpcrLeft, pdrgpcrRight};
		for (ULONG ulInput = 0; ulInput < 2; ulInput++)
		{
			for (ULONG ul = 0; ul < rgpdrgpcr[ulInput]->Size(); ul++)
			{
				CColRef *pcr = (*rgpdrgpcr[ulInput])[ul];
				if (!pcrsSeen->FMember(pcr))
				{
					pcrsSeen->Include(pcr);
					pdrgpcrResult->Append(pcr);
				}
			}
		}
		pcrsSeen->Release();
		return PdrgpcrCacheColumns(psym, pdrgpcrResult);
	}

	if (EdslconAttrsIntersect != pconDef->Edslcon()) return nullptr;
	const CDSLSymbol *psymInput = (*pconDef->Pdrgpsym())[1];
	const CDSLSymbol *psymDomain =
		PsymResolve((*pconDef->Pdrgpsym())[2]);
	if (psymInput->Esymkind() != psym->Esymkind() &&
		EdslsymTable != psymInput->Esymkind())
	{
		return nullptr;
	}
	CColRefArray *pdrgpcrInput =
		PdrgpcrResolveCols(psymInput, pmodel, ulDepth + 1);
	if (nullptr == pdrgpcrInput)
	{
		return nullptr;
	}

	CColRefSet *pcrsDomain = GPOS_NEW(m_mp) CColRefSet(m_mp);
	if (EdslsymTable == psymDomain->Esymkind())
	{
		CExpression *pexprDomain = pmodel->PexprTable(psymDomain);
		if (nullptr == pexprDomain)
		{
			pcrsDomain->Release();
			return nullptr;
		}
		pcrsDomain->Include(pexprDomain->DeriveOutputColumns());
	}
	else if (EdslsymAttrs == psymDomain->Esymkind() ||
			 EdslsymSchema == psymDomain->Esymkind())
	{
		CColRefArray *pdrgpcrDomain =
			PdrgpcrResolveCols(psymDomain, pmodel, ulDepth + 1);
		if (nullptr == pdrgpcrDomain)
		{
			pcrsDomain->Release();
			return nullptr;
		}
		pcrsDomain->Include(pdrgpcrDomain);
	}
	else
	{
		pcrsDomain->Release();
		return nullptr;
	}

	CColRefArray *pdrgpcrResult = GPOS_NEW(m_mp) CColRefArray(m_mp);
	for (ULONG ul = 0; ul < pdrgpcrInput->Size(); ul++)
	{
		CColRef *pcr = (*pdrgpcrInput)[ul];
		if (pcrsDomain->FMember(pcr))
		{
			pdrgpcrResult->Append(pcr);
		}
	}
	pcrsDomain->Release();
	return PdrgpcrCacheColumns(psym, pdrgpcrResult);
}
