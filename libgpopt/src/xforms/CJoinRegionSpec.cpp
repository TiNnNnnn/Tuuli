//---------------------------------------------------------------------------
//	@filename:
//		CJoinRegionSpec.cpp
//---------------------------------------------------------------------------
#include "gpopt/xforms/CJoinRegionSpec.h"

#include "gpos/common/CAutoRef.h"

#include "gpopt/operators/CLogicalInnerJoin.h"

using namespace gpopt;

CJoinRegionSpec::CEdge::CEdge(CMemoryPool *mp,
							  COperator::EOperatorId join_type,
							  const CBitSet *left, const CBitSet *right,
							  CExpression *predicate)
	: m_join_type(join_type),
	  m_left(GPOS_NEW(mp) CBitSet(mp, *left)),
	  m_right(GPOS_NEW(mp) CBitSet(mp, *right)),
	  m_predicate(predicate)
{
	GPOS_ASSERT(nullptr != predicate);
	m_predicate->AddRef();
}

CJoinRegionSpec::CEdge::~CEdge()
{
	m_left->Release();
	m_right->Release();
	m_predicate->Release();
}

CJoinRegionSpec::CJoinRegionSpec(CMemoryPool *mp)
	: m_mp(mp),
	  m_atoms(GPOS_NEW(mp) CExpressionArray(mp)),
	  m_built(false),
	  m_pure_inner(true)
{
}

CJoinRegionSpec::~CJoinRegionSpec()
{
	for (CEdge *edge : m_edges)
	{
		GPOS_DELETE(edge);
	}
	m_atoms->Release();
}

CExpression *
CJoinRegionSpec::PexprMarkDPHyperRegions(CMemoryPool *mp, CExpression *expr,
										BOOL parent_is_inner)
{
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp && nullptr != expr);
	const BOOL is_inner =
		COperator::EopLogicalInnerJoin == expr->Pop()->Eopid();
	if (!is_inner && 0 == expr->Arity() && nullptr != expr->Pgexpr())
	{
		expr->Pop()->AddRef();
		return GPOS_NEW(mp) CExpression(mp, expr->Pop(), expr->Pgexpr());
	}
	CExpressionArray *children = GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG child = 0; child < expr->Arity(); ++child)
	{
		children->Append(PexprMarkDPHyperRegions(
			mp, (*expr)[child], is_inner && child < 2));
	}

	COperator *op = nullptr;
	if (is_inner)
	{
		CLogicalInnerJoin *join = CLogicalInnerJoin::PopConvert(expr->Pop());
		op = GPOS_NEW(mp) CLogicalInnerJoin(
			mp, join->OriginXform(), true /*region member*/,
			!parent_is_inner /*region root*/);
	}
	else
	{
		expr->Pop()->AddRef();
		op = expr->Pop();
	}
	return GPOS_NEW(mp) CExpression(mp, op, children);
}

BOOL
CJoinRegionSpec::FSupportedBinaryJoin(COperator::EOperatorId op_id)
{
	switch (op_id)
	{
		case COperator::EopLogicalInnerJoin:
		case COperator::EopLogicalLeftOuterJoin:
		case COperator::EopLogicalLeftSemiJoin:
		case COperator::EopLogicalLeftAntiSemiJoin:
		case COperator::EopLogicalLeftAntiSemiJoinNotIn:
		case COperator::EopLogicalFullOuterJoin:
		case COperator::EopLogicalRightOuterJoin:
			return true;

		default:
			return false;
	}
}

CBitSet *
CJoinRegionSpec::PbsCollect(CExpression *expr)
{
	const COperator::EOperatorId op_id = expr->Pop()->Eopid();
	if (!FSupportedBinaryJoin(op_id) || 3 != expr->Arity())
	{
		const ULONG node_id = m_atoms->Size();
		expr->AddRef();
		m_atoms->Append(expr);
		CBitSet *atom = GPOS_NEW(m_mp) CBitSet(m_mp);
		(void) atom->ExchangeSet(node_id);
		return atom;
	}

	CAutoRef<CBitSet> left(PbsCollect((*expr)[0]));
	CAutoRef<CBitSet> right(PbsCollect((*expr)[1]));
	m_edges.push_back(GPOS_NEW(m_mp)
						  CEdge(m_mp, op_id, left.Value(), right.Value(),
								(*expr)[2]));
	if (COperator::EopLogicalInnerJoin != op_id)
	{
		m_pure_inner = false;
	}

	CBitSet *result = GPOS_NEW(m_mp) CBitSet(m_mp, *left.Value());
	result->Union(right.Value());
	return result;
}

BOOL
CJoinRegionSpec::Build(CExpression *root)
{
	GPOS_ASSERT(!m_built);
	GPOS_ASSERT(nullptr != root);
	m_built = true;
	if (!FSupportedBinaryJoin(root->Pop()->Eopid()) || 3 != root->Arity())
	{
		return false;
	}

	CAutoRef<CBitSet> all_nodes(PbsCollect(root));
	return 2 <= m_atoms->Size() && !m_edges.empty() &&
		   all_nodes->Size() == m_atoms->Size();
}
