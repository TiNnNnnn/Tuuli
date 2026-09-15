//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CScalarCoerceViaIO.cpp
//
//	@doc:
//		Implementation of scalar CoerceViaIO operators
//
//	@owner:
//
//	@test:
//
//
//---------------------------------------------------------------------------

#include "gpopt/operators/CScalarCoerceViaIO.h"

#include "gpos/base.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpopt;
using namespace gpmd;


//---------------------------------------------------------------------------
//	@function:
//		CScalarCoerceViaIO::CScalarCoerceViaIO
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarCoerceViaIO::CScalarCoerceViaIO(CMemoryPool *mp, IMDId *mdid_type,
									   INT type_modifier, ECoercionForm ecf,
									   INT location, IMDId *input_func, IMDId *output_func)
	: CScalarCoerceBase(mp, mdid_type, type_modifier, ecf, location),
	  m_input_func(input_func), m_output_func(output_func)
{
}

CScalarCoerceViaIO::~CScalarCoerceViaIO()
{
	CRefCount::SafeRelease(m_input_func);
	CRefCount::SafeRelease(m_output_func);
}

CFunctionProp *
CScalarCoerceViaIO::DeriveFunctionProperties(CMemoryPool *mp, CExpressionHandle &exprhdl) const
{
	// Legacy DXL can omit the function identities. Unknown must not become
	// immutable merely because the value argument has no volatile functions.
	IMDFunction::EFuncStbl stability = IMDFunction::EfsVolatile;
	if (nullptr != m_input_func && nullptr != m_output_func)
	{
		CMDAccessor *mda = COptCtxt::PoctxtFromTLS()->Pmda();
		stability = mda->RetrieveFunc(m_input_func)->GetFuncStability();
		IMDFunction::EFuncStbl output = mda->RetrieveFunc(m_output_func)->GetFuncStability();
		if (output > stability)
		{
			stability = output;
		}
	}
	return PfpDeriveFromChildren(mp, exprhdl, stability, false, false);
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarCoerceViaIO::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarCoerceViaIO::Matches(COperator *pop) const
{
	if (pop->Eopid() == Eopid())
	{
		CScalarCoerceViaIO *popCoerce = CScalarCoerceViaIO::PopConvert(pop);

		return popCoerce->MdidType()->Equals(MdidType()) &&
			(nullptr == m_input_func ? nullptr == popCoerce->m_input_func :
				(nullptr != popCoerce->m_input_func && m_input_func->Equals(popCoerce->m_input_func))) &&
			(nullptr == m_output_func ? nullptr == popCoerce->m_output_func :
				(nullptr != popCoerce->m_output_func && m_output_func->Equals(popCoerce->m_output_func))) &&
			   popCoerce->TypeModifier() == TypeModifier() &&
			   popCoerce->Ecf() == Ecf() && popCoerce->Location() == Location();
	}

	return false;
}


// EOF
