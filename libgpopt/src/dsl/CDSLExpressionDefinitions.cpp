//---------------------------------------------------------------------------
// Typed scalar-expression definition graph compiled from RuleIR constraints.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLExpressionDefinitions.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

using namespace gpopt;

namespace
{
EDslExpressionKind
EdslexprKind(const CDSLConstraint *pcon)
{
	if (nullptr == pcon)
	{
		return EdslexprSentinel;
	}
	switch (pcon->Edslcon())
	{
		case EdslconPredicateAnd:
			return EdslexprAnd;
		case EdslconPredicateExists:
			return EdslexprExists;
		case EdslconPredicateNotExists:
			return EdslexprNotExists;
		case EdslconPredicateAny:
			return EdslexprAny;
		case EdslconPredicateAll:
			return EdslexprAll;
		case EdslconPredicateScalarSubquery:
			return EdslexprScalarSubquery;
		case EdslconExprListScalarSubquery:
			return EdslexprExprListScalarSubquery;
		case EdslconExprListExists:
			return EdslexprExprListExists;
		default:
			return EdslexprSentinel;
	}
}

const CDSLSymbol *
PsymOutput(const CDSLConstraint *pcon)
{
	const EDslExpressionKind edslexpr = EdslexprKind(pcon);
	if (EdslexprSentinel == edslexpr || nullptr == pcon->Pdrgpsym() ||
		(EdslexprAnd == edslexpr && 3 != pcon->Pdrgpsym()->Size()) ||
		((EdslexprExists == edslexpr || EdslexprNotExists == edslexpr) &&
		 2 != pcon->Pdrgpsym()->Size()) ||
		((EdslexprAny == edslexpr || EdslexprAll == edslexpr) &&
		 4 != pcon->Pdrgpsym()->Size()) ||
		(EdslexprScalarSubquery == edslexpr &&
		 6 != pcon->Pdrgpsym()->Size()) ||
		(EdslexprExprListScalarSubquery == edslexpr &&
		 8 != pcon->Pdrgpsym()->Size()) ||
		(EdslexprExprListExists == edslexpr &&
		 11 != pcon->Pdrgpsym()->Size()))
	{
		return nullptr;
	}
	return (*pcon->Pdrgpsym())[0];
}

BOOL
FUsesRecursive(const CDSLExpressionDefinitions *pexprdefs,
			   const CDSLSymbol *psymOutput,
			   const CDSLSymbol *psymOperand,
			   std::unordered_set<const CDSLSymbol *> *psetVisited)
{
	if (psymOutput == psymOperand)
	{
		return true;
	}
	if (!psetVisited->insert(psymOutput).second)
	{
		return false;
	}
	const CDSLExpressionDefinitions::CDefinition *pdef =
		pexprdefs->Pdef(psymOutput);
	if (nullptr == pdef)
	{
		return false;
	}
	for (ULONG ul = 0; ul < pdef->Arity(); ul++)
	{
		if (FUsesRecursive(pexprdefs, pdef->PsymOperand(ul), psymOperand,
						   psetVisited))
		{
			return true;
		}
	}
	return false;
}

BOOL
FHasCycle(const CDSLSymbol *psym,
		  const std::unordered_map<const CDSLSymbol *, const CDSLConstraint *>
			  &definitions,
		  std::unordered_set<const CDSLSymbol *> *psetVisiting,
		  std::unordered_set<const CDSLSymbol *> *psetComplete)
{
	if (psetComplete->find(psym) != psetComplete->end())
	{
		return false;
	}
	if (!psetVisiting->insert(psym).second)
	{
		return true;
	}
	auto it = definitions.find(psym);
	if (it != definitions.end())
	{
		CDSLSymbolArray *pdrgpsym = it->second->Pdrgpsym();
		for (ULONG ul = 1; ul < pdrgpsym->Size(); ul++)
		{
			if (FHasCycle((*pdrgpsym)[ul], definitions, psetVisiting,
						  psetComplete))
			{
				return true;
			}
		}
	}
	psetVisiting->erase(psym);
	psetComplete->insert(psym);
	return false;
}
}  // namespace

CDSLExpressionDefinitions::CDefinition::CDefinition(
	CMemoryPool *mp, EDslExpressionKind edslexpr,
	const CDSLSymbol *psymOutput,
	const CDSLSymbolArray *pdrgpsymDefinition)
	: m_edslexpr(edslexpr),
	  m_psymOutput(psymOutput),
	  m_pdrgpsymOperands(GPOS_NEW(mp) COperandArray(mp))
{
	GPOS_ASSERT(EdslexprSentinel != edslexpr);
	GPOS_ASSERT(nullptr != psymOutput);
	GPOS_ASSERT(nullptr != pdrgpsymDefinition);
	for (ULONG ul = 1; ul < pdrgpsymDefinition->Size(); ul++)
	{
		m_pdrgpsymOperands->Append((*pdrgpsymDefinition)[ul]);
	}
}

