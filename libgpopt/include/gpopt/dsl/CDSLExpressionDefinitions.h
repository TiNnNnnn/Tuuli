//---------------------------------------------------------------------------
// Typed scalar-expression definition graph compiled from RuleIR constraints.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLExpressionDefinitions_H
#define GPOPT_CDSLExpressionDefinitions_H

#include "gpos/base.h"

#include "gpopt/dsl/CDSLRule.h"

namespace gpopt
{
using namespace gpos;

enum EDslExpressionKind
{
	EdslexprAnd = 0,
	EdslexprNullSafeEq,
	EdslexprExists,
	EdslexprNotExists,
	EdslexprAny,
	EdslexprAll,
	EdslexprScalarSubquery,
	EdslexprExprListScalarSubquery,
	EdslexprExprListExists,
	EdslexprExprListNotExists,
	EdslexprExprListAny,
	EdslexprExprListAll,
	EdslexprSentinel
};

class CDSLExpressionDefinitions
{
public:
	class CDefinition
	{
	private:
		using COperandArray =
			CDynamicPtrArray<const CDSLSymbol, CleanupNULL>;

		EDslExpressionKind m_edslexpr;
		const CDSLSymbol *m_psymOutput;
		COperandArray *m_pdrgpsymOperands;

	public:
		CDefinition(const CDefinition &) = delete;
		CDefinition(CMemoryPool *mp, EDslExpressionKind edslexpr,
					const CDSLSymbol *psymOutput,
					const CDSLSymbolArray *pdrgpsymDefinition);
		~CDefinition();

		EDslExpressionKind Edslexpr() const { return m_edslexpr; }
		const CDSLSymbol *PsymOutput() const { return m_psymOutput; }
		ULONG Arity() const { return m_pdrgpsymOperands->Size(); }
		const CDSLSymbol *PsymOperand(ULONG ul) const
		{
			return (*m_pdrgpsymOperands)[ul];
		}
	};

private:
	using CDefinitionIndex =
		CDynamicPtrArray<const CDefinition, CleanupNULL>;
	using CDefinitionArray = CDynamicPtrArray<CDefinition, CleanupDelete>;

	CDefinitionIndex *m_pdrgpdefByOutput;
	CDefinitionArray *m_pdrgpdefDefinitions;

public:
	CDSLExpressionDefinitions(const CDSLExpressionDefinitions &) = delete;

	CDSLExpressionDefinitions(CMemoryPool *mp,
						   const CDSLConstraintArray *pdrgpcon);
	~CDSLExpressionDefinitions();

	// Return the unique typed expression definition for output, or NULL.
	const CDefinition *Pdef(
		const CDSLSymbol *psymOutput) const;
	ULONG UlDefinitions() const { return m_pdrgpdefDefinitions->Size(); }
	const CDefinition *PdefAt(ULONG ul) const
	{
		return (*m_pdrgpdefDefinitions)[ul];
	}

	// Find the output of a typed binary expression definition.
	const CDSLSymbol *PsymBinaryResult(
		EDslExpressionKind edslexpr,
		const CDSLSymbol *psymLeft, const CDSLSymbol *psymRight) const;

	// True when operand occurs in output's transitive definition tree.
	BOOL FUses(const CDSLSymbol *psymOutput,
			   const CDSLSymbol *psymOperand) const;

	// Reject duplicate definitions and cycles before a rule is admitted.
	static BOOL FValidate(const CDSLConstraintArray *pdrgpcon);
};
}  // namespace gpopt

#endif  // !GPOPT_CDSLExpressionDefinitions_H
