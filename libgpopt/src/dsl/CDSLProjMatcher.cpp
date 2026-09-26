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
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/dsl/CDSLExpressionDefinitions.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CDSLProjMatcher::FMatchCompute
//---------------------------------------------------------------------------
BOOL
CDSLProjMatcher::FMatchCompute(const CDSLOp *popCompute,
							   CExpression *pexprProject,
							   CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != popCompute);
	GPOS_ASSERT(EdslopCompute == popCompute->Edslop());
	GPOS_ASSERT(nullptr != pexprProject);

	if (COperator::EopLogicalProject != pexprProject->Pop()->Eopid() ||
		2 != pexprProject->Arity() || 1 != popCompute->UlChildren() ||
		nullptr == popCompute->Pdrgpsym() ||
		3 != popCompute->Pdrgpsym()->Size() ||
		COperator::EopScalarProjectList != (*pexprProject)[1]->Pop()->Eopid())
	{
		return false;
	}

	CDSLSymbolArray *pdrgpsym = popCompute->Pdrgpsym();
	CExpression *pexprList = (*pexprProject)[1];
	const BOOL exact = nullptr != m_pmatcher->Prule() &&
		m_pmatcher->Prule()->Pexprdefs()->FHasBindings();
	// One input scope with fresh output columns; scalar certificates do not
	// cover SRFs, correlated subqueries or implicit sibling dependencies.
	if (exact && (pexprList->DeriveHasNonScalarFunction() ||
		pexprList->DeriveDefinedColumns()->Size() != pexprList->Arity() ||
		!(*pexprProject)[0]->DeriveOutputColumns()->IsDisjoint(pexprList->DeriveDefinedColumns()) ||
		!(*pexprProject)[0]->DeriveOutputColumns()->ContainsAll(pexprList->DeriveUsedColumns())))
	{
		return false;
	}
	CColRefArray *pdrgpcrAttrs = PdrgpcrAttrs(pexprList);
	CColRefArray *pdrgpcrSchema = PdrgpcrSchema(pexprList);
	if (nullptr == pdrgpcrAttrs || nullptr == pdrgpcrSchema)
	{
		CRefCount::SafeRelease(pdrgpcrAttrs);
		CRefCount::SafeRelease(pdrgpcrSchema);
		return false;
	}

	const BOOL fBound = m_pmatcher->FMatchExpression((*pdrgpsym)[0], pexprList, pmodel) &&
		pmodel->FBind((*pdrgpsym)[1], pdrgpcrAttrs) &&
		pmodel->FBind((*pdrgpsym)[2], pdrgpcrSchema);
	pdrgpcrAttrs->Release();
	pdrgpcrSchema->Release();
	return fBound &&
		m_pmatcher->FMatch((*popCompute)[0], (*pexprProject)[0], pmodel);
}

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
	CExpression *pexprDedup = nullptr;
	CColRefArray *pdrgpcrGrouping = nullptr;
	if (!CDSLMatchView::FDedupIdentity(
			pexprSelect, &pexprDedup, &pdrgpcrGrouping) ||
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
	for (ULONG ul = 0; ul < pdrgpcrGrouping->Size(); ul++)
	{
		pdrgpcrIdentity->Append((*pdrgpcrGrouping)[ul]);
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

BOOL
CDSLProjMatcher::FMatchIdentityOverInSub(const CDSLOp *popProj,
									 CExpression *pexprCarrier,
									 CDSLModel *pmodel) const
{
	if (1 != popProj->UlChildren() ||
		EdslopInSubFilter != (*popProj)[0]->Edslop())
	{
		return false;
	}
	CDSLSymbolArray *pdrgpsym = popProj->Pdrgpsym();
	if (nullptr == pdrgpsym || 2 != pdrgpsym->Size())
	{
		return false;
	}

	CColRefArray *pdrgpcrOutput =
		pexprCarrier->DeriveOutputColumns()->Pdrgpcr(m_mp);
	BOOL fMatched = 0 < pdrgpcrOutput->Size() &&
		pmodel->FBind((*pdrgpsym)[0], pdrgpcrOutput) &&
		pmodel->FBind((*pdrgpsym)[1], pdrgpcrOutput) &&
		m_pmatcher->FMatch((*popProj)[0], pexprCarrier, pmodel);
	pdrgpcrOutput->Release();
	if (!fMatched)
	{
		return false;
	}

	pexprCarrier->AddRef();
	return pmodel->FSetVirtualIdentityProj((*pdrgpsym)[1], pexprCarrier);
}

BOOL
CDSLProjMatcher::FMatchScalarSubqueryProject(const CDSLOp *popProj,
										 CExpression *pexprProject,
										 CDSLModel *pmodel) const
{
	CExpression *pexprLowered =
		CDSLMatchView::PexprLowerSubqueries(
			m_mp, pexprProject, false /*fEnforceCorrelatedApply*/,
			false /*fScalarOnly*/);
	const EDslOpKind edslopChild = 1 == popProj->UlChildren()
		? (*popProj)[0]->Edslop()
		: EdslopSentinel;
	const COperator::EOperatorId eopidLoweredChild =
		(nullptr != pexprLowered &&
		 COperator::EopLogicalProject == pexprLowered->Pop()->Eopid() &&
		 2 == pexprLowered->Arity())
		? (*pexprLowered)[0]->Pop()->Eopid()
		: COperator::EopSentinel;
	const BOOL fDirectInnerApply = EdslopInnerApply == edslopChild &&
		(COperator::EopLogicalInnerApply == eopidLoweredChild ||
		 COperator::EopLogicalInnerCorrelatedApply == eopidLoweredChild);
	const BOOL fDirectLeftApply = EdslopLeftOuterApply == edslopChild &&
		(COperator::EopLogicalLeftOuterApply == eopidLoweredChild ||
		 COperator::EopLogicalLeftOuterCorrelatedApply == eopidLoweredChild);
	if ((EdslopInnerApply == edslopChild ||
		 EdslopLeftOuterApply == edslopChild) &&
		!fDirectInnerApply && !fDirectLeftApply)
	{
		// The non-enforced handler may choose a semantically equivalent shell,
		// such as Project(Project(LeftApply)) for count-zero compensation. ORCA
		// also produces an enforced-correlated alternative. Select that existing
		// alternative when it is the one whose root shape the DSL names, before
		// any symbols are bound to the model.
		CRefCount::SafeRelease(pexprLowered);
		pexprLowered = CDSLMatchView::PexprLowerSubqueries(
			m_mp, pexprProject, true /*fEnforceCorrelatedApply*/,
			false /*fScalarOnly*/);
	}
	if (nullptr == pexprLowered)
	{
		return false;
	}
	BOOL fMatched = m_pmatcher->FMatch(popProj, pexprLowered, pmodel);
	pexprLowered->Release();
	return fMatched;
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

	const BOOL exact = nullptr != m_pmatcher->Prule() &&
		m_pmatcher->Prule()->Pexprdefs()->FHasBindings();
	// Native expression certificates describe the actual Project and its child,
	// not the legacy compatibility views. Set-returning items are not scalar values.
	if (exact && (COperator::EopLogicalProject != pexprProject->Pop()->Eopid() ||
		2 != pexprProject->Arity() ||
		COperator::EopScalarProjectList != (*pexprProject)[1]->Pop()->Eopid() ||
		(*pexprProject)[1]->DeriveHasNonScalarFunction()))
	{
		return false;
	}
	if (COperator::EopLogicalSelect == pexprProject->Pop()->Eopid())
	{
		return FMatchTrivialSelectOverDedup(popProj, pexprProject, pmodel);
	}
	if (!exact && COperator::EopLogicalProject == pexprProject->Pop()->Eopid() &&
		2 == pexprProject->Arity() &&
		(*pexprProject)[1]->DeriveHasSubquery())
	{
		return FMatchScalarSubqueryProject(popProj, pexprProject, pmodel);
	}

	// the live node must be a Project carrying (relational child, project list).
	if (COperator::EopLogicalProject != pexprProject->Pop()->Eopid() ||
		2 != pexprProject->Arity())
	{
		return COperator::EopLogicalLeftSemiJoin ==
				   pexprProject->Pop()->Eopid() &&
			   FMatchIdentityOverInSub(popProj, pexprProject, pmodel);
	}
	if (!exact && 1 == popProj->UlChildren() &&
		EdslopInput != (*popProj)[0]->Edslop() &&
		EdslopProj != (*popProj)[0]->Edslop() &&
		EdslopAgg != (*popProj)[0]->Edslop() &&
		COperator::EopLogicalGbAgg == (*pexprProject)[0]->Pop()->Eopid())
	{
		return FMatchProjectOverAgg(popProj, pexprProject, pmodel);
	}

	// Proj<a s [e]> exposes an optional exact, ordered SELECT capture.
	CDSLSymbolArray *pdrgpsym = popProj->Pdrgpsym();
	if (nullptr == pdrgpsym ||
		(2 != pdrgpsym->Size() && !(exact && 3 == pdrgpsym->Size())))
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
				  pmodel->FBind(psymSchema, pdrgpcrSchema) &&
				  (3 != pdrgpsym->Size() || m_pmatcher->FMatchExpression(
					  (*pdrgpsym)[2], (*pexprProject)[1], pmodel));
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
	if (!exact && EdslopLimit != (*popProj)[0]->Edslop() &&
		EdslopSort != (*popProj)[0]->Edslop())
	{
		pexprRel = CDSLMatchView::PexprPeelOrderLimit(
			pexprRel, &pexprLimitShell);
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

BOOL
CDSLProjMatcher::FMatchDistinct(const CDSLOp *popProj, CExpression *pexprAgg,
								CDSLModel *pmodel) const
{
	if (COperator::EopLogicalGbAgg != pexprAgg->Pop()->Eopid() ||
		2 != pexprAgg->Arity() ||
		COperator::EopScalarProjectList != (*pexprAgg)[1]->Pop()->Eopid() ||
		0 != (*pexprAgg)[1]->Arity())
		return false;
	CLogicalGbAgg *agg = CLogicalGbAgg::PopConvert(pexprAgg->Pop());
	CColRefArray *keys = agg->Pdrgpcr();
	// Global aggregation with no keys emits a row on empty input, unlike
	// DISTINCT. Local/intermediate aggregation is not a relational DISTINCT.
	if (COperator::EgbaggtypeGlobal != agg->Egbaggtype() || 0 == keys->Size())
		return false;
	CExpression *input = (*pexprAgg)[0];
	CExpression *computed = nullptr;
	if (3 == popProj->Pdrgpsym()->Size() &&
		COperator::EopLogicalProject == input->Pop()->Eopid() &&
		2 == input->Arity() &&
		COperator::EopScalarProjectList == (*input)[1]->Pop()->Eopid() &&
		!(*input)[1]->DeriveHasNonScalarFunction() &&
		!(*input)[1]->DeriveHasSubquery() &&
		(*input)[0]->DeriveOutputColumns()->ContainsAll(
			(*input)[1]->DeriveUsedColumns()))
	{
		// Only absorb a complete SELECT list in its original evaluation order.
		// Unselected expressions (including errors) must stay in the input.
		ULONG next = 0;
		for (ULONG i = 0; i < keys->Size() && next < (*input)[1]->Arity(); ++i)
			if ((*keys)[i] == CScalarProjectElement::PopConvert(
					(*(*input)[1])[next]->Pop())->Pcr())
				++next;
		if (next == (*input)[1]->Arity())
		{
			computed = (*input)[1];
			input = (*input)[0];
		}
	}
	CExpressionArray *items = GPOS_NEW(m_mp) CExpressionArray(m_mp);
	ULONG next = 0;
	for (ULONG i = 0; i < keys->Size(); ++i)
	{
		if (nullptr != computed && next < computed->Arity() &&
			(*keys)[i] == CScalarProjectElement::PopConvert((*computed)[next]->Pop())->Pcr())
		{
			(*computed)[next]->AddRef();
			items->Append((*computed)[next++]);
		}
		else
			items->Append(GPOS_NEW(m_mp) CExpression(m_mp,
				GPOS_NEW(m_mp) CScalarProjectElement(m_mp, (*keys)[i]),
				GPOS_NEW(m_mp) CExpression(m_mp,
					GPOS_NEW(m_mp) CScalarIdent(m_mp, (*keys)[i]))));
	}
	input->AddRef();
	CExpression *projection = GPOS_NEW(m_mp) CExpression(m_mp,
		GPOS_NEW(m_mp) CLogicalProject(m_mp), input,
		GPOS_NEW(m_mp) CExpression(m_mp,
			GPOS_NEW(m_mp) CScalarProjectList(m_mp), items));
	const BOOL matched = FMatch(popProj, projection, pmodel);
	projection->Release();
	return matched;
}

// EOF
