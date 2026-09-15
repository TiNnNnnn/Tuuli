//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CDXLScalarCoerceViaIO.cpp
//
//	@doc:
//		Implementation of DXL scalar coerce
//
//	@owner:
//
//	@test:
//
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarCoerceViaIO.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "naucrates/dxl/xml/dxltokens.h"

using namespace gpopt;
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCoerceViaIO::CDXLScalarCoerceViaIO
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarCoerceViaIO::CDXLScalarCoerceViaIO(CMemoryPool *mp, IMDId *mdid_type,
											 INT type_modifier,
											 EdxlCoercionForm dxl_coerce_format,
											 INT location, IMDId *input_func, IMDId *output_func)
	: CDXLScalarCoerceBase(mp, mdid_type, type_modifier, dxl_coerce_format,
						   location), m_input_func(input_func), m_output_func(output_func)
{
}

CDXLScalarCoerceViaIO::~CDXLScalarCoerceViaIO()
{
	CRefCount::SafeRelease(m_input_func);
	CRefCount::SafeRelease(m_output_func);
}

void
CDXLScalarCoerceViaIO::SerializeToDXL(CXMLSerializer *serializer, const CDXLNode *node) const
{
	const CWStringConst *prefix = CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix);
	serializer->OpenElement(prefix, GetOpNameStr());
	GetResultTypeMdId()->Serialize(serializer, CDXLTokens::GetDXLTokenStr(EdxltokenTypeId));
	if (default_type_modifier != TypeModifier())
	{
		serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenTypeMod), TypeModifier());
	}
	serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenCoercionForm),
		(ULONG) GetDXLCoercionForm());
	serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenLocation), GetLocation());
	if (nullptr != m_input_func)
	{
		m_input_func->Serialize(serializer, CDXLTokens::GetDXLTokenStr(EdxltokenInputFuncId));
	}
	if (nullptr != m_output_func)
	{
		m_output_func->Serialize(serializer, CDXLTokens::GetDXLTokenStr(EdxltokenOutputFuncId));
	}
	node->SerializeChildrenToDXL(serializer);
	serializer->CloseElement(prefix, GetOpNameStr());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarCoerceViaIO::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarCoerceViaIO::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenScalarCoerceViaIO);
}

// EOF
