#include "naucrates/dxl/operators/CDXLPhysicalSetOp.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/xml/dxltokens.h"
using namespace gpdxl;

const CWStringConst *
CDXLPhysicalSetOp::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenPhysicalSetOp);
}

void
CDXLPhysicalSetOp::SerializeToDXL(CXMLSerializer *xml,
								  const CDXLNode *node) const
{
	auto *prefix = CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix);
	xml->OpenElement(prefix, GetOpNameStr());
	xml->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenSetOpType),
					  ULONG(m_kind));
	xml->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenSetOpHash), m_hash);
	node->SerializePropertiesToDXL(xml);
	node->SerializeChildrenToDXL(xml);
	xml->CloseElement(prefix, GetOpNameStr());
}

#ifdef GPOS_DEBUG
void
CDXLPhysicalSetOp::AssertValid(const CDXLNode *node, BOOL children) const
{
	CDXLPhysical::AssertValid(node, children);
	GPOS_ASSERT(node->Arity() >= 4 && (*node)[1]->Arity() == 0);
	for (ULONG i = 2; i < node->Arity(); ++i)
	{
		auto *op = (*node)[i]->GetOperator();
		GPOS_ASSERT(op->GetDXLOperatorType() == EdxloptypePhysical);
		if (children)
			op->AssertValid((*node)[i], children);
	}
}
#endif
