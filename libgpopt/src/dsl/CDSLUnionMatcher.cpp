//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLUnionMatcher.h"

#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/operators/CLogicalSetOp.h"

using namespace gpopt;

BOOL
CDSLUnionMatcher::FMatch(const CDSLOp *popUnion, CExpression *pexprUnion,
						 CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != popUnion);
	GPOS_ASSERT(EdslopUnion == popUnion->Edslop());
	GPOS_ASSERT(nullptr != pexprUnion);
	GPOS_ASSERT(nullptr != pmodel);

	CDSLSymbolArray *pdrgpsym = popUnion->Pdrgpsym();
	if (2 != popUnion->UlChildren() || 2 > pexprUnion->Arity() ||
		nullptr == pdrgpsym ||
		(0 != pdrgpsym->Size() && 2 != pdrgpsym->Size()) ||
		popUnion->Eopid() != pexprUnion->Pop()->Eopid())
	{
		return false;
	}

	// WeTune's rule IR is binary, while ORCA deliberately flattens same-kind
	// set-op chains. Match the exact associative view instead of forcing every
	// proved rule to encode an optimizer representation detail.
	CExpression *pexprView =
		CDSLMatchView::PexprBinarySetOp(m_mp, pexprUnion);
	CExpression *pexprMatch = nullptr == pexprView ? pexprUnion : pexprView;

	CLogicalSetOp *popSet = CLogicalSetOp::PopConvert(pexprMatch->Pop());
	if (2 != popSet->PdrgpdrgpcrInput()->Size() ||
		0 == popSet->PdrgpcrOutput()->Size())
	{
		CRefCount::SafeRelease(pexprView);
		return false;
	}

	if (!m_pmatcher->FMatch((*popUnion)[0], (*pexprMatch)[0], pmodel) ||
		!m_pmatcher->FMatch((*popUnion)[1], (*pexprMatch)[1], pmodel))
	{
		CRefCount::SafeRelease(pexprView);
		return false;
	}

	if (2 == pdrgpsym->Size())
	{
		CColRefArray *pdrgpcrOutput = popSet->PdrgpcrOutput();
		if (!pmodel->FBind((*pdrgpsym)[0], pdrgpcrOutput) ||
			!pmodel->FBind((*pdrgpsym)[1], pdrgpcrOutput))
		{
			CRefCount::SafeRelease(pexprView);
			return false;
		}
	}

	// Preserve the exact ordered output/input mappings. They are semantic state,
	// not derivable from an unordered DeriveOutputColumns() set.
	if (nullptr != pexprView)
	{
		pmodel->AddNaryUnionTail((*pexprView)[1]);
	}
	pmodel->AddUnionBinding(pexprMatch);
	CRefCount::SafeRelease(pexprView);
	return true;
}
