//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLRulePrefixIndex.cpp
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLRulePrefixIndex.h"

#include "gpopt/base/COptCtxt.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/search/CGroupExpression.h"
#include "gpopt/search/CGroupProxy.h"

using namespace gpopt;

CDSLRulePrefixIndex::SExactEdge::~SExactEdge()
{
	GPOS_DELETE(m_pnodeChild);
}

CDSLRulePrefixIndex::SNode::SNode(CMemoryPool *mp)
	: m_pdrgpedgeExact(GPOS_NEW(mp) SExactEdgeArray(mp)),
	  m_pnodeInput(nullptr),
	  m_pdrgpentry(GPOS_NEW(mp) SRuleEntryArray(mp))
{
}

CDSLRulePrefixIndex::SNode::~SNode()
{
	m_pdrgpedgeExact->Release();
	GPOS_DELETE(m_pnodeInput);
	m_pdrgpentry->Release();
}

CDSLRulePrefixIndex::CDSLRulePrefixIndex(CMemoryPool *mp)
	: m_mp(mp),
	  m_pnodeRoot(nullptr),
	  m_ulNodes(0),
	  m_ulRules(0),
	  m_ulFallbackRules(0)
{
	GPOS_ASSERT(nullptr != mp);
	m_pnodeRoot = PnodeNew();
}

CDSLRulePrefixIndex::~CDSLRulePrefixIndex()
{
	GPOS_DELETE(m_pnodeRoot);
}

CDSLRulePrefixIndex::SNode *
CDSLRulePrefixIndex::PnodeNew()
{
	m_ulNodes++;
	return GPOS_NEW(m_mp) SNode(m_mp);
}

CDSLRulePrefixIndex::SNode *
CDSLRulePrefixIndex::PnodeInput(SNode *pnode)
{
	GPOS_ASSERT(nullptr != pnode);
	if (nullptr == pnode->m_pnodeInput)
	{
		pnode->m_pnodeInput = PnodeNew();
	}
	return pnode->m_pnodeInput;
}

CDSLRulePrefixIndex::SNode *
CDSLRulePrefixIndex::PnodeExact(SNode *pnode,
								 COperator::EOperatorId eopid,
								 ULONG ulChildren,
								 ULONG ulAdapterFlags)
{
	GPOS_ASSERT(nullptr != pnode);
	for (ULONG ul = 0; ul < pnode->m_pdrgpedgeExact->Size(); ul++)
	{
		SExactEdge *pedge = (*pnode->m_pdrgpedgeExact)[ul];
		if (eopid == pedge->m_eopid && ulChildren == pedge->m_ulChildren &&
			ulAdapterFlags == pedge->m_ulAdapterFlags)
		{
			return pedge->m_pnodeChild;
		}
	}

	SNode *pnodeChild = PnodeNew();
	pnode->m_pdrgpedgeExact->Append(
		GPOS_NEW(m_mp)
			SExactEdge(eopid, ulChildren, ulAdapterFlags, pnodeChild));
	return pnodeChild;
}

BOOL
CDSLRulePrefixIndex::FStructurallyExact(const CDSLOp *pop)
{
	GPOS_ASSERT(nullptr != pop);
	switch (pop->Edslop())
	{
		case EdslopInput:
		case EdslopInnerJoin:
		case EdslopLeftJoin:
		case EdslopUnion:
		case EdslopCompute:
		case EdslopEmpty:
		case EdslopSemiJoin:
		case EdslopSemiApply:
		case EdslopAntiJoin:
		case EdslopAntiApply:
		case EdslopInnerApply:
		case EdslopLeftOuterApply:
			return true;

		// These matchers expose one or more virtual/normalized views whose live
		// tree is not necessarily the literal DSL tree. Ending the prefix here is
		// the general adapter boundary; the full matcher still sees the rule.
		case EdslopFilter:
		case EdslopInSubFilter:
		case EdslopExists:
		case EdslopNotExists:
		case EdslopAny:
		case EdslopAll:
		case EdslopProj:
		case EdslopAgg:
		case EdslopSort:
		case EdslopLimit:
		case EdslopSentinel:
			return false;
	}
	return false;
}

