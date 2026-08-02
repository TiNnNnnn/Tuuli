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
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"

using namespace gpopt;

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

	// the live node must be a Project carrying (relational child, project list).
	if (COperator::EopLogicalProject != pexprProject->Pop()->Eopid() ||
		2 != pexprProject->Arity())
	{
		return false;
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

	// the relational child recurses through the generic matcher (Proj has exactly
	// one relational child; the template's UlChildren() is 1).
	if (1 != popProj->UlChildren())
	{
		return false;
	}
	if (!m_pmatcher->FMatch((*popProj)[0], (*pexprProject)[0], pmodel))
	{
		return false;
	}

	// record the whole project-list subtree so the instantiator can graft it back
	// (it carries computed-column value subtrees the attrs/schema symbols do not).
	CExpression *pexprProjList = (*pexprProject)[1];
	pexprProjList->AddRef();
	pmodel->SetProjList(pexprProjList);
	return true;
}

// EOF
