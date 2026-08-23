//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLRulePrefixIndex.h
//
//	@doc:
//		Immutable-after-build prefix trie for source rule templates. Exact edges
//		encode a DSL operator and its relational arity; the Input edge is a
//		wildcard which consumes one complete live relational subtree.
//
//		The index is deliberately only a candidate filter. Representation adapters
//		(Filter conjunct chains, HAVING, pre/post unnesting, fused Sort/Limit,
//		etc.) terminate a rule's indexed prefix conservatively. The ordinary DSL
//		matcher remains the final source of truth, so indexing cannot turn a
//		near-match into an applied rewrite or hide an adapter-supported match.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLRulePrefixIndex_H
#define GPOPT_CDSLRulePrefixIndex_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"

#include "gpopt/dsl/CDSLRuleLoader.h"
#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

class CDSLRulePrefixIndex
{
private:
	struct SNode;

	struct SRuleEntry
	{
		CDSLRule *m_prule;  // non-owning; engine rule library owns it
		ULONG m_ulOrdinal;

		SRuleEntry(CDSLRule *prule, ULONG ulOrdinal)
			: m_prule(prule), m_ulOrdinal(ulOrdinal)
		{
		}
	};

	struct SExactEdge
	{
		enum EAdapterFlags
		{
			EafNone = 0,
			EafProjectPeelLimit = 1,
			EafProjectPeelAgg = 2
		};

		COperator::EOperatorId m_eopid;
		ULONG m_ulChildren;
		ULONG m_ulAdapterFlags;
		SNode *m_pnodeChild;  // owned

		SExactEdge(COperator::EOperatorId eopid, ULONG ulChildren,
				   ULONG ulAdapterFlags, SNode *pnodeChild)
			: m_eopid(eopid),
			  m_ulChildren(ulChildren),
			  m_ulAdapterFlags(ulAdapterFlags),
			  m_pnodeChild(pnodeChild)
		{
		}

		~SExactEdge();
	};

	using SRuleEntryArray = CDynamicPtrArray<SRuleEntry, CleanupDelete>;
	using SExactEdgeArray = CDynamicPtrArray<SExactEdge, CleanupDelete>;
	using SNodeArray = CDynamicPtrArray<const SNode, CleanupNULL>;

	struct SNode
	{
		SExactEdgeArray *m_pdrgpedgeExact;  // owned
		SNode *m_pnodeInput;                // owned wildcard edge
		SRuleEntryArray *m_pdrgpentry;      // owned, rule pointers non-owning

		explicit SNode(CMemoryPool *mp);
		~SNode();
	};

	CMemoryPool *m_mp;
	SNode *m_pnodeRoot;
	ULONG m_ulNodes;
	ULONG m_ulRules;
	ULONG m_ulFallbackRules;

	SNode *PnodeNew();
	SNode *PnodeInput(SNode *pnode);
	SNode *PnodeExact(SNode *pnode, COperator::EOperatorId eopid,
					  ULONG ulChildren, ULONG ulAdapterFlags = 0);

	// Operators whose matcher consumes the same live operator and recursively
	// visits the same positional relational children. Other operators have a
	// representation adapter and therefore end the safe indexed prefix.
	static BOOL FStructurallyExact(const CDSLOp *pop);

	// Insert as much of one source template as is guaranteed to be a necessary
	// live-expression prefix. *pfComplete is false when an adapter boundary
	// caused conservative early termination.
	SNode *PnodeInsertOp(SNode *pnode, const CDSLOp *pop, BOOL fSourceRoot,
						 BOOL *pfComplete);

	static void AppendEntries(const SNode *pnode,
						  CDynamicPtrArray<const SRuleEntry, CleanupNULL> *pdrgpentry);

	// Match one template node starting at pnode against one live subtree. The
	// returned states are trie positions after consuming that complete template
	// node; terminals reached along a shorter safe prefix are collected eagerly.
	void MatchOne(CMemoryPool *mp, const SNode *pnode, CExpression *pexpr,
				  CDynamicPtrArray<const SRuleEntry, CleanupNULL> *pdrgpentry,
				  SNodeArray *pdrgpnodeResult) const;

	static INT ICompareRuleEntries(const void *pvLeft, const void *pvRight);

public:
	CDSLRulePrefixIndex(const CDSLRulePrefixIndex &) = delete;

	explicit CDSLRulePrefixIndex(CMemoryPool *mp);
	~CDSLRulePrefixIndex();

	// ulOrdinal is the rule's admitted-library order and is used to preserve
	// rule-file priority after trie traversal.
	void Insert(CDSLRule *prule, ULONG ulOrdinal,
				COperator::EOperatorId eopidBucket);

	// Caller owns the returned array. It owns one ref for every returned rule.
	CDSLRuleArray *PdrgpruleCandidates(CMemoryPool *mp,
								  CExpression *pexpr) const;

	ULONG UlNodes() const { return m_ulNodes; }
	ULONG UlRules() const { return m_ulRules; }
	ULONG UlFallbackRules() const { return m_ulFallbackRules; }
};
}  // namespace gpopt

#endif  // !GPOPT_CDSLRulePrefixIndex_H