BOOL
CDSLRulePrefixIndex::FEdgeMatchesOperator(const SExactEdge *pedge,
									  COperator *pop)
{
	const BOOL fNullRejectedInnerView =
		0 != (pedge->m_ulAdapterFlags &
			  SExactEdge::EafNullRejectedInnerJoin) &&
		COperator::EopLogicalInnerJoin == pedge->m_eopid &&
		(COperator::EopLogicalLeftOuterJoin == pop->Eopid() ||
		 COperator::EopLogicalFullOuterJoin == pop->Eopid());
	const BOOL fDedupAggView =
		COperator::EopLogicalGbAgg == pedge->m_eopid &&
		COperator::EopLogicalGbAggDeduplicate == pop->Eopid();
	if (pedge->m_eopid != pop->Eopid() && !fNullRejectedInnerView &&
		!fDedupAggView)
	{
		return false;
	}
	if (0 == (pedge->m_ulAdapterFlags &
			  (SExactEdge::EafGbAggGlobal |
			   SExactEdge::EafGbAggNoMinimal)))
	{
		return true;
	}

	CLogicalGbAgg *popGbAgg = CLogicalGbAgg::PopConvert(pop);
	return (0 == (pedge->m_ulAdapterFlags &
				  SExactEdge::EafGbAggGlobal) ||
			COperator::EgbaggtypeGlobal == popGbAgg->Egbaggtype()) &&
		   (0 == (pedge->m_ulAdapterFlags &
				  SExactEdge::EafGbAggNoMinimal) ||
			nullptr == popGbAgg->PdrgpcrMinimal());
}

CDSLRulePrefixIndex::SNode *
CDSLRulePrefixIndex::PnodeInsertOp(SNode *pnode, const CDSLOp *pop,
								   BOOL fSourceRoot, BOOL *pfComplete,
								   ULONG ulAdapterFlags)
{
	GPOS_ASSERT(nullptr != pnode);
	GPOS_ASSERT(nullptr != pop);
	GPOS_ASSERT(nullptr != pfComplete);

	// Root-level adapters still have a stable physical prefix in their direct
	// shell. Compile that prefix, then continue into the relation view which the
	// corresponding matcher exposes. Routed shells were already rejected by the
	// bucket/root identity check in Insert().
	if (fSourceRoot && EdslopFilter == pop->Edslop())
	{
		const CDSLOp *popBase = pop;
		while (EdslopFilter == popBase->Edslop() &&
			   1 == popBase->UlChildren())
		{
			popBase = (*popBase)[0];
		}
		if (EdslopLeftOuterApply == popBase->Edslop())
		{
			// A value subquery below a general boolean expression is still encoded
			// in the live Select predicate before Select2Apply.  Its production
			// view is Filter(Apply), so the Apply is not a physical child that the
			// prefix can inspect yet.  Index the stable Select shell only and leave
			// the complete Filter/Apply validation to CDSLFilterMatcher.  This is
			// the Filter counterpart of the Project(Apply) adapter below.
			*pfComplete = false;
			return PnodeExact(
				pnode, COperator::EopLogicalSelect, 0 /*relational children*/);
		}
		SNode *pnodeCurrent = PnodeExact(
			pnode, COperator::EopLogicalSelect, 1 /*relational child*/);
		const ULONG ulBaseAdapterFlags =
			EdslopInnerJoin == popBase->Edslop()
				? SExactEdge::EafNullRejectedInnerJoin
				: SExactEdge::EafNone;
		return PnodeInsertOp(pnodeCurrent, popBase, false, pfComplete,
							 ulBaseAdapterFlags);
	}

	if (fSourceRoot && EdslopProj == pop->Edslop() &&
		1 == pop->UlChildren())
	{
		const EDslOpKind edslopChild = (*pop)[0]->Edslop();
		const CDSLOp *popAfterProjectChain = (*pop)[0];
		while (EdslopProj == popAfterProjectChain->Edslop() &&
			   1 == popAfterProjectChain->UlChildren())
		{
			popAfterProjectChain = (*popAfterProjectChain)[0];
		}
		if (!pop->FDistinct() &&
			(EdslopInnerApply == popAfterProjectChain->Edslop() ||
			 EdslopLeftOuterApply == popAfterProjectChain->Edslop()))
		{
			// A pre-unnest Project has the Apply and any compensation Project
			// encoded in its scalar project list, not in its live relational child.
			// Index only the stable Project root for this Apply-terminated Project
			// chain; the shared match view remains the complete semantic gate. Using
			// zero relational children also gives memo binding construction a valid
			// witness instead of consuming the not-yet-materialized chain.
			*pfComplete = false;
			return PnodeExact(pnode, pop->Eopid(), 0);
		}

		ULONG ulAdapterFlags = SExactEdge::EafNone;
		if (pop->FDistinct())
		{
			// A source-root Proj* is accepted only on the original, unsplit
			// Global dedup. Encode that stable matcher gate in the trie token.
			ulAdapterFlags |= SExactEdge::EafGbAggGlobal |
							  SExactEdge::EafGbAggNoMinimal;
		}
		else
		{
			if (EdslopLimit != edslopChild && EdslopSort != edslopChild)
			{
				ulAdapterFlags |= SExactEdge::EafProjectPeelLimit;
			}
			if (EdslopInput != edslopChild && EdslopProj != edslopChild &&
				EdslopAgg != edslopChild)
			{
				ulAdapterFlags |= SExactEdge::EafProjectPeelAgg;
			}
		}
		SNode *pnodeCurrent =
			PnodeExact(pnode, pop->Eopid(), 1, ulAdapterFlags);
		return PnodeInsertOp(pnodeCurrent, (*pop)[0], false, pfComplete);
	}

	if (fSourceRoot && EdslopAgg == pop->Edslop() &&
		1 == pop->UlChildren())
	{
		// A real aggregate remains the same DSL operator when ORCA annotates a
		// Global GbAgg with a minimal grouping set before memo insertion.  Unlike
		// Proj* (the empty aggregate-list/dedup view), matching Agg does not delete
		// the grouping operator, so the annotation is not a semantic eligibility
		// gate.  The target builder conservatively recreates the full grouping set
		// and lets ORCA derive any valid minimal set for the rewritten child.
		SNode *pnodeCurrent =
			PnodeExact(pnode, COperator::EopLogicalGbAgg, 1,
					   SExactEdge::EafGbAggGlobal);
		return PnodeInsertOp(pnodeCurrent, (*pop)[0], false, pfComplete);
	}

	if (!FStructurallyExact(pop))
	{
		*pfComplete = false;
		return pnode;
	}

	if (EdslopInput == pop->Edslop())
	{
		*pfComplete = true;
		return PnodeInput(pnode);
	}

	const COperator::EOperatorId eopid = pop->Eopid();
	if (COperator::EopSentinel == eopid)
	{
		*pfComplete = false;
		return pnode;
	}

	SNode *pnodeCurrent =
		PnodeExact(pnode, eopid, pop->UlChildren(), ulAdapterFlags);
	for (ULONG ul = 0; ul < pop->UlChildren(); ul++)
	{
		BOOL fChildComplete = false;
		pnodeCurrent =
			PnodeInsertOp(pnodeCurrent, (*pop)[ul], false, &fChildComplete);
		if (!fChildComplete)
		{
			*pfComplete = false;
			return pnodeCurrent;
		}
	}

	*pfComplete = true;
	return pnodeCurrent;
}

