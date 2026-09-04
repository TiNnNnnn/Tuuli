//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLConstraintChecker.cpp
//
//	@doc:
//		Implementation of the constraint checker (see CDSLConstraintChecker.h).
//		Migrates the SEMANTICS of WeTune Model.checkConstraints for the four
//		structural constraints (AttrsSub / Unique / NotNull / Reference).
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLConstraintChecker.h"

#include "gpopt/dsl/CDSLInstantiator.h"
#include "gpopt/dsl/CDSLExprListUtils.h"
#include "gpopt/dsl/CDSLExpressionDefinitions.h"
#include "gpopt/dsl/CDSLQuantifiedMatcher.h"

#include "gpos/base.h"
#include "gpos/error/CException.h"

#include "naucrates/exception.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CColRefTable.h"
#include "gpopt/base/CCastUtils.h"
#include "gpopt/base/CPropConstraint.h"
#include "gpopt/base/CKeyCollection.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CLogical.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalGet.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CScalarAggFunc.h"
#include "gpopt/operators/CScalarCast.h"
#include "gpopt/operators/CScalarConst.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarIf.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarSubquery.h"
#include "gpopt/operators/CScalarSubqueryQuantified.h"
#include "gpopt/operators/CScalarWindowFunc.h"
#include "naucrates/md/CMDForeignKey.h"
#include "naucrates/md/CMDIdGPDB.h"
#include "naucrates/md/CMDTypeInt2GPDB.h"
#include "naucrates/md/CMDTypeInt4GPDB.h"
#include "naucrates/md/CMDTypeInt8GPDB.h"
#include "naucrates/dxl/gpdb_types.h"
#include "naucrates/md/IMDFunction.h"
#include "naucrates/md/IMDRelation.h"
#include "naucrates/md/IMDTypeBool.h"
#include "naucrates/base/IDatumInt2.h"
#include "naucrates/base/IDatumInt4.h"
#include "naucrates/base/IDatumInt8.h"

using namespace gpopt;
using namespace gpnaucrates;

namespace
{
BOOL
FScalarCastProvablyErrorFree(CExpression *pexpr)
{
	CScalarCast *popCast = CScalarCast::PopConvert(pexpr->Pop());
	if (popCast->IsBinaryCoercible())
	{
		return true;
	}
	if (1 != pexpr->Arity())
	{
		return false;
	}

	IMDId *pmdidSource = CScalar::PopConvert((*pexpr)[0]->Pop())->MdidType();
	IMDId *pmdidTarget = popCast->MdidType();
	if (IMDId::EmdidGeneral != pmdidSource->MdidType() ||
		IMDId::EmdidGeneral != pmdidTarget->MdidType())
	{
		return false;
	}
	const OID oidSource = CMDIdGPDB::CastMdid(pmdidSource)->Oid();
	const OID oidTarget = CMDIdGPDB::CastMdid(pmdidTarget)->Oid();

	// PostgreSQL's signed-integer widening casts are total over their source
	// domains. Keep the whitelist directional so narrowing and parsing casts
	// remain conservatively rejected.
	return (GPDB_INT2 == oidSource &&
		 (GPDB_INT4 == oidTarget || GPDB_INT8 == oidTarget)) ||
		(GPDB_INT4 == oidSource && GPDB_INT8 == oidTarget);
}

BOOL
FAggFuncProvablyErrorFree(CScalarAggFunc *popAgg)
{
	if (popAgg->FCountStar() || popAgg->FCountAny())
	{
		return true;
	}
	IMDId *pmdid = popAgg->MDId();
	if (IMDId::EmdidGeneral != pmdid->MdidType())
	{
		return false;
	}
	const OID oid = CMDIdGPDB::CastMdid(pmdid)->Oid();
	switch (oid)
	{
		case GPDB_INT2_AGG_MIN:
		case GPDB_INT2_AGG_MAX:
		case GPDB_INT4_AGG_MIN:
		case GPDB_INT4_AGG_MAX:
		case GPDB_INT8_AGG_MIN:
		case GPDB_INT8_AGG_MAX:
			return true;
		default:
			return false;
	}
}

BOOL
FWindowFuncProvablyErrorFree(CScalarWindowFunc *popWindow)
{
	if (!popWindow->FAgg() ||
		IMDId::EmdidGeneral != popWindow->FuncMdId()->MdidType())
	{
		return false;
	}
	const OID oid = CMDIdGPDB::CastMdid(popWindow->FuncMdId())->Oid();
	switch (oid)
	{
		case GPDB_COUNT_STAR:
		case GPDB_INT4_AGG_COUNT:
		case GPDB_INT2_AGG_MIN:
		case GPDB_INT2_AGG_MAX:
		case GPDB_INT4_AGG_MIN:
		case GPDB_INT4_AGG_MAX:
		case GPDB_INT8_AGG_MIN:
		case GPDB_INT8_AGG_MAX:
			return true;
		default:
			return false;
	}
}

CExpression *
PexprProjectListForAttrs(const CDSLOp *pop, const CDSLSymbol *psymAttrs,
						 const CDSLModel *pmodel)
{
	if (EdslopProj == pop->Edslop() && !pop->FDistinct() &&
		nullptr != pop->Pdrgpsym() && 2 == pop->Pdrgpsym()->Size() &&
		(*pop->Pdrgpsym())[0] == psymAttrs)
	{
		return pmodel->PexprProjList((*pop->Pdrgpsym())[1]);
	}
	for (ULONG ul = 0; ul < pop->UlChildren(); ul++)
	{
		CExpression *pexpr =
			PexprProjectListForAttrs((*pop)[ul], psymAttrs, pmodel);
		if (nullptr != pexpr)
		{
			return pexpr;
		}
	}
	return nullptr;
}

const CColRef *
PcrIdentityInputInProjects(const CDSLRule *prule, const CDSLOp *pop,
						   const CDSLModel *pmodel,
						   const CColRef *pcrOutput)
{
	BOOL fSafeProject = false;
	if (EdslopProj == pop->Edslop() && !pop->FDistinct() &&
		nullptr != pop->Pdrgpsym() && 2 == pop->Pdrgpsym()->Size())
	{
		const CDSLSymbol *psymAttrs = (*pop->Pdrgpsym())[0];
		BOOL fErrorFree = false;
		BOOL fDeterministic = false;
		CDSLConstraintArray *pdrgpcon = prule->Pdrgpcon();
		for (ULONG ul = 0; ul < pdrgpcon->Size(); ul++)
		{
			const CDSLConstraint *pcon = (*pdrgpcon)[ul];
			if (1 != pcon->Pdrgpsym()->Size() ||
				(*pcon->Pdrgpsym())[0] != psymAttrs)
			{
				continue;
			}
			fErrorFree = fErrorFree || EdslconErrorFree == pcon->Edslcon();
			fDeterministic =
				fDeterministic || EdslconDeterministic == pcon->Edslcon();
		}
		fSafeProject = fErrorFree && fDeterministic;
	}
	if (fSafeProject)
	{
		CExpression *pexprList =
			pmodel->PexprProjList((*pop->Pdrgpsym())[1]);
		if (nullptr != pexprList &&
			COperator::EopScalarProjectList == pexprList->Pop()->Eopid())
		{
			for (ULONG ul = 0; ul < pexprList->Arity(); ul++)
			{
				CExpression *pexprElem = (*pexprList)[ul];
				if (COperator::EopScalarProjectElement !=
						pexprElem->Pop()->Eopid() ||
					1 != pexprElem->Arity() ||
					CScalarProjectElement::PopConvert(pexprElem->Pop())->Pcr() !=
						pcrOutput ||
					COperator::EopScalarIdent !=
						(*pexprElem)[0]->Pop()->Eopid())
				{
					continue;
				}
				return CScalarIdent::PopConvert((*pexprElem)[0]->Pop())->Pcr();
			}
		}
	}
	for (ULONG ul = 0; ul < pop->UlChildren(); ul++)
	{
		const CColRef *pcrInput =
			PcrIdentityInputInProjects(prule, (*pop)[ul], pmodel, pcrOutput);
		if (nullptr != pcrInput)
		{
			return pcrInput;
		}
	}
	return nullptr;
}

const CColRef *
PcrResolveIdentityLineage(const CDSLRule *prule, const CDSLModel *pmodel,
						  const CColRef *pcr)
{
	const CColRef *pcrCurrent = pcr;
	// A source fragment cannot contain an unbounded Project chain. The explicit
	// cap is defensive against malformed self-referential project elements.
	for (ULONG ul = 0; ul < 64; ul++)
	{
		const CColRef *pcrNext = PcrIdentityInputInProjects(
			prule, prule->PfragSrc()->PopRoot(), pmodel, pcrCurrent);
		if (nullptr == pcrNext || pcrNext == pcrCurrent)
		{
			break;
		}
		pcrCurrent = pcrNext;
	}
	return pcrCurrent;
}

BOOL
FScalarTreeProvablyErrorFree(CExpression *pexpr)
{
	switch (pexpr->Pop()->Eopid())
	{
		case COperator::EopScalarIdent:
		case COperator::EopScalarConst:
			return true;
		case COperator::EopScalarCmp:
			// Admit every comparison in ORCA's explicit built-in strict whitelist.
			// This includes the <>/< <=/> >= operators used by quantified ALL, while
			// still rejecting user-defined comparisons whose evaluation may throw.
			if (!CPredicateUtils::FBuiltInComparisonIsVeryStrict(
					CScalarCmp::PopConvert(pexpr->Pop())->MdIdOp()))
			{
				return false;
			}
			break;
		case COperator::EopScalarCast:
			// Binary coercions and explicitly whitelisted widening casts are total.
			// Parsing or narrowing casts can still fail and remain rejected.
			if (!FScalarCastProvablyErrorFree(pexpr))
			{
				return false;
			}
			break;
		case COperator::EopScalarAggFunc:
		{
			// Admit only aggregates whose exact built-in OID has a total transition:
			// COUNT and signed-integer MIN/MAX. SUM/AVG can overflow, and user-defined
			// aggregates remain rejected without an equivalent metadata property.
			CScalarAggFunc *popAgg =
				CScalarAggFunc::PopConvert(pexpr->Pop());
			if (!FAggFuncProvablyErrorFree(popAgg))
			{
				return false;
			}
			break;
		}
		case COperator::EopScalarWindowFunc:
			if (!FWindowFuncProvablyErrorFree(
					CScalarWindowFunc::PopConvert(pexpr->Pop())))
			{
				return false;
			}
			break;
		case COperator::EopScalarNullTest:
		case COperator::EopScalarBoolOp:
		case COperator::EopScalarValuesList:
		case COperator::EopScalarProjectElement:
		case COperator::EopScalarProjectList:
			break;
		default:
			// Function/operator error behavior is not represented in the current
			// ORCA scalar metadata. Reject unknown shapes instead of assuming that
			// evaluation can be duplicated, removed, or reordered.
			return false;
	}
	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		if (!FScalarTreeProvablyErrorFree((*pexpr)[ul]))
		{
			return false;
		}
	}
	return true;
}

CTableDescriptor *
PtabdescBaseAccess(CExpression *pexpr)
{
	if (nullptr == pexpr)
	{
		return nullptr;
	}

	switch (pexpr->Pop()->Eopid())
	{
		case COperator::EopLogicalGet:
		case COperator::EopLogicalForeignGet:
		case COperator::EopLogicalIndexGet:
		case COperator::EopLogicalIndexOnlyGet:
		case COperator::EopLogicalBitmapTableGet:
		case COperator::EopLogicalDynamicGet:
		case COperator::EopLogicalDynamicForeignGet:
		case COperator::EopLogicalDynamicIndexGet:
		case COperator::EopLogicalDynamicIndexOnlyGet:
		case COperator::EopLogicalDynamicBitmapTableGet:
			return CLogical::PtabdescFromTableGet(pexpr->Pop());
		default:
			return nullptr;
	}
}

CExpression *
PexprSingleBaseGet(CExpression *pexpr, BOOL *pfAmbiguous)
{
	if (nullptr == pexpr || *pfAmbiguous)
	{
		return nullptr;
	}
	if (nullptr != PtabdescBaseAccess(pexpr))
	{
		return pexpr;
	}

	CExpression *pexprFound = nullptr;
	for (ULONG ul = 0; ul < pexpr->Arity() && !*pfAmbiguous; ul++)
	{
		CExpression *pexprChild =
			PexprSingleBaseGet((*pexpr)[ul], pfAmbiguous);
		if (nullptr == pexprChild)
		{
			continue;
		}
		if (nullptr != pexprFound && pexprFound != pexprChild)
		{
			*pfAmbiguous = true;
			return nullptr;
		}
		pexprFound = pexprChild;
	}
	return *pfAmbiguous ? nullptr : pexprFound;
}

CExpression *
PexprOwningGetInSubtree(CExpression *pexpr, const CColRef *pcr)
{
	if (nullptr == pexpr)
	{
		return nullptr;
	}
	if (nullptr != PtabdescBaseAccess(pexpr))
	{
		return pexpr->DeriveOutputColumns()->FMember(pcr) ? pexpr : nullptr;
	}
	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		CExpression *pexprGet = PexprOwningGetInSubtree((*pexpr)[ul], pcr);
		if (nullptr != pexprGet)
		{
			return pexprGet;
		}
	}
	return nullptr;
}

CExpression *
PexprOwningGet(const CDSLRule *prule, const CDSLModel *pmodel,
			   const CColRef *pcr)
{
	CDSLSymbolArray *pdrgpsym = prule->PfragSrc()->Pdrgpsym();
	for (ULONG ul = 0; ul < pdrgpsym->Size(); ul++)
	{
		const CDSLSymbol *psym = (*pdrgpsym)[ul];
		if (EdslsymTable != psym->Esymkind())
		{
			continue;
		}
		CExpression *pexpr = pmodel->PexprTable(psym);
		CExpression *pexprGet = PexprOwningGetInSubtree(pexpr, pcr);
		if (nullptr != pexprGet)
		{
			return pexprGet;
		}
	}
	return nullptr;
}

BOOL
FColRefSemanticEqual(const CDSLRule *prule, const CDSLModel *pmodel,
					 const CColRef *pcrFirst, const CColRef *pcrSecond)
{
	pcrFirst = PcrResolveIdentityLineage(prule, pmodel, pcrFirst);
	pcrSecond = PcrResolveIdentityLineage(prule, pmodel, pcrSecond);
	if (pcrFirst == pcrSecond)
	{
		return true;
	}
	if (CColRef::EcrtTable != pcrFirst->Ecrt() ||
		CColRef::EcrtTable != pcrSecond->Ecrt() ||
		CColRefTable::PcrConvert(const_cast<CColRef *>(pcrFirst))->AttrNum() !=
			CColRefTable::PcrConvert(const_cast<CColRef *>(pcrSecond))->AttrNum())
	{
		return false;
	}

	CExpression *pexprFirst = PexprOwningGet(prule, pmodel, pcrFirst);
	CExpression *pexprSecond = PexprOwningGet(prule, pmodel, pcrSecond);
	CTableDescriptor *ptabdescFirst = PtabdescBaseAccess(pexprFirst);
	CTableDescriptor *ptabdescSecond = PtabdescBaseAccess(pexprSecond);
	return nullptr != ptabdescFirst && nullptr != ptabdescSecond &&
		   ptabdescFirst->MDId()->Equals(ptabdescSecond->MDId());
}

