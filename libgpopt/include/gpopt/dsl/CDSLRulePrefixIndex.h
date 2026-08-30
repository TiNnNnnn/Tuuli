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

class CGroup;
class CGroupExpression;

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
			EafProjectPeelAgg = 2,
			EafGbAggGlobal = 4,
			EafGbAggNoMinimal = 8,
			// A Filter(InnerJoin) matcher can expose a null-rejected
			// Select(LeftJoin) as the same logical source shape. The index may
			// admit that conservative candidate; the matcher proves null rejection.
			EafNullRejectedInnerJoin = 16,
			// A pre-unnest Agg(Apply) view exists only while the Global GbAgg's
			// scalar project list still contains a subquery.
			EafGbAggHasSubquery = 32
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

	struct SBindingState
	{
		const SNode *m_pnode;
		CExpression *m_pexpr;  // owned

		SBindingState(const SNode *pnode, CExpression *pexpr)
			: m_pnode(pnode), m_pexpr(pexpr)
		{
		}

		~SBindingState() { m_pexpr->Release(); }
	};

	struct SChildBindingState
	{
		const SNode *m_pnode;
		CExpressionArray *m_pdrgpexpr;  // owned

		SChildBindingState(const SNode *pnode,
						 CExpressionArray *pdrgpexpr)
			: m_pnode(pnode), m_pdrgpexpr(pdrgpexpr)
		{
		}

		~SChildBindingState() { m_pdrgpexpr->Release(); }
	};

	using SBindingStateArray =
		CDynamicPtrArray<SBindingState, CleanupDelete>;
	using SChildBindingStateArray =
		CDynamicPtrArray<SChildBindingState, CleanupDelete>;

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
	static BOOL FEdgeMatchesOperator(const SExactEdge *pedge,
								 COperator *pop,
								 BOOL fGbAggHasSubquery);

	// Insert as much of one source template as is guaranteed to be a necessary
	// live-expression prefix. *pfComplete is false when an adapter boundary
	// caused conservative early termination.
	SNode *PnodeInsertOp(SNode *pnode, const CDSLOp *pop, BOOL fSourceRoot,
						 BOOL *pfComplete, ULONG ulAdapterFlags = 0);

	static BOOL FRuleAvailable(const SRuleEntry *pentry);
	static BOOL FNodeHasAvailableTerminal(const SNode *pnode);
	static BOOL FNodeHasAvailableRule(const SNode *pnode);
	static void AppendAvailableEntries(
		const SNode *pnode,
		CDynamicPtrArray<const SRuleEntry, CleanupNULL> *pdrgpentry);

	// Match one template node starting at pnode against one live subtree. The
	// returned states are trie positions after consuming that complete template
	// node; terminals reached along a shorter safe prefix are collected eagerly.
	void MatchOne(CMemoryPool *mp, const SNode *pnode, CExpression *pexpr,
				  CDynamicPtrArray<const SRuleEntry, CleanupNULL> *pdrgpentry,
				  SNodeArray *pdrgpnodeResult) const;

	static INT ICompareRuleEntries(const void *pvLeft, const void *pvRight);

	// Build one deterministic complete representative for an opaque Input
	// group. Unlike CPatternTree binding this does not enumerate the Cartesian
	// product below a wildcard whose internal shape is irrelevant to the DSL.
	static CExpression *PexprRepresentative(CMemoryPool *mp, CGroup *pgroup,
										 ULONG ulDepth = 0);
	static CExpression *PexprRepresentative(CMemoryPool *mp,
										 CGroupExpression *pgexpr,
										 ULONG ulDepth = 0);

	// Consume one relational memo group (or one fixed root group expression)
	// while advancing through the serialized source-template trie.
	SBindingStateArray *PdrgpstateConsumeGroup(CMemoryPool *mp,
										 const SNode *pnode,
										 CGroup *pgroup) const;
	SBindingStateArray *PdrgpstateConsumeGExpr(
		CMemoryPool *mp, const SNode *pnode,
		CGroupExpression *pgexpr) const;

	// Project's DSL view may look through fused Limit/Sort shells and one
	// canonical GbAgg shell. Consume the exposed relation while rebuilding the
	// exact memo wrappers around every selected binding.
	SBindingStateArray *PdrgpstateConsumeProjectChild(
		CMemoryPool *mp, const SNode *pnode, CGroup *pgroup,
		ULONG ulAdapterFlags) const;
	static void AppendWrappedStates(CMemoryPool *mp,
								 CGroupExpression *pgexprWrapper,
								 SBindingStateArray *pdrgpstateInner,
								 SBindingStateArray *pdrgpstateResult);

	static BOOL FContainsEquivalentBinding(const CExpressionArray *pdrgpexpr,
										 CExpression *pexpr);

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

	// Build memo bindings by walking the source trie before invoking a DSL
	// shell. Exact prefixes enumerate only matching memo alternatives; Input
	// wildcards retain each top-level alternative but use one representative
	// below it, avoiding recursive Cartesian expansion. Caller owns the array.
	CExpressionArray *PdrgpexprBindings(CMemoryPool *mp,
									 CGroupExpression *pgexprRoot) const;

	ULONG UlNodes() const { return m_ulNodes; }
	ULONG UlRules() const { return m_ulRules; }
	ULONG UlFallbackRules() const { return m_ulFallbackRules; }
};
}  // namespace gpopt

#endif  // !GPOPT_CDSLRulePrefixIndex_H
