//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformDSLRule_SetOp_H
#define GPOPT_CXformDSLRule_SetOp_H

#include "gpopt/xforms/CXformExploration.h"

namespace gpopt
{
class CXformDSLRule_SetOp : public CXformExploration
{
private:
	const EXformId m_exfid;
	const COperator::EOperatorId m_eopid;
	const CHAR *m_szId;

public:
	CXformDSLRule_SetOp(const CXformDSLRule_SetOp &) = delete;
	CXformDSLRule_SetOp(CMemoryPool *mp, COperator::EOperatorId eopid,
						 EXformId exfid, const CHAR *szId);
	~CXformDSLRule_SetOp() override = default;

	EXformId Exfid() const override { return m_exfid; }
	const CHAR *SzId() const override { return m_szId; }
	EXformPromise Exfp(CExpressionHandle &) const override;
	void Transform(CXformContext *, CXformResult *, CExpression *) const override;
};
}  // namespace gpopt

#endif	// !GPOPT_CXformDSLRule_SetOp_H