BOOL
FColArraysSemanticEqual(const CDSLRule *prule, const CDSLModel *pmodel,
						const CColRefArray *pdrgpcrFirst,
						const CColRefArray *pdrgpcrSecond)
{
	if (nullptr == pdrgpcrFirst || nullptr == pdrgpcrSecond ||
		pdrgpcrFirst->Size() != pdrgpcrSecond->Size())
	{
		return false;
	}
	for (ULONG ul = 0; ul < pdrgpcrFirst->Size(); ul++)
	{
		if (!FColRefSemanticEqual(prule, pmodel, (*pdrgpcrFirst)[ul],
							  (*pdrgpcrSecond)[ul]))
		{
			return false;
		}
	}
	return true;
}

BOOL
FPredicateFixesColumn(CExpression *pexprPred, const CColRef *pcr)
{
	if (CPredicateUtils::FAnd(pexprPred))
	{
		for (ULONG ul = 0; ul < pexprPred->Arity(); ul++)
		{
			if (FPredicateFixesColumn((*pexprPred)[ul], pcr))
			{
				return true;
			}
		}
		return false;
	}
	if (!CPredicateUtils::IsEqualityOp(pexprPred))
	{
		return false;
	}

	CExpression *pexprLeft = (*pexprPred)[0];
	CExpression *pexprRight = (*pexprPred)[1];
	const BOOL fLeftConst =
		COperator::EopScalarConst == pexprLeft->Pop()->Eopid() ||
		CCastUtils::FBinaryCoercibleCastedConst(pexprLeft);
	const BOOL fRightConst =
		COperator::EopScalarConst == pexprRight->Pop()->Eopid() ||
		CCastUtils::FBinaryCoercibleCastedConst(pexprRight);
	return (fLeftConst &&
			(CCastUtils::FBinaryCoercibleCastedScId(pexprRight,
												 const_cast<CColRef *>(pcr)) ||
			 CUtils::FScalarIdent(pexprRight, const_cast<CColRef *>(pcr)))) ||
		   (fRightConst &&
			(CCastUtils::FBinaryCoercibleCastedScId(pexprLeft,
												 const_cast<CColRef *>(pcr)) ||
			 CUtils::FScalarIdent(pexprLeft, const_cast<CColRef *>(pcr))));
}

BOOL
FFilterFixesColumn(CExpression *pexpr, const CColRef *pcr)
{
	if (COperator::EopLogicalSelect != pexpr->Pop()->Eopid() ||
		2 != pexpr->Arity())
	{
		return false;
	}
	return FPredicateFixesColumn((*pexpr)[1], pcr) ||
		   FFilterFixesColumn((*pexpr)[0], pcr);
}

BOOL
FAttrsAndFixedColumnsCoverKey(CMemoryPool *mp, CExpression *pexpr,
							  const CColRefSet *pcrsAttrs,
							  CKeyCollection *pkc)
{
	if (nullptr == pcrsAttrs || nullptr == pkc)
	{
		return false;
	}

	for (ULONG ulKey = 0; ulKey < pkc->Keys(); ulKey++)
	{
		CColRefArray *pdrgpcrKey = pkc->PdrgpcrKey(mp, ulKey);
		BOOL fCovered = 0 < pdrgpcrKey->Size();
		for (ULONG ulCol = 0; fCovered && ulCol < pdrgpcrKey->Size(); ulCol++)
		{
			CColRef *pcrKey = (*pdrgpcrKey)[ulCol];
			fCovered = pcrsAttrs->FMember(pcrKey) ||
					   FFilterFixesColumn(pexpr, pcrKey);
		}
		pdrgpcrKey->Release();
		if (fCovered)
		{
			return true;
		}
	}
	return false;
}

const CColRef *
PcrEqualityOperand(CExpression *pexpr)
{
	if (COperator::EopScalarIdent == pexpr->Pop()->Eopid())
	{
		return CScalarIdent::PopConvert(pexpr->Pop())->Pcr();
	}
	if (CCastUtils::FBinaryCoercibleCastedScId(pexpr))
	{
		return CScalarIdent::PopConvert((*pexpr)[0]->Pop())->Pcr();
	}
	return nullptr;
}

BOOL
FCollectCrossEqualityColumns(CExpression *pexprPred,
							 const CColRefSet *pcrsLocal,
							 const CColRefSet *pcrsOuter,
							 CColRefSet *pcrsSeenLocal,
							 CColRefSet *pcrsSeenOuter)
{
	if (CPredicateUtils::FAnd(pexprPred))
	{
		if (0 == pexprPred->Arity())
		{
			return false;
		}
		for (ULONG ul = 0; ul < pexprPred->Arity(); ul++)
		{
			if (!FCollectCrossEqualityColumns(
					(*pexprPred)[ul], pcrsLocal, pcrsOuter,
					pcrsSeenLocal, pcrsSeenOuter))
			{
				return false;
			}
		}
		return true;
	}
	if (!CPredicateUtils::IsEqualityOp(pexprPred) || 2 != pexprPred->Arity())
	{
		return false;
	}

	const CColRef *pcrLeft = PcrEqualityOperand((*pexprPred)[0]);
	const CColRef *pcrRight = PcrEqualityOperand((*pexprPred)[1]);
	if (nullptr == pcrLeft || nullptr == pcrRight)
	{
		return false;
	}
	if (pcrsLocal->FMember(pcrLeft) && pcrsOuter->FMember(pcrRight))
	{
		pcrsSeenLocal->Include(const_cast<CColRef *>(pcrLeft));
		pcrsSeenOuter->Include(const_cast<CColRef *>(pcrRight));
		return true;
	}
	if (pcrsLocal->FMember(pcrRight) && pcrsOuter->FMember(pcrLeft))
	{
		pcrsSeenLocal->Include(const_cast<CColRef *>(pcrRight));
		pcrsSeenOuter->Include(const_cast<CColRef *>(pcrLeft));
		return true;
	}
	return false;
}

BOOL
FCorrelationEqualityHolds(CMemoryPool *mp, CExpression *pexprPred,
						  CColRefArray *pdrgpcrLocal,
						  CColRefArray *pdrgpcrOuter)
{
	if (nullptr == pexprPred || nullptr == pdrgpcrLocal ||
		nullptr == pdrgpcrOuter || 0 == pdrgpcrLocal->Size() ||
		0 == pdrgpcrOuter->Size())
	{
		return false;
	}

	CColRefSet *pcrsLocal = GPOS_NEW(mp) CColRefSet(mp);
	CColRefSet *pcrsOuter = GPOS_NEW(mp) CColRefSet(mp);
	CColRefSet *pcrsSeenLocal = GPOS_NEW(mp) CColRefSet(mp);
	CColRefSet *pcrsSeenOuter = GPOS_NEW(mp) CColRefSet(mp);
	pcrsLocal->Include(pdrgpcrLocal);
	pcrsOuter->Include(pdrgpcrOuter);

	const BOOL fDisjoint = !pcrsLocal->FIntersects(pcrsOuter);
	const BOOL fEqualityOnly = fDisjoint && FCollectCrossEqualityColumns(
		pexprPred, pcrsLocal, pcrsOuter, pcrsSeenLocal, pcrsSeenOuter);
	const BOOL fComplete = fEqualityOnly &&
		pcrsSeenLocal->ContainsAll(pcrsLocal) &&
		pcrsSeenOuter->ContainsAll(pcrsOuter);

	pcrsSeenOuter->Release();
	pcrsSeenLocal->Release();
	pcrsOuter->Release();
	pcrsLocal->Release();
	return fComplete;
}

void
CollectConnectedColumns(CExpression *pexprPred,
						const CColRefSet *pcrsCandidate,
						const CColRefSet *pcrsJoined,
						CColRefSet *pcrsConnected)
{
	if (CPredicateUtils::FAnd(pexprPred))
	{
		for (ULONG ul = 0; ul < pexprPred->Arity(); ul++)
		{
			CollectConnectedColumns((*pexprPred)[ul], pcrsCandidate,
								pcrsJoined, pcrsConnected);
		}
		return;
	}
	if (!CPredicateUtils::IsEqualityOp(pexprPred))
	{
		return;
	}

	const CColRef *pcrLeft = PcrEqualityOperand((*pexprPred)[0]);
	const CColRef *pcrRight = PcrEqualityOperand((*pexprPred)[1]);
	if (nullptr == pcrLeft || nullptr == pcrRight)
	{
		return;
	}
	if (pcrsCandidate->FMember(pcrLeft) && pcrsJoined->FMember(pcrRight))
	{
		pcrsConnected->Include(const_cast<CColRef *>(pcrLeft));
	}
	else if (pcrsCandidate->FMember(pcrRight) &&
			 pcrsJoined->FMember(pcrLeft))
	{
		pcrsConnected->Include(const_cast<CColRef *>(pcrRight));
	}
}

BOOL FExpressionUniqueOnAttrs(CMemoryPool *mp, CExpression *pexpr,
							  const CColRefSet *pcrsAttrs);

BOOL
FBinaryJoinPreservesUniqueAttrs(CMemoryPool *mp, CExpression *pexprJoin,
								const CColRefSet *pcrsAttrs,
								BOOL fAllowRightAnchor)
{
	if (3 != pexprJoin->Arity())
	{
		return false;
	}

	for (ULONG ulAnchor = 0; ulAnchor < 2; ulAnchor++)
	{
		if (1 == ulAnchor && !fAllowRightAnchor)
		{
			continue;
		}
		CExpression *pexprAnchor = (*pexprJoin)[ulAnchor];
		CExpression *pexprOther = (*pexprJoin)[1 - ulAnchor];
		CColRefSet *pcrsAnchorAttrs = GPOS_NEW(mp) CColRefSet(mp);
		pcrsAnchorAttrs->Include(pcrsAttrs);
		pcrsAnchorAttrs->Intersection(pexprAnchor->DeriveOutputColumns());
		const BOOL fAnchorUnique = 0 < pcrsAnchorAttrs->Size() &&
			FExpressionUniqueOnAttrs(mp, pexprAnchor, pcrsAnchorAttrs);
		pcrsAnchorAttrs->Release();
		if (!fAnchorUnique)
		{
			continue;
		}

		CColRefSet *pcrsJoinCols = GPOS_NEW(mp) CColRefSet(mp);
		CollectConnectedColumns((*pexprJoin)[2],
							pexprOther->DeriveOutputColumns(),
							pexprAnchor->DeriveOutputColumns(), pcrsJoinCols);
		BOOL fOtherUnique =
			FExpressionUniqueOnAttrs(mp, pexprOther, pcrsJoinCols);
		pcrsJoinCols->Release();
		if (fOtherUnique)
		{
			return true;
		}
	}
	return false;
}

BOOL
FExpressionUniqueOnAttrs(CMemoryPool *mp, CExpression *pexpr,
							 const CColRefSet *pcrsAttrs)
{
	CExpression *pexprKeySource = pexpr;
	while (COperator::EopLogicalSelect == pexprKeySource->Pop()->Eopid() &&
		   2 == pexprKeySource->Arity())
	{
		pexprKeySource = (*pexprKeySource)[0];
	}

	switch (pexprKeySource->Pop()->Eopid())
	{
		case COperator::EopLogicalInnerJoin:
			return FBinaryJoinPreservesUniqueAttrs(
				mp, pexprKeySource, pcrsAttrs, true /*fAllowRightAnchor*/);
		case COperator::EopLogicalLeftOuterJoin:
			return FBinaryJoinPreservesUniqueAttrs(
				mp, pexprKeySource, pcrsAttrs, false /*fAllowRightAnchor*/);
		default:
			break;
	}

	CKeyCollection *pkc = pexprKeySource->DeriveKeyCollection();
	return nullptr != pkc && FAttrsAndFixedColumnsCoverKey(
							  mp, pexpr, pcrsAttrs, pkc);
}

BOOL
FSelectionChainRejectsNull(CMemoryPool *mp, CExpression *pexpr,
						   const CColRef *pcr)
{
	// Only inspect Selects above the bound relation. Descending through an
	// arbitrary relational child is unsafe: a lower Select may reject NULL, but
	// a parent outer join can null-extend that same column again.
	CExpression *pexprCurrent = pexpr;
	while (COperator::EopLogicalSelect == pexprCurrent->Pop()->Eopid() &&
		   2 == pexprCurrent->Arity())
	{
		CExpression *pexprPred = (*pexprCurrent)[1];
		if (pexprPred->DeriveUsedColumns()->FMember(pcr))
		{
			CColRefSet *pcrs = GPOS_NEW(mp) CColRefSet(mp);
			pcrs->Include(const_cast<CColRef *>(pcr));
			BOOL fRejects =
				CPredicateUtils::FNullRejecting(mp, pexprPred, pcrs);
			pcrs->Release();
			if (fRejects)
			{
				return true;
			}
		}
		pexprCurrent = (*pexprCurrent)[0];
	}
	return false;
}

