#ifndef GPDXL_CDXLPhysicalSetOp_H
#define GPDXL_CDXLPhysicalSetOp_H
#include "naucrates/dxl/operators/CDXLLogicalSetOp.h"
#include "naucrates/dxl/operators/CDXLPhysical.h"
namespace gpdxl
{
// Same positional projection/filter/children layout as Append; distinct
// execution semantics. N-ary inputs are evaluated left-associatively.
class CDXLPhysicalSetOp : public CDXLPhysical
{
	EdxlSetOpType m_kind;
	BOOL m_hash;

  public:
	CDXLPhysicalSetOp(CMemoryPool *mp, EdxlSetOpType kind, BOOL hash)
		: CDXLPhysical(mp), m_kind(kind), m_hash(hash)
	{
		GPOS_ASSERT(kind >= EdxlsetopIntersect && kind < EdxlsetopSentinel);
	}
	Edxlopid
	GetDXLOperator() const override
	{
		return EdxlopPhysicalSetOp;
	}
	const CWStringConst *
	GetOpNameStr() const override;
	EdxlSetOpType
	Kind() const
	{
		return m_kind;
	}
	BOOL
	FHash() const
	{
		return m_hash;
	}
	void
	SerializeToDXL(CXMLSerializer *, const CDXLNode *) const override;
#ifdef GPOS_DEBUG
	void
	AssertValid(const CDXLNode *, BOOL) const override;
#endif
};
} // namespace gpdxl
#endif