void
CDSLRulePrefixIndex::Insert(CDSLRule *prule, ULONG ulOrdinal,
							 COperator::EOperatorId eopidBucket)
{
	GPOS_ASSERT(nullptr != prule);
	m_ulRules++;

	const CDSLOp *popRoot = prule->PfragSrc()->PopRoot();
	SNode *pnodeTerminal = m_pnodeRoot;
	BOOL fComplete = false;

	// A rule routed to a different shell is handled through a representation
	// adapter. Its literal source root is not a necessary prefix in this bucket.
	const BOOL fDedupAggView =
		EdslopProj == popRoot->Edslop() && popRoot->FDistinct() &&
		COperator::EopLogicalGbAggDeduplicate == eopidBucket;
	if (popRoot->Eopid() == eopidBucket || fDedupAggView)
	{
		pnodeTerminal =
			PnodeInsertOp(m_pnodeRoot, popRoot, true, &fComplete);
	}

	if (m_pnodeRoot == pnodeTerminal)
	{
		m_ulFallbackRules++;
	}
	pnodeTerminal->m_pdrgpentry->Append(
		GPOS_NEW(m_mp) SRuleEntry(prule, ulOrdinal));
}

BOOL
CDSLRulePrefixIndex::FRuleAvailable(const SRuleEntry *pentry)
{
	GPOS_ASSERT(nullptr != pentry);
	COptCtxt *poctxt = COptCtxt::PoctxtFromTLS();
	if (nullptr == poctxt)
	{
		return true;
	}

	const ULONG ulSourceLine = pentry->m_prule->UlSourceLine();
	const ULONG ulRuleId =
		0 == ulSourceLine ? pentry->m_ulOrdinal + 1 : ulSourceLine;
	if (!poctxt->FDSLAlternativeBudgetExhausted(ulRuleId))
	{
		return true;
	}

	poctxt->RecordDSLRuleBudgetSkip(ulRuleId);
	return false;
}

