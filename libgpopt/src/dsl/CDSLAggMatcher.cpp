//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLAggMatcher.cpp
//
//	@doc:
//		Implementation of the dedup-Agg symbol binder (see CDSLAggMatcher.h).
//		Migrates the SEMANTICS of eliminating a redundant SELECT DISTINCT: bind
//		the grouping columns, recurse the relational child, flag a dedup drop.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLAggMatcher.h"

#include "gpos/base.h"

#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/operators/CLogicalGbAgg.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CDSLAggMatcher::PdrgpcrGrouping
//
//	@doc:
//		Copy the GbAgg's grouping columns into a fresh ordered array. Caller owns
//		the returned ref.
//---------------------------------------------------------------------------
CColRefArray *
CDSLAggMatcher::PdrgpcrGrouping(CExpression *pexprAgg) const
{
	CLogicalGbAgg *popGbAgg = CLogicalGbAgg::PopConvert(pexprAgg->Pop());
	CColRefArray *pdrgpcrGrp = popGbAgg->Pdrgpcr();

	CColRefArray *pdrgpcr = GPOS_NEW(m_mp) CColRefArray(m_mp);
	const ULONG ulCols = (nullptr == pdrgpcrGrp) ? 0 : pdrgpcrGrp->Size();
	for (ULONG ul = 0; ul < ulCols; ul++)
	{
		pdrgpcr->Append((*pdrgpcrGrp)[ul]);
	}
	return pdrgpcr;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLAggMatcher::FMatch
//---------------------------------------------------------------------------
BOOL
CDSLAggMatcher::FMatch(const CDSLOp *popAgg, CExpression *pexprAgg,
					   CDSLModel *pmodel) const
{
	GPOS_ASSERT(nullptr != popAgg);
	GPOS_ASSERT(EdslopProj == popAgg->Edslop() ||
				EdslopAgg == popAgg->Edslop());
	GPOS_ASSERT(nullptr != pexprAgg);

	// the live node must be a GbAgg carrying (relational child, agg project list).
	if (COperator::EopLogicalGbAgg != pexprAgg->Pop()->Eopid() ||
		2 != pexprAgg->Arity())
	{
		return false;
	}

	// pure-dedup gate: reject any GbAgg that computes aggregate functions (a
	// non-empty project list). Mirrors CXformSimplifyGbAgg::FDropGbAgg's
	// `if (0 < pexprProjectList->Arity()) return false;`. Real aggregate
	// functions (and any Agg-with-funcs rule) are a later milestone.
	if (0 != (*pexprAgg)[1]->Arity())
	{
		return false;
	}

	// fire only on the ORIGINAL user-level global dedup, exactly like native
	// CXformSimplifyGbAgg::Exfp: skip scalar aggs (no grouping), skip the
	// FD-annotated / split-generated aggs (PdrgpcrMinimal set — Local and
	// intermediate stages produced by CXformSplitGbAgg), and require Global.
	// Firing on the split-generated stages would insert dedup-drop alternatives
	// into groups where they are invalid and break memo plan extraction
	// (CMemo.cpp PocLookupBest returns null).
	CLogicalGbAgg *popGbAgg = CLogicalGbAgg::PopConvert(pexprAgg->Pop());
	if (COperator::EgbaggtypeGlobal != popGbAgg->Egbaggtype() ||
		nullptr != popGbAgg->PdrgpcrMinimal() ||
		nullptr == popGbAgg->Pdrgpcr() || 0 == popGbAgg->Pdrgpcr()->Size())
	{
		return false;
	}

	// locate the attrs and schema symbols. Proj*<a s>: attrs=[0], schema=[1].
	// Agg<a a a f s p>: grouping attrs=[0], schema=[4]. (An Agg here is degenerate
	// — it survived the empty-agg gate — and its func/pred symbols stay unbound, so
	// a constraint referencing them fails the check and the rule does not fire.)
	CDSLSymbolArray *pdrgpsym = popAgg->Pdrgpsym();
	const CDSLSymbol *psymAttrs = nullptr;
	const CDSLSymbol *psymSchema = nullptr;
	if (EdslopProj == popAgg->Edslop())
	{
		if (nullptr == pdrgpsym || 2 != pdrgpsym->Size())
		{
			return false;
		}
		psymAttrs = (*pdrgpsym)[0];
		psymSchema = (*pdrgpsym)[1];
	}
	else
	{
		if (nullptr == pdrgpsym || 6 != pdrgpsym->Size())
		{
			return false;
		}
		psymAttrs = (*pdrgpsym)[0];
		psymSchema = (*pdrgpsym)[4];
	}

	// bind <a> and <s> to the grouping columns — for a pure dedup the referenced,
	// output and grouping columns coincide. FBind AddRefs; release the local copy.
	CColRefArray *pdrgpcrGrp = PdrgpcrGrouping(pexprAgg);
	BOOL fBound = pmodel->FBind(psymAttrs, pdrgpcrGrp) &&
				  pmodel->FBind(psymSchema, pdrgpcrGrp);
	pdrgpcrGrp->Release();
	if (!fBound)
	{
		return false;
	}

	// the relational child recurses through the generic matcher (GbAgg has exactly
	// one relational child; the template's UlChildren() is 1).
	if (1 != popAgg->UlChildren())
	{
		return false;
	}
	if (!m_pmatcher->FMatch((*popAgg)[0], (*pexprAgg)[0], pmodel))
	{
		return false;
	}

	// flag the source root as a redundant dedup GbAgg: the instantiator drops it
	// by wrapping the resolved child in Select(child, TRUE), mirroring
	// CXformSimplifyGbAgg::FDropGbAgg (the memo-safe operator-drop idiom).
	pmodel->SetDedupDrop();
	return true;
}

// EOF
