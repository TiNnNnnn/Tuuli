//---------------------------------------------------------------------------
//	@filename:
//		CJoinRegionSpec.cpp
//---------------------------------------------------------------------------
#include "gpopt/xforms/CJoinRegionSpec.h"

#include "gpos/common/CAutoRef.h"
#include "gpos/common/CBitSetIter.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CLogicalFullOuterJoin.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalLeftAntiSemiJoin.h"
#include "gpopt/operators/CLogicalLeftOuterJoin.h"
#include "gpopt/operators/CLogicalLeftSemiJoin.h"
#include "gpopt/operators/CPredicateUtils.h"

using namespace gpopt;

CJoinRegionSpec::CConflictRule::CConflictRule(CMemoryPool *mp,
										  const CBitSet *activate,
										  const CBitSet *required)
	: m_activate(GPOS_NEW(mp) CBitSet(mp, *activate)),
	  m_required(GPOS_NEW(mp) CBitSet(mp, *required))
{
}

CJoinRegionSpec::CConflictRule::~CConflictRule()
{
	m_activate->Release();
	m_required->Release();
}

CJoinRegionSpec::CEdge::CEdge(CMemoryPool *mp,
								  COperator::EOperatorId join_type,
								  const CBitSet *left, const CBitSet *right,
								  CExpression *predicate)
	: m_mp(mp),
	  m_join_type(join_type),
	  m_left(GPOS_NEW(mp) CBitSet(mp, *left)),
	  m_right(GPOS_NEW(mp) CBitSet(mp, *right)),
	  m_all(GPOS_NEW(mp) CBitSet(mp, *left)),
	  m_ses(nullptr),
	  m_tes(nullptr),
	  m_predicate(predicate)
{
	GPOS_ASSERT(nullptr != predicate);
	m_all->Union(right);
	m_predicate->AddRef();
}

CJoinRegionSpec::CEdge::~CEdge()
{
	m_left->Release();
	m_right->Release();
	m_all->Release();
	CRefCount::SafeRelease(m_ses);
	CRefCount::SafeRelease(m_tes);
	m_predicate->Release();
	for (CConflictRule *rule : m_conflict_rules)
	{
		GPOS_DELETE(rule);
	}
}

BOOL
CJoinRegionSpec::CEdge::FApplicable(const CBitSet *left,
									const CBitSet *right) const
{
	GPOS_ASSERT(nullptr != m_tes);
	if (nullptr == left || nullptr == right || !left->IsDisjoint(right))
	{
		return false;
	}
	CAutoRef<CBitSet> joined(GPOS_NEW(m_mp) CBitSet(m_mp, *left));
	joined->Union(right);
	if (!joined->ContainsAll(m_tes) || m_tes->IsDisjoint(left) ||
		m_tes->IsDisjoint(right))
	{
		return false;
	}
	for (const CConflictRule *rule : m_conflict_rules)
	{
		if (!joined->IsDisjoint(rule->Activate()) &&
			!joined->ContainsAll(rule->Required()))
		{
			return false;
		}
	}
	if (COperator::EopLogicalInnerJoin == m_join_type)
	{
		return true;
	}

	CBitSetIter tes_iter(*m_tes);
	while (tes_iter.Advance())
	{
		const ULONG node = tes_iter.Bit();
		if ((m_left->Get(node) && !left->Get(node)) ||
			(m_right->Get(node) && !right->Get(node)))
		{
			return false;
		}
	}
	return true;
}