BOOL
CDSLRulePrefixIndex::FNodeHasAvailableTerminal(const SNode *pnode)
{
	for (ULONG ul = 0; ul < pnode->m_pdrgpentry->Size(); ul++)
	{
		if (FRuleAvailable((*pnode->m_pdrgpentry)[ul]))
		{
			return true;
		}
	}
	return false;
}

BOOL
CDSLRulePrefixIndex::FNodeHasAvailableRule(const SNode *pnode)
{
	if (FNodeHasAvailableTerminal(pnode))
	{
		return true;
	}
	if (nullptr != pnode->m_pnodeInput &&
		FNodeHasAvailableRule(pnode->m_pnodeInput))
	{
		return true;
	}
	for (ULONG ul = 0; ul < pnode->m_pdrgpedgeExact->Size(); ul++)
	{
		if (FNodeHasAvailableRule(
				(*pnode->m_pdrgpedgeExact)[ul]->m_pnodeChild))
		{
			return true;
		}
	}
	return false;
}

void
CDSLRulePrefixIndex::AppendAvailableEntries(
	const SNode *pnode,
	CDynamicPtrArray<const SRuleEntry, CleanupNULL> *pdrgpentry)
{
	for (ULONG ul = 0; ul < pnode->m_pdrgpentry->Size(); ul++)
	{
		const SRuleEntry *pentry = (*pnode->m_pdrgpentry)[ul];
		if (FRuleAvailable(pentry))
		{
			pdrgpentry->Append(pentry);
		}
	}
}

void
CDSLRulePrefixIndex::MatchOne(
	CMemoryPool *mp, const SNode *pnode, CExpression *pexpr,
	CDynamicPtrArray<const SRuleEntry, CleanupNULL> *pdrgpentry,
	SNodeArray *pdrgpnodeResult) const
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pnode);
	GPOS_ASSERT(nullptr != pexpr);

	// Input<t> consumes this complete subtree without looking at its root or
	// descendants. The next trie state therefore continues with the next sibling
	// token, not with any live child of this expression.
	if (nullptr != pnode->m_pnodeInput)
	{
		if (FNodeHasAvailableRule(pnode->m_pnodeInput))
		{
			AppendAvailableEntries(pnode->m_pnodeInput, pdrgpentry);
			pdrgpnodeResult->Append(pnode->m_pnodeInput);
		}
	}

	for (ULONG ulEdge = 0; ulEdge < pnode->m_pdrgpedgeExact->Size(); ulEdge++)
	{
		const SExactEdge *pedge = (*pnode->m_pdrgpedgeExact)[ulEdge];
		if (!FEdgeMatchesOperator(pedge, pexpr->Pop()) ||
			pexpr->Arity() < pedge->m_ulChildren ||
			!FNodeHasAvailableRule(pedge->m_pnodeChild))
		{
			continue;
		}

		AppendAvailableEntries(pedge->m_pnodeChild, pdrgpentry);
		SNodeArray *pdrgpnodeCurrent = GPOS_NEW(mp) SNodeArray(mp);
		pdrgpnodeCurrent->Append(pedge->m_pnodeChild);

		for (ULONG ulChild = 0;
			 ulChild < pedge->m_ulChildren && 0 < pdrgpnodeCurrent->Size();
			 ulChild++)
		{
			SNodeArray *pdrgpnodeNext = GPOS_NEW(mp) SNodeArray(mp);
			for (ULONG ulState = 0; ulState < pdrgpnodeCurrent->Size();
				 ulState++)
			{
				const SNode *pnodeState = (*pdrgpnodeCurrent)[ulState];
				CExpression *pexprChild = (*pexpr)[ulChild];
				MatchOne(mp, pnodeState, pexprChild, pdrgpentry,
						 pdrgpnodeNext);

				// PostgreSQL may retain ORDER/LIMIT below a target-list Project,
				// whereas the DSL matcher exposes the relation below that shell.
				// Try every peeled level as an additional view; the ordinary matcher
				// still decides whether one is semantically admissible.
				if (0 == ulChild &&
					0 != (pedge->m_ulAdapterFlags &
						  SExactEdge::EafProjectPeelLimit))
				{
					CExpression *pexprPeeled = pexprChild;
					while (COperator::EopLogicalLimit ==
							   pexprPeeled->Pop()->Eopid() &&
						   3 == pexprPeeled->Arity())
					{
						pexprPeeled = (*pexprPeeled)[0];
						MatchOne(mp, pnodeState, pexprPeeled, pdrgpentry,
								 pdrgpnodeNext);
					}
				}

				// A Project directly over one canonical unsplit GbAgg may expose
				// the aggregate's relational input to a non-Agg DSL child.
				if (0 == ulChild &&
					0 != (pedge->m_ulAdapterFlags &
						  SExactEdge::EafProjectPeelAgg) &&
					COperator::EopLogicalGbAgg ==
						pexprChild->Pop()->Eopid() &&
					2 == pexprChild->Arity())
				{
					MatchOne(mp, pnodeState, (*pexprChild)[0], pdrgpentry,
							 pdrgpnodeNext);
				}
			}
			pdrgpnodeCurrent->Release();
			pdrgpnodeCurrent = pdrgpnodeNext;
		}

		pdrgpnodeResult->AppendArray(pdrgpnodeCurrent);
		pdrgpnodeCurrent->Release();
	}
}

