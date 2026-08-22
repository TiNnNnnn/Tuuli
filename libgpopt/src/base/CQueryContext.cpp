//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CQueryContext.cpp
//
//	@doc:
//		Implementation of optimization context
//---------------------------------------------------------------------------

#include "gpopt/base/CQueryContext.h"

#include "gpos/base.h"
#include "gpos/error/CAutoTrace.h"

#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CColumnFactory.h"
#include "gpopt/base/CDistributionSpecAny.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/dsl/CDSLRuleEngine.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CLogicalProject.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"

using namespace gpopt;

FORCE_GENERATE_DBGSTR(CQueryContext);

//---------------------------------------------------------------------------
//	@function:
//		CQueryContext::CQueryContext
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CQueryContext::CQueryContext(CMemoryPool *mp, CExpression *pexpr,
							 CReqdPropPlan *prpp, CColRefArray *colref_array,
							 CMDNameArray *pdrgpmdname, BOOL fDeriveStats)
	: m_prpp(prpp),
	  m_pdrgpcr(colref_array),
	  m_pdrgpcrSystemCols(nullptr),
	  m_pdrgpmdname(pdrgpmdname),
	  m_fDeriveStats(fDeriveStats)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != prpp);
	GPOS_ASSERT(nullptr != colref_array);
	GPOS_ASSERT(nullptr != pdrgpmdname);
	GPOS_ASSERT(colref_array->Size() == pdrgpmdname->Size());

#ifdef GPOS_DEBUG
	const ULONG ulReqdColumns = m_pdrgpcr->Size();
#endif	//GPOS_DEBUG

	CColRefSet *pcrsOutputAndOrderingCols = GPOS_NEW(mp) CColRefSet(mp);
	CColRefSet *pcrsOrderSpec = prpp->Peo()->PosRequired()->PcrsUsed(mp);

	pcrsOutputAndOrderingCols->Include(colref_array);
	pcrsOutputAndOrderingCols->Include(pcrsOrderSpec);
	pcrsOrderSpec->Release();

	m_pexpr = CExpressionPreprocessor::PexprPreprocess(
		mp, pexpr, pcrsOutputAndOrderingCols);

	pcrsOutputAndOrderingCols->Release();
	GPOS_ASSERT(m_pdrgpcr->Size() == ulReqdColumns);

	// collect required system columns
	SetSystemCols(mp);

	// collect CTE predicates and add them to CTE producer expressions
	CExpressionPreprocessor::AddPredsToCTEProducers(mp, m_pexpr);

	CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();

	// create the mapping between the computed column, defined in the expression
	// and all CTEs, and its corresponding used columns
	MapComputedToUsedCols(col_factory, m_pexpr);

	CCTEInfo *pcteinfo = COptCtxt::PoctxtFromTLS()->Pcteinfo();
	pcteinfo->MapComputedToUsedCols(col_factory);
}


//---------------------------------------------------------------------------
//	@function:
//		CQueryContext::~CQueryContext
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CQueryContext::~CQueryContext()
{
	m_pexpr->Release();
	m_prpp->Release();
	m_pdrgpcr->Release();
	m_pdrgpmdname->Release();
	CRefCount::SafeRelease(m_pdrgpcrSystemCols);
}


//---------------------------------------------------------------------------
//	@function:
//		CQueryContext::PopTop
//
//	@doc:
// 		 Return top level operator in the given expression
//
//---------------------------------------------------------------------------
COperator *
CQueryContext::PopTop(CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	// skip CTE anchors if any
	CExpression *pexprCurr = pexpr;
	while (COperator::EopLogicalCTEAnchor == pexprCurr->Pop()->Eopid())
	{
		pexprCurr = (*pexprCurr)[0];
		GPOS_ASSERT(nullptr != pexprCurr);
	}

	return pexprCurr->Pop();
}