CJoinRegionSpec::CJoinRegionSpec(CMemoryPool *mp)
	: m_mp(mp),
	  m_atoms(GPOS_NEW(mp) CExpressionArray(mp)),
	  m_built(false),
	  m_pure_inner(true),
	  m_cdc_supported(true)
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
										BOOL include_complex,
										BOOL parent_is_join)
{
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp && nullptr != expr);
	const COperator::EOperatorId op_id = expr->Pop()->Eopid();
	const BOOL is_join = COperator::EopLogicalInnerJoin == op_id ||
						 (include_complex && FCDCSupportedJoin(op_id));
	if (!is_join && 0 == expr->Arity() && nullptr != expr->Pgexpr())
	{
		expr->Pop()->AddRef();
		return GPOS_NEW(mp) CExpression(mp, expr->Pop(), expr->Pgexpr());
	}
	CExpressionArray *children = GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG child = 0; child < expr->Arity(); ++child)
	{
		children->Append(PexprMarkDPHyperRegions(
			mp, (*expr)[child], include_complex, is_join && child < 2));
	}

	COperator *op = nullptr;
	if (is_join)
	{
		CLogicalJoin *join = CLogicalJoin::PopConvert(expr->Pop());
		const BOOL region_root = !parent_is_join;
		switch (op_id)
		{
			case COperator::EopLogicalInnerJoin:
				op = GPOS_NEW(mp) CLogicalInnerJoin(
					mp, join->OriginXform(), true, region_root);
				break;
			case COperator::EopLogicalLeftOuterJoin:
				op = GPOS_NEW(mp) CLogicalLeftOuterJoin(
					mp, join->OriginXform(), true, region_root);
				break;
			case COperator::EopLogicalLeftSemiJoin:
				op = GPOS_NEW(mp) CLogicalLeftSemiJoin(
					mp, join->OriginXform(), true, region_root);
				break;
			case COperator::EopLogicalLeftAntiSemiJoin:
				op = GPOS_NEW(mp) CLogicalLeftAntiSemiJoin(
					mp, join->OriginXform(), true, region_root);
				break;
			case COperator::EopLogicalFullOuterJoin:
				op = GPOS_NEW(mp) CLogicalFullOuterJoin(
					mp, join->OriginXform(), true, region_root);
				break;
			default:
				GPOS_ASSERT(!"Unsupported mixed DPHyper join type");
		}
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

BOOL
CJoinRegionSpec::FCDCSupportedJoin(COperator::EOperatorId op_id)
{
	switch (op_id)
	{
		case COperator::EopLogicalInnerJoin:
		case COperator::EopLogicalLeftOuterJoin:
		case COperator::EopLogicalLeftSemiJoin:
		case COperator::EopLogicalLeftAntiSemiJoin:
		case COperator::EopLogicalFullOuterJoin:
			return true;

		default:
			return false;
	}
}

BOOL
CJoinRegionSpec::FInnerJoin(COperator::EOperatorId op_id)
{
	return COperator::EopLogicalInnerJoin == op_id;
}

CBitSet *
CJoinRegionSpec::PbsPredicateCover(CExpression *predicate) const
{
	CBitSet *cover = GPOS_NEW(m_mp) CBitSet(m_mp);
	CColRefSet *used = predicate->DeriveUsedColumns();
	for (ULONG node = 0; node < m_atoms->Size(); ++node)
	{
		if (!used->IsDisjoint((*m_atoms)[node]->DeriveOutputColumns()))
		{
			(void) cover->ExchangeSet(node);
		}
	}
	return cover;
}

CColRefSet *
CJoinRegionSpec::PcrsNodes(const CBitSet *nodes) const
{
	CColRefSet *columns = GPOS_NEW(m_mp) CColRefSet(m_mp);
	CBitSetIter iter(*nodes);
	while (iter.Advance())
	{
		columns->Union((*m_atoms)[iter.Bit()]->DeriveOutputColumns());
	}
	return columns;
}

BOOL
CJoinRegionSpec::FNullRejecting(const CEdge *edge,
								const CBitSet *nodes) const
{
	CAutoRef<CColRefSet> columns(PcrsNodes(nodes));
	return CPredicateUtils::FNullRejecting(m_mp, edge->Predicate(),
										 columns.Value());
}

BOOL
CJoinRegionSpec::FAssociative(const CEdge *left,
								 const CEdge *right) const
{
	const COperator::EOperatorId left_type = left->JoinType();
	const COperator::EOperatorId right_type = right->JoinType();
	if ((COperator::EopLogicalLeftOuterJoin == left_type ||
		 COperator::EopLogicalFullOuterJoin == left_type) &&
		COperator::EopLogicalLeftOuterJoin == right_type)
	{
		return FNullRejecting(right, right->Left());
	}
	if (COperator::EopLogicalFullOuterJoin == left_type &&
		COperator::EopLogicalFullOuterJoin == right_type)
	{
		return FNullRejecting(left, left->Right()) &&
			   FNullRejecting(right, right->Left());
	}
	return FInnerJoin(left_type) &&
		   COperator::EopLogicalFullOuterJoin != right_type;
}

BOOL
CJoinRegionSpec::FLeftAsscom(const CEdge *left,
								const CEdge *right) const
{
	const COperator::EOperatorId left_type = left->JoinType();
	const COperator::EOperatorId right_type = right->JoinType();
	if (COperator::EopLogicalLeftOuterJoin == left_type)
	{
		return COperator::EopLogicalFullOuterJoin != right_type ||
			   FNullRejecting(left, left->Left());
	}
	if (COperator::EopLogicalFullOuterJoin == left_type)
	{
		if (COperator::EopLogicalLeftOuterJoin == right_type)
		{
			return FNullRejecting(right, right->Right());
		}
		if (COperator::EopLogicalFullOuterJoin == right_type)
		{
			return FNullRejecting(left, left->Left()) &&
				   FNullRejecting(right, right->Left());
		}
		return false;
	}
	return COperator::EopLogicalFullOuterJoin != right_type;
}

BOOL
CJoinRegionSpec::FRightAsscom(const CEdge *left,
								 const CEdge *right) const
{
	if (COperator::EopLogicalFullOuterJoin == left->JoinType() &&
		COperator::EopLogicalFullOuterJoin == right->JoinType())
	{
		return FNullRejecting(left, left->Right()) &&
			   FNullRejecting(right, right->Right());
	}
	return FInnerJoin(left->JoinType()) && FInnerJoin(right->JoinType());
}

CBitSet *
CJoinRegionSpec::PbsIntersectIfNotDegenerate(
	const CBitSet *used, const CBitSet *available) const
{
	CBitSet *result = GPOS_NEW(m_mp) CBitSet(m_mp, *used);
	result->Intersection(available);
	if (0 == result->Size())
	{
		result->Release();
		result = GPOS_NEW(m_mp) CBitSet(m_mp, *available);
	}
	return result;
}

void
CJoinRegionSpec::AddConflictRule(CEdge *edge, const CBitSet *activate,
								 const CBitSet *required)
{
	GPOS_ASSERT(0 < activate->Size() && 0 < required->Size());
	edge->m_conflict_rules.push_back(
		GPOS_NEW(m_mp) CConflictRule(m_mp, activate, required));
}

void
CJoinRegionSpec::AbsorbConflictRules(CEdge *edge)
{
	BOOL changed = true;
	while (changed)
	{
		changed = false;
		for (const CConflictRule *rule : edge->m_conflict_rules)
		{
			if (!edge->m_tes->IsDisjoint(rule->Activate()) &&
				!edge->m_tes->ContainsAll(rule->Required()))
			{
				edge->m_tes->Union(rule->Required());
				changed = true;
			}
		}
	}

	auto iter = edge->m_conflict_rules.begin();
	while (iter != edge->m_conflict_rules.end())
	{
		CConflictRule *rule = *iter;
		if (edge->m_tes->ContainsAll(rule->Required()))
		{
			GPOS_DELETE(rule);
			iter = edge->m_conflict_rules.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

void
CJoinRegionSpec::BuildEligibility(CEdge *edge)
{
	edge->m_ses = PbsPredicateCover(edge->Predicate());
	edge->m_tes = GPOS_NEW(m_mp) CBitSet(m_mp, *edge->m_ses);
	for (CEdge *child : m_edges)
	{
		if (child == edge)
		{
			break;
		}
		if (edge->Left()->ContainsAll(child->All()))
		{
			if (!FAssociative(child, edge))
			{
				CAutoRef<CBitSet> required(PbsIntersectIfNotDegenerate(
					child->SES(), child->Left()));
				AddConflictRule(edge, child->Right(), required.Value());
			}
			if (!FLeftAsscom(child, edge))
			{
				CAutoRef<CBitSet> required(PbsIntersectIfNotDegenerate(
					child->SES(), child->Right()));
				AddConflictRule(edge, child->Left(), required.Value());
			}
		}
		else if (edge->Right()->ContainsAll(child->All()))
		{
			if (!FAssociative(edge, child))
			{
				CAutoRef<CBitSet> required(PbsIntersectIfNotDegenerate(
					child->SES(), child->Right()));
				AddConflictRule(edge, child->Left(), required.Value());
			}
			if (!FRightAsscom(edge, child))
			{
				CAutoRef<CBitSet> required(PbsIntersectIfNotDegenerate(
					child->SES(), child->Left()));
				AddConflictRule(edge, child->Right(), required.Value());
			}
		}
	}

	AbsorbConflictRules(edge);
	if (edge->m_tes->IsDisjoint(edge->Left()))
	{
		edge->m_tes->Union(edge->Left());
		AbsorbConflictRules(edge);
	}
	if (edge->m_tes->IsDisjoint(edge->Right()))
	{
		edge->m_tes->Union(edge->Right());
		AbsorbConflictRules(edge);
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
	if (!FCDCSupportedJoin(op_id))
	{
		m_cdc_supported = false;
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
	const BOOL valid = 2 <= m_atoms->Size() && !m_edges.empty() &&
					   all_nodes->Size() == m_atoms->Size();
	if (valid && m_cdc_supported)
	{
		for (CEdge *edge : m_edges)
		{
			BuildEligibility(edge);
		}
	}
	return valid;
}