INT
CDSLRulePrefixIndex::ICompareRuleEntries(const void *pvLeft,
									  const void *pvRight)
{
	const SRuleEntry *pentryLeft =
		*static_cast<const SRuleEntry *const *>(pvLeft);
	const SRuleEntry *pentryRight =
		*static_cast<const SRuleEntry *const *>(pvRight);
	if (pentryLeft->m_ulOrdinal < pentryRight->m_ulOrdinal)
	{
		return -1;
	}
	if (pentryLeft->m_ulOrdinal > pentryRight->m_ulOrdinal)
	{
		return 1;
	}
	return 0;
}

CDSLRuleArray *
CDSLRulePrefixIndex::PdrgpruleCandidates(CMemoryPool *mp,
									 CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	using SRuleEntryConstArray =
		CDynamicPtrArray<const SRuleEntry, CleanupNULL>;
	SRuleEntryConstArray *pdrgpentry = GPOS_NEW(mp) SRuleEntryConstArray(mp);
	AppendAvailableEntries(m_pnodeRoot, pdrgpentry);

	SNodeArray *pdrgpnodeResult = GPOS_NEW(mp) SNodeArray(mp);
	MatchOne(mp, m_pnodeRoot, pexpr, pdrgpentry, pdrgpnodeResult);
	pdrgpnodeResult->Release();

	pdrgpentry->Sort(ICompareRuleEntries);
	CDSLRuleArray *pdrgprule = GPOS_NEW(mp) CDSLRuleArray(mp);
	CDSLRule *prulePrevious = nullptr;
	for (ULONG ul = 0; ul < pdrgpentry->Size(); ul++)
	{
		CDSLRule *prule = (*pdrgpentry)[ul]->m_prule;
		// A rule currently has one compiled prefix, but keep the result stable if
		// future representation adapters add multiple physical index paths.
		if (prule == prulePrevious)
		{
			continue;
		}
		prule->AddRef();
		pdrgprule->Append(prule);
		prulePrevious = prule;
	}
	pdrgpentry->Release();
	return pdrgprule;
}

CExpression *
CDSLRulePrefixIndex::PexprRepresentative(CMemoryPool *mp, CGroup *pgroup,
									 ULONG ulDepth)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pgroup);

	CGroupProxy gp(pgroup);
	CGroupExpression *pgexpr = nullptr;
	if (pgroup->FScalar())
	{
		pgexpr = gp.PgexprFirst();
	}
	else
	{
		// Prefer a native/original alternative. DSL-produced alternatives are
		// semantically equivalent, but choosing them as opaque Input witnesses can
		// feed a rewrite back into itself and make attribution/order less stable.
		for (CGroupExpression *pgexprCurrent = gp.PgexprNextLogical(nullptr);
			 nullptr != pgexprCurrent;
			 pgexprCurrent = gp.PgexprNextLogical(pgexprCurrent))
		{
			if (nullptr == pgexpr)
			{
				pgexpr = pgexprCurrent;
			}
			if (!pgexprCurrent->FHasDSLProvenance())
			{
				pgexpr = pgexprCurrent;
				break;
			}
		}
	}
	GPOS_ASSERT(nullptr != pgexpr);
	return PexprRepresentative(mp, pgexpr, ulDepth);
}

