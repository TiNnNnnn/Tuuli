//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CLogicalLeftAntiSemiJoinNotIn.cpp
//
//	@doc:
//		Implementation of left anti semi join operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalLeftAntiSemiJoinNotIn.h"

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftAntiSemiJoinNotIn::CLogicalLeftAntiSemiJoinNotIn
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CLogicalLeftAntiSemiJoinNotIn::CLogicalLeftAntiSemiJoinNotIn(
	CMemoryPool *mp, CXform::EXformId origin_xform,
	BOOL dphyper_region_member, BOOL dphyper_region_root)
	: CLogicalLeftAntiSemiJoin(mp, origin_xform, dphyper_region_member,
						   dphyper_region_root),
	  m_pexprNotInComparison(nullptr)
{
	GPOS_ASSERT(nullptr != mp);
}

CLogicalLeftAntiSemiJoinNotIn::CLogicalLeftAntiSemiJoinNotIn(
	CMemoryPool *mp, CExpression *pexprNotInComparison,
	CXform::EXformId origin_xform, BOOL dphyper_region_member,
	BOOL dphyper_region_root)
	: CLogicalLeftAntiSemiJoin(mp, origin_xform, dphyper_region_member,
						   dphyper_region_root),
	  m_pexprNotInComparison(pexprNotInComparison)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexprNotInComparison);
}

CLogicalLeftAntiSemiJoinNotIn::~CLogicalLeftAntiSemiJoinNotIn()
{
	CRefCount::SafeRelease(m_pexprNotInComparison);
}

ULONG
CLogicalLeftAntiSemiJoinNotIn::HashValue() const
{
	ULONG ulHash = COperator::HashValue();
	if (nullptr != m_pexprNotInComparison)
	{
		ulHash = gpos::CombineHashes(
			ulHash, CExpression::UlHashDedup(m_pexprNotInComparison));
	}
	return ulHash;
}

BOOL
CLogicalLeftAntiSemiJoinNotIn::Matches(COperator *pop) const
{
	if (nullptr == pop || Eopid() != pop->Eopid())
	{
		return false;
	}
	CLogicalLeftAntiSemiJoinNotIn *popOther = PopConvert(pop);
	return CUtils::Equals(m_pexprNotInComparison,
					  popOther->m_pexprNotInComparison);
}

COperator *
CLogicalLeftAntiSemiJoinNotIn::PopCopyWithRemappedColumns(
	CMemoryPool *mp, UlongToColRefMap *colref_mapping, BOOL must_exist)
{
	CExpression *pexprComparison =
		nullptr == m_pexprNotInComparison
			? nullptr
			: m_pexprNotInComparison->PexprCopyWithRemappedColumns(
				  mp, colref_mapping, must_exist);
	if (nullptr == pexprComparison)
	{
		return GPOS_NEW(mp) CLogicalLeftAntiSemiJoinNotIn(
			mp, OriginXform(), FDPHyperRegionMember(), FDPHyperRegionRoot());
	}
	return GPOS_NEW(mp) CLogicalLeftAntiSemiJoinNotIn(
		mp, pexprComparison, OriginXform(), FDPHyperRegionMember(),
		FDPHyperRegionRoot());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalLeftAntiSemiJoinNotIn::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
CXformSet *
CLogicalLeftAntiSemiJoinNotIn::PxfsCandidates(CMemoryPool *mp) const
{
	CXformSet *xform_set = GPOS_NEW(mp) CXformSet(mp);

	(void) xform_set->ExchangeSet(
		CXform::ExfAntiSemiJoinNotInAntiSemiJoinNotInSwap);
	(void) xform_set->ExchangeSet(CXform::ExfAntiSemiJoinNotInAntiSemiJoinSwap);
	(void) xform_set->ExchangeSet(CXform::ExfAntiSemiJoinNotInSemiJoinSwap);
	(void) xform_set->ExchangeSet(CXform::ExfAntiSemiJoinNotInInnerJoinSwap);
	(void) xform_set->ExchangeSet(
		CXform::ExfLeftAntiSemiJoinNotIn2CrossProduct);
	(void) xform_set->ExchangeSet(CXform::ExfDSLRuleAll);
	(void) xform_set->ExchangeSet(CXform::ExfLeftAntiSemiJoinNotIn2NLJoinNotIn);
	(void) xform_set->ExchangeSet(
		CXform::ExfLeftAntiSemiJoinNotIn2HashJoinNotIn);
	return xform_set;
}

// EOF
