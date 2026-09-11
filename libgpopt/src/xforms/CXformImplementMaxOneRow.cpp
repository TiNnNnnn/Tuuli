#include "gpopt/xforms/CXformImplementMaxOneRow.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/cost/ICostModel.h"
#include "gpopt/operators/CLogicalMaxOneRow.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPhysicalAssert.h"

using namespace gpopt;

CXformImplementMaxOneRow::CXformImplementMaxOneRow(CMemoryPool *mp)
	: CXformImplementation(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalMaxOneRow(mp),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp))))
{
}

CXform::EXformPromise
CXformImplementMaxOneRow::Exfp(CExpressionHandle &) const
{
	return COptCtxt::PoctxtFromTLS()->GetCostModel()->Ecmt() ==
				   ICostModel::EcmtPG
			   ? ExfpHigh
			   : ExfpNone;
}

void
CXformImplementMaxOneRow::Transform(CXformContext *context,
									CXformResult *result,
									CExpression *expr) const
{
	CMemoryPool *mp = context->Pmp();
	// The existing Assert executor checks cardinality as it consumes tuples.
	// Keep the relational child (and its Memo statistics contract) unchanged.
	(*expr)[0]->AddRef();
	result->Add(GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CPhysicalAssert(
			mp,
			GPOS_NEW(mp)
				CException(CException::ExmaSQL, CException::ExmiSQLMaxOneRow),
			true),
		(*expr)[0]));
}