CExpression *
CDSLRulePrefixIndex::PexprRepresentative(CMemoryPool *mp,
									 CGroupExpression *pgexpr, ULONG ulDepth)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pgexpr);

	// Memo expressions are expected to form an acyclic child-group graph. Keep
	// a defensive ceiling: a group-bound leaf still carries the group's complete
	// derived relational properties and can be grafted into an xform result
	// without recursive memo insertion.
	if (128 <= ulDepth)
	{
		pgexpr->Pop()->AddRef();
		return GPOS_NEW(mp) CExpression(mp, pgexpr->Pop(), pgexpr);
	}

	CExpressionArray *pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < pgexpr->Arity(); ul++)
	{
		pdrgpexpr->Append(
			PexprRepresentative(mp, (*pgexpr)[ul], ulDepth + 1));
	}
	pgexpr->Pop()->AddRef();
	return GPOS_NEW(mp) CExpression(mp, pgexpr->Pop(), pgexpr, pdrgpexpr,
								  nullptr /*prpp*/, nullptr /*input_stats*/);
}

CDSLRulePrefixIndex::SBindingStateArray *
CDSLRulePrefixIndex::PdrgpstateConsumeGroup(CMemoryPool *mp,
										const SNode *pnode,
										CGroup *pgroup) const
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pnode);
	GPOS_ASSERT(nullptr != pgroup);

	SBindingStateArray *pdrgpstate = GPOS_NEW(mp) SBindingStateArray(mp);
	if (!FNodeHasAvailableRule(pnode))
	{
		return pdrgpstate;
	}

	// Input consumes the entire equivalence group. Preserve each top-level memo
	// alternative because its group-expression identity affects safe duplicate-
	// group merging and therefore final plan selection. Descendants still use one
	// stable representative each, avoiding the recursive Cartesian product which
	// made CPatternTree binding unbounded.
	if (nullptr != pnode->m_pnodeInput &&
		FNodeHasAvailableRule(pnode->m_pnodeInput))
	{
		CGroupProxy gpInput(pgroup);
		if (pgroup->FScalar())
		{
			pdrgpstate->Append(GPOS_NEW(mp) SBindingState(
				pnode->m_pnodeInput,
				PexprRepresentative(mp, gpInput.PgexprFirst())));
		}
		else
		{
			for (CGroupExpression *pgexprInput =
					 gpInput.PgexprNextLogical(nullptr);
				 nullptr != pgexprInput;
				 pgexprInput = gpInput.PgexprNextLogical(pgexprInput))
			{
				pdrgpstate->Append(GPOS_NEW(mp) SBindingState(
					pnode->m_pnodeInput,
					PexprRepresentative(mp, pgexprInput)));
			}
		}
	}

	CGroupProxy gp(pgroup);
	if (pgroup->FScalar())
	{
		CGroupExpression *pgexpr = gp.PgexprFirst();
		SBindingStateArray *pdrgpstateCurrent =
			PdrgpstateConsumeGExpr(mp, pnode, pgexpr);
		for (ULONG ul = 0; ul < pdrgpstateCurrent->Size(); ul++)
		{
			SBindingState *pstate = (*pdrgpstateCurrent)[ul];
			pstate->m_pexpr->AddRef();
			pdrgpstate->Append(GPOS_NEW(mp)
				SBindingState(pstate->m_pnode, pstate->m_pexpr));
		}
		pdrgpstateCurrent->Release();
		return pdrgpstate;
	}

	for (CGroupExpression *pgexpr = gp.PgexprNextLogical(nullptr);
		 nullptr != pgexpr; pgexpr = gp.PgexprNextLogical(pgexpr))
	{
		SBindingStateArray *pdrgpstateCurrent =
			PdrgpstateConsumeGExpr(mp, pnode, pgexpr);
		for (ULONG ul = 0; ul < pdrgpstateCurrent->Size(); ul++)
		{
			SBindingState *pstate = (*pdrgpstateCurrent)[ul];
			pstate->m_pexpr->AddRef();
			pdrgpstate->Append(GPOS_NEW(mp)
				SBindingState(pstate->m_pnode, pstate->m_pexpr));
		}
		pdrgpstateCurrent->Release();
	}
	return pdrgpstate;
}