//---------------------------------------------------------------------------
//	@function:
//		CQueryContext::SetReqdSystemCols
//
//	@doc:
// 		Collect system columns from output columns
//
//---------------------------------------------------------------------------
void
CQueryContext::SetSystemCols(CMemoryPool *mp)
{
	GPOS_ASSERT(nullptr == m_pdrgpcrSystemCols);
	GPOS_ASSERT(nullptr != m_pdrgpcr);

	m_pdrgpcrSystemCols = GPOS_NEW(mp) CColRefArray(mp);
	const ULONG ulReqdCols = m_pdrgpcr->Size();
	for (ULONG ul = 0; ul < ulReqdCols; ul++)
	{
		CColRef *colref = (*m_pdrgpcr)[ul];
		if (colref->IsSystemCol())
		{
			m_pdrgpcrSystemCols->Append(colref);
		}
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CQueryContext::PqcGenerate
//
//	@doc:
// 		Generate the query context for the given expression and array of
//		output column ref ids
//
//---------------------------------------------------------------------------
CQueryContext *
CQueryContext::PqcGenerate(CMemoryPool *mp, CExpression *pexpr,
						   ULongPtrArray *pdrgpulQueryOutputColRefId,
						   CMDNameArray *pdrgpmdname, BOOL fDeriveStats)
{
	GPOS_ASSERT(nullptr != pexpr && nullptr != pdrgpulQueryOutputColRefId);

	CColRefSet *pcrs = GPOS_NEW(mp) CColRefSet(mp);
	CColRefArray *colref_array = GPOS_NEW(mp) CColRefArray(mp);

	COptCtxt *poptctxt = COptCtxt::PoctxtFromTLS();
	CColumnFactory *col_factory = poptctxt->Pcf();
	GPOS_ASSERT(nullptr != col_factory);

	// Collect required column references (colref_array)
	const ULONG length = pdrgpulQueryOutputColRefId->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		ULONG *pul = (*pdrgpulQueryOutputColRefId)[ul];
		GPOS_ASSERT(nullptr != pul);

		CColRef *colref = col_factory->LookupColRef(*pul);
		GPOS_ASSERT(nullptr != colref);

		pcrs->Include(colref);
		colref_array->Append(colref);
	}

	CExpression *pexprResult = pexpr;
	COperator *popOriginalTop = PopTop(pexpr);
	BOOL fDML = CUtils::FLogicalDML(pexpr->Pop());
	CDSLRuleEngine *pengine = CDSLRuleEngine::Instance();
	BOOL fHasSystemOutput = false;
	for (ULONG ul = 0; ul < colref_array->Size(); ul++)
	{
		fHasSystemOutput =
			fHasSystemOutput || (*colref_array)[ul]->IsSystemCol();
	}

	// ORCA normally carries a pass-through SELECT list only as the query's
	// required-column property. That makes a source-root Proj rule invisible to
	// Cascades even though the SQL/WeTune tree has a real projection boundary.
	// When the loaded data rules need that surface, preserve it as fresh output
	// aliases over the translated query. Requiring the fresh aliases prevents
	// preprocessing from pruning the identity Project; keeping this disabled for
	// DML/system columns and already-explicit Projects avoids changing special
	// query contracts or adding redundant compute-scalar layers.
	if (!fDML && !fHasSystemOutput && 0 < colref_array->Size() &&
		nullptr != pengine && pengine->FHasOrdinaryProjSourceRoot() &&
		COperator::EopLogicalProject != popOriginalTop->Eopid())
	{
		CColRefArray *pdrgpcrProjected = GPOS_NEW(mp) CColRefArray(mp);
		CExpressionArray *pdrgpexprElems =
			GPOS_NEW(mp) CExpressionArray(mp);
		for (ULONG ul = 0; ul < colref_array->Size(); ul++)
		{
			CColRef *pcrInput = (*colref_array)[ul];
			CColRef *pcrOutput = col_factory->PcrCreate(
				pcrInput->RetrieveType(), pcrInput->TypeModifier(),
				pcrInput->Name());
			pdrgpcrProjected->Append(pcrOutput);
			pdrgpexprElems->Append(GPOS_NEW(mp) CExpression(
				mp, GPOS_NEW(mp) CScalarProjectElement(mp, pcrOutput),
				GPOS_NEW(mp) CExpression(
					mp, GPOS_NEW(mp) CScalarIdent(mp, pcrInput))));
		}
		CExpression *pexprProjList = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarProjectList(mp), pdrgpexprElems);
		pexpr->AddRef();
		pexprResult = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CLogicalProject(mp), pexpr, pexprProjList);

		colref_array->Release();
		colref_array = pdrgpcrProjected;
		pcrs->Release();
		pcrs = GPOS_NEW(mp) CColRefSet(mp);
		pcrs->Include(colref_array);
	}

	// Collect required properties (prpp) at the top level:

	// By default no sort order requirement is added, unless the root operator in
	// the input logical expression is a LIMIT. This is because Orca always
	// attaches top level Sort to a LIMIT node.
	COrderSpec *pos = nullptr;
	if (COperator::EopLogicalLimit == popOriginalTop->Eopid())
	{
		// top level operator is a limit, copy order spec to query context
		pos = CLogicalLimit::PopConvert(popOriginalTop)->Pos();
		pos->AddRef();
	}
	else
	{
		// no order required
		pos = GPOS_NEW(mp) COrderSpec(mp);
	}

	CDistributionSpec *pds = nullptr;

	poptctxt->MarkDMLQuery(fDML);

	// DML commands do not have distribution requirement. Otherwise the
	// distribution requirement is Singleton.
	if (fDML)
	{
		pds = GPOS_NEW(mp) CDistributionSpecAny(COperator::EopSentinel);
	}
	else
	{
		pds = GPOS_NEW(mp)
			CDistributionSpecSingleton(CDistributionSpecSingleton::EstMaster);
	}

	// By default, no rewindability requirement needs to be satisfied at the top level
	CRewindabilitySpec *prs = GPOS_NEW(mp) CRewindabilitySpec(
		CRewindabilitySpec::ErtNone, CRewindabilitySpec::EmhtNoMotion);

	// No partition propagation required at the top
	CPartitionPropagationSpec *ppps =
		GPOS_NEW(mp) CPartitionPropagationSpec(mp);

	// Ensure order, distribution and rewindability meet 'satisfy' matching at the top level
	CEnfdOrder *peo = GPOS_NEW(mp) CEnfdOrder(pos, CEnfdOrder::EomSatisfy);
	CEnfdDistribution *ped =
		GPOS_NEW(mp) CEnfdDistribution(pds, CEnfdDistribution::EdmSatisfy);
	CEnfdRewindability *per =
		GPOS_NEW(mp) CEnfdRewindability(prs, CEnfdRewindability::ErmSatisfy);
	CEnfdPartitionPropagation *pepp = GPOS_NEW(mp)
		CEnfdPartitionPropagation(ppps, CEnfdPartitionPropagation::EppmSatisfy);

	// Required CTEs are obtained from the CTEInfo global information in the optimizer context
	CCTEReq *pcter = poptctxt->Pcteinfo()->PcterProducers(mp);

	// NB: Partition propagation requirements are not initialized here.  They are
	// constructed later based on derived relation properties (CPartInfo) by
	// CReqdPropPlan::InitReqdPartitionPropagation().

	CReqdPropPlan *prpp =
		GPOS_NEW(mp) CReqdPropPlan(pcrs, peo, ped, per, pepp, pcter);

	// Finally, create the CQueryContext
	pdrgpmdname->AddRef();
	return GPOS_NEW(mp) CQueryContext(mp, pexprResult, prpp, colref_array,
									  pdrgpmdname, fDeriveStats);
}