CDSLExpressionDefinitions::CDefinition::~CDefinition()
{
	m_pdrgpsymOperands->Release();
}

CDSLExpressionDefinitions::CDSLExpressionDefinitions(
	CMemoryPool *mp, const CDSLConstraintArray *pdrgpcon)
	: m_pdrgpdefByOutput(GPOS_NEW(mp) CDefinitionIndex(mp)),
	  m_pdrgpdefDefinitions(GPOS_NEW(mp) CDefinitionArray(mp))
{
	GPOS_ASSERT(FValidate(pdrgpcon));
	ULONG ulMaxId = 0;
	BOOL fHasDefinition = false;
	for (ULONG ul = 0; ul < pdrgpcon->Size(); ul++)
	{
		const CDSLConstraint *pcon = (*pdrgpcon)[ul];
		const CDSLSymbol *psym = PsymOutput(pcon);
		if (nullptr == psym)
		{
			continue;
		}
		fHasDefinition = true;
		ulMaxId = std::max(ulMaxId, psym->Id());
		m_pdrgpdefDefinitions->Append(GPOS_NEW(mp) CDefinition(
			mp, EdslexprKind(pcon), psym, pcon->Pdrgpsym()));
	}
	for (ULONG ul = 0; fHasDefinition && ul <= ulMaxId; ul++)
	{
		m_pdrgpdefByOutput->Append(nullptr);
	}
	for (ULONG ul = 0; ul < m_pdrgpdefDefinitions->Size(); ul++)
	{
		const CDefinition *pdef = (*m_pdrgpdefDefinitions)[ul];
		m_pdrgpdefByOutput->Replace(pdef->PsymOutput()->Id(), pdef);
	}
}

CDSLExpressionDefinitions::~CDSLExpressionDefinitions()
{
	m_pdrgpdefByOutput->Release();
	m_pdrgpdefDefinitions->Release();
}

const CDSLExpressionDefinitions::CDefinition *
CDSLExpressionDefinitions::Pdef(
	const CDSLSymbol *psymOutput) const
{
	if (nullptr == psymOutput ||
		psymOutput->Id() >= m_pdrgpdefByOutput->Size())
	{
		return nullptr;
	}
	return (*m_pdrgpdefByOutput)[psymOutput->Id()];
}

const CDSLSymbol *
CDSLExpressionDefinitions::PsymBinaryResult(
	EDslExpressionKind edslexpr,
	const CDSLSymbol *psymLeft, const CDSLSymbol *psymRight) const
{
	if (EdslexprSentinel == edslexpr)
	{
		return nullptr;
	}
	for (ULONG ul = 0; ul < m_pdrgpdefDefinitions->Size(); ul++)
	{
		const CDefinition *pdef = (*m_pdrgpdefDefinitions)[ul];
		if (edslexpr == pdef->Edslexpr() && 2 == pdef->Arity() &&
			pdef->PsymOperand(0) == psymLeft &&
			pdef->PsymOperand(1) == psymRight)
		{
			return pdef->PsymOutput();
		}
	}
	return nullptr;
}

BOOL
CDSLExpressionDefinitions::FUses(const CDSLSymbol *psymOutput,
								 const CDSLSymbol *psymOperand) const
{
	if (nullptr == psymOutput || nullptr == psymOperand)
	{
		return false;
	}
	std::unordered_set<const CDSLSymbol *> setVisited;
	return FUsesRecursive(this, psymOutput, psymOperand, &setVisited);
}

BOOL
CDSLExpressionDefinitions::FValidate(const CDSLConstraintArray *pdrgpcon)
{
	if (nullptr == pdrgpcon)
	{
		return false;
	}
	std::unordered_map<const CDSLSymbol *, const CDSLConstraint *> definitions;
	for (ULONG ul = 0; ul < pdrgpcon->Size(); ul++)
	{
		const CDSLConstraint *pcon = (*pdrgpcon)[ul];
		const CDSLSymbol *psym = PsymOutput(pcon);
		if (nullptr != psym && !definitions.emplace(psym, pcon).second)
		{
			return false;
		}
	}
	std::unordered_set<const CDSLSymbol *> setVisiting;
	std::unordered_set<const CDSLSymbol *> setComplete;
	for (const auto &entry : definitions)
	{
		if (FHasCycle(entry.first, definitions, &setVisiting, &setComplete))
		{
			return false;
		}
	}
	return true;
}