void
CDSLRulePrefixIndex::AppendWrappedStates(
	CMemoryPool *mp, CGroupExpression *pgexprWrapper,
	SBindingStateArray *pdrgpstateInner,
	SBindingStateArray *pdrgpstateResult)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pgexprWrapper);
	GPOS_ASSERT(nullptr != pdrgpstateInner);
	GPOS_ASSERT(nullptr != pdrgpstateResult);

	for (ULONG ulState = 0; ulState < pdrgpstateInner->Size(); ulState++)
	{
		SBindingState *pstate = (*pdrgpstateInner)[ulState];
		CExpressionArray *pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
		pstate->m_pexpr->AddRef();
		pdrgpexpr->Append(pstate->m_pexpr);
		for (ULONG ulChild = 1; ulChild < pgexprWrapper->Arity(); ulChild++)
		{
			pdrgpexpr->Append(
				PexprRepresentative(mp, (*pgexprWrapper)[ulChild]));
		}
		pgexprWrapper->Pop()->AddRef();
		CExpression *pexpr = GPOS_NEW(mp) CExpression(
			mp, pgexprWrapper->Pop(), pgexprWrapper, pdrgpexpr,
			nullptr /*prpp*/, nullptr /*input_stats*/);
		pdrgpstateResult->Append(
			GPOS_NEW(mp) SBindingState(pstate->m_pnode, pexpr));
	}
}

CDSLRulePrefixIndex::SBindingStateArray *
CDSLRulePrefixIndex::PdrgpstateConsumeProjectChild(
	CMemoryPool *mp, const SNode *pnode, CGroup *pgroup,
	ULONG ulAdapterFlags) const
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pnode);
	GPOS_ASSERT(nullptr != pgroup);

	SBindingStateArray *pdrgpstateResult =
		PdrgpstateConsumeGroup(mp, pnode, pgroup);
	CGroupProxy gp(pgroup);
	for (CGroupExpression *pgexpr = gp.PgexprNextLogical(nullptr);
		 nullptr != pgexpr; pgexpr = gp.PgexprNextLogical(pgexpr))
	{
		if (0 != (ulAdapterFlags & SExactEdge::EafProjectPeelLimit) &&
			COperator::EopLogicalLimit == pgexpr->Pop()->Eopid() &&
			3 == pgexpr->Arity())
		{
			// Limit chains are one fused ORCA representation of nested DSL
			// Sort/Limit views. Recurse so every peeled depth remains visible.
			SBindingStateArray *pdrgpstateInner =
				PdrgpstateConsumeProjectChild(mp, pnode, (*pgexpr)[0],
					ulAdapterFlags);
			AppendWrappedStates(mp, pgexpr, pdrgpstateInner,
							pdrgpstateResult);
			pdrgpstateInner->Release();
		}

		if (0 != (ulAdapterFlags & SExactEdge::EafProjectPeelAgg) &&
			COperator::EopLogicalGbAgg == pgexpr->Pop()->Eopid() &&
			2 == pgexpr->Arity())
		{
			SBindingStateArray *pdrgpstateInner =
				PdrgpstateConsumeGroup(mp, pnode, (*pgexpr)[0]);
			AppendWrappedStates(mp, pgexpr, pdrgpstateInner,
							pdrgpstateResult);
			pdrgpstateInner->Release();
		}
	}
	return pdrgpstateResult;
}