BOOL
FExpressionProvesNotNull(CMemoryPool *mp, CExpression *pexpr,
						 const CColRef *pcr)
{
	if (nullptr == pexpr ||
		!pexpr->DeriveOutputColumns()->FMember(pcr))
	{
		return false;
	}
	if (pexpr->DeriveNotNullColumns()->FMember(pcr) ||
		FSelectionChainRejectsNull(mp, pexpr, pcr))
	{
		return true;
	}

	// ORCA's root not-null property is deliberately conservative for several
	// join shapes. Follow only children whose rows cannot be null-extended by
	// the current operator; the opposite side of an outer join remains rejected.
	switch (pexpr->Pop()->Eopid())
	{
		case COperator::EopLogicalSelect:
		case COperator::EopLogicalProject:
		case COperator::EopLogicalSequenceProject:
		case COperator::EopLogicalLimit:
			return 0 < pexpr->Arity() &&
				   FExpressionProvesNotNull(mp, (*pexpr)[0], pcr);

		case COperator::EopLogicalInnerJoin:
		case COperator::EopLogicalInnerApply:
			for (ULONG ul = 0; ul < 2 && ul < pexpr->Arity(); ul++)
			{
				if (FExpressionProvesNotNull(mp, (*pexpr)[ul], pcr))
				{
					return true;
				}
			}
			return false;

		case COperator::EopLogicalLeftOuterJoin:
		case COperator::EopLogicalLeftOuterApply:
		case COperator::EopLogicalLeftSemiJoin:
		case COperator::EopLogicalLeftAntiSemiJoin:
		case COperator::EopLogicalLeftAntiSemiJoinNotIn:
			return 0 < pexpr->Arity() &&
				   FExpressionProvesNotNull(mp, (*pexpr)[0], pcr);

		case COperator::EopLogicalRightOuterJoin:
			return 1 < pexpr->Arity() &&
				   FExpressionProvesNotNull(mp, (*pexpr)[1], pcr);

		default:
			return false;
	}
}
}  // namespace

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintChecker::PcrsFromAttrsSym
//
//	@doc:
//		Materialize the columns bound to an attrs symbol as a CColRefSet. NULL if
//		the symbol is unbound (a constraint over an unbound symbol cannot hold).
//---------------------------------------------------------------------------
CColRefSet *
CDSLConstraintChecker::PcrsFromAttrsSym(const CDSLSymbol *psymAttrs,
										const CDSLModel *pmodel) const
{
	CColRefArray *pdrgpcr = pmodel->PdrgpcrAttrs(psymAttrs);
	if (nullptr == pdrgpcr)
	{
		return nullptr;
	}
	CColRefSet *pcrs = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrs->Include(pdrgpcr);
	return pcrs;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintChecker::FCheckAttrsSub
//
//	@doc:
//		AttrsSub(a,x): x is an attrs, table/subtree, or schema symbol.
//---------------------------------------------------------------------------
BOOL
CDSLConstraintChecker::FCheckAttrsSub(const CDSLConstraint *pcon,
									  const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (2 != pdrgpsym->Size())
	{
		return false;
	}

	const CDSLSymbol *psymAttrs = (*pdrgpsym)[0];
	const CDSLSymbol *psymSource = (*pdrgpsym)[1];
	if (EdslsymAttrs != psymAttrs->Esymkind() ||
		(EdslsymAttrs != psymSource->Esymkind() &&
		 EdslsymTable != psymSource->Esymkind() &&
		 EdslsymSchema != psymSource->Esymkind()))
	{
		return false;
	}

	// Matching binds only source-fragment symbols. An AttrsSub that describes a
	// target operator's attrs is a construction-time well-formedness condition,
	// not a source applicability premise. Defer it just as WeTune's Model does;
	// operator builders subsequently require the instantiated predicate/attrs to
	// be produced by the rebuilt child. Missing source symbols remain a hard
	// matcher error.
	if (nullptr == pmodel->PvalLookup(psymAttrs))
	{
		return EdslsideTarget == psymAttrs->Eside();
	}
	if (nullptr == pmodel->PvalLookup(psymSource))
	{
		return EdslsideTarget == psymSource->Eside();
	}

	CColRefSet *pcrsAttrs = PcrsFromAttrsSym(psymAttrs, pmodel);
	if (nullptr == pcrsAttrs)
	{
		return false;
	}

	BOOL fHolds = false;
	if (EdslsymAttrs == psymSource->Esymkind())
	{
		CColRefSet *pcrsSource = PcrsFromAttrsSym(psymSource, pmodel);
		if (nullptr != pcrsSource)
		{
			fHolds = pcrsSource->ContainsAll(pcrsAttrs);
			pcrsSource->Release();
		}
	}
	else if (EdslsymTable == psymSource->Esymkind())
	{
		CExpression *pexprTable = pmodel->PexprTable(psymSource);
		fHolds = nullptr != pexprTable &&
				 pexprTable->DeriveOutputColumns()->ContainsAll(pcrsAttrs);
	}
	else
	{
		CColRefArray *pdrgpcrSchema = pmodel->PdrgpcrSchema(psymSource);
		if (nullptr != pdrgpcrSchema)
		{
			CColRefSet *pcrsSchema = GPOS_NEW(m_mp) CColRefSet(m_mp);
			pcrsSchema->Include(pdrgpcrSchema);
			fHolds = pcrsSchema->ContainsAll(pcrsAttrs);
			pcrsSchema->Release();
		}
	}
	pcrsAttrs->Release();
	return fHolds;
}

BOOL
CDSLConstraintChecker::FCheckAttrsEmpty(const CDSLConstraint *pcon,
										 const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (1 != pdrgpsym->Size() ||
		EdslsymAttrs != (*pdrgpsym)[0]->Esymkind())
	{
		return false;
	}
	const CDSLSymbol *psymAttrs = (*pdrgpsym)[0];
	CColRefArray *pdrgpcr = pmodel->PdrgpcrAttrs(psymAttrs);
	if (nullptr == pdrgpcr)
	{
		return EdslsideTarget == psymAttrs->Eside();
	}
	return 0 == pdrgpcr->Size();
}

BOOL
CDSLConstraintChecker::FCheckAttrsNonEmpty(const CDSLConstraint *pcon,
											const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (1 != pdrgpsym->Size() ||
		EdslsymAttrs != (*pdrgpsym)[0]->Esymkind())
	{
		return false;
	}
	CColRefArray *pdrgpcr = pmodel->PdrgpcrAttrs((*pdrgpsym)[0]);
	return nullptr != pdrgpcr && 0 < pdrgpcr->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintChecker::FCheckOutputAttrs
//
//	@doc:
//		OutputAttrs(a,t): a is exactly t's logical output set. ORCA Get also
//		derives implicit system columns (ctid/xmin/etc.); those
//		are storage artifacts rather than DSL table attributes and cannot safely
//		become grouping columns.
//		The attrs symbol is normally target-only, so matching leaves materialization
//		to CDSLInstantiator. Key requirements use the independent Unique constraint.
//---------------------------------------------------------------------------
BOOL
CDSLConstraintChecker::FCheckOutputAttrs(const CDSLConstraint *pcon,
										 const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (2 != pdrgpsym->Size() ||
		EdslsymAttrs != (*pdrgpsym)[0]->Esymkind() ||
		EdslsymTable != (*pdrgpsym)[1]->Esymkind())
	{
		return false;
	}

	const CDSLSymbol *psymAttrs = (*pdrgpsym)[0];
	CExpression *pexprTable = pmodel->PexprTable((*pdrgpsym)[1]);
	if (nullptr == pexprTable)
	{
		return false;
	}

	CColRefArray *pdrgpcrAttrs = pmodel->PdrgpcrAttrs(psymAttrs);
	if (nullptr == pdrgpcrAttrs)
	{
		return EdslsideTarget == psymAttrs->Eside();
	}

	CColRefSet *pcrsLogicalOutput = GPOS_NEW(m_mp) CColRefSet(m_mp);
	CColRefSetIter iter(*pexprTable->DeriveOutputColumns());
	while (iter.Advance())
	{
		if (!iter.Pcr()->IsSystemCol())
		{
			pcrsLogicalOutput->Include(iter.Pcr());
		}
	}
	CColRefSet *pcrsAttrs = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsAttrs->Include(pdrgpcrAttrs);
	const BOOL fEqual = pcrsLogicalOutput->Equals(pcrsAttrs);
	pcrsLogicalOutput->Release();
	pcrsAttrs->Release();
	return fEqual;
}

BOOL
CDSLConstraintChecker::FCheckSchemaFromAttrs(
	const CDSLConstraint *pcon, const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (2 != pdrgpsym->Size() ||
		EdslsymSchema != (*pdrgpsym)[0]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[1]->Esymkind())
	{
		return false;
	}
	CColRefArray *pdrgpcrAttrs =
		pmodel->PdrgpcrAttrs((*pdrgpsym)[1]);
	if (nullptr == pdrgpcrAttrs)
	{
		return EdslsideTarget == (*pdrgpsym)[1]->Eside();
	}
	CColRefArray *pdrgpcrSchema =
		pmodel->PdrgpcrSchema((*pdrgpsym)[0]);
	if (nullptr == pdrgpcrSchema)
	{
		return EdslsideTarget == (*pdrgpsym)[0]->Eside();
	}
	return CColRef::Equals(pdrgpcrSchema, pdrgpcrAttrs);
}

BOOL
CDSLConstraintChecker::FCheckFuncAttrs(
	const CDSLConstraint *pcon, const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (2 != pdrgpsym->Size() ||
		EdslsymAttrs != (*pdrgpsym)[0]->Esymkind() ||
		EdslsymFunc != (*pdrgpsym)[1]->Esymkind())
	{
		return false;
	}
	CExpressionArray *pdrgpexprFuncs =
		pmodel->PdrgpexprFunc((*pdrgpsym)[1]);
	if (nullptr == pdrgpexprFuncs)
	{
		return false;
	}
	CColRefArray *pdrgpcrAttrs =
		pmodel->PdrgpcrAttrs((*pdrgpsym)[0]);
	if (nullptr == pdrgpcrAttrs)
	{
		return EdslsideTarget == (*pdrgpsym)[0]->Eside();
	}
	CColRefSet *pcrsExpected = GPOS_NEW(m_mp) CColRefSet(m_mp);
	for (ULONG ul = 0; ul < pdrgpexprFuncs->Size(); ul++)
	{
		pcrsExpected->Include((*pdrgpexprFuncs)[ul]->DeriveUsedColumns());
	}
	CColRefSet *pcrsActual = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsActual->Include(pdrgpcrAttrs);
	const BOOL fMatches = pcrsExpected->Equals(pcrsActual);
	pcrsExpected->Release();
	pcrsActual->Release();
	return fMatches;
}

BOOL
CDSLConstraintChecker::FCheckPredicateDomainSplit(
	const CDSLRule *prule, const CDSLConstraint *pcon,
	const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (9 != pdrgpsym->Size())
	{
		return false;
	}
	const EDslSymbolKind rgExpected[] = {
		EdslsymPred, EdslsymPred, EdslsymPred,
		EdslsymAttrs, EdslsymAttrs, EdslsymAttrs, EdslsymAttrs,
		EdslsymTable, EdslsymTable};
	for (ULONG ul = 0; ul < GPOS_ARRAY_SIZE(rgExpected); ul++)
	{
		if (rgExpected[ul] != (*pdrgpsym)[ul]->Esymkind())
		{
			return false;
		}
	}

	// Positions 1..6 are an atomic target-side decomposition. Accepting a
	// partially bound mixture would let independently matched artifacts disagree
	// with the partition recomputed by the instantiator.
	for (ULONG ul = 1; ul <= 6; ul++)
	{
		if (EdslsideTarget != (*pdrgpsym)[ul]->Eside() ||
			(nullptr != pmodel->PvalLookup((*pdrgpsym)[ul]) &&
			 !pmodel->FDerivedBinding((*pdrgpsym)[ul])))
		{
			return false;
		}
		for (ULONG ulOther = ul + 1; ulOther <= 6; ulOther++)
		{
			if ((*pdrgpsym)[ul] == (*pdrgpsym)[ulOther])
			{
				return false;
			}
		}
	}
	if (nullptr == pmodel->PexprPred((*pdrgpsym)[0]) &&
		EdslsideTarget != (*pdrgpsym)[0]->Eside())
	{
		return false;
	}

	// Partitioning changes conjunction evaluation order. Admit it only when the
	// rule explicitly asks the ordinary scalar checker to establish both safety
	// properties on the complete source predicate. This keeps the split
	// constraint structural and reuses the same proof guards as every other DSL
	// rewrite that moves scalar evaluation.
	BOOL fErrorFree = false;
	BOOL fDeterministic = false;
	CDSLConstraintArray *pdrgpcon = prule->Pdrgpcon();
	for (ULONG ul = 0; ul < pdrgpcon->Size(); ul++)
	{
		const CDSLConstraint *pconProperty = (*pdrgpcon)[ul];
		if (EdslconPredicateEq == pconProperty->Edslcon() ||
			EdslconAttrsEq == pconProperty->Edslcon())
		{
			for (ULONG ulArg = 0; ulArg < pconProperty->Pdrgpsym()->Size();
				 ulArg++)
			{
				for (ULONG ulOutput = 1; ulOutput <= 6; ulOutput++)
				{
					if ((*pconProperty->Pdrgpsym())[ulArg] ==
						(*pdrgpsym)[ulOutput])
					{
						return false;
					}
				}
			}
		}
		if (1 != pconProperty->Pdrgpsym()->Size())
		{
			continue;
		}
		const CDSLSymbol *psymProperty = (*pconProperty->Pdrgpsym())[0];
		if (psymProperty == (*pdrgpsym)[0])
		{
			fErrorFree = fErrorFree ||
				EdslconErrorFree == pconProperty->Edslcon();
			fDeterministic = fDeterministic ||
				EdslconDeterministic == pconProperty->Edslcon();
		}
	}
	if (!fErrorFree || !fDeterministic)
	{
		return false;
	}

	CExpression *pexprOuter = pmodel->PexprTable((*pdrgpsym)[7]);
	CExpression *pexprInner = pmodel->PexprTable((*pdrgpsym)[8]);
	return nullptr != pexprOuter && nullptr != pexprInner &&
		pexprOuter->DeriveOutputColumns()->IsDisjoint(
			pexprInner->DeriveOutputColumns());
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintChecker::FCheckAttrsIntersect
//
//	@doc:
//		Validate the generic ordered column-vector intersection relation. The
//		output is commonly target-side and therefore unbound while matching; in
//		that case the instantiator materializes the same relation lazily.
//---------------------------------------------------------------------------
BOOL
CDSLConstraintChecker::FCheckAttrsIntersect(const CDSLConstraint *pcon,
										 const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (3 != pdrgpsym->Size())
	{
		return false;
	}
	const CDSLSymbol *psymOut = (*pdrgpsym)[0];
	const CDSLSymbol *psymInput = (*pdrgpsym)[1];
	const CDSLSymbol *psymDomain = (*pdrgpsym)[2];
	if ((EdslsymAttrs != psymOut->Esymkind() &&
		 EdslsymSchema != psymOut->Esymkind()) ||
		psymOut->Esymkind() != psymInput->Esymkind())
	{
		return false;
	}

	CRefCount *pvalInput = pmodel->PvalLookup(psymInput);
	CColRefArray *pdrgpcrInput =
		dynamic_cast<CColRefArray *>(pvalInput);
	if (nullptr == pdrgpcrInput)
	{
		return false;
	}

	CColRefSet *pcrsDomain = GPOS_NEW(m_mp) CColRefSet(m_mp);
	if (EdslsymTable == psymDomain->Esymkind())
	{
		CExpression *pexprDomain = pmodel->PexprTable(psymDomain);
		if (nullptr == pexprDomain)
		{
			pcrsDomain->Release();
			return false;
		}
		pcrsDomain->Include(pexprDomain->DeriveOutputColumns());
	}
	else if (EdslsymAttrs == psymDomain->Esymkind() ||
			 EdslsymSchema == psymDomain->Esymkind())
	{
		CColRefArray *pdrgpcrDomain =
			dynamic_cast<CColRefArray *>(pmodel->PvalLookup(psymDomain));
		if (nullptr == pdrgpcrDomain)
		{
			pcrsDomain->Release();
			return false;
		}
		pcrsDomain->Include(pdrgpcrDomain);
	}
	else
	{
		pcrsDomain->Release();
		return false;
	}

	CRefCount *pvalOut = pmodel->PvalLookup(psymOut);
	if (nullptr == pvalOut)
	{
		pcrsDomain->Release();
		return EdslsideTarget == psymOut->Eside();
	}
	CColRefArray *pdrgpcrOut = dynamic_cast<CColRefArray *>(pvalOut);
	ULONG ulExpected = 0;
	BOOL fEqual = nullptr != pdrgpcrOut;
	for (ULONG ul = 0; fEqual && ul < pdrgpcrInput->Size(); ul++)
	{
		CColRef *pcr = (*pdrgpcrInput)[ul];
		if (!pcrsDomain->FMember(pcr))
		{
			continue;
		}
		fEqual = ulExpected < pdrgpcrOut->Size() &&
			(*pdrgpcrOut)[ulExpected] == pcr;
		++ulExpected;
	}
	fEqual = fEqual && ulExpected == pdrgpcrOut->Size();
	pcrsDomain->Release();
	return fEqual;
}

BOOL
CDSLConstraintChecker::FCheckAttrsUnion(const CDSLConstraint *pcon,
									 const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (3 != pdrgpsym->Size())
	{
		return false;
	}
	const BOOL fAttrsUnion = EdslconAttrsUnion == pcon->Edslcon();
	if (!fAttrsUnion && EdslconSchemaUnion != pcon->Edslcon())
	{
		return false;
	}
	if ((fAttrsUnion &&
		 (EdslsymAttrs != (*pdrgpsym)[0]->Esymkind() ||
		  EdslsymAttrs != (*pdrgpsym)[1]->Esymkind() ||
		  EdslsymAttrs != (*pdrgpsym)[2]->Esymkind())) ||
		(!fAttrsUnion &&
		 (EdslsymSchema != (*pdrgpsym)[0]->Esymkind() ||
		  EdslsymSchema != (*pdrgpsym)[1]->Esymkind() ||
		  EdslsymAttrs != (*pdrgpsym)[2]->Esymkind())))
	{
		return false;
	}

	CColRefArray *pdrgpcrLeft =
		dynamic_cast<CColRefArray *>(pmodel->PvalLookup((*pdrgpsym)[1]));
	CColRefArray *pdrgpcrRight =
		dynamic_cast<CColRefArray *>(pmodel->PvalLookup((*pdrgpsym)[2]));
	if (nullptr == pdrgpcrLeft || nullptr == pdrgpcrRight)
	{
		return false;
	}
	CRefCount *pvalOut = pmodel->PvalLookup((*pdrgpsym)[0]);
	if (nullptr == pvalOut)
	{
		return EdslsideTarget == (*pdrgpsym)[0]->Eside();
	}
	CColRefArray *pdrgpcrOut = dynamic_cast<CColRefArray *>(pvalOut);
	if (nullptr == pdrgpcrOut)
	{
		return false;
	}

	CColRefSet *pcrsSeen = GPOS_NEW(m_mp) CColRefSet(m_mp);
	ULONG ulExpected = 0;
	BOOL fEqual = true;
	CColRefArray *rgpdrgpcr[] = {pdrgpcrLeft, pdrgpcrRight};
	for (ULONG ulInput = 0; fEqual && ulInput < 2; ulInput++)
	{
		for (ULONG ul = 0; fEqual && ul < rgpdrgpcr[ulInput]->Size(); ul++)
		{
			CColRef *pcr = (*rgpdrgpcr[ulInput])[ul];
			if (pcrsSeen->FMember(pcr))
			{
				continue;
			}
			pcrsSeen->Include(pcr);
			fEqual = ulExpected < pdrgpcrOut->Size() &&
				(*pdrgpcrOut)[ulExpected] == pcr;
			++ulExpected;
		}
	}
	fEqual = fEqual && ulExpected == pdrgpcrOut->Size();
	pcrsSeen->Release();
	return fEqual;
}

BOOL
CDSLConstraintChecker::FCheckCorrelationEquality(
	const CDSLConstraint *pcon, const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (3 != pdrgpsym->Size() ||
		EdslsymPred != (*pdrgpsym)[0]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[1]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[2]->Esymkind())
	{
		return false;
	}

	return FCorrelationEqualityHolds(
		m_mp, pmodel->PexprPred((*pdrgpsym)[0]),
		pmodel->PdrgpcrAttrs((*pdrgpsym)[1]),
		pmodel->PdrgpcrAttrs((*pdrgpsym)[2]));
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintChecker::FCheckUnique
//
//	@doc:
//		Unique(t,a): the columns bound to <a> form a key of the subtree bound to
//		<t>. Uses the derived key collection (a superset key is acceptable — a
//		non-exact match, since the DSL asserts "these columns are unique", not
//		"these are exactly the key").
//---------------------------------------------------------------------------
BOOL
CDSLConstraintChecker::FCheckUnique(const CDSLConstraint *pcon,
									const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (2 != pdrgpsym->Size())
	{
		return false;
	}

	const CDSLSymbol *psymAttrs = nullptr;
	const CDSLSymbol *psymTable = nullptr;
	for (ULONG ul = 0; ul < 2; ul++)
	{
		const CDSLSymbol *psym = (*pdrgpsym)[ul];
		if (EdslsymAttrs == psym->Esymkind())
		{
			psymAttrs = psym;
		}
		else if (EdslsymTable == psym->Esymkind())
		{
			psymTable = psym;
		}
	}
	if (nullptr == psymAttrs || nullptr == psymTable)
	{
		return false;
	}

	CExpression *pexprTable = pmodel->PexprTable(psymTable);
	CColRefSet *pcrsAttrs = PcrsFromAttrsSym(psymAttrs, pmodel);
	if (nullptr == pexprTable || nullptr == pcrsAttrs)
	{
		CRefCount::SafeRelease(pcrsAttrs);
		return false;
	}

	// In addition to directly derived keys, propagate uniqueness conservatively
	// through joins whose other inputs are unique on their connecting columns.
	// This covers normalized N-ary joins, for which ORCA does not derive a key
	// collection, without assuming arbitrary join outputs are unique.
	BOOL fHolds =
		FExpressionUniqueOnAttrs(m_mp, pexprTable, pcrsAttrs);
	pcrsAttrs->Release();
	return fHolds;
}

BOOL
CDSLConstraintChecker::FCheckMinimalGrouping(
	const CDSLConstraint *pcon, const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (2 != pdrgpsym->Size() ||
		EdslsymAttrs != (*pdrgpsym)[0]->Esymkind() ||
		EdslsymSchema != (*pdrgpsym)[1]->Esymkind())
	{
		return false;
	}

	CRefCount *pvalGroup = pmodel->PvalLookup((*pdrgpsym)[0]);
	CColRefArray *pdrgpcrGroup =
		dynamic_cast<CColRefArray *>(pvalGroup);
	CExpression *pexprAgg =
		pmodel->PexprAggBinding((*pdrgpsym)[1]);
	if (nullptr == pdrgpcrGroup || nullptr == pexprAgg ||
		COperator::EopLogicalGbAgg != pexprAgg->Pop()->Eopid())
	{
		return false;
	}

	CLogicalGbAgg *popAgg = CLogicalGbAgg::PopConvert(pexprAgg->Pop());
	if (!popAgg->FGlobal() || nullptr != popAgg->PdrgpcrMinimal() ||
		0 == pdrgpcrGroup->Size() ||
		!CColRef::Equals(popAgg->Pdrgpcr(), pdrgpcrGroup))
	{
		return false;
	}

	CColRefSet *pcrsGroup = GPOS_NEW(m_mp) CColRefSet(m_mp);
	pcrsGroup->Include(pdrgpcrGroup);
	CColRefSet *pcrsCovered = GPOS_NEW(m_mp) CColRefSet(m_mp);
	CColRefSet *pcrsMinimal = GPOS_NEW(m_mp) CColRefSet(m_mp);
	CFunctionalDependencyArray *pdrgpfd =
		pexprAgg->DeriveFunctionalDependencies();
	for (ULONG ul = 0; nullptr != pdrgpfd && ul < pdrgpfd->Size(); ul++)
	{
		CFunctionalDependency *pfd = (*pdrgpfd)[ul];
		if (pfd->FIncluded(pcrsGroup))
		{
			pcrsCovered->Include(pfd->PcrsDetermined());
			pcrsCovered->Include(pfd->PcrsKey());
			pcrsMinimal->Include(pfd->PcrsKey());
		}
	}
	const BOOL fHolds = pcrsCovered->Equals(pcrsGroup) &&
		0 < pcrsMinimal->Size();
	pcrsMinimal->Release();
	pcrsCovered->Release();
	pcrsGroup->Release();
	return fHolds;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintChecker::FCheckNotNull
//
//	@doc:
//		NotNull(t,a): every column bound to <a> is non-nullable in the subtree
//		bound to <t> (derived not-null columns contain all of <a>).
//---------------------------------------------------------------------------
BOOL
CDSLConstraintChecker::FCheckNotNull(const CDSLConstraint *pcon,
									 const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (2 != pdrgpsym->Size())
	{
		return false;
	}

	const CDSLSymbol *psymAttrs = nullptr;
	const CDSLSymbol *psymTable = nullptr;
	for (ULONG ul = 0; ul < 2; ul++)
	{
		const CDSLSymbol *psym = (*pdrgpsym)[ul];
		if (EdslsymAttrs == psym->Esymkind())
		{
			psymAttrs = psym;
		}
		else if (EdslsymTable == psym->Esymkind())
		{
			psymTable = psym;
		}
	}
	if (nullptr == psymAttrs || nullptr == psymTable)
	{
		return false;
	}

	CExpression *pexprTable = pmodel->PexprTable(psymTable);
	CColRefSet *pcrsAttrs = PcrsFromAttrsSym(psymAttrs, pmodel);
	if (nullptr == pexprTable || nullptr == pcrsAttrs)
	{
		CRefCount::SafeRelease(pcrsAttrs);
		return false;
	}

	BOOL fHolds = true;
	CColRefArray *pdrgpcrAttrs = pcrsAttrs->Pdrgpcr(m_mp);
	for (ULONG ul = 0; fHolds && ul < pdrgpcrAttrs->Size(); ul++)
	{
		CColRef *pcr = (*pdrgpcrAttrs)[ul];
		fHolds = FExpressionProvesNotNull(m_mp, pexprTable, pcr);
	}
	pdrgpcrAttrs->Release();
	pcrsAttrs->Release();
	return fHolds;
}

BOOL
CDSLConstraintChecker::FCheckPredicateFalse(
	const CDSLConstraint *pcon, const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (nullptr == pdrgpsym || 1 != pdrgpsym->Size() ||
		EdslsymPred != (*pdrgpsym)[0]->Esymkind())
	{
		return false;
	}
	CExpression *pexprPred = pmodel->PexprPred((*pdrgpsym)[0]);
	return nullptr != pexprPred && CUtils::FScalarConstFalse(pexprPred);
}

BOOL
CDSLConstraintChecker::FCheckPredicateAnd(
	const CDSLConstraint *pcon, const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (nullptr == pdrgpsym || 3 != pdrgpsym->Size())
	{
		return false;
	}
	for (ULONG ul = 0; ul < 3; ul++)
	{
		if (EdslsymPred != (*pdrgpsym)[ul]->Esymkind())
		{
			return false;
		}
	}

	CExpression *pexprLeft = pmodel->PexprPred((*pdrgpsym)[1]);
	CExpression *pexprRight = pmodel->PexprPred((*pdrgpsym)[2]);
	if (nullptr == pexprLeft || nullptr == pexprRight)
	{
		return false;
	}

	CExpression *pexprResult = pmodel->PexprPred((*pdrgpsym)[0]);
	if (nullptr == pexprResult)
	{
		return EdslsideTarget == (*pdrgpsym)[0]->Eside();
	}
	CExpression *pexprExpected =
		CPredicateUtils::PexprConjunction(m_mp, pexprLeft, pexprRight);
	const BOOL fMatches = pexprResult->Matches(pexprExpected);
	pexprExpected->Release();
	return fMatches;
}

BOOL
CDSLConstraintChecker::FCheckPredicateExists(
	const CDSLConstraint *pcon, CDSLModel *pmodel, BOOL fNegated) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (nullptr == pdrgpsym || 2 != pdrgpsym->Size() ||
		EdslsymPred != (*pdrgpsym)[0]->Esymkind() ||
		EdslsymTable != (*pdrgpsym)[1]->Esymkind())
	{
		return false;
	}

	CExpression *pexprPredicate = pmodel->PexprPred((*pdrgpsym)[0]);
	if (nullptr == pexprPredicate)
	{
		return false;
	}
	const COperator::EOperatorId eopid =
		fNegated ? COperator::EopScalarSubqueryNotExists
				 : COperator::EopScalarSubqueryExists;
	if (1 != pexprPredicate->Arity() ||
		eopid != pexprPredicate->Pop()->Eopid())
	{
		return false;
	}
	CExpression *pexprInput = (*pexprPredicate)[0];
	CExpression *pexprBound = pmodel->PexprTable((*pdrgpsym)[1]);
	return nullptr == pexprBound ? pmodel->FBind((*pdrgpsym)[1], pexprInput)
							 : pexprBound->Matches(pexprInput);
}

BOOL
CDSLConstraintChecker::FCheckPredicateQuantified(const CDSLConstraint *pcon,
											 CDSLModel *pmodel,
											 BOOL fAll) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (nullptr == pdrgpsym || 4 != pdrgpsym->Size() ||
		EdslsymPred != (*pdrgpsym)[0]->Esymkind() ||
		EdslsymPred != (*pdrgpsym)[1]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[2]->Esymkind() ||
		EdslsymTable != (*pdrgpsym)[3]->Esymkind())
	{
		return false;
	}

	CExpression *pexprQuantified = pmodel->PexprPred((*pdrgpsym)[0]);
	const COperator::EOperatorId eopid =
		fAll ? COperator::EopScalarSubqueryAll
			 : COperator::EopScalarSubqueryAny;
	if (nullptr == pexprQuantified || 2 != pexprQuantified->Arity() ||
		eopid != pexprQuantified->Pop()->Eopid() ||
		(*pexprQuantified)[1]->DeriveHasSubquery())
	{
		return false;
	}

	CExpression *pexprComparison =
		CDSLQuantifiedMatcher::PexprComparison(m_mp, pexprQuantified);
	CColRefArray *pdrgpcrOuter =
		(*pexprQuantified)[1]->DeriveUsedColumns()->Pdrgpcr(m_mp);
	const BOOL fMatches = pmodel->FBind((*pdrgpsym)[1], pexprComparison) &&
		pmodel->FBind((*pdrgpsym)[2], pdrgpcrOuter) &&
		pmodel->FBind((*pdrgpsym)[3], (*pexprQuantified)[0]);
	pexprComparison->Release();
	pdrgpcrOuter->Release();
	return fMatches;
}

namespace
{
CExpression *
PexprOnlySubquery(CExpression *pexpr, COperator::EOperatorId eopid,
				  ULONG *pulCount)
{
	if (eopid == pexpr->Pop()->Eopid())
	{
		(*pulCount)++;
		return pexpr;
	}
	if (!pexpr->Pop()->FScalar())
	{
		return nullptr;
	}
	CExpression *pexprFound = nullptr;
	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		CExpression *pexprChild =
			PexprOnlySubquery((*pexpr)[ul], eopid, pulCount);
		if (nullptr != pexprChild && nullptr == pexprFound)
		{
			pexprFound = pexprChild;
		}
	}
	return pexprFound;
}

CExpression *
PexprReplaceNode(CMemoryPool *mp, CExpression *pexpr,
				 CExpression *pexprNeedle, CExpression *pexprReplacement)
{
	if (pexpr == pexprNeedle)
	{
		pexprReplacement->AddRef();
		return pexprReplacement;
	}
	CExpressionArray *pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
	{
		pdrgpexpr->Append(PexprReplaceNode(
			mp, (*pexpr)[ul], pexprNeedle, pexprReplacement));
	}
	pexpr->Pop()->AddRef();
	return GPOS_NEW(mp) CExpression(mp, pexpr->Pop(), pdrgpexpr);
}

CExpression *
PexprSubqueryInSequence(CDSLModel *pmodel, const CDSLSymbol *psym,
						COperator::EOperatorId eopid)
{
	if (EdslsymExpr == psym->Esymkind() ||
		EdslsymPred == psym->Esymkind() ||
		EdslsymWindow == psym->Esymkind())
	{
		CExpression *pexpr = EdslsymExpr == psym->Esymkind()
			? pmodel->PexprExpr(psym)
			: (EdslsymPred == psym->Esymkind()
				   ? pmodel->PexprPred(psym)
				   : pmodel->PexprWindow(psym));
		ULONG ulCount = 0;
		return nullptr == pexpr
			? nullptr
			: PexprOnlySubquery(pexpr, eopid, &ulCount);
	}
	CExpressionArray *pdrgpexpr = pmodel->PdrgpexprFunc(psym);
	for (ULONG ul = 0; nullptr != pdrgpexpr && ul < pdrgpexpr->Size(); ul++)
	{
		ULONG ulCount = 0;
		CExpression *pexprCandidate =
			PexprOnlySubquery((*pdrgpexpr)[ul], eopid, &ulCount);
		if (nullptr != pexprCandidate)
		{
			return pexprCandidate;
		}
	}
	return nullptr;
}

CExpression *
PexprNextSubqueryInSequence(CDSLModel *pmodel, const CDSLSymbol *psym)
{
	// Fixed semantic priority makes a mixed list a linear rewrite chain rather
	// than one alternative for every possible lowering order.
	const COperator::EOperatorId rgeopid[] = {
		COperator::EopScalarSubquery,
		COperator::EopScalarSubqueryExists,
		COperator::EopScalarSubqueryNotExists,
		COperator::EopScalarSubqueryAny,
		COperator::EopScalarSubqueryAll,
	};
	for (ULONG ul = 0; ul < GPOS_ARRAY_SIZE(rgeopid); ul++)
	{
		CExpression *pexpr =
			PexprSubqueryInSequence(pmodel, psym, rgeopid[ul]);
		if (nullptr != pexpr)
		{
			return pexpr;
		}
	}
	return nullptr;
}

CRefCount *
PvalReplaceNodeInSequence(CMemoryPool *mp, CDSLModel *pmodel,
						  const CDSLSymbol *psym, CExpression *pexprNeedle,
						  CExpression *pexprReplacement)
{
	if (EdslsymExpr == psym->Esymkind() ||
		EdslsymPred == psym->Esymkind() ||
		EdslsymWindow == psym->Esymkind())
	{
		CExpression *pexprSource = EdslsymExpr == psym->Esymkind()
			? pmodel->PexprExpr(psym)
			: (EdslsymPred == psym->Esymkind()
				   ? pmodel->PexprPred(psym)
				   : pmodel->PexprWindow(psym));
		CExpression *pexprLowered = PexprReplaceNode(
			mp, pexprSource, pexprNeedle, pexprReplacement);
		return pexprLowered;
	}
	CExpressionArray *pdrgpexpr = pmodel->PdrgpexprFunc(psym);
	CExpressionArray *pdrgpexprLowered =
		GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < pdrgpexpr->Size(); ul++)
	{
		CExpression *pexprLowered = PexprReplaceNode(
			mp, (*pdrgpexpr)[ul], pexprNeedle, pexprReplacement);
		pdrgpexprLowered->Append(pexprLowered);
	}
	return pdrgpexprLowered;
}
}  // namespace

BOOL
CDSLConstraintChecker::FCheckPredicateScalarSubquery(
	const CDSLConstraint *pcon, CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (nullptr == pdrgpsym || 6 != pdrgpsym->Size() ||
		EdslsymPred != (*pdrgpsym)[0]->Esymkind() ||
		EdslsymPred != (*pdrgpsym)[1]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[2]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[3]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[4]->Esymkind() ||
		EdslsymTable != (*pdrgpsym)[5]->Esymkind())
	{
		return false;
	}
	CExpressionArray *pdrgpexprResidual = pmodel->PdrgpexprResidual();
	for (ULONG ul = 0;
		 nullptr != pdrgpexprResidual && ul < pdrgpexprResidual->Size(); ul++)
	{
		if ((*pdrgpexprResidual)[ul]->DeriveHasSubquery())
		{
			return false;
		}
	}

	CExpression *pexprPredicate = pmodel->PexprPred((*pdrgpsym)[0]);
	ULONG ulSubqueries = 0;
	CExpression *pexprSubquery = nullptr == pexprPredicate
		? nullptr
		: PexprOnlySubquery(pexprPredicate,
						COperator::EopScalarSubquery, &ulSubqueries);
	if (1 != ulSubqueries || nullptr == pexprSubquery ||
		1 != pexprSubquery->Arity())
	{
		return false;
	}

	CScalarSubquery *popSubquery =
		CScalarSubquery::PopConvert(pexprSubquery->Pop());
	CColRef *pcrInner = const_cast<CColRef *>(popSubquery->Pcr());
	CExpression *pexprIdent = CUtils::PexprScalarIdent(m_mp, pcrInner);
	CExpression *pexprLowered = PexprReplaceNode(
		m_mp, pexprPredicate, pexprSubquery, pexprIdent);
	pexprIdent->Release();

	CColRefSet *pcrsLeft =
		GPOS_NEW(m_mp) CColRefSet(m_mp, *pexprLowered->DeriveUsedColumns());
	pcrsLeft->Exclude(pcrInner);
	CColRefArray *pdrgpcrLeft = pcrsLeft->Pdrgpcr(m_mp);
	pcrsLeft->Release();
	CColRefArray *pdrgpcrRight = GPOS_NEW(m_mp) CColRefArray(m_mp);
	pdrgpcrRight->Append(pcrInner);
	CExpression *pexprInner = (*pexprSubquery)[0];
	CColRefArray *pdrgpcrCorrelation =
		pexprInner->DeriveOuterReferences()->Pdrgpcr(m_mp);

	const BOOL fMatches =
		pmodel->FBind((*pdrgpsym)[1], pexprLowered) &&
		pmodel->FBind((*pdrgpsym)[2], pdrgpcrLeft) &&
		pmodel->FBind((*pdrgpsym)[3], pdrgpcrRight) &&
		pmodel->FBind((*pdrgpsym)[4], pdrgpcrCorrelation) &&
		pmodel->FBind((*pdrgpsym)[5], pexprInner);
	pexprLowered->Release();
	pdrgpcrLeft->Release();
	pdrgpcrRight->Release();
	pdrgpcrCorrelation->Release();
	return fMatches;
}

BOOL
CDSLConstraintChecker::FCheckExprListScalarSubquery(
	const CDSLConstraint *pcon, CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (nullptr == pdrgpsym || 8 != pdrgpsym->Size() ||
		(EdslsymExpr != (*pdrgpsym)[0]->Esymkind() &&
		 EdslsymFunc != (*pdrgpsym)[0]->Esymkind() &&
		 EdslsymPred != (*pdrgpsym)[0]->Esymkind() &&
		 EdslsymWindow != (*pdrgpsym)[0]->Esymkind()) ||
		(*pdrgpsym)[0]->Esymkind() != (*pdrgpsym)[1]->Esymkind() ||
		EdslsymPred != (*pdrgpsym)[2]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[3]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[4]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[5]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[6]->Esymkind() ||
		EdslsymTable != (*pdrgpsym)[7]->Esymkind())
	{
		return false;
	}

	CExpression *pexprSubquery =
		PexprNextSubqueryInSequence(pmodel, (*pdrgpsym)[0]);
	if (nullptr == pexprSubquery ||
		COperator::EopScalarSubquery != pexprSubquery->Pop()->Eopid() ||
		1 != pexprSubquery->Arity())
	{
		return false;
	}

	CScalarSubquery *popSubquery =
		CScalarSubquery::PopConvert(pexprSubquery->Pop());
	CColRef *pcrInner = const_cast<CColRef *>(popSubquery->Pcr());
	CExpression *pexprIdent = CUtils::PexprScalarIdent(m_mp, pcrInner);
	CRefCount *pvalLowered = PvalReplaceNodeInSequence(
		m_mp, pmodel, (*pdrgpsym)[0], pexprSubquery, pexprIdent);
	pexprIdent->Release();

	CExpression *pexprTrue = CUtils::PexprScalarConstBool(m_mp, true);
	CColRefArray *pdrgpcrLeft = GPOS_NEW(m_mp) CColRefArray(m_mp);
	CColRefArray *pdrgpcrRight = GPOS_NEW(m_mp) CColRefArray(m_mp);
	CColRefArray *pdrgpcrInner = GPOS_NEW(m_mp) CColRefArray(m_mp);
	pdrgpcrInner->Append(pcrInner);
	CExpression *pexprInner = (*pexprSubquery)[0];
	CColRefArray *pdrgpcrCorrelation =
		pexprInner->DeriveOuterReferences()->Pdrgpcr(m_mp);

	const BOOL fLowered = EdslsymPred == (*pdrgpsym)[1]->Esymkind()
		? pmodel->FBindDerived((*pdrgpsym)[1], pvalLowered)
		: pmodel->FBind((*pdrgpsym)[1], pvalLowered);
	const BOOL fMatches =
		fLowered &&
		pmodel->FBind((*pdrgpsym)[2], pexprTrue) &&
		pmodel->FBind((*pdrgpsym)[3], pdrgpcrLeft) &&
		pmodel->FBind((*pdrgpsym)[4], pdrgpcrRight) &&
		pmodel->FBind((*pdrgpsym)[5], pdrgpcrCorrelation) &&
		pmodel->FBind((*pdrgpsym)[6], pdrgpcrInner) &&
		pmodel->FBind((*pdrgpsym)[7], pexprInner);
	pvalLowered->Release();
	pexprTrue->Release();
	pdrgpcrLeft->Release();
	pdrgpcrRight->Release();
	pdrgpcrCorrelation->Release();
	pdrgpcrInner->Release();
	return fMatches;
}

BOOL
CDSLConstraintChecker::FCheckExprListExistential(
	const CDSLConstraint *pcon, CDSLModel *pmodel, BOOL fNegated) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (nullptr == pdrgpsym || 11 != pdrgpsym->Size() ||
		(EdslsymExpr != (*pdrgpsym)[0]->Esymkind() &&
		 EdslsymFunc != (*pdrgpsym)[0]->Esymkind() &&
		 EdslsymPred != (*pdrgpsym)[0]->Esymkind() &&
		 EdslsymWindow != (*pdrgpsym)[0]->Esymkind()) ||
		(*pdrgpsym)[0]->Esymkind() != (*pdrgpsym)[1]->Esymkind() ||
		EdslsymExpr != (*pdrgpsym)[2]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[3]->Esymkind() ||
		EdslsymSchema != (*pdrgpsym)[4]->Esymkind() ||
		EdslsymPred != (*pdrgpsym)[5]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[6]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[7]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[8]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[9]->Esymkind() ||
		EdslsymTable != (*pdrgpsym)[10]->Esymkind())
	{
		return false;
	}

	CExpression *pexprSubquery =
		PexprNextSubqueryInSequence(pmodel, (*pdrgpsym)[0]);
	const COperator::EOperatorId eopid =
		fNegated ? COperator::EopScalarSubqueryNotExists
				 : COperator::EopScalarSubqueryExists;
	if (nullptr == pexprSubquery || eopid != pexprSubquery->Pop()->Eopid() ||
		1 != pexprSubquery->Arity())
	{
		return false;
	}

	CExpression *pexprInner = (*pexprSubquery)[0];
	CColRefSet *pcrsInnerOutput = pexprInner->DeriveOutputColumns();
	if (0 == pcrsInnerOutput->Size())
	{
		return false;
	}

	const IMDTypeBool *pmdtypebool =
		COptCtxt::PoctxtFromTLS()->Pmda()->PtMDType<IMDTypeBool>();
	CColRef *pcrMarker = COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
		pmdtypebool, default_type_modifier);
	CExpressionArray *pdrgpexprMarker = GPOS_NEW(m_mp) CExpressionArray(m_mp);
	pdrgpexprMarker->Append(CUtils::PexprScalarProjectElement(
		m_mp, pcrMarker, CUtils::PexprScalarConstBool(m_mp, true)));
	CExpression *pexprMarkerList = GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CScalarProjectList(m_mp), pdrgpexprMarker);

	IMDId *pmdidBool = pmdtypebool->MDId();
	pmdidBool->AddRef();
	CExpression *pexprExistsValue = GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CScalarIf(m_mp, pmdidBool),
		CUtils::PexprIsNotNull(
			m_mp, CUtils::PexprScalarIdent(m_mp, pcrMarker)),
		CUtils::PexprScalarConstBool(m_mp, !fNegated),
		CUtils::PexprScalarConstBool(m_mp, fNegated));
	CRefCount *pvalLowered = PvalReplaceNodeInSequence(
		m_mp, pmodel, (*pdrgpsym)[0], pexprSubquery, pexprExistsValue);
	pexprExistsValue->Release();

	CColRefArray *pdrgpcrMarkerAttrs =
		pexprMarkerList->DeriveUsedColumns()->Pdrgpcr(m_mp);
	CColRefArray *pdrgpcrMarkerSchema = GPOS_NEW(m_mp) CColRefArray(m_mp);
	pdrgpcrMarkerSchema->Append(pcrMarker);
	CExpression *pexprTrue = CUtils::PexprScalarConstBool(m_mp, true);
	CColRefArray *pdrgpcrLeft = GPOS_NEW(m_mp) CColRefArray(m_mp);
	CColRefArray *pdrgpcrRight = GPOS_NEW(m_mp) CColRefArray(m_mp);
	CColRefArray *pdrgpcrCorrelation =
		pexprInner->DeriveOuterReferences()->Pdrgpcr(m_mp);
	CColRefArray *pdrgpcrRequiredInner = GPOS_NEW(m_mp) CColRefArray(m_mp);
	pdrgpcrRequiredInner->Append(pcrMarker);
	pdrgpcrRequiredInner->Append(pcrsInnerOutput->PcrFirst());

	const BOOL fLowered = EdslsymPred == (*pdrgpsym)[1]->Esymkind()
		? pmodel->FBindDerived((*pdrgpsym)[1], pvalLowered)
		: pmodel->FBind((*pdrgpsym)[1], pvalLowered);
	const BOOL fMatches =
		fLowered &&
		pmodel->FBind((*pdrgpsym)[2], pexprMarkerList) &&
		pmodel->FBind((*pdrgpsym)[3], pdrgpcrMarkerAttrs) &&
		pmodel->FBind((*pdrgpsym)[4], pdrgpcrMarkerSchema) &&
		pmodel->FBind((*pdrgpsym)[5], pexprTrue) &&
		pmodel->FBind((*pdrgpsym)[6], pdrgpcrLeft) &&
		pmodel->FBind((*pdrgpsym)[7], pdrgpcrRight) &&
		pmodel->FBind((*pdrgpsym)[8], pdrgpcrCorrelation) &&
		pmodel->FBind((*pdrgpsym)[9], pdrgpcrRequiredInner) &&
		pmodel->FBind((*pdrgpsym)[10], pexprInner);
	pvalLowered->Release();
	pexprMarkerList->Release();
	pdrgpcrMarkerAttrs->Release();
	pdrgpcrMarkerSchema->Release();
	pexprTrue->Release();
	pdrgpcrLeft->Release();
	pdrgpcrRight->Release();
	pdrgpcrCorrelation->Release();
	pdrgpcrRequiredInner->Release();
	return fMatches;
}

