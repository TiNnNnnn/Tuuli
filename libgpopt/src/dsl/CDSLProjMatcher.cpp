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
CColRefArray *
CDSLProjMatcher::PdrgpcrProjected(CExpression *pexprProjList) const
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

	// collect the projected columns and bind them to both <a> and <s>. FBind
	// AddRefs, so we release our local ref after binding.
	CColRefArray *pdrgpcr = PdrgpcrProjected((*pexprProject)[1]);
	if (nullptr == pdrgpcr)
	{
		return false;
	}

	BOOL fBound = pmodel->FBind(psymAttrs, pdrgpcr) &&
				  pmodel->FBind(psymSchema, pdrgpcr);
	pdrgpcr->Release();
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