CDSLRulePrefixIndex::SBindingStateArray *
CDSLRulePrefixIndex::PdrgpstateConsumeGExpr(CMemoryPool *mp,
										const SNode *pnode,
										CGroupExpression *pgexpr) const
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pnode);
	GPOS_ASSERT(nullptr != pgexpr);

	SBindingStateArray *pdrgpstateResult =
		GPOS_NEW(mp) SBindingStateArray(mp);

	for (ULONG ulEdge = 0; ulEdge < pnode->m_pdrgpedgeExact->Size(); ulEdge++)
	{
		const SExactEdge *pedge = (*pnode->m_pdrgpedgeExact)[ulEdge];
		if (!FEdgeMatchesOperator(pedge, pgexpr->Pop()) ||
			pgexpr->Arity() < pedge->m_ulChildren ||
			!FNodeHasAvailableRule(pedge->m_pnodeChild))
		{
			continue;
		}

		SChildBindingStateArray *pdrgpstateChildren =
			GPOS_NEW(mp) SChildBindingStateArray(mp);
		pdrgpstateChildren->Append(GPOS_NEW(mp) SChildBindingState(
			pedge->m_pnodeChild, GPOS_NEW(mp) CExpressionArray(mp)));

		for (ULONG ulChild = 0;
			 ulChild < pedge->m_ulChildren &&
			 0 < pdrgpstateChildren->Size();
			 ulChild++)
		{
			SChildBindingStateArray *pdrgpstateNext =
				GPOS_NEW(mp) SChildBindingStateArray(mp);
			for (ULONG ulState = 0; ulState < pdrgpstateChildren->Size();
				 ulState++)
			{
				SChildBindingState *pstate = (*pdrgpstateChildren)[ulState];
				const ULONG ulProjectAdapterFlags =
					pedge->m_ulAdapterFlags &
					(SExactEdge::EafProjectPeelLimit |
					 SExactEdge::EafProjectPeelAgg);
				SBindingStateArray *pdrgpchild =
					(0 == ulChild && SExactEdge::EafNone !=
									 ulProjectAdapterFlags)
						? PdrgpstateConsumeProjectChild(
							  mp, pstate->m_pnode, (*pgexpr)[ulChild],
							  ulProjectAdapterFlags)
						: PdrgpstateConsumeGroup(
							  mp, pstate->m_pnode, (*pgexpr)[ulChild]);
				for (ULONG ulBinding = 0; ulBinding < pdrgpchild->Size();
					 ulBinding++)
				{
					SBindingState *pchild = (*pdrgpchild)[ulBinding];
					CExpressionArray *pdrgpexpr =
						GPOS_NEW(mp) CExpressionArray(mp);
					for (ULONG ulExisting = 0;
						 ulExisting < pstate->m_pdrgpexpr->Size(); ulExisting++)
					{
						CExpression *pexprExisting =
							(*pstate->m_pdrgpexpr)[ulExisting];
						pexprExisting->AddRef();
						pdrgpexpr->Append(pexprExisting);
					}
					pchild->m_pexpr->AddRef();
					pdrgpexpr->Append(pchild->m_pexpr);
					pdrgpstateNext->Append(GPOS_NEW(mp) SChildBindingState(
						pchild->m_pnode, pdrgpexpr));
				}
				pdrgpchild->Release();
			}
			pdrgpstateChildren->Release();
			pdrgpstateChildren = pdrgpstateNext;
		}

		for (ULONG ulState = 0; ulState < pdrgpstateChildren->Size();
			 ulState++)
		{
			SChildBindingState *pstate = (*pdrgpstateChildren)[ulState];
			for (ULONG ulChild = pedge->m_ulChildren;
				 ulChild < pgexpr->Arity(); ulChild++)
			{
				pstate->m_pdrgpexpr->Append(
					PexprRepresentative(mp, (*pgexpr)[ulChild]));
			}
			pgexpr->Pop()->AddRef();
			pstate->m_pdrgpexpr->AddRef();
			CExpression *pexpr = GPOS_NEW(mp) CExpression(
				mp, pgexpr->Pop(), pgexpr, pstate->m_pdrgpexpr,
				nullptr /*prpp*/, nullptr /*input_stats*/);
			pdrgpstateResult->Append(
				GPOS_NEW(mp) SBindingState(pstate->m_pnode, pexpr));
		}
		pdrgpstateChildren->Release();
	}
	return pdrgpstateResult;
}

BOOL
CDSLRulePrefixIndex::FContainsEquivalentBinding(
	const CExpressionArray *pdrgpexpr, CExpression *pexpr)
{
	for (ULONG ul = 0; ul < pdrgpexpr->Size(); ul++)
	{
		if ((*pdrgpexpr)[ul]->Matches(pexpr))
		{
			return true;
		}
	}
	return false;
}

CExpressionArray *
CDSLRulePrefixIndex::PdrgpexprBindings(CMemoryPool *mp,
									   CGroupExpression *pgexprRoot) const
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pgexprRoot);

	CExpressionArray *pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);

	// Root-terminal entries are routed/fallback rules without a safe physical
	// prefix. Do not build their conservative witness after all such rules have
	// exhausted their configured alternative budget.
	if (0 == m_pnodeRoot->m_pdrgpentry->Size() ||
		FNodeHasAvailableTerminal(m_pnodeRoot))
	{
		CExpression *pexprRepresentative =
			PexprRepresentative(mp, pgexprRoot);
		pdrgpexpr->Append(pexprRepresentative);
	}

	SBindingStateArray *pdrgpstate =
		PdrgpstateConsumeGExpr(mp, m_pnodeRoot, pgexprRoot);
	for (ULONG ul = 0; ul < pdrgpstate->Size(); ul++)
	{
		SBindingState *pstate = (*pdrgpstate)[ul];
		// A state without a terminal rule is only a partial prefix and must not
		// invoke the shell yet.
		if (!FNodeHasAvailableTerminal(pstate->m_pnode) ||
			FContainsEquivalentBinding(pdrgpexpr, pstate->m_pexpr))
		{
			continue;
		}
		pstate->m_pexpr->AddRef();
		pdrgpexpr->Append(pstate->m_pexpr);
	}
	pdrgpstate->Release();
	return pdrgpexpr;
}

// EOF