#ifdef GPOS_DEBUG

//---------------------------------------------------------------------------
//	@function:
//		CQueryContext::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CQueryContext::OsPrint(IOstream &os) const
{
	return os << *m_pexpr << std::endl << *m_prpp;
}
#endif	// GPOS_DEBUG


//---------------------------------------------------------------------------
//	@function:
//		CQueryContext::MapComputedToUsedCols
//
//	@doc:
//		Walk the expression and add the mapping between computed column
//		and its used columns
//
//---------------------------------------------------------------------------
void
CQueryContext::MapComputedToUsedCols(CColumnFactory *col_factory,
									 CExpression *pexpr)
{
	GPOS_ASSERT(nullptr != pexpr);

	if (COperator::EopLogicalProject == pexpr->Pop()->Eopid())
	{
		CExpression *pexprPrL = (*pexpr)[1];

		const ULONG arity = pexprPrL->Arity();
		for (ULONG ul = 0; ul < arity; ul++)
		{
			CExpression *pexprPrEl = (*pexprPrL)[ul];
			col_factory->AddComputedToUsedColsMap(pexprPrEl);
		}
	}

	// process children
	const ULONG ulChildren = pexpr->Arity();
	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		MapComputedToUsedCols(col_factory, (*pexpr)[ul]);
	}
}

// EOF
