//---------------------------------------------------------------------------
// MONSOON DSL rule engine — CDSLInstantiatorProjection.cpp
// Compute, projection and aggregate builders, including their existing compatibility views.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLExpressionDefinitions.h"
#include "CDSLInstantiatorUtils.h"

#include "gpos/common/CAutoRef.h"
#include "gpopt/base/CColRef.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/operators/CScalarAggFunc.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"

using namespace gpopt;
using namespace gpopt::dslinstantiator;

namespace
{
BOOL
FAggNameEquals(CMemoryPool *mp, const CWStringConst *pstrActual,
			   const CHAR *szExpected)
{
	CWStringConst strExpected(mp, szExpected);
	return pstrActual->Equals(&strExpected);
}

BOOL
FAggFuncMatches(CMemoryPool *mp, const CDSLOp *popAgg,
				const CScalarAggFunc *popFunc)
{
	// WeTune's Agg does not encode DISTINCT; it is part of the bound function.
	switch (popAgg->Edslaggfunc())
	{
		case EdslaggfuncUnknown:
			return true;
		case EdslaggfuncSentinel:
			return false;
		case EdslaggfuncAverage:
			return FAggNameEquals(mp, popFunc->PstrAggFunc(), "avg") ||
				   FAggNameEquals(mp, popFunc->PstrAggFunc(), "average");
		default:
			return FAggNameEquals(
				mp, popFunc->PstrAggFunc(),
				CDSLOpKindTable::SzAggFuncName(popAgg->Edslaggfunc()));
	}
}

BOOL
FColArraysSameSet(CMemoryPool *mp, const CColRefArray *pdrgpcrFirst,
				  const CColRefArray *pdrgpcrSecond)
{
	CColRefSet *pcrsFirst = GPOS_NEW(mp) CColRefSet(mp);
	CColRefSet *pcrsSecond = GPOS_NEW(mp) CColRefSet(mp);
	pcrsFirst->Include(const_cast<CColRefArray *>(pdrgpcrFirst));
	pcrsSecond->Include(const_cast<CColRefArray *>(pdrgpcrSecond));
	BOOL fEqual = pcrsFirst->Equals(pcrsSecond);
	pcrsFirst->Release();
	pcrsSecond->Release();
	return fEqual;
}

// A project list made exclusively of column references can be deduplicated on
// those referenced columns directly. Besides avoiding a redundant Project, this
// preserves the dependency columns that existing parents may still require.
// Any non-leaf scalar must instead be evaluated before deduplication.
BOOL
FProjectListIsColumnOnly(const CExpression *pexprProjectList)
{
	if (nullptr == pexprProjectList ||
		COperator::EopScalarProjectList != pexprProjectList->Pop()->Eopid())
	{
		return false;
	}
	for (ULONG ul = 0; ul < pexprProjectList->Arity(); ul++)
	{
		CExpression *pexprElem = (*pexprProjectList)[ul];
		if (COperator::EopScalarProjectElement != pexprElem->Pop()->Eopid() ||
			1 != pexprElem->Arity() ||
			COperator::EopScalarIdent != (*pexprElem)[0]->Pop()->Eopid())
		{
			return false;
		}
	}
	return true;
}

// Copy a recorded LogicalLimit chain while replacing its deepest relational
// child. Operators retain their exact order specs/global flags and scalar
// offset/count expressions; the recorded source tree itself is never mutated.
CExpression *
PexprRestoreLimitShell(CMemoryPool *mp, CExpression *pexprShell,
					   CExpression *pexprChild)
{
	GPOS_ASSERT(COperator::EopLogicalLimit == pexprShell->Pop()->Eopid());
	GPOS_ASSERT(3 == pexprShell->Arity());

	CExpression *pexprRestoredChild = nullptr;
	if (COperator::EopLogicalLimit == (*pexprShell)[0]->Pop()->Eopid() &&
		3 == (*pexprShell)[0]->Arity())
	{
		pexprRestoredChild =
			PexprRestoreLimitShell(mp, (*pexprShell)[0], pexprChild);
	}
	else
	{
		pexprChild->AddRef();
		pexprRestoredChild = pexprChild;
	}
	pexprShell->Pop()->AddRef();
	(*pexprShell)[1]->AddRef();
	(*pexprShell)[2]->AddRef();
	return GPOS_NEW(mp) CExpression(mp, pexprShell->Pop(),
									pexprRestoredChild, (*pexprShell)[1],
									(*pexprShell)[2]);
}

// Remap only the scalar inputs of a project list. Project-element operators
// define the shell's stable output schema and must not be substituted merely
// because an equivalent input column is selected by the target rule.
CExpression *
PexprRemapProjectListInputs(CMemoryPool *mp, CExpression *pexprList,
							 UlongToColRefMap *colref_mapping)
{
	if (COperator::EopScalarProjectList != pexprList->Pop()->Eopid())
	{
		return nullptr;
	}

	CExpressionArray *pdrgpexprElems = GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < pexprList->Arity(); ul++)
	{
		CExpression *pexprElem = (*pexprList)[ul];
		if (COperator::EopScalarProjectElement !=
				pexprElem->Pop()->Eopid() ||
			1 != pexprElem->Arity())
		{
			pdrgpexprElems->Release();
			return nullptr;
		}
		CExpression *pexprScalar =
			PexprRemapPredicate(mp, (*pexprElem)[0], colref_mapping);
		pexprElem->Pop()->AddRef();
		pdrgpexprElems->Append(GPOS_NEW(mp) CExpression(
			mp, pexprElem->Pop(), pexprScalar));
	}
	return GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprElems);
}

// Column substitution can turn an alias into x := x. Project is compute-scalar
// in ORCA: x already passes through from the child and must not be redefined.
// Consume both arguments, preserving the original list when no alias changes.
CExpression *
PexprProjectWithoutSelfAliases(CMemoryPool *mp, CExpression *child,
								 CExpression *list)
{
	CExpressionArray *elements = GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < list->Arity(); ul++)
	{
		CExpression *element = (*list)[ul];
		CColRef *output = CScalarProjectElement::PopConvert(element->Pop())->Pcr();
		if (COperator::EopScalarIdent == (*element)[0]->Pop()->Eopid() &&
			output == CScalarIdent::PopConvert((*element)[0]->Pop())->Pcr())
		{
			if (!child->DeriveOutputColumns()->FMember(output))
			{
				elements->Release();
				list->Release();
				child->Release();
				return nullptr;
			}
			continue;
		}
		element->AddRef();
		elements->Append(element);
	}
	if (elements->Size() != list->Arity())
	{
		list->Release();
		list = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarProjectList(mp), elements);
	}
	else
	{
		elements->Release();
	}
	return GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalProject(mp), child, list);
}

