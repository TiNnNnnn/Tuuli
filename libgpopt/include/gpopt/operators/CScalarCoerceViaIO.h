//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CScalarCoerceViaIO.h
//
//	@doc:
//		Scalar CoerceViaIO operator,
//		the operator captures coercing a value from one type to another, by
//		calling the output function of the argument type, and passing the
//		result to the input function of the result type.
//
//	@owner:
//
//	@test:
//
//
//---------------------------------------------------------------------------
#ifndef GPOPT_CScalarCoerceViaIO_H
#define GPOPT_CScalarCoerceViaIO_H

#include "gpos/base.h"

#include "gpopt/operators/CScalarCoerceBase.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CScalarCoerceViaIO
//
//	@doc:
//		Scalar CoerceViaIO operator
//
//---------------------------------------------------------------------------
class CScalarCoerceViaIO : public CScalarCoerceBase
{
private:
	IMDId *m_input_func;
	IMDId *m_output_func;
public:
	CScalarCoerceViaIO(const CScalarCoerceViaIO &) = delete;

	// ctor
	CScalarCoerceViaIO(CMemoryPool *mp, IMDId *mdid_type, INT type_modifier,
					   ECoercionForm dxl_coerce_format, INT location,
					   IMDId *input_func = nullptr, IMDId *output_func = nullptr);

	// dtor
	~CScalarCoerceViaIO() override;
	IMDId *InputFuncMdId() const { return m_input_func; }
	IMDId *OutputFuncMdId() const { return m_output_func; }
	CFunctionProp *DeriveFunctionProperties(CMemoryPool *mp,
		CExpressionHandle &exprhdl) const override;

	EBoolEvalResult Eber(ULongPtrArray *children) const override
	{
		// PostgreSQL's I/O conversion protocol preserves NULL, independently
		// of the implementation functions' strictness declarations.
		return EberNullOnAnyNullChild(children);
	}

	EOperatorId
	Eopid() const override
	{
		return EopScalarCoerceViaIO;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CScalarCoerceViaIO";
	}

	// match function
	BOOL Matches(COperator *) const override;

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		return false;
	}

	// conversion function
	static CScalarCoerceViaIO *
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopScalarCoerceViaIO == pop->Eopid());

		return dynamic_cast<CScalarCoerceViaIO *>(pop);
	}

};	// class CScalarCoerceViaIO

}  // namespace gpopt


#endif	// !GPOPT_CScalarCoerceViaIO_H

// EOF
