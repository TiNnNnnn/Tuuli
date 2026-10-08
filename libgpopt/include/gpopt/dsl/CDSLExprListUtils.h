//---------------------------------------------------------------------------
//	MONSOON DSL expression-list algebra
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLExprListUtils_H
#define GPOPT_CDSLExprListUtils_H

#include "gpos/base.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/operators/CExpression.h"
#include <vector>

namespace gpopt
{
using namespace gpos;

// One occurrence owned by a source Context binding. Never equate occurrences
// just because their selected expression pointers or printed trees agree.
class CDSLScalarContext : public CRefCount
{
	CExpression *m_root;
	std::vector<ULONG> m_path;
public:
	CDSLScalarContext(CExpression *root, const std::vector<ULONG> &path)
		: m_root(root), m_path(path) { m_root->AddRef(); }
	~CDSLScalarContext() override { m_root->Release(); }
	// Borrow the matched occurrence witness; no new search or ownership transfer.
	const CExpression *PexprRoot() const { return m_root; }
	const std::vector<ULONG> &Path() const { return m_path; }
	BOOL Matches(const CDSLScalarContext *other) const;
	CExpression *PexprPlug(CMemoryPool *mp, CExpression *replacement) const;
};

class CDSLExprListUtils
{
public:
	CDSLExprListUtils() = delete;
	// Preserve FUNC's array representation while sharing scalar-list contexts.
	static CExpressionArray *PdrgpexprFunctions(CMemoryPool *mp, CExpression *list);
	// A context is a scalar child-index path, not a node pointer or an
	// evaluation order. Paths cannot cross a relational subquery boundary.
	// Lookup borrows; replacement owns its result and preserves untouched trees.
	// Type compatibility is structural evidence only, not an equivalence proof
	// or permission to move the selected computation out of this context.
	static CExpression *PexprAtScalarPath(CExpression *root,
		const std::vector<ULONG> &path);
	static CExpression *PexprReplaceAt(CMemoryPool *mp, CExpression *root,
		const std::vector<ULONG> &path, CExpression *replacement);

	static BOOL FProjectList(const CExpression *pexpr);
	// Native typed captures must agree with each item's output column type.
	static BOOL FTypedProjectElement(const CExpression *pexpr);
	// Complete lists also require unique output identities, including captures
	// and independently constructed lists. Repeated values remain valid.
	static BOOL FTypedProjectList(const CExpression *pexpr);
	// Exact dependency set and ordered output identities for a captured or
	// rebuilt list. Borrows all inputs; types, freshness and scope are separate.
	static BOOL FProjectListColumns(CMemoryPool *mp, CExpression *list,
		const CColRefArray *attrs, const CColRefArray *schema);
	// Row-level scalar scope: no SRFs, aggregate or window calls. Relational
	// subquery children have their own evaluation phase and are not inspected.
	static BOOL FRowScalar(CExpression *pexpr);
	// Typed Compute is a parallel scalar list: unique outputs, no SRFs and
	// no references to outputs defined by this same list. Outer refs are valid.
	static BOOL FComputeList(CExpression *pexpr);
	// Allocate fresh NULL outputs with the template's ordered types/typmods.
	static CExpression *PexprNulls(CMemoryPool *mp, CColRefArray *columns);
	static CColRefArray *PdrgpcrOutput(CMemoryPool *mp, CExpression *list);
	// Volatile expression lists cannot be composed or partitioned: independent
	// columns do not imply independent effects or invariant evaluation counts.
	static BOOL FConcatSafe(CExpression *pexprUpper,
							CExpression *pexprLower);
	static BOOL FDepsDisjoint(CMemoryPool *mp, CExpression *pexprList,
							 CColRefArray *pdrgpcrSchema);

	// Merge two evaluation layers. Movable ordinary upper elements precede the
	// upper SRF cohort, followed by lower, matching ORCA's canonical SRF order.
	// Caller owns the returned ProjectList.
	static CExpression *PexprConcat(CMemoryPool *mp, CExpression *pexprUpper,
								 CExpression *pexprLower);

	// Partition an upper list around the columns defined by lower. Independent
	// elements move into merged=(movable upper)++lower; dependent elements remain
	// in residual. SRFs move as one cohort only when lower has no SRF, preserving
	// row-expansion layers. Caller owns both returned lists.
	static BOOL FSplit(CMemoryPool *mp, CExpression *pexprUpper,
					   CExpression *pexprLower, CExpression **ppexprMerged,
					   CExpression **ppexprResidual);
};
}  // namespace gpopt

#endif  // !GPOPT_CDSLExprListUtils_H