// A GbAgg grouping CColRef is both an input identity and an output identity.
// Replacing it in the operator would leak the target-side column into the
// parent memo group. When a rewritten child no longer produces a grouping
// column, define that stable source identity from its mapped target column in a
// projection below the aggregate instead. The projection is omitted when all
// grouping identities remain available.
CExpression *
PexprEnsureAggGroupingColumns(CMemoryPool *mp, CExpression *pexprChild,
							  CColRefArray *pdrgpcrGrouping,
							  UlongToColRefMap *colref_mapping)
{
	CColRefSet *pcrsChild = pexprChild->DeriveOutputColumns();
	CExpressionArray *pdrgpexprElems = GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < pdrgpcrGrouping->Size(); ul++)
	{
		CColRef *pcrSource = (*pdrgpcrGrouping)[ul];
		if (pcrsChild->FMember(pcrSource))
		{
			continue;
		}

		const ULONG ulSourceId = pcrSource->Id();
		CColRef *pcrTarget = colref_mapping->Find(&ulSourceId);
		if (nullptr == pcrTarget || !pcrsChild->FMember(pcrTarget) ||
			!pcrSource->RetrieveType()->MDId()->Equals(
				pcrTarget->RetrieveType()->MDId()) ||
			pcrSource->TypeModifier() != pcrTarget->TypeModifier())
		{
			pdrgpexprElems->Release();
			return nullptr;
		}

		CExpression *pexprIdent = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarIdent(mp, pcrTarget));
		pdrgpexprElems->Append(GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarProjectElement(mp, pcrSource),
			pexprIdent));
	}

	if (0 == pdrgpexprElems->Size())
	{
		pdrgpexprElems->Release();
		pexprChild->AddRef();
		return pexprChild;
	}

	CExpression *pexprList = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprElems);
	pexprChild->AddRef();
	return GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalProject(mp), pexprChild, pexprList);
}

// Restore Project(GbAgg(...)) around a rewritten deepest input. Aggregate
// scalar arguments follow the target attrs, while grouping and project-element
// output identities remain the source schema exposed to parent operators.
CExpression *
PexprRestoreProjectAggShell(CMemoryPool *mp, CExpression *pexprShell,
							CExpression *pexprChild,
							UlongToColRefMap *colref_mapping)
{
	const COperator::EOperatorId eopid = pexprShell->Pop()->Eopid();
	GPOS_ASSERT(COperator::EopLogicalProject == eopid ||
				COperator::EopLogicalGbAgg == eopid);
	GPOS_ASSERT(2 == pexprShell->Arity());

	CExpression *pexprRestoredChild = nullptr;
	if (COperator::EopLogicalGbAgg == (*pexprShell)[0]->Pop()->Eopid())
	{
		pexprRestoredChild = PexprRestoreProjectAggShell(
			mp, (*pexprShell)[0], pexprChild, colref_mapping);
	}
	else
	{
		pexprChild->AddRef();
		pexprRestoredChild = pexprChild;
	}
	if (nullptr == pexprRestoredChild)
	{
		return nullptr;
	}

	COperator *popRestored = nullptr;
	if (COperator::EopLogicalGbAgg == eopid)
	{
		CLogicalGbAgg *popGbAgg =
			CLogicalGbAgg::PopConvert(pexprShell->Pop());
		CExpression *pexprGroupingChild = PexprEnsureAggGroupingColumns(
			mp, pexprRestoredChild, popGbAgg->Pdrgpcr(), colref_mapping);
		pexprRestoredChild->Release();
		pexprRestoredChild = pexprGroupingChild;
		if (nullptr == pexprRestoredChild)
		{
			return nullptr;
		}
		pexprShell->Pop()->AddRef();
		popRestored = pexprShell->Pop();
	}
	else
	{
		pexprShell->Pop()->AddRef();
		popRestored = pexprShell->Pop();
	}

	CExpression *pexprList = nullptr;
	// A restored aggregate exposes its stable source identities again, so an
	// outer Project should keep using its original scalar inputs when they are
	// available. Aggregate arguments, by contrast, consume the rewritten child
	// and must follow the target-side mapping.
	if (COperator::EopLogicalProject == eopid &&
		pexprRestoredChild->DeriveOutputColumns()->ContainsAll(
			(*pexprShell)[1]->DeriveUsedColumns()))
	{
		(*pexprShell)[1]->AddRef();
		pexprList = (*pexprShell)[1];
	}
	else
	{
		pexprList = PexprRemapProjectListInputs(
			mp, (*pexprShell)[1], colref_mapping);
	}
	if (nullptr == pexprList ||
		!pexprRestoredChild->DeriveOutputColumns()->ContainsAll(
			pexprList->DeriveUsedColumns()))
	{
		CRefCount::SafeRelease(pexprList);
		popRestored->Release();
		pexprRestoredChild->Release();
		return nullptr;
	}
	return GPOS_NEW(mp) CExpression(mp, popRestored,
									pexprRestoredChild, pexprList);
}
}  // namespace

