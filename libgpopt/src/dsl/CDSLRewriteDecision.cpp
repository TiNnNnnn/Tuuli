//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLRewriteDecision.cpp
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLRewriteDecision.h"

using namespace gpopt;

CDSLRewriteDecision::CDSLRewriteDecision(
	CDSLModel *pmodel, CExpression *pexprTarget,
	EDslRewriteDecisionStatus status, const CDSLConstraint *pconFailed,
	ULONG ulFailedConstraint, ULONG ulMatchUs, ULONG ulConstraintUs,
	ULONG ulInstantiateUs, ULONG ulSourceFingerprint,
	ULONG ulTargetFingerprint)
	: m_status(status),
	  m_pmodel(pmodel),
	  m_pexprTarget(pexprTarget),
	  m_pconFailed(pconFailed),
	  m_ulFailedConstraint(ulFailedConstraint),
	  m_ulMatchUs(ulMatchUs),
	  m_ulConstraintUs(ulConstraintUs),
	  m_ulInstantiateUs(ulInstantiateUs),
	  m_ulSourceFingerprint(ulSourceFingerprint),
	  m_ulTargetFingerprint(ulTargetFingerprint)
{
	GPOS_ASSERT(nullptr != pmodel);
	GPOS_ASSERT((EdsldecisionReady == status || EdsldecisionDuplicate == status) ==
				(nullptr != pexprTarget));
}

CDSLRewriteDecision::~CDSLRewriteDecision()
{
	CRefCount::SafeRelease(m_pexprTarget);
	CRefCount::SafeRelease(m_pmodel);
}

CDSLModel *
CDSLRewriteDecision::PmodelDetach()
{
	CDSLModel *pmodel = m_pmodel;
	m_pmodel = nullptr;
	return pmodel;
}

CExpression *
CDSLRewriteDecision::PexprDetachTarget()
{
	CExpression *target = m_pexprTarget;
	m_pexprTarget = nullptr;
	return target;
}
