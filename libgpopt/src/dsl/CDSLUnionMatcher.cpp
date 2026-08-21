//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLUnionMatcher.h"

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

	// WeTune's Union is binary. ORCA may collapse a chain into an n-ary set-op;
	// matching only its first two children would silently change semantics, so
	// reject that representation until a rule explicitly models n-ary Union.
	if (2 != popUnion->UlChildren() || 2 != pexprUnion->Arity() ||
		nullptr == popUnion->Pdrgpsym() || 0 != popUnion->Pdrgpsym()->Size() ||
		popUnion->Eopid() != pexprUnion->Pop()->Eopid())
	{
		return false;
	}

	CLogicalSetOp *popSet = CLogicalSetOp::PopConvert(pexprUnion->Pop());
	if (2 != popSet->PdrgpdrgpcrInput()->Size() ||
		0 == popSet->PdrgpcrOutput()->Size())
	{
		return false;
	}

	if (!m_pmatcher->FMatch((*popUnion)[0], (*pexprUnion)[0], pmodel) ||
		!m_pmatcher->FMatch((*popUnion)[1], (*pexprUnion)[1], pmodel))
	{
		return false;
	}

	// Preserve the exact ordered output/input mappings. They are semantic state,
	// not derivable from an unordered DeriveOutputColumns() set.
	pmodel->AddUnionBinding(pexprUnion);
	return true;
}