CColRefArray *
CDSLInstantiator::PdrgpcrMinimalGrouping(
	const CDSLSymbol *psymGroup, const CDSLSymbol *psymSchema,
	const CDSLModel *pmodel) const
{
	if (nullptr == m_prule)
	{
		return nullptr;
	}
	psymGroup = PsymResolve(psymGroup);
	psymSchema = PsymResolve(psymSchema);
	BOOL fDeclared = false;
	CDSLConstraintArray *pdrgpcon = m_prule->Pdrgpcon();
	for (ULONG ul = 0; ul < pdrgpcon->Size(); ul++)
	{
		const CDSLConstraint *pcon = (*pdrgpcon)[ul];
		if (EdslconMinimalGrouping != pcon->Edslcon() ||
			2 != pcon->Pdrgpsym()->Size())
		{
			continue;
		}
		if (PsymResolve((*pcon->Pdrgpsym())[0]) == psymGroup &&
			PsymResolve((*pcon->Pdrgpsym())[1]) == psymSchema)
		{
			if (fDeclared)
			{
				return nullptr;
			}
			fDeclared = true;
		}
	}
	if (!fDeclared)
	{
		return nullptr;
	}

	CColRefArray *pdrgpcrGroup = PdrgpcrResolveCols(psymGroup, pmodel);
	CExpression *pexprAgg = pmodel->PexprAggBinding(psymSchema);
	if (nullptr == pdrgpcrGroup || nullptr == pexprAgg ||
		COperator::EopLogicalGbAgg != pexprAgg->Pop()->Eopid())
	{
		return nullptr;
	}
	CLogicalGbAgg *popAgg = CLogicalGbAgg::PopConvert(pexprAgg->Pop());
	if (!popAgg->FGlobal() || nullptr != popAgg->PdrgpcrMinimal() ||
		0 == pdrgpcrGroup->Size() ||
		!CColRef::Equals(popAgg->Pdrgpcr(), pdrgpcrGroup))
	{
		return nullptr;
	}

	CColRefSet *pcrsGroup = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsGroup->Include(pdrgpcrGroup);
	CColRefSet *pcrsCovered = GPOS_NEW(m_mp) CColRefSet(m_mp);
	CColRefSet *pcrsMinimal = GPOS_NEW(m_mp) CColRefSet(m_mp);
	CFunctionalDependencyArray *pdrgpfd =
		pexprAgg->DeriveFunctionalDependencies();
	for (ULONG ul = 0; nullptr != pdrgpfd && ul < pdrgpfd->Size(); ul++)
	{
		CFunctionalDependency *pfd = (*pdrgpfd)[ul];
		if (pfd->FIncluded(pcrsGroup))
		{
			pcrsCovered->Include(pfd->PcrsDetermined());
			pcrsCovered->Include(pfd->PcrsKey());
			pcrsMinimal->Include(pfd->PcrsKey());
		}
	}
	CColRefArray *pdrgpcrMinimal = nullptr;
	if (pcrsCovered->Equals(pcrsGroup) && 0 < pcrsMinimal->Size())
	{
		pdrgpcrMinimal = pcrsMinimal->Pdrgpcr(m_mp);
	}
	pcrsMinimal->Release();
	pcrsCovered->Release();
	pcrsGroup->Release();
	return pdrgpcrMinimal;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildCompute
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildCompute(const CDSLOp *pop,
								 const CDSLModel *pmodel) const
{
	if (1 != pop->UlChildren() || nullptr == pop->Pdrgpsym() ||
		3 != pop->Pdrgpsym()->Size())
	{
		return nullptr;
	}

	const CDSLSymbol *psymExpr = PsymResolve((*pop->Pdrgpsym())[0]);
	const CDSLSymbol *psymAttrs = PsymResolve((*pop->Pdrgpsym())[1]);
	const CDSLSymbol *psymSchema = PsymResolve((*pop->Pdrgpsym())[2]);
	CExpression *pexprList = PexprResolveExpr(psymExpr, pmodel);
	CColRefArray *pdrgpcrAttrs =
		PdrgpcrResolveCols(psymAttrs, pmodel);
	CColRefArray *pdrgpcrSchema =
		PdrgpcrResolveCols(psymSchema, pmodel);
	BOOL fOwnAttrs = false;
	BOOL fOwnSchema = false;
	if (nullptr != pexprList && nullptr == pdrgpcrAttrs)
	{
		pdrgpcrAttrs = pexprList->DeriveUsedColumns()->Pdrgpcr(m_mp);
		fOwnAttrs = true;
	}
	if (nullptr != pexprList && nullptr == pdrgpcrSchema &&
		COperator::EopScalarProjectList == pexprList->Pop()->Eopid())
	{
		pdrgpcrSchema = GPOS_NEW(m_mp) CColRefArray(m_mp);
		fOwnSchema = true;
		for (ULONG ul = 0; ul < pexprList->Arity(); ul++)
		{
			CExpression *pexprElem = (*pexprList)[ul];
			if (COperator::EopScalarProjectElement !=
				pexprElem->Pop()->Eopid())
			{
				pdrgpcrSchema->Release();
				pdrgpcrSchema = nullptr;
				fOwnSchema = false;
				break;
			}
			pdrgpcrSchema->Append(
				CScalarProjectElement::PopConvert(pexprElem->Pop())->Pcr());
		}
	}
	if (nullptr == pexprList || nullptr == pdrgpcrAttrs ||
		nullptr == pdrgpcrSchema ||
		COperator::EopScalarProjectList != pexprList->Pop()->Eopid() ||
		pexprList->Arity() != pdrgpcrSchema->Size())
	{
		if (fOwnAttrs)
		{
			pdrgpcrAttrs->Release();
		}
		if (fOwnSchema)
		{
			pdrgpcrSchema->Release();
		}
		CRefCount::SafeRelease(pexprList);
		return nullptr;
	}

	// Guard the three independently aliased target symbols against an invalid
	// combination. The expression artifact is authoritative: attrs must be its
	// exact dependency set and schema its ordered list of defined columns.
	CColRefSet *pcrsAttrs = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsAttrs->Include(pdrgpcrAttrs);
	if (!pcrsAttrs->Equals(pexprList->DeriveUsedColumns()))
	{
		pcrsAttrs->Release();
		if (fOwnAttrs)
		{
			pdrgpcrAttrs->Release();
		}
		if (fOwnSchema)
		{
			pdrgpcrSchema->Release();
		}
		pexprList->Release();
		return nullptr;
	}
	pcrsAttrs->Release();
	for (ULONG ul = 0; ul < pexprList->Arity(); ul++)
	{
		CExpression *pexprElem = (*pexprList)[ul];
		if (COperator::EopScalarProjectElement != pexprElem->Pop()->Eopid() ||
			CScalarProjectElement::PopConvert(pexprElem->Pop())->Pcr() !=
				(*pdrgpcrSchema)[ul])
		{
			if (fOwnAttrs)
			{
				pdrgpcrAttrs->Release();
			}
			if (fOwnSchema)
			{
				pdrgpcrSchema->Release();
			}
			pexprList->Release();
			return nullptr;
		}
	}

	CExpression *pexprChild = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprChild)
	{
		if (fOwnAttrs)
		{
			pdrgpcrAttrs->Release();
		}
		if (fOwnSchema)
		{
			pdrgpcrSchema->Release();
		}
		pexprList->Release();
		return nullptr;
	}
	if (!FColSetContainsArray(pexprChild->DeriveOutputColumns(), pdrgpcrAttrs) ||
		(m_prule->Pexprdefs()->FHasBindings() &&
		 (pexprList->DeriveDefinedColumns()->Size() != pexprList->Arity() ||
		  !pexprChild->DeriveOutputColumns()->IsDisjoint(pexprList->DeriveDefinedColumns()))))
	{
		pexprChild->Release();
		if (fOwnAttrs)
		{
			pdrgpcrAttrs->Release();
		}
		if (fOwnSchema)
		{
			pdrgpcrSchema->Release();
		}
		pexprList->Release();
		return nullptr;
	}

	if (fOwnAttrs)
	{
		pdrgpcrAttrs->Release();
	}
	if (fOwnSchema)
	{
		pdrgpcrSchema->Release();
	}
	return PexprProjectWithoutSelfAliases(m_mp, pexprChild, pexprList);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildProj
//
//	@doc:
//		Proj<a s>: rebuild the relational child and the SOURCE-matched project list.
//		The list's project-element operators (and therefore schema/output CColRefs)
//		stay unchanged, while scalar children are remapped positionally from the
//		source Proj attrs to the target Proj attrs. This implements proven equality
//		column substitutions such as projecting the other side of an inner-join
//		equality; merely grafting the old list would report "applied" without doing
//		the requested rewrite.
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildProj(const CDSLOp *pop,
								 const CDSLModel *pmodel) const
{
	if (1 != pop->UlChildren() || nullptr == pop->Pdrgpsym() ||
		(2 != pop->Pdrgpsym()->Size() &&
		 !(m_prule->Pexprdefs()->FHasBindings() && 3 == pop->Pdrgpsym()->Size())))
	{
		return nullptr;
	}

	CExpression *pexprChild = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprChild)
	{
		return nullptr;
	}

	const CDSLSymbol *psymAttrs = PsymResolve((*pop->Pdrgpsym())[0]);
	const CDSLSymbol *psymSchema = PsymResolve((*pop->Pdrgpsym())[1]);
	if (m_prule->Pexprdefs()->FHasBindings())
	{
		// Keep the source schema/dependency context, whether reusing a whole
		// capture or constructing independently typed SELECT items.
		const CDSLOp *source = PopSourceProjForSchema(
			m_prule->PfragSrc()->PopRoot(), psymSchema);
		CExpression *list = pmodel->PexprProjList(psymSchema);
		if (3 == pop->Pdrgpsym()->Size())
			list = PexprResolveExpr((*pop->Pdrgpsym())[2], pmodel);
		else if (nullptr != list)
			list->AddRef();
		CColRefArray *schema = PdrgpcrResolveCols(psymSchema, pmodel);
		CColRefArray *attrs = PdrgpcrResolveCols(psymAttrs, pmodel);
		CColRefArray *source_attrs = nullptr == source ? nullptr :
			PdrgpcrResolveCols((*source->Pdrgpsym())[0], pmodel);
		BOOL outputs_match = nullptr != list && nullptr != schema && list->Arity() == schema->Size();
		for (ULONG i = 0; outputs_match && i < schema->Size(); ++i)
		{
			CExpression *value = (*(*list)[i])[0];
			outputs_match = CScalarProjectElement::PopConvert((*list)[i]->Pop())->Pcr() == (*schema)[i] &&
				(!pexprChild->DeriveOutputColumns()->FMember((*schema)[i]) ||
				 (COperator::EopScalarIdent == value->Pop()->Eopid() &&
				  CScalarIdent::PopConvert(value->Pop())->Pcr() == (*schema)[i]));
		}
		if (!outputs_match || nullptr == attrs || nullptr == source_attrs ||
			!CColRef::Equals(attrs, source_attrs) ||
			!pexprChild->DeriveOutputColumns()->ContainsAll(list->DeriveUsedColumns()))
		{
			CRefCount::SafeRelease(list);
			pexprChild->Release();
			return nullptr;
		}
		if (pop->FDistinct() && 0 == schema->Size())
		{
			list->Release();
			pexprChild->Release();
			return nullptr;
		}
		CExpression *project = PexprProjectWithoutSelfAliases(m_mp, pexprChild, list);
		if (nullptr == project || !pop->FDistinct())
			return project;
		if (0 == (*project)[1]->Arity())
		{
			pexprChild = (*project)[0];
			pexprChild->AddRef();
			project->Release();
			project = pexprChild;
		}
		// Group by SELECT outputs, not its dependencies or pass-through columns.
		schema->AddRef();
		return GPOS_NEW(m_mp) CExpression(m_mp,
			GPOS_NEW(m_mp) CLogicalGbAgg(m_mp, schema, COperator::EgbaggtypeGlobal),
			project, GPOS_NEW(m_mp) CExpression(m_mp,
				GPOS_NEW(m_mp) CScalarProjectList(m_mp)));
	}
	if (pmodel->FVirtualIdentityProj(psymSchema) && !pop->FDistinct())
	{
		CColRefArray *pdrgpcrAttrs =
			PdrgpcrResolveCols(psymAttrs, pmodel);
		CColRefArray *pdrgpcrSchema =
			PdrgpcrResolveCols(psymSchema, pmodel);
		CColRefSet *pcrsRequired = GPOS_NEW(m_mp) CColRefSet(m_mp);
		if (nullptr != pdrgpcrSchema)
		{
			pcrsRequired->Include(pdrgpcrSchema);
		}
		const BOOL fIdentity = nullptr != pdrgpcrAttrs &&
			nullptr != pdrgpcrSchema &&
			CColRef::Equals(pdrgpcrAttrs, pdrgpcrSchema) &&
			pexprChild->DeriveOutputColumns()->ContainsAll(pcrsRequired);
		pcrsRequired->Release();
		if (!fIdentity)
		{
			pexprChild->Release();
			return nullptr;
		}
		return pexprChild;
	}
	CExpression *pexprAggShell = pmodel->PexprProjAggShell(psymSchema);
	if (nullptr != pexprAggShell)
	{
		const CDSLOp *popSourceProj = PopSourceProjForSchema(
			m_prule->PfragSrc()->PopRoot(), psymSchema);
		if (nullptr == popSourceProj || nullptr == popSourceProj->Pdrgpsym() ||
			2 != popSourceProj->Pdrgpsym()->Size())
		{
			pexprChild->Release();
			return nullptr;
		}
		CColRefArray *pdrgpcrSourceAttrs = PdrgpcrResolveCols(
			(*popSourceProj->Pdrgpsym())[0], pmodel);
		CColRefArray *pdrgpcrTargetAttrs =
			PdrgpcrResolveCols(psymAttrs, pmodel);
		if (nullptr == pdrgpcrSourceAttrs ||
			nullptr == pdrgpcrTargetAttrs ||
			pdrgpcrSourceAttrs->Size() != pdrgpcrTargetAttrs->Size())
		{
			pexprChild->Release();
			return nullptr;
		}

		UlongToColRefMap *colref_mapping =
			GPOS_NEW(m_mp) UlongToColRefMap(m_mp);
		CColRefSet *pcrsTarget = GPOS_NEW(m_mp) CColRefSet(m_mp);
		BOOL fValid = true;
		for (ULONG ul = 0; fValid && ul < pdrgpcrSourceAttrs->Size(); ul++)
		{
			CColRef *pcrSource = (*pdrgpcrSourceAttrs)[ul];
			CColRef *pcrTarget = (*pdrgpcrTargetAttrs)[ul];
			pcrsTarget->Include(pcrTarget);
			if (!pcrSource->RetrieveType()->MDId()->Equals(
					pcrTarget->RetrieveType()->MDId()) ||
				pcrSource->TypeModifier() != pcrTarget->TypeModifier())
			{
				fValid = false;
				break;
			}
			if (pcrSource == pcrTarget)
			{
				continue;
			}
			const ULONG ulSourceId = pcrSource->Id();
			CColRef *pcrExisting = colref_mapping->Find(&ulSourceId);
			if (nullptr != pcrExisting)
			{
				fValid = pcrExisting == pcrTarget;
				continue;
			}
			BOOL fInserted GPOS_ASSERTS_ONLY = colref_mapping->Insert(
				GPOS_NEW(m_mp) ULONG(ulSourceId), pcrTarget);
			GPOS_ASSERT(fInserted);
		}
		fValid = fValid &&
			pexprChild->DeriveOutputColumns()->ContainsAll(pcrsTarget);
		pcrsTarget->Release();
		if (!fValid)
		{
			colref_mapping->Release();
			pexprChild->Release();
			return nullptr;
		}

		CExpression *pexprRestored = PexprRestoreProjectAggShell(
			m_mp, pexprAggShell, pexprChild, colref_mapping);
		colref_mapping->Release();
		pexprChild->Release();
		return pexprRestored;
	}
	CExpression *pexprLimitShell =
		pmodel->PexprProjLimitShell(psymSchema);
	if (nullptr != pexprLimitShell)
	{
		CExpression *pexprWrapped =
			PexprRestoreLimitShell(m_mp, pexprLimitShell, pexprChild);
		pexprChild->Release();
		pexprChild = pexprWrapped;
	}

	// Proj* is ORCA's pure-dedup Global GbAgg. Unlike the special root-level
	// elimination rule, a Proj* nested in Union/Join must be rebuilt, not dropped.
	if (pop->FDistinct())
	{
		CColRefArray *pdrgpcrAttrsBound =
			PdrgpcrResolveCols(psymAttrs, pmodel);
		CColRefArray *pdrgpcrSchemaBound =
			PdrgpcrResolveCols(psymSchema, pmodel);
		if (nullptr == pdrgpcrAttrsBound || nullptr == pdrgpcrSchemaBound ||
			0 == pdrgpcrSchemaBound->Size() ||
			pdrgpcrAttrsBound->Size() != pdrgpcrSchemaBound->Size())
		{
			pexprChild->Release();
			return nullptr;
		}

		// A target projection can move from a SetOp output into one of its
		// branches. Resolve that positional edge before checking the concrete
		// child: output CColRefs are commonly the first branch's identities and
		// therefore cannot be used directly in later branches.
		CAutoRef<CColRefArray> aMappedAttrs;
		CAutoRef<CColRefArray> aMappedSchema;
		CColRefArray *pdrgpcrAttrs = pdrgpcrAttrsBound;
		CColRefArray *pdrgpcrSchema = pdrgpcrSchemaBound;
		CExpression *pexprBoundProjectList =
			pmodel->PexprProjList(psymSchema);
		const BOOL fMapSetOpPosition = nullptr == pexprBoundProjectList ||
			FProjectListIsColumnOnly(pexprBoundProjectList);
		if (fMapSetOpPosition &&
			!FColSetContainsArray(pexprChild->DeriveOutputColumns(),
								  pdrgpcrAttrsBound))
		{
			aMappedAttrs = PdrgpcrMapToTarget(
				(*pop)[0], pexprChild, pdrgpcrAttrsBound, pmodel);
			pdrgpcrAttrs = aMappedAttrs.Value();
		}
		if (fMapSetOpPosition &&
			!FColSetContainsArray(pexprChild->DeriveOutputColumns(),
								  pdrgpcrSchemaBound))
		{
			aMappedSchema = PdrgpcrMapToTarget(
				(*pop)[0], pexprChild, pdrgpcrSchemaBound, pmodel);
			pdrgpcrSchema = aMappedSchema.Value();
		}
		if (nullptr == pdrgpcrAttrs || nullptr == pdrgpcrSchema ||
			pdrgpcrAttrs->Size() != pdrgpcrSchema->Size())
		{
			pexprChild->Release();
			return nullptr;
		}

		// Proj* over a matched computed Proj means SELECT DISTINCT e(a), not
		// DISTINCT a. Rebuild the exact captured expression list first and group
		// by its output schema. attrs remain the expression dependencies while the
		// saved scalar tree remains the expression itself.
		CExpression *pexprBoundProjList =
			pmodel->PexprProjList(psymSchema);
		if (nullptr != pexprBoundProjList &&
			!FProjectListIsColumnOnly(pexprBoundProjList))
		{
			CExpression *pexprTargetProjList =
				PexprRemapProjectList(psymAttrs, psymSchema, pmodel);
			if (nullptr == pexprTargetProjList ||
				!pexprChild->DeriveOutputColumns()->ContainsAll(
					pexprTargetProjList->DeriveUsedColumns()))
			{
				CRefCount::SafeRelease(pexprTargetProjList);
				pexprChild->Release();
				return nullptr;
			}

			CExpression *pexprProject = PexprProjectWithoutSelfAliases(
				m_mp, pexprChild, pexprTargetProjList);
			if (nullptr == pexprProject)
			{
				return nullptr;
			}
			CColRefSet *pcrsSchema = GPOS_NEW(m_mp) CColRefSet(m_mp);
			pcrsSchema->Include(pdrgpcrSchema);
			const BOOL fSchemaProduced =
				pexprProject->DeriveOutputColumns()->ContainsAll(pcrsSchema);
			pcrsSchema->Release();
			if (!fSchemaProduced)
			{
				pexprProject->Release();
				return nullptr;
			}

			pdrgpcrSchema->AddRef();
			pdrgpcrSchema->AddRef();
			CExpression *pexprEmptyList = GPOS_NEW(m_mp) CExpression(
				m_mp, GPOS_NEW(m_mp) CScalarProjectList(m_mp),
				GPOS_NEW(m_mp) CExpressionArray(m_mp));
			return GPOS_NEW(m_mp) CExpression(
				m_mp,
				GPOS_NEW(m_mp) CLogicalGbAgg(
					m_mp, pdrgpcrSchema, pdrgpcrSchema,
					COperator::EgbaggtypeGlobal,
					false /* fGeneratesDuplicates */,
					nullptr /* pdrgpcrArgDQA */),
				pexprProject, pexprEmptyList);
		}

		CColRefSet *pcrsChild = pexprChild->DeriveOutputColumns();
		CColRefSet *pcrsGrouping = GPOS_NEW(m_mp) CColRefSet(m_mp);
		// The target attrs may deliberately name an equivalent join-key column
		// while SchemaEq keeps the source projection schema. Proj* has no scalar
		// project list in ORCA; its concrete operation is therefore grouping by
		// the resolved attrs. Requiring attrs == schema rejected precisely these
		// proven column-substitution rules before they could enter the memo.
		pcrsGrouping->Include(pdrgpcrAttrs);
		BOOL fValid = pcrsChild->ContainsAll(pcrsGrouping);
		if (!fValid)
		{
			pcrsGrouping->Release();
			pexprChild->Release();
			return nullptr;
		}
		for (ULONG ul = 0; ul < pdrgpcrSchema->Size(); ul++)
		{
			CColRef *pcrOutput = (*pdrgpcrSchema)[ul];
			CColRef *pcrInput = (*pdrgpcrAttrs)[ul];
			if (pcrOutput != pcrInput &&
				(!pcrOutput->RetrieveType()->MDId()->Equals(
					 pcrInput->RetrieveType()->MDId()) ||
				 pcrOutput->TypeModifier() != pcrInput->TypeModifier()))
			{
				pcrsGrouping->Release();
				pexprChild->Release();
				return nullptr;
			}
		}

		// DISTINCT is idempotent. Reuse an existing pure global dedup with the
		// same grouping set instead of manufacturing an indefinitely deep chain
		// when a bottom-up or Cascade rule reaches its own result again.
		if (COperator::EopLogicalGbAgg == pexprChild->Pop()->Eopid() &&
			2 == pexprChild->Arity() && 0 == (*pexprChild)[1]->Arity())
		{
			CLogicalGbAgg *popChildGbAgg =
				CLogicalGbAgg::PopConvert(pexprChild->Pop());
			CColRefSet *pcrsSchema = GPOS_NEW(m_mp) CColRefSet(m_mp);
			pcrsSchema->Include(pdrgpcrSchema);
			const BOOL fSameDedup = popChildGbAgg->FGlobal() &&
				FColArraysSameSet(m_mp, popChildGbAgg->Pdrgpcr(),
								 pdrgpcrAttrs) &&
				pexprChild->DeriveOutputColumns()->ContainsAll(pcrsSchema);
			pcrsSchema->Release();
			if (fSameDedup)
			{
				pcrsGrouping->Release();
				return pexprChild;
			}
		}

		// The rewritten key is already the rule-selected minimal grouping. Keep
		// that provenance on the generated GbAgg so the Proj* source matcher does
		// not consume its own result and alternate equivalent join keys forever.
		pdrgpcrAttrs->AddRef();
		pdrgpcrAttrs->AddRef();
		CExpression *pexprEmptyList = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CScalarProjectList(m_mp),
			GPOS_NEW(m_mp) CExpressionArray(m_mp));
		CExpression *pexprGbAgg = GPOS_NEW(m_mp) CExpression(
			m_mp,
			GPOS_NEW(m_mp) CLogicalGbAgg(
				m_mp, pdrgpcrAttrs, pdrgpcrAttrs,
				COperator::EgbaggtypeGlobal,
				false /* fGeneratesDuplicates */, nullptr /* pdrgpcrArgDQA */),
			pexprChild, pexprEmptyList);

		if (FColArraysSameSet(m_mp, pdrgpcrAttrs, pdrgpcrSchema))
		{
			pcrsGrouping->Release();
			return pexprGbAgg;
		}

		// Restore the source-visible schema after grouping on substituted keys.
		// CLogicalProject is compute-scalar (not column pruning), but that is
		// sufficient: the memo's required columns request the original schema and
		// the substituted grouping columns may remain as harmless extra outputs.
		CExpressionArray *pdrgpexprPrEl =
			GPOS_NEW(m_mp) CExpressionArray(m_mp);
		for (ULONG ul = 0; ul < pdrgpcrSchema->Size(); ul++)
		{
			CColRef *pcrOutput = (*pdrgpcrSchema)[ul];
			CColRef *pcrInput = (*pdrgpcrAttrs)[ul];
			if (pcrOutput == pcrInput || pcrsGrouping->FMember(pcrOutput))
			{
				continue;
			}
			pdrgpexprPrEl->Append(GPOS_NEW(m_mp) CExpression(
				m_mp, GPOS_NEW(m_mp) CScalarProjectElement(m_mp, pcrOutput),
				GPOS_NEW(m_mp) CExpression(
					m_mp, GPOS_NEW(m_mp) CScalarIdent(m_mp, pcrInput))));
		}
		if (0 == pdrgpexprPrEl->Size())
		{
			pcrsGrouping->Release();
			pdrgpexprPrEl->Release();
			return pexprGbAgg;
		}
		pcrsGrouping->Release();
		CExpression *pexprProjectList = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CScalarProjectList(m_mp), pdrgpexprPrEl);
		return GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalProject(m_mp), pexprGbAgg,
			pexprProjectList);
	}

	CExpression *pexprProjList = pmodel->PexprProjList(psymSchema);
	if (nullptr == pexprProjList)
	{
		// ORCA has no column-pruning logical Project. A target Proj without a
		// matched scalar list is therefore a pass-through view whenever its exact
		// attrs/schema are already produced by the child. This covers both a
		// Proj* -> Proj dedup drop and projections that hide generated columns.
		CColRefArray *pdrgpcrAttrs =
			PdrgpcrResolveCols(psymAttrs, pmodel);
		CColRefArray *pdrgpcrSchema =
			PdrgpcrResolveCols(psymSchema, pmodel);
		if (nullptr == pdrgpcrAttrs || nullptr == pdrgpcrSchema ||
			0 == pdrgpcrSchema->Size() ||
			!FColArraysSameSet(m_mp, pdrgpcrAttrs, pdrgpcrSchema))
		{
			pexprChild->Release();
			return nullptr;
		}

		CColRefSet *pcrsSchema = GPOS_NEW(m_mp) CColRefSet(m_mp);
		pcrsSchema->Include(pdrgpcrSchema);
		const BOOL fContains =
			pexprChild->DeriveOutputColumns()->ContainsAll(pcrsSchema);
		pcrsSchema->Release();
		if (!fContains)
		{
			pexprChild->Release();
			return nullptr;
		}
		return pexprChild;
	}

	CExpression *pexprTargetProjList =
		PexprRemapProjectList(psymAttrs, psymSchema, pmodel);
	if (nullptr == pexprTargetProjList)
	{
		pexprChild->Release();
		return nullptr;
	}

	if (!pexprChild->DeriveOutputColumns()->ContainsAll(
			pexprTargetProjList->DeriveUsedColumns()))
	{
		pexprTargetProjList->Release();
		pexprChild->Release();
		return nullptr;
	}

	return PexprProjectWithoutSelfAliases(m_mp, pexprChild, pexprTargetProjList);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLInstantiator::PexprBuildAgg