BOOL
CDSLConstraintChecker::FCheckExprListQuantified(
	const CDSLConstraint *pcon, CDSLModel *pmodel, BOOL fAll) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (nullptr == pdrgpsym || 11 != pdrgpsym->Size() ||
		(EdslsymExpr != (*pdrgpsym)[0]->Esymkind() &&
		 EdslsymFunc != (*pdrgpsym)[0]->Esymkind() &&
		 EdslsymPred != (*pdrgpsym)[0]->Esymkind() &&
		 EdslsymWindow != (*pdrgpsym)[0]->Esymkind()) ||
		(*pdrgpsym)[0]->Esymkind() != (*pdrgpsym)[1]->Esymkind() ||
		EdslsymExpr != (*pdrgpsym)[2]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[3]->Esymkind() ||
		EdslsymSchema != (*pdrgpsym)[4]->Esymkind() ||
		EdslsymPred != (*pdrgpsym)[5]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[6]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[7]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[8]->Esymkind() ||
		EdslsymAttrs != (*pdrgpsym)[9]->Esymkind() ||
		EdslsymTable != (*pdrgpsym)[10]->Esymkind())
	{
		return false;
	}

	CExpression *pexprSubquery =
		PexprNextSubqueryInSequence(pmodel, (*pdrgpsym)[0]);
	const COperator::EOperatorId eopid =
		fAll ? COperator::EopScalarSubqueryAll
			 : COperator::EopScalarSubqueryAny;
	if (nullptr == pexprSubquery || eopid != pexprSubquery->Pop()->Eopid() ||
		2 != pexprSubquery->Arity())
	{
		return false;
	}

	CScalarSubqueryQuantified *popQuantified =
		CScalarSubqueryQuantified::PopConvert(pexprSubquery->Pop());
	CColRef *pcrInner = const_cast<CColRef *>(popQuantified->Pcr());
	CExpression *pexprInner = (*pexprSubquery)[0];

	const IMDTypeBool *pmdtypebool =
		COptCtxt::PoctxtFromTLS()->Pmda()->PtMDType<IMDTypeBool>();
	CColRef *pcrMarker = COptCtxt::PoctxtFromTLS()->Pcf()->PcrCreate(
		pmdtypebool, default_type_modifier);
	CExpressionArray *pdrgpexprMarker = GPOS_NEW(m_mp) CExpressionArray(m_mp);
	pdrgpexprMarker->Append(CUtils::PexprScalarProjectElement(
		m_mp, pcrMarker, CUtils::PexprScalarConstBool(m_mp, true)));
	CExpression *pexprMarkerList = GPOS_NEW(m_mp) CExpression(
		m_mp, GPOS_NEW(m_mp) CScalarProjectList(m_mp), pdrgpexprMarker);

	CExpression *pexprMarker = CUtils::PexprScalarIdent(m_mp, pcrMarker);
	CRefCount *pvalLowered = PvalReplaceNodeInSequence(
		m_mp, pmodel, (*pdrgpsym)[0], pexprSubquery, pexprMarker);
	pexprMarker->Release();

	CExpression *pexprComparison =
		CDSLQuantifiedMatcher::PexprComparison(m_mp, pexprSubquery);
	CColRefSet *pcrsLeft = GPOS_NEW(m_mp)
		CColRefSet(m_mp, *pexprComparison->DeriveUsedColumns());
	pcrsLeft->Exclude(pcrInner);
	CColRefArray *pdrgpcrLeft = pcrsLeft->Pdrgpcr(m_mp);
	pcrsLeft->Release();
	CColRefArray *pdrgpcrRight = GPOS_NEW(m_mp) CColRefArray(m_mp);
	pdrgpcrRight->Append(pcrInner);
	CColRefArray *pdrgpcrCorrelation =
		pexprInner->DeriveOuterReferences()->Pdrgpcr(m_mp);
	CColRefArray *pdrgpcrRequiredInner = GPOS_NEW(m_mp) CColRefArray(m_mp);
	pdrgpcrRequiredInner->Append(pcrMarker);
	pdrgpcrRequiredInner->Append(pcrInner);
	CColRefArray *pdrgpcrMarkerAttrs =
		pexprMarkerList->DeriveUsedColumns()->Pdrgpcr(m_mp);
	CColRefArray *pdrgpcrMarkerSchema = GPOS_NEW(m_mp) CColRefArray(m_mp);
	pdrgpcrMarkerSchema->Append(pcrMarker);

	const BOOL fLowered = EdslsymPred == (*pdrgpsym)[1]->Esymkind()
		? pmodel->FBindDerived((*pdrgpsym)[1], pvalLowered)
		: pmodel->FBind((*pdrgpsym)[1], pvalLowered);
	const BOOL fMatches =
		fLowered &&
		pmodel->FBind((*pdrgpsym)[2], pexprMarkerList) &&
		pmodel->FBind((*pdrgpsym)[3], pdrgpcrMarkerAttrs) &&
		pmodel->FBind((*pdrgpsym)[4], pdrgpcrMarkerSchema) &&
		pmodel->FBind((*pdrgpsym)[5], pexprComparison) &&
		pmodel->FBind((*pdrgpsym)[6], pdrgpcrLeft) &&
		pmodel->FBind((*pdrgpsym)[7], pdrgpcrRight) &&
		pmodel->FBind((*pdrgpsym)[8], pdrgpcrCorrelation) &&
		pmodel->FBind((*pdrgpsym)[9], pdrgpcrRequiredInner) &&
		pmodel->FBind((*pdrgpsym)[10], pexprInner);
	pvalLowered->Release();
	pexprMarkerList->Release();
	pdrgpcrMarkerAttrs->Release();
	pdrgpcrMarkerSchema->Release();
	pexprComparison->Release();
	pdrgpcrLeft->Release();
	pdrgpcrRight->Release();
	pdrgpcrCorrelation->Release();
	pdrgpcrRequiredInner->Release();
	return fMatches;
}

