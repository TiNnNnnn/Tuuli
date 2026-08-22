//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLProjMatcher.cpp
//
//	@doc:
//		Implementation of the Proj symbol binder (see CDSLProjMatcher.h). Migrates
//		the SEMANTICS of WeTune's Proj match: bind the projected-column symbols,
//		recurse the relational child.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLProjMatcher.h"

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CDSLProjMatcher::FMatchTrivialSelectOverDedup
//
//	@doc:
//		PexprInstantiate represents a removed DISTINCT as Select(child, TRUE),
//		because a bare child cannot safely be inserted as a new memo alternative.
//		That Select is also an identity projection of the pure-dedup child's full
//		(grouping-only) output. Expose the ordinary Proj view here so DSL results
//		can feed a later Proj(Proj*) rule. A non-TRUE predicate, aggregate-bearing
//		GbAgg, split stage, or FD/minimal-generated GbAgg remains ineligible.
//---------------------------------------------------------------------------
BOOL
CDSLProjMatcher::FMatchTrivialSelectOverDedup(const CDSLOp *popProj,
								  CExpression *pexprSelect,
								  CDSLModel *pmodel) const
{
	if (COperator::EopLogicalSelect != pexprSelect->Pop()->Eopid() ||
		2 != pexprSelect->Arity() ||
		!CUtils::FScalarConstTrue((*pexprSelect)[1]))
	{
		return false;
	}

	CExpression *pexprDedup = (*pexprSelect)[0];
	if (COperator::EopLogicalGbAgg != pexprDedup->Pop()->Eopid() ||
		2 != pexprDedup->Arity() || 0 != (*pexprDedup)[1]->Arity())
	{
		return false;
	}
	CLogicalGbAgg *popGbAgg =
		CLogicalGbAgg::PopConvert(pexprDedup->Pop());
	if (COperator::EgbaggtypeGlobal != popGbAgg->Egbaggtype() ||
		nullptr != popGbAgg->PdrgpcrMinimal() ||
		nullptr == popGbAgg->Pdrgpcr() || 0 == popGbAgg->Pdrgpcr()->Size() ||
		1 != popProj->UlChildren())
	{
		return false;
	}

	CDSLSymbolArray *pdrgpsym = popProj->Pdrgpsym();
	if (nullptr == pdrgpsym || 2 != pdrgpsym->Size())
	{
		return false;
	}
	CColRefArray *pdrgpcrIdentity = GPOS_NEW(m_mp) CColRefArray(m_mp);
	for (ULONG ul = 0; ul < popGbAgg->Pdrgpcr()->Size(); ul++)
	{
		pdrgpcrIdentity->Append((*popGbAgg->Pdrgpcr())[ul]);
	}
	BOOL fBound = pmodel->FBind((*pdrgpsym)[0], pdrgpcrIdentity) &&
				  pmodel->FBind((*pdrgpsym)[1], pdrgpcrIdentity);
	pdrgpcrIdentity->Release();
	return fBound &&
		   m_pmatcher->FMatch((*popProj)[0], pexprDedup, pmodel);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLProjMatcher::PdrgpcrProjected
//
//	@doc:
//		Collect the CColRef each CScalarProjectElement defines, in list order.
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//	@function:
//		CDSLProjMatcher::PdrgpcrSchema
//
//	@doc:
//		Collect the CColRef each CScalarProjectElement DEFINES (its output
//		column), in list order. This is WeTune's `outValues` = valuesOf(projNode),
//		bound to the schema symbol <s> (see Match.matchProj).
//---------------------------------------------------------------------------
CColRefArray *
CDSLProjMatcher::PdrgpcrSchema(CExpression *pexprProjList) const
{
	if (nullptr == pexprProjList ||
		COperator::EopScalarProjectList != pexprProjList->Pop()->Eopid())
	{
		return nullptr;
	}

	CColRefArray *pdrgpcr = GPOS_NEW(m_mp) CColRefArray(m_mp);
	const ULONG ulElems = pexprProjList->Arity();
	for (ULONG ul = 0; ul < ulElems; ul++)
	{
		CExpression *pexprElem = (*pexprProjList)[ul];
		if (COperator::EopScalarProjectElement != pexprElem->Pop()->Eopid())
		{
			pdrgpcr->Release();
			return nullptr;
		}
		CScalarProjectElement *popElem =
			CScalarProjectElement::PopConvert(pexprElem->Pop());
		pdrgpcr->Append(popElem->Pcr());
	}
	return pdrgpcr;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLProjMatcher::PdrgpcrAttrs
//
//	@doc:
//		Collect the columns the project elements' value expressions REFERENCE
//		(their input dependencies), de-duplicated, in first-seen order. This is
//		WeTune's `inValues` = flatMap(attrExprs, valueRefsOf), bound to the attrs
//		symbol <a> (see Match.matchProj). For a plain pass-through projection this
//		equals the schema columns; for a COMPUTED column (e.g. cname||'x') it is
//		the underlying column(s) the expression reads (cname) — which is what
//		AttrsSub(a,t) must test, NOT the freshly-defined output column.
//---------------------------------------------------------------------------
CColRefArray *
CDSLProjMatcher::PdrgpcrAttrs(CExpression *pexprProjList) const
{
	if (nullptr == pexprProjList ||
		COperator::EopScalarProjectList != pexprProjList->Pop()->Eopid())
	{
		return nullptr;
	}

	CColRefSet *pcrsSeen = GPOS_NEW(m_mp) CColRefSet(m_mp);
	CColRefArray *pdrgpcr = GPOS_NEW(m_mp) CColRefArray(m_mp);
	const ULONG ulElems = pexprProjList->Arity();
	for (ULONG ul = 0; ul < ulElems; ul++)
	{
		CExpression *pexprElem = (*pexprProjList)[ul];
		if (COperator::EopScalarProjectElement != pexprElem->Pop()->Eopid())
		{
			pcrsSeen->Release();
			pdrgpcr->Release();
			return nullptr;
		}
		// the value expression is the project element's scalar child.
		CColRefSet *pcrsUsed = (*pexprElem)[0]->DeriveUsedColumns();
		CColRefSetIter crsi(*pcrsUsed);
		while (crsi.Advance())
		{
			CColRef *pcr = crsi.Pcr();
			if (!pcrsSeen->FMember(pcr))
			{
				pcrsSeen->Include(pcr);
				pdrgpcr->Append(pcr);
			}
		}
	}
	pcrsSeen->Release();
	return pdrgpcr;
}

BOOL
CDSLProjMatcher::FMatchProjectOverAgg(const CDSLOp *popProj,
								  CExpression *pexprProject,
								  CDSLModel *pmodel) const
{
	if (1 != popProj->UlChildren() ||
		EdslopInput == (*popProj)[0]->Edslop() ||
		EdslopProj == (*popProj)[0]->Edslop() ||
		EdslopAgg == (*popProj)[0]->Edslop() ||
		COperator::EopLogicalProject != pexprProject->Pop()->Eopid() ||
		2 != pexprProject->Arity() ||
		COperator::EopLogicalGbAgg != (*pexprProject)[0]->Pop()->Eopid())
	{
		return false;
	}

	CDSLSymbolArray *pdrgpsym = popProj->Pdrgpsym();
	if (nullptr == pdrgpsym || 2 != pdrgpsym->Size())
	{
		return false;
	}

	// A split aggregate is a unary Global(Local(...)) chain. Collect every
	// grouping column and scalar dependency, then retain only columns produced by
	// the deepest relational input. Intermediate aggregate outputs disappear in
	// that intersection; genuine input arguments remain.
	CColRefSet *pcrsRequired = GPOS_NEW(m_mp) CColRefSet(m_mp);
	CExpression *pexprRel = (*pexprProject)[0];
	ULONG ulAggNodes = 0;
	while (COperator::EopLogicalGbAgg == pexprRel->Pop()->Eopid() &&
		   2 == pexprRel->Arity())
	{
		CLogicalGbAgg *popGbAgg =
			CLogicalGbAgg::PopConvert(pexprRel->Pop());
		// Local aggregates are optimizer-generated implementation alternatives.
		// Re-inserting a captured Global(Local(...)) chain as a fresh logical
		// result exposes Local GbAgg to exploration xforms that accept only Global.
		// Match the canonical unsplit Global shell; native splitting can happen
		// again after the rewritten input enters the memo.
		if (0 < ulAggNodes ||
			COperator::EgbaggtypeGlobal != popGbAgg->Egbaggtype())
		{
			pcrsRequired->Release();
			return false;
		}
		ulAggNodes++;
		if (nullptr != popGbAgg->Pdrgpcr())
		{
			pcrsRequired->Include(popGbAgg->Pdrgpcr());
		}
		pcrsRequired->Include((*pexprRel)[1]->DeriveUsedColumns());
		pexprRel = (*pexprRel)[0];
	}
	pcrsRequired->Intersection(pexprRel->DeriveOutputColumns());
	CColRefArray *pdrgpcrRequired = pcrsRequired->Pdrgpcr(m_mp);
	pcrsRequired->Release();

	const CDSLSymbol *psymAttrs = (*pdrgpsym)[0];
	const CDSLSymbol *psymSchema = (*pdrgpsym)[1];
	BOOL fMatched = pmodel->FBind(psymAttrs, pdrgpcrRequired) &&
		pmodel->FBind(psymSchema, pdrgpcrRequired) &&
		m_pmatcher->FMatch((*popProj)[0], pexprRel, pmodel);
	pdrgpcrRequired->Release();
	if (!fMatched)
	{
		return false;
	}

	pexprProject->AddRef();
	return pmodel->FSetProjAggShell(psymSchema, pexprProject);
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLProjMatcher::FMatch
//---------------------------------------------------------------------------
BOOL
CDSLProjMatcher::FMatch(const CDSLOp *popProj, CExpression *pexprProject,
						CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != popProj);
	GPOS_ASSERT(EdslopProj == popProj->Edslop());
	GPOS_ASSERT(nullptr != pexprProject);

	if (COperator::EopLogicalSelect == pexprProject->Pop()->Eopid())
	{
		return FMatchTrivialSelectOverDedup(popProj, pexprProject, pmodel);
	}

	// the live node must be a Project carrying (relational child, project list).
	if (COperator::EopLogicalProject != pexprProject->Pop()->Eopid() ||
		2 != pexprProject->Arity())
	{
		return false;
	}
	if (1 == popProj->UlChildren() &&
		EdslopInput != (*popProj)[0]->Edslop() &&
		EdslopProj != (*popProj)[0]->Edslop() &&
		EdslopAgg != (*popProj)[0]->Edslop() &&
		COperator::EopLogicalGbAgg == (*pexprProject)[0]->Pop()->Eopid())
	{
		return FMatchProjectOverAgg(popProj, pexprProject, pmodel);
	}

	// Proj schema is <a s> — attrs first, schema second (validated at parse).
	CDSLSymbolArray *pdrgpsym = popProj->Pdrgpsym();
	if (nullptr == pdrgpsym || 2 != pdrgpsym->Size())
	{
		return false;
	}
	const CDSLSymbol *psymAttrs = (*pdrgpsym)[0];
	const CDSLSymbol *psymSchema = (*pdrgpsym)[1];

	// bind <a> and <s> per WeTune Match.matchProj:
	//   <a> attrs  = columns the projection expressions REFERENCE (valueRefsOf)
	//   <s> schema = columns the projection DEFINES / outputs (valuesOf)
	// These differ for computed columns; conflating them (the old code bound both
	// to the defined column) made AttrsSub(a,t) test the wrong set and wrongly
	// rejected any rule over a computed projection. FBind AddRefs; release locals.
	CColRefArray *pdrgpcrAttrs = PdrgpcrAttrs((*pexprProject)[1]);
	CColRefArray *pdrgpcrSchema = PdrgpcrSchema((*pexprProject)[1]);
	if (nullptr == pdrgpcrAttrs || nullptr == pdrgpcrSchema)
	{
		CRefCount::SafeRelease(pdrgpcrAttrs);
		CRefCount::SafeRelease(pdrgpcrSchema);
		return false;
	}

	BOOL fBound = pmodel->FBind(psymAttrs, pdrgpcrAttrs) &&
				  pmodel->FBind(psymSchema, pdrgpcrSchema);
	pdrgpcrAttrs->Release();
	pdrgpcrSchema->Release();
	if (!fBound)
	{
		return false;
	}

	// PostgreSQL keeps the target-list Project above ORDER/LIMIT, whereas WeTune's
	// tree exposes the Project directly above the relation to be rewritten. Peel
	// an unmentioned Limit chain only for matching and retain its root in the
	// model; the instantiator restores the exact operators, order and scalars.
	if (1 != popProj->UlChildren())
	{
		return false;
	}
	CExpression *pexprRel = (*pexprProject)[0];
	CExpression *pexprLimitShell = nullptr;
	if (EdslopLimit != (*popProj)[0]->Edslop() &&
		EdslopSort != (*popProj)[0]->Edslop())
	{
		while (COperator::EopLogicalLimit == pexprRel->Pop()->Eopid() &&
			   3 == pexprRel->Arity())
		{
			if (nullptr == pexprLimitShell)
			{
				pexprLimitShell = pexprRel;
			}
			pexprRel = (*pexprRel)[0];
		}
	}
	if (!m_pmatcher->FMatch((*popProj)[0], pexprRel, pmodel))
	{
		return false;
	}
	if (nullptr != pexprLimitShell)
	{
		pexprLimitShell->AddRef();
		if (!pmodel->FSetProjLimitShell(psymSchema, pexprLimitShell))
		{
			return false;
		}
	}

	// record the whole project-list subtree so the instantiator can graft it back
	// (it carries computed-column value subtrees the attrs/schema symbols do not).
	CExpression *pexprProjList = (*pexprProject)[1];
	pexprProjList->AddRef();
	return pmodel->FSetProjList(psymSchema, pexprProjList);
}

// EOF