//---------------------------------------------------------------------------
CExpression *
CDSLInstantiator::PexprBuildAgg(const CDSLOp *pop,
								const CDSLModel *pmodel) const
{
	if (1 != pop->UlChildren())
	{
		return nullptr;
	}
	CDSLSymbolArray *pdrgpsym = pop->Pdrgpsym();
	if (nullptr == pdrgpsym ||
		(5 != pdrgpsym->Size() && 6 != pdrgpsym->Size()))
	{
		return nullptr;
	}
	const BOOL fLegacy = 5 == pdrgpsym->Size();
	const ULONG ulFunc = fLegacy ? 2 : 3;
	const ULONG ulSchema = fLegacy ? 3 : 4;
	const ULONG ulHaving = fLegacy ? 4 : 5;

	const CDSLSymbol *psymGroup = PsymResolve((*pdrgpsym)[0]);
	const CDSLSymbol *psymAggInputs = PsymResolve((*pdrgpsym)[1]);
	const CDSLSymbol *psymFuncs = PsymResolve((*pdrgpsym)[ulFunc]);
	const CDSLSymbol *psymSchema = PsymResolve((*pdrgpsym)[ulSchema]);
	const CDSLSymbol *psymHaving = PsymResolve((*pdrgpsym)[ulHaving]);

	CColRefArray *pdrgpcrGroup =
		PdrgpcrResolveCols(psymGroup, pmodel);
	CColRefArray *pdrgpcrAggInputs =
		PdrgpcrResolveCols(psymAggInputs, pmodel);
	CExpressionArray *pdrgpexprFuncs = pmodel->PdrgpexprFunc(psymFuncs);
	CColRefArray *pdrgpcrSchema =
		PdrgpcrResolveCols(psymSchema, pmodel);
	if (nullptr == pdrgpcrGroup || nullptr == pdrgpcrAggInputs ||
		nullptr == pdrgpexprFuncs || nullptr == pdrgpcrSchema)
	{
		return nullptr;
	}

	// The repository's established Agg<a a f s p> format has no explicit
	// aggregate-output symbol. In a GbAgg schema, grouping columns are passed
	// through and every remaining schema column is defined by one aggregate
	// project element, so recover the output array as schema - groupByAttrs.
	CColRefArray *pdrgpcrAggOutputs = nullptr;
	BOOL fOwnAggOutputs = false;
	if (fLegacy)
	{
		fOwnAggOutputs = true;
		pdrgpcrAggOutputs = GPOS_NEW(m_mp) CColRefArray(m_mp);
		CColRefSet *pcrsGroup = GPOS_NEW(m_mp) CColRefSet(m_mp);
		pcrsGroup->Include(pdrgpcrGroup);
		for (ULONG ul = 0; ul < pdrgpcrSchema->Size(); ul++)
		{
			CColRef *pcr = (*pdrgpcrSchema)[ul];
			if (!pcrsGroup->FMember(pcr))
			{
				pdrgpcrAggOutputs->Append(pcr);
			}
		}
		pcrsGroup->Release();
	}
	else
	{
		const CDSLSymbol *psymAggOutputs = PsymResolve((*pdrgpsym)[2]);
		pdrgpcrAggOutputs = PdrgpcrResolveCols(psymAggOutputs, pmodel);
	}
	if (nullptr == pdrgpcrAggOutputs ||
		pdrgpcrAggOutputs->Size() != pdrgpexprFuncs->Size())
	{
		if (fOwnAggOutputs)
		{
			pdrgpcrAggOutputs->Release();
		}
		return nullptr;
	}

	CExpression *pexprChild = PexprBuild((*pop)[0], pmodel);
	if (nullptr == pexprChild)
	{
		if (fOwnAggOutputs)
		{
			pdrgpcrAggOutputs->Release();
		}
		return nullptr;
	}

	CColRefSet *pcrsChild = pexprChild->DeriveOutputColumns();
	CColRefSet *pcrsGroup = GPOS_NEW(m_mp) CColRefSet(m_mp);
	CColRefSet *pcrsFuncInputs = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsGroup->Include(pdrgpcrGroup);
	for (ULONG ul = 0; ul < pdrgpexprFuncs->Size(); ul++)
	{
		CExpression *pexprFunc = (*pdrgpexprFuncs)[ul];
		if (COperator::EopScalarAggFunc != pexprFunc->Pop()->Eopid() ||
			!FAggFuncMatches(
				m_mp, pop,
				CScalarAggFunc::PopConvert(pexprFunc->Pop())))
		{
			pcrsGroup->Release();
			pcrsFuncInputs->Release();
			pexprChild->Release();
			if (fOwnAggOutputs)
			{
				pdrgpcrAggOutputs->Release();
			}
			return nullptr;
		}
		pcrsFuncInputs->Include(pexprFunc->DeriveUsedColumns());
	}

	CColRefArray *pdrgpcrActualInputs = pcrsFuncInputs->Pdrgpcr(m_mp);
	BOOL fInputsValid = FColArraysSameSet(
		m_mp, pdrgpcrAggInputs, pdrgpcrActualInputs);
	pdrgpcrActualInputs->Release();

	CColRefSet *pcrsExpectedSchema = GPOS_NEW(m_mp) CColRefSet(m_mp);
	CColRefSet *pcrsSchema = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsExpectedSchema->Include(pdrgpcrGroup);
	pcrsExpectedSchema->Include(pdrgpcrAggOutputs);
	pcrsSchema->Include(pdrgpcrSchema);
	BOOL fSchemaValid = pcrsExpectedSchema->Equals(pcrsSchema);

	BOOL fColumnsValid = fInputsValid && fSchemaValid &&
					 pcrsChild->ContainsAll(pcrsGroup) &&
					 pcrsChild->ContainsAll(pcrsFuncInputs);
	pcrsGroup->Release();
	pcrsFuncInputs->Release();
	pcrsExpectedSchema->Release();
	pcrsSchema->Release();
	if (!fColumnsValid)
	{
		pexprChild->Release();
		if (fOwnAggOutputs)
		{
			pdrgpcrAggOutputs->Release();
		}
		return nullptr;
	}

	CExpressionArray *pdrgpexprPrEl =
		GPOS_NEW(m_mp) CExpressionArray(m_mp);
	for (ULONG ul = 0; ul < pdrgpexprFuncs->Size(); ul++)
	{
		CExpression *pexprFunc = (*pdrgpexprFuncs)[ul];
		pexprFunc->AddRef();
		pdrgpexprPrEl->Append(GPOS_NEW(m_mp) CExpression(
			m_mp,
			GPOS_NEW(m_mp) CScalarProjectElement(
				m_mp, (*pdrgpcrAggOutputs)[ul]),
			pexprFunc));
	}
	CExpression *pexprAggList = GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CScalarProjectList(m_mp), pdrgpexprPrEl);

	// Rebuild from the semantic (full) grouping set. Preserve minimal-grouping
	// metadata only when the target keeps both that set and the relational child;
	// otherwise it was derived for an old child and may no longer be valid.
	CColRefArray *pdrgpcrMinimal =
		PdrgpcrMinimalGrouping(psymGroup, psymSchema, pmodel);
	CExpression *pexprSourceAgg = pmodel->PexprAggBinding(psymSchema);
	if (nullptr == pdrgpcrMinimal && nullptr != pexprSourceAgg &&
		COperator::EopLogicalGbAgg == pexprSourceAgg->Pop()->Eopid())
	{
		CLogicalGbAgg *popSourceAgg =
			CLogicalGbAgg::PopConvert(pexprSourceAgg->Pop());
		if (nullptr != popSourceAgg->PdrgpcrMinimal() &&
			CColRef::Equals(popSourceAgg->Pdrgpcr(), pdrgpcrGroup) &&
			((*pexprSourceAgg)[0] == pexprChild ||
			 (*pexprSourceAgg)[0]->Matches(pexprChild)))
		{
			pdrgpcrMinimal = popSourceAgg->PdrgpcrMinimal();
			pdrgpcrMinimal->AddRef();
		}
	}
	pdrgpcrGroup->AddRef();
	CLogicalGbAgg *popTargetAgg = nullptr;
	if (nullptr == pdrgpcrMinimal)
	{
		popTargetAgg = GPOS_NEW(m_mp) CLogicalGbAgg(
			m_mp, pdrgpcrGroup, COperator::EgbaggtypeGlobal);
	}
	else
	{
		popTargetAgg = GPOS_NEW(m_mp) CLogicalGbAgg(
			m_mp, pdrgpcrGroup, pdrgpcrMinimal,
			COperator::EgbaggtypeGlobal);
	}
	CExpression *pexprResult = GPOS_NEW(m_mp) CExpression(
		m_mp, popTargetAgg, pexprChild, pexprAggList);

	// HAVING is evaluated on aggregate outputs. Resolve the same typed binding
	// program as Filter, and reject references outside the constructed schema.
	CExpression *pexprHaving = PexprResolvePredicate(psymHaving, pmodel);
	if (nullptr == pexprHaving ||
		!pexprResult->DeriveOutputColumns()->ContainsAll(pexprHaving->DeriveUsedColumns()))
	{
		CRefCount::SafeRelease(pexprHaving);
		pexprResult->Release();
		if (fOwnAggOutputs) pdrgpcrAggOutputs->Release();
		return nullptr;
	}
	if (!CUtils::FScalarConstTrue(pexprHaving))
	{
		pexprResult = GPOS_NEW(m_mp) CExpression(
			m_mp, GPOS_NEW(m_mp) CLogicalSelect(m_mp), pexprResult,
			pexprHaving);
	}
	else
	{
		pexprHaving->Release();
	}
	if (fOwnAggOutputs)
	{
		pdrgpcrAggOutputs->Release();
	}
	return pexprResult;
}
