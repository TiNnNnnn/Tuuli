//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CLogicalLeftAntiSemiJoinNotIn.h
//
//	@doc:
//		Left anti semi join operator with the NotIn semantics
//			1 not in (2,3) --> true
//			1 not in (1,2,3) --> false
//			1 not in (null, 2) --> unknown
//			1 not in (1, null, 2) --> false
//			null not in (1,2) --> unknown
//			null not in (empty) --> true
//			null not in (1,2,null) --> unknown
//---------------------------------------------------------------------------
#ifndef GPOS_CLogicalLeftAntiSemiJoinNotIn_H
#define GPOS_CLogicalLeftAntiSemiJoinNotIn_H

#include "gpos/base.h"

#include "gpopt/operators/CLogicalLeftAntiSemiJoin.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CLogicalLeftAntiSemiJoinNotIn
//
//	@doc:
//		Left anti semi join operator with the NotIn semantics
//
//---------------------------------------------------------------------------
class CLogicalLeftAntiSemiJoinNotIn : public CLogicalLeftAntiSemiJoin
{
private:
	// Optional raw (violation) comparison within the scalar join predicate.
	// A qualified NOT IN predicate is represented by ORCA as comparison AND
	// qualifier. AND is commutative in the memo, so the DSL cannot recover that
	// semantic boundary from child position. Keeping the exact comparison here
	// makes the qualified DSL view lossless while the executor continues to
	// consume the ordinary scalar child.
	CExpression *m_pexprNotInComparison;

public:
	CLogicalLeftAntiSemiJoinNotIn(const CLogicalLeftAntiSemiJoinNotIn &) =
		delete;

	// ctor
	explicit CLogicalLeftAntiSemiJoinNotIn(
		CMemoryPool *mp, CXform::EXformId origin_xform = CXform::ExfSentinel,
		BOOL dphyper_region_member = false,
		BOOL dphyper_region_root = false);

	// ctor with an owned marker for the comparison component of a qualified
	// NOT IN predicate
	CLogicalLeftAntiSemiJoinNotIn(
		CMemoryPool *mp, CExpression *pexprNotInComparison,
		CXform::EXformId origin_xform = CXform::ExfSentinel,
		BOOL dphyper_region_member = false,
		BOOL dphyper_region_root = false);

	// dtor
	~CLogicalLeftAntiSemiJoinNotIn() override;

	// raw comparison component retained for the qualified DSL view; non-owning
	CExpression *
	PexprNotInComparison() const
	{
		return m_pexprNotInComparison;
	}

	ULONG HashValue() const override;
	BOOL Matches(COperator *pop) const override;
	COperator *PopCopyWithRemappedColumns(CMemoryPool *mp,
									  UlongToColRefMap *colref_mapping,
									  BOOL must_exist) override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalLeftAntiSemiJoinNotIn;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CLogicalLeftAntiSemiJoinNotIn";
	}

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	CXformSet *PxfsCandidates(CMemoryPool *mp) const override;

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// conversion function
	static CLogicalLeftAntiSemiJoinNotIn *
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalLeftAntiSemiJoinNotIn == pop->Eopid());

		return dynamic_cast<CLogicalLeftAntiSemiJoinNotIn *>(pop);
	}

};	// class CLogicalLeftAntiSemiJoinNotIn

}  // namespace gpopt


#endif	// !GPOS_CLogicalLeftAntiSemiJoinNotIn_H

// EOF