BOOL
CDSLConstraintChecker::FCheckCumulativeFrame(
	const CDSLConstraint *pcon, const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (1 != pdrgpsym->Size() ||
		EdslsymFrame != (*pdrgpsym)[0]->Esymkind())
	{
		return false;
	}
	CWindowFrameArray *pdrgpwf = pmodel->PdrgpwfFrame((*pdrgpsym)[0]);
	if (nullptr == pdrgpwf || 0 == pdrgpwf->Size())
	{
		return false;
	}
	for (ULONG ul = 0; ul < pdrgpwf->Size(); ul++)
	{
		CWindowFrame *pwf = (*pdrgpwf)[ul];
		if (CWindowFrame::IsEmpty(pwf) ||
			CWindowFrame::EfbUnboundedPreceding != pwf->EfbLeading() ||
			CWindowFrame::EfbCurrentRow != pwf->EfbTrailing() ||
			nullptr != pwf->PexprLeading() || nullptr != pwf->PexprTrailing() ||
			(CWindowFrame::EfesNone != pwf->Efes() &&
			 CWindowFrame::EfesNulls != pwf->Efes()))
		{
			return false;
		}
	}
	return true;
}

BOOL
CDSLConstraintChecker::FCheckScalarConstant(
	const CDSLConstraint *pcon, const CDSLModel *pmodel, LINT value) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (nullptr == pdrgpsym || 1 != pdrgpsym->Size() ||
		EdslsymScalar != (*pdrgpsym)[0]->Esymkind())
	{
		return false;
	}

	CExpression *pexpr = pmodel->PexprScalar((*pdrgpsym)[0]);
	if (nullptr == pexpr)
	{
		// A target-only scalar is constructed by CDSLInstantiator.
		return EdslsideTarget == (*pdrgpsym)[0]->Eside();
	}
	if (COperator::EopScalarConst != pexpr->Pop()->Eopid())
	{
		return false;
	}
	IDatum *pdatum = CScalarConst::PopConvert(pexpr->Pop())->GetDatum();
	if (pdatum->IsNull())
	{
		return false;
	}
	switch (pdatum->GetDatumType())
	{
		case IMDType::EtiInt2:
			return value == dynamic_cast<IDatumInt2 *>(pdatum)->Value();
		case IMDType::EtiInt4:
			return value == dynamic_cast<IDatumInt4 *>(pdatum)->Value();
		case IMDType::EtiInt8:
			return value == dynamic_cast<IDatumInt8 *>(pdatum)->Value();
		default:
			return false;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintChecker::FCheckReference
//
//	@doc:
//		Reference(t0,a0,t1,a1): the columns bound to a0 (in the relation bound to
//		t0) reference the columns bound to a1 (in the relation bound to t1) via a
//		foreign key. Symbols are positional: [t0, a0, t1, a1].
//
//		Live-metadata path (M2): resolve t0's bound subtree to its CLogicalGet ->
//		CTableDescriptor -> relation MDId -> IMDRelation, then look for a foreign
//		key whose referenced relation is t1's MDId and whose local/referenced attno
//		pairs equal the bound a0 / a1 pairs. FK metadata is populated only by
//		CMDRelationGPDB from a live relcache; the programmatic test fixture carries
//		none, so on a synthetic relation ForeignKeyCount()==0 and this returns false
//		(a Reference-guarded rule then simply does not fire — no regression).
//---------------------------------------------------------------------------

// collect the (table) attnos of the columns bound to an attrs symbol into pais.
// Returns false if the symbol is unbound or any bound column is not a table
// column (a Reference over computed columns cannot be an FK).
static BOOL
FCollectAttnos(CMemoryPool *mp, const CDSLSymbol *psymAttrs,
			   const CDSLModel *pmodel, IntPtrArray *pais)
{
	CColRefArray *pdrgpcr = pmodel->PdrgpcrAttrs(psymAttrs);
	if (nullptr == pdrgpcr || 0 == pdrgpcr->Size())
	{
		return false;
	}
	const ULONG ulCols = pdrgpcr->Size();
	for (ULONG ul = 0; ul < ulCols; ul++)
	{
		CColRef *pcr = (*pdrgpcr)[ul];
		if (CColRef::EcrtTable != pcr->Ecrt())
		{
			return false;
		}
		pais->Append(GPOS_NEW(mp) INT(CColRefTable::PcrConvert(pcr)->AttrNum()));
	}
	return true;
}

// Match an FK's local/referenced pairs against the bound join-key pairs. Pair
// order is irrelevant because conjunct order is irrelevant, but correspondence
// within each pair is essential for composite foreign keys.
static BOOL
FSameAttnoPairs(const IntPtrArray *paisFkLocal,
				const IntPtrArray *paisFkRef,
				const IntPtrArray *paisBoundLocal,
				const IntPtrArray *paisBoundRef)
{
	const ULONG ulKeys = paisFkLocal->Size();
	if (ulKeys != paisFkRef->Size() ||
		ulKeys != paisBoundLocal->Size() ||
		ulKeys != paisBoundRef->Size())
	{
		return false;
	}
	for (ULONG ul = 0; ul < ulKeys; ul++)
	{
		BOOL fFound = false;
		for (ULONG ulBound = 0; ulBound < ulKeys && !fFound; ulBound++)
		{
			fFound =
				*(*paisFkLocal)[ul] == *(*paisBoundLocal)[ulBound] &&
				*(*paisFkRef)[ul] == *(*paisBoundRef)[ulBound];
		}
		if (!fFound)
		{
			return false;
		}
	}
	return true;
}

// Reference is an inclusion dependency over an ordered column vector.  It is
// reflexive when both sides name the same base relation and the same attnos in
// the same order; unlike FK metadata comparison, a mere set equality is not
// sufficient here because (a,b) and (b,a) need not contain the same tuples.
static BOOL
FSameAttnoSequence(const IntPtrArray *paisFst, const IntPtrArray *paisSnd)
{
	if (paisFst->Size() != paisSnd->Size())
	{
		return false;
	}
	for (ULONG ul = 0; ul < paisFst->Size(); ul++)
	{
		if (*(*paisFst)[ul] != *(*paisSnd)[ul])
		{
			return false;
		}
	}
	return true;
}

// Resolve the one base Get that owns every column bound to the attrs symbol.
// Unary relational wrappers such as Select, pass-through Project and GbAgg
// retain table CColRefs, so requiring the table symbol itself to be a bare Get
// unnecessarily rejects valid FK proofs. Computed columns, mixed owners and
// columns not present below the bound subtree remain conservatively rejected.
static CExpression *
PexprOwningGetForAttrs(const CDSLSymbol *psymTable,
					   const CDSLSymbol *psymAttrs,
					   const CDSLModel *pmodel)
{
	CExpression *pexpr = pmodel->PexprTable(psymTable);
	CColRefArray *pdrgpcr = pmodel->PdrgpcrAttrs(psymAttrs);
	if (nullptr == pexpr || nullptr == pdrgpcr || 0 == pdrgpcr->Size())
	{
		return nullptr;
	}

	CExpression *pexprOwner = nullptr;
	for (ULONG ul = 0; ul < pdrgpcr->Size(); ul++)
	{
		CColRef *pcr = (*pdrgpcr)[ul];
		if (CColRef::EcrtTable != pcr->Ecrt())
		{
			return nullptr;
		}
		CExpression *pexprCurrent = PexprOwningGetInSubtree(pexpr, pcr);
		if (nullptr == pexprCurrent ||
			(nullptr != pexprOwner && pexprOwner != pexprCurrent))
		{
			return nullptr;
		}
		pexprOwner = pexprCurrent;
	}
	return pexprOwner;
}

// Prove that the referred subtree still contains every value vector of the
// bound base columns.  A pass-through Project and a grouping aggregate that
// groups on all referenced columns preserve that coverage; Select/HAVING,
// joins and other row-reducing operators do not.  This is the missing lineage
// distinction between an unfiltered SELECT DISTINCT key domain and an
// arbitrary derived relation over the same base Get.
static BOOL
FReferringDomainCoveredByFilter(CMemoryPool *mp,
							CExpression *pexprReferring,
							const CColRefArray *pdrgpcrLocal,
							const CColRefArray *pdrgpcrReferred,
							CExpression *pexprFilter)
{
	if (nullptr == pexprReferring || nullptr == pdrgpcrLocal ||
		nullptr == pdrgpcrReferred || nullptr == pexprFilter ||
		pdrgpcrLocal->Size() != pdrgpcrReferred->Size())
	{
		return false;
	}

	// A filter on the referenced side preserves FK coverage when every column it
	// constrains is a referenced key column and the referring side's domain is a
	// subset after positional FK remapping. This handles normalized predicate
	// pushdown (for example local_fk = 2 and referred_key = 2) without assuming
	// that arbitrary filtered relations still contain the complete key domain.
	CColRefSet *pcrsUsed = pexprFilter->DeriveUsedColumns();
	if (0 == pcrsUsed->Size())
	{
		return false;
	}
	CColRefSetIter crsi(*pcrsUsed);
	while (crsi.Advance())
	{
		CColRef *pcrReferred = crsi.Pcr();
		BOOL fMatched = false;
		for (ULONG ul = 0; ul < pdrgpcrReferred->Size(); ul++)
		{
			if (pcrReferred == (*pdrgpcrReferred)[ul])
			{
				fMatched = true;
				break;
			}
		}
		if (!fMatched)
		{
			return false;
		}
	}

	// Derive the admitted key domain from this filter predicate itself. Using
	// the Select's combined property would be unsound when the predicate is not
	// constraint-convertible (for example a quantified subquery): in that case
	// the property can contain only inherited base-table constraints and make an
	// arbitrary row-reducing filter look domain preserving.
	// Constraint derivation deliberately skips unsupported AND children. That is
	// useful for estimation, but an inclusion-dependency proof must account for
	// every conjunct: an omitted conjunct can further reduce the referred key
	// domain. Require each flattened conjunct to be representable before using
	// the combined constraint.
	CExpressionArray *pdrgpexprConj =
		CPredicateUtils::PdrgpexprConjuncts(mp, pexprFilter);
	BOOL fFullyConstrained = true;
	for (ULONG ul = 0; fFullyConstrained && ul < pdrgpexprConj->Size(); ul++)
	{
		CColRefSetArray *pdrgpcrsConj = nullptr;
		CConstraint *pcnstrConj = CConstraint::PcnstrFromScalarExpr(
			mp, (*pdrgpexprConj)[ul], &pdrgpcrsConj);
		fFullyConstrained = nullptr != pcnstrConj &&
			!pcnstrConj->IsConstraintUnbounded();
		CRefCount::SafeRelease(pcnstrConj);
		CRefCount::SafeRelease(pdrgpcrsConj);
	}
	pdrgpexprConj->Release();
	if (!fFullyConstrained)
	{
		return false;
	}

	CColRefSetArray *pdrgpcrsFilter = nullptr;
	CConstraint *pcnstrFilterRoot = CConstraint::PcnstrFromScalarExpr(
		mp, pexprFilter, &pdrgpcrsFilter);
	CPropConstraint *ppcLocal = pexprReferring->DerivePropertyConstraint();
	CConstraint *pcnstrLocalRoot = ppcLocal->Pcnstr();
	if (nullptr == pcnstrLocalRoot || nullptr == pcnstrFilterRoot)
	{
		CRefCount::SafeRelease(pdrgpcrsFilter);
		CRefCount::SafeRelease(pcnstrFilterRoot);
		return false;
	}

	BOOL fCovered = true;
	CColRefSetIter crsiConstraints(*pcrsUsed);
	while (fCovered && crsiConstraints.Advance())
	{
		CColRef *pcrReferred = crsiConstraints.Pcr();
		ULONG ulMatch = 0;
		while (pcrReferred != (*pdrgpcrReferred)[ulMatch])
		{
			ulMatch++;
		}
		CColRef *pcrLocal = (*pdrgpcrLocal)[ulMatch];
		CConstraint *pcnstrReferred =
			pcnstrFilterRoot->Pcnstr(mp, pcrReferred);
		CConstraint *pcnstrLocal = pcnstrLocalRoot->Pcnstr(mp, pcrLocal);
		if (nullptr == pcnstrReferred || nullptr == pcnstrLocal)
		{
			CRefCount::SafeRelease(pcnstrReferred);
			CRefCount::SafeRelease(pcnstrLocal);
			fCovered = false;
			continue;
		}
		CConstraint *pcnstrMapped =
			pcnstrReferred->PcnstrRemapForColumn(mp, pcrLocal);
		fCovered = pcnstrMapped->Contains(pcnstrLocal);
		pcnstrMapped->Release();
		pcnstrReferred->Release();
		pcnstrLocal->Release();
	}
	pcnstrFilterRoot->Release();
	CRefCount::SafeRelease(pdrgpcrsFilter);
	return fCovered;
}

static BOOL
FReferredSubtreeCoversAttrs(CMemoryPool *mp, CExpression *pexpr,
							CExpression *pexprOwner,
							const CColRefArray *pdrgpcrAttrs,
							CExpression *pexprReferring,
							const CColRefArray *pdrgpcrLocal)
{
	if (nullptr == pexpr || nullptr == pexprOwner || nullptr == pdrgpcrAttrs)
	{
		return false;
	}

	if (pexpr == pexprOwner)
	{
		return COperator::EopLogicalGet == pexpr->Pop()->Eopid() &&
			   !CLogicalGet::PopConvert(pexpr->Pop())->HasSecurityQuals();
	}

	for (ULONG ul = 0; ul < pdrgpcrAttrs->Size(); ul++)
	{
		if (!pexpr->DeriveOutputColumns()->FMember((*pdrgpcrAttrs)[ul]))
		{
			return false;
		}
	}

	switch (pexpr->Pop()->Eopid())
	{
		case COperator::EopLogicalSelect:
			return 1 < pexpr->Arity() &&
				   FReferringDomainCoveredByFilter(
					   mp, pexprReferring, pdrgpcrLocal,
					   pdrgpcrAttrs, (*pexpr)[1]) &&
				   FReferredSubtreeCoversAttrs(
					   mp, (*pexpr)[0], pexprOwner, pdrgpcrAttrs,
					   pexprReferring, pdrgpcrLocal);

		case COperator::EopLogicalProject:
			return 0 < pexpr->Arity() &&
				   FReferredSubtreeCoversAttrs(
					   mp, (*pexpr)[0], pexprOwner, pdrgpcrAttrs,
					   pexprReferring, pdrgpcrLocal);

		case COperator::EopLogicalGbAgg:
		{
			CColRefArray *pdrgpcrGroup =
				CLogicalGbAgg::PopConvert(pexpr->Pop())->Pdrgpcr();
			for (ULONG ul = 0; ul < pdrgpcrAttrs->Size(); ul++)
			{
				BOOL fGrouped = false;
				for (ULONG ulGroup = 0;
					 ulGroup < pdrgpcrGroup->Size() && !fGrouped; ulGroup++)
				{
					fGrouped = ((*pdrgpcrAttrs)[ul] == (*pdrgpcrGroup)[ulGroup]);
				}
				if (!fGrouped)
				{
					return false;
				}
			}
			return 0 < pexpr->Arity() &&
				   FReferredSubtreeCoversAttrs(
					   mp, (*pexpr)[0], pexprOwner, pdrgpcrAttrs,
					   pexprReferring, pdrgpcrLocal);
		}

		default:
			return false;
	}
}

BOOL
CDSLConstraintChecker::FCheckReference(const CDSLConstraint *pcon,
									   const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (4 != pdrgpsym->Size())
	{
		return false;
	}
	// positional schema: Reference(t0, a0, t1, a1)
	const CDSLSymbol *psymTab0 = (*pdrgpsym)[0];
	const CDSLSymbol *psymAttr0 = (*pdrgpsym)[1];
	const CDSLSymbol *psymTab1 = (*pdrgpsym)[2];
	const CDSLSymbol *psymAttr1 = (*pdrgpsym)[3];
	if (EdslsymTable != psymTab0->Esymkind() ||
		EdslsymAttrs != psymAttr0->Esymkind() ||
		EdslsymTable != psymTab1->Esymkind() ||
		EdslsymAttrs != psymAttr1->Esymkind())
	{
		return false;
	}

	CExpression *pexprGet0 =
		PexprOwningGetForAttrs(psymTab0, psymAttr0, pmodel);
	CExpression *pexprGet1 =
		PexprOwningGetForAttrs(psymTab1, psymAttr1, pmodel);
	if (nullptr == pexprGet0 || nullptr == pexprGet1)
	{
		return false;
	}
	CTableDescriptor *ptabdesc0 = PtabdescBaseAccess(pexprGet0);
	CTableDescriptor *ptabdesc1 = PtabdescBaseAccess(pexprGet1);
	if (nullptr == ptabdesc0 || nullptr == ptabdesc1)
	{
		return false;
	}
	IMDId *pmdidRel0 = ptabdesc0->MDId();
	IMDId *pmdidRel1 = ptabdesc1->MDId();

	IntPtrArray *paisLocal = GPOS_NEW(m_mp) IntPtrArray(m_mp);
	IntPtrArray *paisRef = GPOS_NEW(m_mp) IntPtrArray(m_mp);
	if (!FCollectAttnos(m_mp, psymAttr0, pmodel, paisLocal) ||
		!FCollectAttnos(m_mp, psymAttr1, pmodel, paisRef))
	{
		paisLocal->Release();
		paisRef->Release();
		return false;
	}

	CMDAccessor *pmda = COptCtxt::PoctxtFromTLS()->Pmda();
	CExpression *pexprReferred = pmodel->PexprTable(psymTab1);
	CExpression *pexprReferring = pmodel->PexprTable(psymTab0);
	CColRefArray *pdrgpcrLocal = pmodel->PdrgpcrAttrs(psymAttr0);
	CColRefArray *pdrgpcrReferred = pmodel->PdrgpcrAttrs(psymAttr1);
	const BOOL fReferredCovers = FReferredSubtreeCoversAttrs(
		m_mp, pexprReferred, pexprGet1, pdrgpcrReferred,
		pexprReferring, pdrgpcrLocal);

	// Inclusion in the same unfiltered base relation is true without an
	// explicit FK: every value vector produced by the referring subtree occurs in
	// the complete key domain on the referred side. The same coverage proof is
	// also required for a declared FK: an FK into a filtered derived relation is
	// not an inclusion dependency at the point where the rule is applied.
	BOOL fHolds = pmdidRel0->Equals(pmdidRel1) &&
				  FSameAttnoSequence(paisLocal, paisRef) &&
				  fReferredCovers;
	// RetrieveRel raises ExmiMDCacheEntryNotFound when the relation isn't cached
	// (e.g. the synthetic programmatic-test fixture registers only scalar types).
	// A best-effort FK check must never abort optimization, so swallow that one
	// exception and treat it as "cannot confirm the FK" => reject.
	GPOS_TRY
	{
		if (!fHolds && fReferredCovers)
		{
			const IMDRelation *prel = pmda->RetrieveRel(pmdidRel0);
			const ULONG ulFK = prel->ForeignKeyCount();
			for (ULONG ul = 0; ul < ulFK && !fHolds; ul++)
			{
				const CMDForeignKey *pfk = prel->ForeignKeyAt(ul);
				if (pfk->RefMdid()->Equals(pmdidRel1) &&
					FSameAttnoPairs(pfk->LocalAttnos(), pfk->RefAttnos(),
								   paisLocal, paisRef))
				{
					fHolds = true;
				}
			}
		}
	}
	GPOS_CATCH_EX(ex)
	{
		if (GPOS_MATCH_EX(ex, gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryNotFound))
		{
			GPOS_RESET_EX;
			fHolds = false;
		}
		else
		{
			paisLocal->Release();
			paisRef->Release();
			GPOS_RETHROW(ex);
		}
	}
	GPOS_CATCH_END;

	paisLocal->Release();
	paisRef->Release();
	return fHolds;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintChecker::FCheckExprConcat
//---------------------------------------------------------------------------
BOOL
CDSLConstraintChecker::FCheckExprConcat(const CDSLConstraint *pcon,
									 const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (3 != pdrgpsym->Size() ||
		EdslsymExpr != (*pdrgpsym)[0]->Esymkind() ||
		EdslsymExpr != (*pdrgpsym)[1]->Esymkind() ||
		EdslsymExpr != (*pdrgpsym)[2]->Esymkind())
	{
		return false;
	}
	CExpression *pexprOut = pmodel->PexprExpr((*pdrgpsym)[0]);
	CExpression *pexprLeft = pmodel->PexprExpr((*pdrgpsym)[1]);
	CExpression *pexprRight = pmodel->PexprExpr((*pdrgpsym)[2]);
	if (nullptr == pexprLeft || nullptr == pexprRight ||
		COperator::EopScalarProjectList != pexprLeft->Pop()->Eopid() ||
		COperator::EopScalarProjectList != pexprRight->Pop()->Eopid())
	{
		return false;
	}

	// A parent SRF cannot be flattened into a child layer that also contains an
	// SRF: doing so changes the row-expansion product. This is the same semantic
	// boundary enforced by ORCA's native project collapse, expressed once in the
	// generic list-composition primitive.
	if (!CDSLExprListUtils::FConcatSafe(pexprLeft, pexprRight))
	{
		return false;
	}
	if (nullptr == pexprOut)
	{
		return EdslsideTarget == (*pdrgpsym)[0]->Eside();
	}
	CExpression *pexprExpected = CDSLExprListUtils::PexprConcat(
		m_mp, pexprLeft, pexprRight);
	const BOOL fMatches = nullptr != pexprExpected &&
		pexprOut->Matches(pexprExpected);
	CRefCount::SafeRelease(pexprExpected);
	return fMatches;
}

BOOL
CDSLConstraintChecker::FCheckDepsDisjoint(
	const CDSLConstraint *pcon, const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (2 != pdrgpsym->Size() ||
		(EdslsymAttrs != (*pdrgpsym)[1]->Esymkind() &&
		 EdslsymSchema != (*pdrgpsym)[1]->Esymkind()))
	{
		return false;
	}

	const CDSLSymbol *psymLeft = (*pdrgpsym)[0];
	const CDSLSymbol *psymRight = (*pdrgpsym)[1];
	if (EdslsymExpr == psymLeft->Esymkind() &&
		EdslsymSchema == psymRight->Esymkind())
	{
		return CDSLExprListUtils::FDepsDisjoint(
			m_mp, pmodel->PexprExpr(psymLeft),
			pmodel->PdrgpcrSchema(psymRight));
	}

	auto pcrsDependencies = [this, pmodel](const CDSLSymbol *psym) {
		CColRefSet *pcrs = GPOS_NEW(m_mp) CColRefSet(m_mp);
		switch (psym->Esymkind())
		{
			case EdslsymAttrs:
				if (nullptr == pmodel->PdrgpcrAttrs(psym))
				{
					pcrs->Release();
					return static_cast<CColRefSet *>(nullptr);
				}
				pcrs->Include(pmodel->PdrgpcrAttrs(psym));
				break;
			case EdslsymSchema:
				if (nullptr == pmodel->PdrgpcrSchema(psym))
				{
					pcrs->Release();
					return static_cast<CColRefSet *>(nullptr);
				}
				pcrs->Include(pmodel->PdrgpcrSchema(psym));
				break;
			case EdslsymPred:
			case EdslsymExpr:
			case EdslsymWindow:
			{
				CExpression *pexpr = EdslsymPred == psym->Esymkind()
					? pmodel->PexprPred(psym)
					: (EdslsymExpr == psym->Esymkind()
						   ? pmodel->PexprExpr(psym)
						   : pmodel->PexprWindow(psym));
				if (nullptr == pexpr)
				{
					pcrs->Release();
					return static_cast<CColRefSet *>(nullptr);
				}
				pcrs->Include(pexpr->DeriveUsedColumns());
				break;
			}
			case EdslsymOrder:
			{
				COrderSpecArray *pdrgpos = pmodel->PdrgposOrder(psym);
				if (nullptr == pdrgpos)
				{
					pcrs->Release();
					return static_cast<CColRefSet *>(nullptr);
				}
				CColRefSet *pcrsOrder = COrderSpec::GetColRefSet(m_mp, pdrgpos);
				pcrs->Include(pcrsOrder);
				pcrsOrder->Release();
				break;
			}
			case EdslsymFrame:
			{
				CWindowFrameArray *pdrgpwf = pmodel->PdrgpwfFrame(psym);
				if (nullptr == pdrgpwf)
				{
					pcrs->Release();
					return static_cast<CColRefSet *>(nullptr);
				}
				for (ULONG ul = 0; ul < pdrgpwf->Size(); ul++)
				{
					pcrs->Include((*pdrgpwf)[ul]->PcrsUsed());
				}
				break;
			}
			default:
				pcrs->Release();
				return static_cast<CColRefSet *>(nullptr);
		}
		return pcrs;
	};

	CColRefSet *pcrsLeft = pcrsDependencies(psymLeft);
	CColRefSet *pcrsRight = pcrsDependencies(psymRight);
	if (nullptr == pcrsLeft || nullptr == pcrsRight)
	{
		CRefCount::SafeRelease(pcrsLeft);
		CRefCount::SafeRelease(pcrsRight);
		return false;
	}
	const BOOL fDisjoint = pcrsLeft->IsDisjoint(pcrsRight);
	pcrsRight->Release();
	pcrsLeft->Release();
	return fDisjoint;
}

BOOL
CDSLConstraintChecker::FCheckExprSplit(const CDSLConstraint *pcon,
									const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (4 != pdrgpsym->Size())
	{
		return false;
	}
	for (ULONG ul = 0; ul < 4; ul++)
	{
		if (EdslsymExpr != (*pdrgpsym)[ul]->Esymkind())
		{
			return false;
		}
	}
	CExpression *pexprUpper = pmodel->PexprExpr((*pdrgpsym)[2]);
	CExpression *pexprLower = pmodel->PexprExpr((*pdrgpsym)[3]);
	CExpression *pexprMerged = nullptr;
	CExpression *pexprResidual = nullptr;
	if (!CDSLExprListUtils::FSplit(m_mp, pexprUpper, pexprLower,
									&pexprMerged, &pexprResidual))
	{
		return false;
	}
	CExpression *pexprBoundMerged = pmodel->PexprExpr((*pdrgpsym)[0]);
	CExpression *pexprBoundResidual = pmodel->PexprExpr((*pdrgpsym)[1]);
	const BOOL fMergedValid = nullptr == pexprBoundMerged
		? EdslsideTarget == (*pdrgpsym)[0]->Eside()
		: pexprBoundMerged->Matches(pexprMerged);
	const BOOL fResidualValid = nullptr == pexprBoundResidual
		? EdslsideTarget == (*pdrgpsym)[1]->Eside()
		: pexprBoundResidual->Matches(pexprResidual);
	pexprMerged->Release();
	pexprResidual->Release();
	return fMergedValid && fResidualValid;
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintChecker::FCheckScalarProperty
//
//	@doc:
//		Port the expression-safety guards used by WeTune's newer proof backends.
//		Project attrs bind only their dependency columns, so recover the exact
//		captured project list before checking it. Error freedom is intentionally
//		conservative until ORCA metadata exposes operator/function error behavior;
//		determinism additionally excludes stable/volatile and set-returning trees.
//---------------------------------------------------------------------------
BOOL
CDSLConstraintChecker::FCheckScalarProperty(const CDSLRule *prule,
										 const CDSLConstraint *pcon,
										 const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (1 != pdrgpsym->Size())
	{
		return false;
	}
	const CDSLSymbol *psym = (*pdrgpsym)[0];
	const CDSLSymbol *psymBound = psym;
	if (nullptr == pmodel->PvalLookup(psymBound))
	{
		// A target predicate synthesized by PredicateAnd inherits both scalar
		// properties from its bound operands. This is construction semantics, not
		// an assumption about an arbitrary unbound target symbol.
		if (EdslsymPred == psym->Esymkind())
		{
			const CDSLExpressionDefinitions::CDefinition *pdef =
				prule->Pexprdefs()->Pdef(psym);
			if (nullptr != pdef && EdslexprAnd == pdef->Edslexpr())
			{
				CExpression *pexprLeft =
					pmodel->PexprPred(pdef->PsymOperand(0));
				CExpression *pexprRight =
					pmodel->PexprPred(pdef->PsymOperand(1));
				if (nullptr == pexprLeft || nullptr == pexprRight)
				{
					return false;
				}
				if (EdslconErrorFree == pcon->Edslcon())
				{
					return FScalarTreeProvablyErrorFree(pexprLeft) &&
						FScalarTreeProvablyErrorFree(pexprRight);
				}
				const BOOL fLeftHasNonScalar =
					pexprLeft->DeriveHasNonScalarFunction();
				const BOOL fRightHasNonScalar =
					pexprRight->DeriveHasNonScalarFunction();
				const IMDFunction::EFuncStbl efsLeft =
					pexprLeft->DeriveScalarFunctionProperties()->Efs();
				const IMDFunction::EFuncStbl efsRight =
					pexprRight->DeriveScalarFunctionProperties()->Efs();
				return !fLeftHasNonScalar && !fRightHasNonScalar &&
					IMDFunction::EfsImmutable == efsLeft &&
					IMDFunction::EfsImmutable == efsRight;
			}
		}
		// Resolve a target annotation only through an explicit equality to an
		// already-bound source artifact. Treating an arbitrary unbound target as
		// safe would silently discard the proof precondition.
		psymBound = nullptr;
		CDSLConstraintArray *pdrgpcon = prule->Pdrgpcon();
		for (ULONG ul = 0; ul < pdrgpcon->Size() && nullptr == psymBound; ul++)
		{
			const CDSLConstraint *pconEq = (*pdrgpcon)[ul];
			const EDslConstraintKind edslconEq = pconEq->Edslcon();
			const BOOL fEquality = EdslconTableEq == edslconEq ||
				EdslconAttrsEq == edslconEq ||
				EdslconPredicateEq == edslconEq ||
				EdslconSchemaEq == edslconEq ||
				EdslconFuncEq == edslconEq ||
				EdslconScalarEq == edslconEq ||
				EdslconExprListEq == edslconEq ||
				EdslconOrderEq == edslconEq ||
				EdslconWindowEq == edslconEq ||
				EdslconFrameEq == edslconEq;
			if (!fEquality || 2 != pconEq->Pdrgpsym()->Size())
			{
				continue;
			}
			const CDSLSymbol *psym0 = (*pconEq->Pdrgpsym())[0];
			const CDSLSymbol *psym1 = (*pconEq->Pdrgpsym())[1];
			const CDSLSymbol *psymPeer =
				psym0 == psym ? psym1 : (psym1 == psym ? psym0 : nullptr);
			if (nullptr != psymPeer && psymPeer->Esymkind() == psym->Esymkind() &&
				nullptr != pmodel->PvalLookup(psymPeer))
			{
				psymBound = psymPeer;
			}
		}
		if (nullptr == psymBound)
		{
			return false;
		}
	}

	CExpression *pexpr = nullptr;
	switch (psymBound->Esymkind())
	{
		case EdslsymAttrs:
			pexpr = PexprProjectListForAttrs(
				prule->PfragSrc()->PopRoot(), psymBound, pmodel);
			// A bare attribute vector denotes existing column references and has no
			// evaluation of its own. Project-defined attrs retain their exact scalar
			// list above and must pass the normal checks.
			if (nullptr == pexpr)
			{
				return nullptr != pmodel->PdrgpcrAttrs(psymBound);
			}
			break;
		case EdslsymFunc:
		{
			CExpressionArray *pdrgpexpr = pmodel->PdrgpexprFunc(psymBound);
			if (nullptr == pdrgpexpr)
			{
				return false;
			}
			for (ULONG ul = 0; ul < pdrgpexpr->Size(); ul++)
			{
				CExpression *pexprFunc = (*pdrgpexpr)[ul];
				if (EdslconErrorFree == pcon->Edslcon())
				{
					if (!FScalarTreeProvablyErrorFree(pexprFunc))
					{
						return false;
					}
				}
				else if (pexprFunc->DeriveHasNonScalarFunction() ||
						 IMDFunction::EfsImmutable !=
							 pexprFunc->DeriveScalarFunctionProperties()->Efs())
				{
					return false;
				}
			}
			return true;
		}
		case EdslsymPred:
			pexpr = pmodel->PexprPred(psymBound);
			break;
		case EdslsymScalar:
			pexpr = pmodel->PexprScalar(psymBound);
			break;
		case EdslsymExpr:
			pexpr = pmodel->PexprExpr(psymBound);
			break;
		case EdslsymWindow:
			pexpr = pmodel->PexprWindow(psymBound);
			break;
		default:
			break;
	}
	if (nullptr == pexpr)
	{
		return false;
	}

	if (EdslconErrorFree == pcon->Edslcon())
	{
		return FScalarTreeProvablyErrorFree(pexpr);
	}
	GPOS_ASSERT(EdslconDeterministic == pcon->Edslcon());
	return !pexpr->DeriveHasNonScalarFunction() &&
		IMDFunction::EfsImmutable ==
			pexpr->DeriveScalarFunctionProperties()->Efs();
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintChecker::FCheckEquality
//---------------------------------------------------------------------------
BOOL
CDSLConstraintChecker::FCheckEquality(const CDSLRule *prule,
								  const CDSLConstraint *pcon,
								  const CDSLModel *pmodel) const
{
	CDSLSymbolArray *pdrgpsym = pcon->Pdrgpsym();
	if (2 != pdrgpsym->Size())
	{
		return false;
	}
	const CDSLSymbol *psymFirst = (*pdrgpsym)[0];
	const CDSLSymbol *psymSecond = (*pdrgpsym)[1];
	CRefCount *pvalFirst = pmodel->PvalLookup(psymFirst);
	CRefCount *pvalSecond = pmodel->PvalLookup(psymSecond);
	if (nullptr == pvalFirst || nullptr == pvalSecond)
	{
		// Target symbols are deliberately unbound during source matching.
		return true;
	}

	switch (pcon->Edslcon())
	{
		case EdslconTableEq:
		{
			CExpression *pexprFirst = pmodel->PexprTable(psymFirst);
			CExpression *pexprSecond = pmodel->PexprTable(psymSecond);
			if (pexprFirst == pexprSecond)
			{
				return true;
			}
			BOOL fFirstAmbiguous = false;
			BOOL fSecondAmbiguous = false;
			CExpression *pexprFirstGet =
				PexprSingleBaseGet(pexprFirst, &fFirstAmbiguous);
			CExpression *pexprSecondGet =
				PexprSingleBaseGet(pexprSecond, &fSecondAmbiguous);
			if (nullptr != pexprFirstGet && nullptr != pexprSecondGet)
			{
				CTableDescriptor *ptabdescFirst =
					PtabdescBaseAccess(pexprFirstGet);
				CTableDescriptor *ptabdescSecond =
					PtabdescBaseAccess(pexprSecondGet);
				return nullptr != ptabdescFirst && nullptr != ptabdescSecond &&
					   ptabdescFirst->MDId()->Equals(ptabdescSecond->MDId());
			}
			return pexprFirst->Matches(pexprSecond);
		}
		case EdslconAttrsEq:
			return FColArraysSemanticEqual(
				prule, pmodel, pmodel->PdrgpcrAttrs(psymFirst),
				pmodel->PdrgpcrAttrs(psymSecond));
		case EdslconSchemaEq:
			return FColArraysSemanticEqual(
				prule, pmodel, pmodel->PdrgpcrSchema(psymFirst),
				pmodel->PdrgpcrSchema(psymSecond));
		case EdslconPredicateEq:
			return pmodel->PexprPred(psymFirst)->Matches(
				pmodel->PexprPred(psymSecond));
		case EdslconFuncEq:
		{
			CExpressionArray *pdrgpexprFirst =
				pmodel->PdrgpexprFunc(psymFirst);
			CExpressionArray *pdrgpexprSecond =
				pmodel->PdrgpexprFunc(psymSecond);
			if (pdrgpexprFirst->Size() != pdrgpexprSecond->Size())
			{
				return false;
			}
			for (ULONG ul = 0; ul < pdrgpexprFirst->Size(); ul++)
			{
				if (!(*pdrgpexprFirst)[ul]->Matches((*pdrgpexprSecond)[ul]))
				{
					return false;
				}
			}
			return true;
		}
		case EdslconScalarEq:
			return pmodel->PexprScalar(psymFirst)->Matches(
				pmodel->PexprScalar(psymSecond));
		case EdslconExprListEq:
			return pmodel->PexprExpr(psymFirst)->Matches(
				pmodel->PexprExpr(psymSecond));
		case EdslconOrderEq:
			return COrderSpec::Equals(pmodel->PdrgposOrder(psymFirst),
									 pmodel->PdrgposOrder(psymSecond));
		case EdslconWindowEq:
			return pmodel->PexprWindow(psymFirst)->Matches(
				pmodel->PexprWindow(psymSecond));
		case EdslconFrameEq:
			return CWindowFrame::Equals(pmodel->PdrgpwfFrame(psymFirst),
									   pmodel->PdrgpwfFrame(psymSecond));
		default:
			return false;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintChecker::FCheckOne
//---------------------------------------------------------------------------
BOOL
CDSLConstraintChecker::FCheckOne(const CDSLRule *prule,
							 const CDSLConstraint *pcon,
								 CDSLModel *pmodel) const
{
	switch (pcon->Edslcon())
	{
			case EdslconAttrsSub:
				return FCheckAttrsSub(pcon, pmodel);
			case EdslconAttrsEmpty:
				return FCheckAttrsEmpty(pcon, pmodel);
		case EdslconAttrsNonEmpty:
			return FCheckAttrsNonEmpty(pcon, pmodel);
		case EdslconOutputAttrs:
			return FCheckOutputAttrs(pcon, pmodel);
		case EdslconSchemaFromAttrs:
			return FCheckSchemaFromAttrs(pcon, pmodel);
		case EdslconFuncAttrs:
			return FCheckFuncAttrs(pcon, pmodel);
		case EdslconPredicateDomainSplit:
			return FCheckPredicateDomainSplit(prule, pcon, pmodel);
		case EdslconAttrsIntersect:
			return FCheckAttrsIntersect(pcon, pmodel);
		case EdslconAttrsUnion:
		case EdslconSchemaUnion:
			return FCheckAttrsUnion(pcon, pmodel);
		case EdslconCorrelationEquality:
			return FCheckCorrelationEquality(pcon, pmodel);
		case EdslconMinimalGrouping:
			return FCheckMinimalGrouping(pcon, pmodel);
		case EdslconUnique:
			return FCheckUnique(pcon, pmodel);
		case EdslconNotNull:
			return FCheckNotNull(pcon, pmodel);
		case EdslconPredicateFalse:
			return FCheckPredicateFalse(pcon, pmodel);
		case EdslconPredicateAnd:
			return FCheckPredicateAnd(pcon, pmodel);
		case EdslconPredicateExists:
			return FCheckPredicateExists(pcon, pmodel, false);
		case EdslconPredicateNotExists:
			return FCheckPredicateExists(pcon, pmodel, true);
		case EdslconPredicateAny:
			return FCheckPredicateQuantified(pcon, pmodel, false);
		case EdslconPredicateAll:
			return FCheckPredicateQuantified(pcon, pmodel, true);
		case EdslconPredicateScalarSubquery:
			return FCheckPredicateScalarSubquery(pcon, pmodel);
		case EdslconExprListScalarSubquery:
			return FCheckExprListScalarSubquery(pcon, pmodel);
		case EdslconExprListExists:
			return FCheckExprListExistential(pcon, pmodel, false);
		case EdslconExprListNotExists:
			return FCheckExprListExistential(pcon, pmodel, true);
		case EdslconExprListAny:
			return FCheckExprListQuantified(pcon, pmodel, false);
		case EdslconExprListAll:
			return FCheckExprListQuantified(pcon, pmodel, true);
		case EdslconCumulativeFrame:
			return FCheckCumulativeFrame(pcon, pmodel);
		case EdslconScalarOne:
			return FCheckScalarConstant(pcon, pmodel, 1);
		case EdslconScalarZero:
			return FCheckScalarConstant(pcon, pmodel, 0);
		case EdslconReference:
			return FCheckReference(pcon, pmodel);
		case EdslconErrorFree:
		case EdslconDeterministic:
			return FCheckScalarProperty(prule, pcon, pmodel);
		case EdslconExprConcat:
			return FCheckExprConcat(pcon, pmodel);
		case EdslconDepsDisjoint:
			return FCheckDepsDisjoint(pcon, pmodel);
		case EdslconExprSplit:
			return FCheckExprSplit(pcon, pmodel);

		case EdslconTableEq:
		case EdslconAttrsEq:
		case EdslconPredicateEq:
		case EdslconSchemaEq:
		case EdslconFuncEq:
		case EdslconScalarEq:
		case EdslconExprListEq:
		case EdslconOrderEq:
		case EdslconWindowEq:
		case EdslconFrameEq:
			return FCheckEquality(prule, pcon, pmodel);

		default:
			// unknown constraint kind: be safe, do not fire.
			return false;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDSLConstraintChecker::FCheck
//
//	@doc:
//		All constraints must hold. Early-abort on the first failure (WeTune
//		Model.checkConstraints behaviour).
//---------------------------------------------------------------------------
BOOL
CDSLConstraintChecker::FCheck(const CDSLRule *prule,
								  CDSLModel *pmodel,
							  const CDSLConstraint **ppconFailed,
							  ULONG *pulFailed) const
{
	GPOS_ASSERT(nullptr != prule);
	GPOS_ASSERT(nullptr != pmodel);
	if (nullptr != ppconFailed)
	{
		*ppconFailed = nullptr;
	}
	if (nullptr != pulFailed)
	{
		*pulFailed = gpos::ulong_max;
	}

	CDSLConstraintArray *pdrgpcon = prule->Pdrgpcon();
	if (nullptr == pdrgpcon)
	{
		return true;  // no constraints => trivially satisfied
	}

	const ULONG ulCon = pdrgpcon->Size();
	CDSLInstantiator materializer(m_mp);
	for (ULONG ul = 0; ul < ulCon; ul++)
	{
		if (!FCheckOne(prule, (*pdrgpcon)[ul], pmodel) ||
			!materializer.FMaterializeConstraintOutputs(
				prule, (*pdrgpcon)[ul], pmodel))
		{
			if (nullptr != ppconFailed)
			{
				*ppconFailed = (*pdrgpcon)[ul];
			}
			if (nullptr != pulFailed)
			{
				*pulFailed = ul;
			}
			return false;
		}
	}
	return true;
}

// EOF
