//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CScalarNullIf.cpp
//
//	@doc:
//		Implementation of scalar NullIf operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CScalarNullIf.h"

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CDrvdPropScalar.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/mdcache/CMDAccessorUtils.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "naucrates/md/IMDTypeBool.h"

using namespace gpopt;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CScalarNullIf::CScalarNullIf
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarNullIf::CScalarNullIf(CMemoryPool *mp, IMDId *mdid_op, IMDId *mdid_type)
	: CScalar(mp),
	  m_mdid_op(mdid_op),
	  m_mdid_type(mdid_type),
	  m_fBoolReturnType(false)
{
	GPOS_ASSERT(mdid_op->IsValid());
	GPOS_ASSERT(mdid_type->IsValid());

	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	m_fBoolReturnType = CMDAccessorUtils::FBoolType(md_accessor, m_mdid_type);
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarNullIf::~CScalarNullIf
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CScalarNullIf::~CScalarNullIf()
{
	m_mdid_op->Release();
	m_mdid_type->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarNullIf::HashValue
//
//	@doc:
//		Operator specific hash function; combined hash of operator id and
//		return type id
//
//---------------------------------------------------------------------------
ULONG
CScalarNullIf::HashValue() const
{
	return gpos::CombineHashes(
		COperator::HashValue(),
		gpos::CombineHashes(m_mdid_op->HashValue(), m_mdid_type->HashValue()));
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarNullIf::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarNullIf::Matches(COperator *pop) const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}

	CScalarNullIf *popScNullIf = CScalarNullIf::PopConvert(pop);

	// match if operators and return types are identical
	return m_mdid_op->Equals(popScNullIf->MdIdOp()) &&
		   m_mdid_type->Equals(popScNullIf->MdidType());
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarNullIf::Eber
//
//	@doc:
//		Perform boolean expression evaluation
//
//---------------------------------------------------------------------------
CScalar::EBoolEvalResult
CScalarNullIf::Eber(ULongPtrArray *pdrgpulChildren) const
{
	GPOS_ASSERT(2 == pdrgpulChildren->Size());
	const EBoolEvalResult left = (EBoolEvalResult) * (*pdrgpulChildren)[0];
	const EBoolEvalResult right = (EBoolEvalResult) * (*pdrgpulChildren)[1];
	// NULLIF returns its first operand if either operand is NULL; the
	// comparison function is not called. Its strictness is not NULLIF's.
	if (EberNull == left || EberNull == right)
	{
		return left;
	}

	return EberAny;
}



// EOF
