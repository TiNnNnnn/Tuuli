//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLRewriteDecision.h
//
//	@doc:
//		Scheduler-neutral result of match -> constraints -> instantiate.  The
//		CBO adapter and the forthcoming pre-Memo RBO scheduler consume the same
//		decision and independently perform budget reservation/insertion/replacement.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLRewriteDecision_H
#define GPOPT_CDSLRewriteDecision_H

#include "gpos/base.h"

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/operators/CExpression.h"

namespace gpopt
{
using namespace gpos;

enum EDslRewriteDecisionStatus
{
	EdsldecisionMatchRejected,
	EdsldecisionConstraintRejected,
	EdsldecisionInstantiateRejected,
	EdsldecisionDuplicate,
	EdsldecisionReady
};

class CDSLRewriteDecision
{
private:
	EDslRewriteDecisionStatus m_status;
	CDSLModel *m_pmodel;
	CExpression *m_pexprTarget;
	const CDSLConstraint *m_pconFailed;
	ULONG m_ulFailedConstraint;
	ULONG m_ulMatchUs;
	ULONG m_ulConstraintUs;
	ULONG m_ulInstantiateUs;
	ULONG m_ulSourceFingerprint;
	ULONG m_ulTargetFingerprint;

public:
	CDSLRewriteDecision(const CDSLRewriteDecision &) = delete;

	CDSLRewriteDecision(CDSLModel *pmodel, CExpression *pexprTarget,
						EDslRewriteDecisionStatus status,
						const CDSLConstraint *pconFailed,
						ULONG ulFailedConstraint, ULONG ulMatchUs,
						ULONG ulConstraintUs, ULONG ulInstantiateUs,
						ULONG ulSourceFingerprint, ULONG ulTargetFingerprint);
	~CDSLRewriteDecision();

	EDslRewriteDecisionStatus Status() const { return m_status; }
	CDSLModel *Pmodel() const { return m_pmodel; }
	CExpression *PexprTarget() const { return m_pexprTarget; }
	const CDSLConstraint *PconFailed() const { return m_pconFailed; }
	ULONG UlFailedConstraint() const { return m_ulFailedConstraint; }
	ULONG UlMatchUs() const { return m_ulMatchUs; }
	ULONG UlConstraintUs() const { return m_ulConstraintUs; }
	ULONG UlInstantiateUs() const { return m_ulInstantiateUs; }
	ULONG UlSourceFingerprint() const { return m_ulSourceFingerprint; }
	ULONG UlTargetFingerprint() const { return m_ulTargetFingerprint; }

	// Transfer the sole target reference to a scheduler.
	CExpression *PexprDetachTarget();
};
}  // namespace gpopt

#endif  // !GPOPT_CDSLRewriteDecision_H
