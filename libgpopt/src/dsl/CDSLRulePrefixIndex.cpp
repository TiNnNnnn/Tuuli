//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLRulePrefixIndex.cpp
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLRulePrefixIndex.h"

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
			return true;

		// These matchers expose one or more virtual/normalized views whose live
		// tree is not necessarily the literal DSL tree. Ending the prefix here is
		// the general adapter boundary; the full matcher still sees the rule.
		case EdslopFilter:
		case EdslopInSubFilter:
		case EdslopExists:
		case EdslopProj:
		case EdslopAgg:
		case EdslopSort:
		case EdslopLimit:
		case EdslopSentinel:
			return false;
	}
	return false;
}

CDSLRulePrefixIndex::SNode *
CDSLRulePrefixIndex::PnodeInsertOp(SNode *pnode, const CDSLOp *pop,
								   BOOL fSourceRoot, BOOL *pfComplete)
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
		SNode *pnodeCurrent = PnodeExact(
			pnode, COperator::EopLogicalSelect, 1 /*relational child*/);
		return PnodeInsertOp(pnodeCurrent, popBase, false, pfComplete);
	}

	if (fSourceRoot && EdslopProj == pop->Edslop() &&
		1 == pop->UlChildren())
	{
		ULONG ulAdapterFlags = SExactEdge::EafNone;
		if (!pop->FDistinct())
		{
			const EDslOpKind edslopChild = (*pop)[0]->Edslop();
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
		SNode *pnodeCurrent =
			PnodeExact(pnode, COperator::EopLogicalGbAgg, 1);
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

	SNode *pnodeCurrent = PnodeExact(pnode, eopid, pop->UlChildren());
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
	if (popRoot->Eopid() == eopidBucket)
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

void
CDSLRulePrefixIndex::AppendEntries(
	const SNode *pnode,
	CDynamicPtrArray<const SRuleEntry, CleanupNULL> *pdrgpentry)
{
	for (ULONG ul = 0; ul < pnode->m_pdrgpentry->Size(); ul++)
	{
		pdrgpentry->Append((*pnode->m_pdrgpentry)[ul]);
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
		AppendEntries(pnode->m_pnodeInput, pdrgpentry);
		pdrgpnodeResult->Append(pnode->m_pnodeInput);
	}

	for (ULONG ulEdge = 0; ulEdge < pnode->m_pdrgpedgeExact->Size(); ulEdge++)
	{
		const SExactEdge *pedge = (*pnode->m_pdrgpedgeExact)[ulEdge];
		if (pedge->m_eopid != pexpr->Pop()->Eopid() ||
			pexpr->Arity() < pedge->m_ulChildren)
		{
			continue;
		}

		AppendEntries(pedge->m_pnodeChild, pdrgpentry);
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
	AppendEntries(m_pnodeRoot, pdrgpentry);

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

// EOF
