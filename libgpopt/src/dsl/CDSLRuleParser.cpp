//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLRuleParser.cpp
//
//	@doc:
//		ANTLR4-backed implementation of the DSL rule parser + semantic builder.
//
//		All ANTLR4 runtime types, std::string and exceptions live in THIS file
//		only. The parse tree is walked manually over the typed contexts (rather
//		than via listener callbacks) so the IR can be built bottom-up with early
//		error return and precise refcount discipline.
//
//		Semantic checks mirror WeTune exactly:
//		  * operator token -> kind via CDSLOpKindTable::Parse (aliases, '*', dir)
//		    == OpKind.parse
//		  * per-operator symbol count + positional kind == SymbolsImpl.bindSymbol
//		  * child arity == OpKind.numPredecessors
//		  * ONE shared symbol namespace across source+target; a name may be
//		    DECLARED (inside <...>) exactly once — redeclaring, including reusing
//		    a name on the other side, is the "value already present" error
//		    (BiMap semantics in SymbolNamingImpl.setName)
//		  * constraint name -> kind + arity == Constraint.parse / Kind.numSyms
//		  * constraints only REFERENCE already-declared symbols
//
//		Refcount convention (Append does NOT AddRef): a freshly created symbol
//		has rc=1 consumed by the op's symbol array (first owner); every further
//		owning array (the fragment's symbol list, each constraint's symbol list)
//		AddRefs before Append.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLRuleParser.h"

#include <exception>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "gpopt/dsl/CDSLExpressionDefinitions.h"

#include "DSLRuleLexer.h"
#include "DSLRuleParser.h"
#include "antlr4-runtime.h"

using namespace gpopt;

namespace
{
// Collects ANTLR lexer/parser syntax errors into a message buffer.
class CCollectingErrorListener : public antlr4::BaseErrorListener
{
public:
	std::string m_errs;
	bool m_had_error = false;

	void
	syntaxError(antlr4::Recognizer *, antlr4::Token *, size_t, size_t charPos,
				const std::string &msg, std::exception_ptr) override
	{
		m_had_error = true;
		if (!m_errs.empty())
		{
			m_errs += "; ";
		}
		m_errs += "col " + std::to_string(charPos) + ": " + msg;
	}
};

// Per-parse mutable state threaded through the manual walk.
struct SBuildCtx
{
	CMemoryPool *mp;
	// name -> declared symbol (shared across source & target); the pointer is
	// borrowed (owned by the op/fragment arrays), valid for the parse duration.
	std::unordered_map<std::string, CDSLSymbol *> symtab;
	ULONG next_id = 0;
	BOOL has_select_list = false;
	std::string err;

	void
	Fail(const std::string &msg)
	{
		if (err.empty())
		{
			err = msg;
		}
	}
	bool
	Failed() const
	{
		return !err.empty();
	}
};

// forward decl
CDSLOp *PopBuild(SBuildCtx &bctx, dsl::DSLRuleParser::OpContext *op_ctx,
				 EDslSide eside, CDSLSymbolArray *pdrgpsym_frag);

// Build the symbol array for one operator's <...> list, DECLARING each name in
// the shared namespace. On success, symbols are owned by the returned array
// (rc=1) and additionally AddRef'd into pdrgpsym_frag and registered in symtab.
CDSLSymbolArray *
PdrgpsymBuildDecls(SBuildCtx &bctx, EDslOpKind edslop,
				   EDslSortDir edslsort,
				   dsl::DSLRuleParser::SymlistContext *symlist_ctx,
				   EDslSide eside, CDSLSymbolArray *pdrgpsym_frag)
{
	CMemoryPool *mp = bctx.mp;
	const ULONG ul_expected = CDSLOpKindTable::UlSyms(edslop);
	const ULONG ul_given =
		(nullptr == symlist_ctx) ? 0 : (ULONG) symlist_ctx->term().size();

	// MONSOON's checked-in rule corpus predates aggregateOutputAttrs and uses
	// Agg<groupByAttrs aggregateAttrs aggFunc schema havingPred> (5 symbols).
	// Current SQLSolver adds aggregateOutputAttrs as the third symbol (6 total).
	// Accept both wire formats; the matcher/instantiator infer legacy aggregate
	// output columns from schema - groupByAttrs.
	const BOOL fLegacyAgg = EdslopAgg == edslop && 5 == ul_given;
	const BOOL fSelectList = EdslopProj == edslop && 3 == ul_given;
	bctx.has_select_list |= fSelectList;
	// A Join may bind a complete predicate (<p a a>), or
	// equality keys followed by optional output and/or residual bindings. Keep
	// every historical form wire-compatible.
	const BOOL fJoin =
		EdslopInnerJoin == edslop || EdslopLeftJoin == edslop ||
		EdslopFullJoin == edslop;
	const BOOL fCompatibleJoin = fJoin &&
		(2 == ul_given || 3 == ul_given || 4 == ul_given ||
		 5 == ul_given || 7 == ul_given);
	const BOOL fExists = EdslopExists == edslop;
	const BOOL fPredicateExists = fExists && 3 == ul_given;
	const BOOL fSemiJoin = EdslopSemiJoin == edslop;
	const BOOL fSemiApply = EdslopSemiApply == edslop;
	const BOOL fAntiJoin = EdslopAntiJoin == edslop;
	const BOOL fAntiApply = EdslopAntiApply == edslop;
	const BOOL fAntiJoinNotIn = EdslopAntiJoinNotIn == edslop;
	const BOOL fLegacyAntiJoinNotIn = fAntiJoinNotIn && 3 == ul_given;
	const BOOL fAntiApplyNotIn = EdslopAntiApplyNotIn == edslop;
	const BOOL fInnerApply = EdslopInnerApply == edslop ||
		EdslopLeftOuterApply == edslop;
	// Filter<p,localDeps> is the established spelling. The extended
	// Filter<p,localDeps,outerDeps> form makes correlation explicit.
	const BOOL fLegacyFilter = EdslopFilter == edslop && 2 == ul_given;
	// Legacy InSubFilter<a> takes its inner equality key from the RHS projection.
	// The extended form binds both key vectors and a residual predicate.
	const BOOL fLegacyInSub = EdslopInSubFilter == edslop && 1 == ul_given;
	// Existing WeTune corpora declare no Union symbols. The extended
	// Union<a s> form exposes the ordered full-row output so a later operator can
	// reference it (for example, full-row dedup above UnionAll).
	const BOOL fSetOp = EdslopUnion == edslop || EdslopIntersect == edslop ||
		EdslopExcept == edslop;
	const BOOL fCompatibleSetOp = fSetOp && (0 == ul_given || 2 == ul_given);
	if (ul_given != ul_expected && !fLegacyAgg && !fCompatibleJoin &&
		!fPredicateExists && !fLegacyInSub && !fCompatibleSetOp && !fLegacyFilter &&
		!fLegacyAntiJoinNotIn && !fSelectList)
	{
		std::ostringstream os;
		os << "operator " << CDSLOpKindTable::SzName(edslop) << " expects ";
		if (EdslopAgg == edslop)
			os << "5 or 6";
		else if (EdslopProj == edslop)
			os << "2 or 3";
		else if (fJoin)
			os << "2, 3, 4, 5, or 7";
		else if (fExists)
			os << "0 or 3";
		else if (EdslopInSubFilter == edslop)
			os << "1 or 5";
		else if (fSetOp)
			os << "0, 2, or 4";
		else if (EdslopFilter == edslop)
			os << "2 or 3";
		else if (fAntiJoinNotIn)
			os << "3 or 6";
		else
			os << ul_expected;
		os << " symbol(s) in <...>, got " << ul_given;
		bctx.Fail(os.str());
		return nullptr;
	}

	CDSLSymbolArray *pdrgpsym = GPOS_NEW(mp) CDSLSymbolArray(mp);
	for (ULONG ul = 0; ul < ul_given; ul++)
	{
		std::string name = symlist_ctx->term(ul)->SYMBOL()->getText();
		const BOOL fValidNotInSymbol = !fAntiJoinNotIn ||
			(3 == ul_given ? (0 == ul ? 'p' : 'a')
						   : ((0 == ul || 3 == ul) ? 'p' : 'a')) == name[0];
		const BOOL fSinglePredicateLayout =
			(fJoin && 3 == ul_given) || fPredicateExists || fSemiJoin ||
			fSemiApply || fAntiJoin || fAntiApply || fAntiApplyNotIn ||
			fInnerApply;
		const BOOL fValidSinglePredicateSymbol =
			!fSinglePredicateLayout ||
			(0 == ul ? 'p' == name[0] : 'a' == name[0]);
		if (!fValidSinglePredicateSymbol || !fValidNotInSymbol)
		{
			bctx.Fail(
				"predicate-bearing operator expects one <p> followed by <a> symbols");
			pdrgpsym->Release();
			return nullptr;
		}
		if (bctx.symtab.find(name) != bctx.symtab.end())
		{
			// redeclaration (incl. cross-side reuse) — WeTune BiMap collision
			bctx.Fail("value already present: symbol '" + name +
					  "' declared more than once");
			pdrgpsym->Release();
			return nullptr;
		}
		EDslSymbolKind esymk;
		if (fSelectList && 2 == ul)
		{
			esymk = EdslsymExpr;
		}
		else if (EdslopSort == edslop && EdslsortSpec == edslsort)
		{
			esymk = EdslsymOrder;
		}
		else if (fLegacyAgg && 2 <= ul)
		{
			// Current schema is [a,a,a,f,s,p]; removing aggregateOutputAttrs
			// yields the legacy [a,a,f,s,p] layout.
			esymk = CDSLOpKindTable::EsymkindAt(edslop, ul + 1);
		}
		else if ((fJoin && 3 == ul_given) || fPredicateExists)
		{
			// Predicate-only binary operators use [p,a,a], independent of their
			// canonical descriptor's historical symbol layout.
			static const EDslSymbolKind rgesymkPredicate[] = {
				EdslsymPred, EdslsymAttrs, EdslsymAttrs};
			esymk = rgesymkPredicate[ul];
		}
		else if (fJoin && 5 == ul_given && 2 <= ul)
		{
			// Predicate-only Join<a a p a a> skips the optional output pair in
			// the canonical [a,a,a,s,p,a,a] descriptor.
			esymk = CDSLOpKindTable::EsymkindAt(edslop, ul + 2);
		}
		else
		{
			esymk = CDSLOpKindTable::EsymkindAt(edslop, ul);
		}
		CDSLSymbol *psym =
			GPOS_NEW(mp) CDSLSymbol(mp, esymk, name.c_str(), bctx.next_id++, eside);
		pdrgpsym->Append(psym);	 // op array owns rc=1
		bctx.symtab[name] = psym;

		psym->AddRef();				  // extra owner: fragment symbol list
		pdrgpsym_frag->Append(psym);
	}
	return pdrgpsym;
}

// Recursively build a CDSLOp from an OpContext.
CDSLOp *
PopBuild(SBuildCtx &bctx, dsl::DSLRuleParser::OpContext *op_ctx, EDslSide eside,
		 CDSLSymbolArray *pdrgpsym_frag)
{
	CMemoryPool *mp = bctx.mp;

	// resolve operator token (aliases, '*', Sort direction)
	std::string token = op_ctx->ID()->getText();
	if (nullptr != op_ctx->STAR())
	{
		token += "*";
	}
	BOOL fStar = false;
	EDslSortDir edslsort = EdslsortNone;
	EDslAggFuncKind edslaggfunc = EdslaggfuncUnknown;
	EDslOpKind edslop = CDSLOpKindTable::Parse(token.c_str(), &fStar, &edslsort,
											  &edslaggfunc);
	if (EdslopSentinel == edslop)
	{
		bctx.Fail("unknown operator: " + token);
		return nullptr;
	}

	// symbols (declarations)
	CDSLSymbolArray *pdrgpsym = PdrgpsymBuildDecls(
		bctx, edslop, edslsort, op_ctx->symlist(), eside, pdrgpsym_frag);
	if (nullptr == pdrgpsym)
	{
		return nullptr;
	}
	if (EdslopProj == edslop && fStar && 3 == pdrgpsym->Size())
	{
		bctx.Fail("Proj* does not expose a scalar SELECT list");
		pdrgpsym->Release();
		return nullptr;
	}

	// children
	const ULONG ul_expected_children = CDSLOpKindTable::UlChildren(edslop);
	std::vector<dsl::DSLRuleParser::OpContext *> child_ctxs = op_ctx->op();
	if ((ULONG) child_ctxs.size() != ul_expected_children)
	{
		std::ostringstream os;
		os << "operator " << CDSLOpKindTable::SzName(edslop) << " expects "
		   << ul_expected_children << " child(ren), got " << child_ctxs.size();
		bctx.Fail(os.str());
		pdrgpsym->Release();
		return nullptr;
	}

	CDSLOpArray *pdrgpchild = GPOS_NEW(mp) CDSLOpArray(mp);
	for (ULONG ul = 0; ul < (ULONG) child_ctxs.size(); ul++)
	{
		CDSLOp *pchild = PopBuild(bctx, child_ctxs[ul], eside, pdrgpsym_frag);
		if (nullptr == pchild)
		{
			pdrgpchild->Release();	// cascades to already-built children
			pdrgpsym->Release();
			return nullptr;
		}
		pdrgpchild->Append(pchild);	 // owns rc=1
	}

	return GPOS_NEW(mp)
		CDSLOp(mp, edslop, fStar, edslsort, edslaggfunc, pdrgpsym,
				 pdrgpchild);
}

// Build one fragment (source or target). Returns NULL on failure.
CDSLFragment *
PfragBuild(SBuildCtx &bctx, dsl::DSLRuleParser::FragContext *frag_ctx,
		   EDslSide eside)
{
	CMemoryPool *mp = bctx.mp;
	CDSLSymbolArray *pdrgpsym_frag = GPOS_NEW(mp) CDSLSymbolArray(mp);
	CDSLOp *pop_root = PopBuild(bctx, frag_ctx->op(), eside, pdrgpsym_frag);
	if (nullptr == pop_root)
	{
		pdrgpsym_frag->Release();
		return nullptr;
	}
	return GPOS_NEW(mp) CDSLFragment(mp, pop_root, pdrgpsym_frag);
}

// Build the constraint list. Inputs reference symbols declared by the two
// operator fragments or by an earlier restricted LET output. A constructive
// output may introduce a target-local symbol into pdrgpsymTarget.
CDSLConstraintArray *
PdrgpconBuild(SBuildCtx &bctx,
			  dsl::DSLRuleParser::ConstraintsContext *cons_ctx,
			  CDSLSymbolArray *pdrgpsymTarget)
{
	CMemoryPool *mp = bctx.mp;
	CDSLConstraintArray *pdrgpcon = GPOS_NEW(mp) CDSLConstraintArray(mp);
	if (nullptr == cons_ctx)
	{
		return pdrgpcon;  // constraints are optional
	}

	for (auto *con_ctx : cons_ctx->constraint())
	{
		std::string cname = con_ctx->ID()->getText();
		std::vector<antlr4::tree::TerminalNode *> syms = con_ctx->SYMBOL();
		EDslConstraintKind edslcon = CDSLConstraintKindTable::Parse(cname.c_str());
		if ("Eq" == cname)
		{
			if (2 != syms.size())
			{
				bctx.Fail("Eq expects two symbols");
				pdrgpcon->Release();
				return nullptr;
			}
			const auto left = bctx.symtab.find(syms[0]->getText());
			const auto right = bctx.symtab.find(syms[1]->getText());
			if (left == bctx.symtab.end() || right == bctx.symtab.end() ||
				left->second->Esymkind() != right->second->Esymkind())
			{
				bctx.Fail("Eq expects two declared symbols of the same type");
				pdrgpcon->Release();
				return nullptr;
			}
			edslcon = CDSLConstraintKindTable::EdslconEquality(
				left->second->Esymkind());
			if (EdslconSentinel == edslcon)
			{
				bctx.Fail("Eq is not implemented for this symbol type");
				pdrgpcon->Release();
				return nullptr;
			}
		}
		if (EdslconSentinel == edslcon)
		{
			bctx.Fail("unknown constraint: " + cname);
			pdrgpcon->Release();
			return nullptr;
		}
		const ULONG ul_arity = CDSLConstraintKindTable::UlArity(edslcon);
		if ((ULONG) syms.size() != ul_arity)
		{
			std::ostringstream os;
			os << "constraint " << CDSLConstraintKindTable::SzName(edslcon)
			   << " expects " << ul_arity << " symbol(s), got " << syms.size();
			bctx.Fail(os.str());
			pdrgpcon->Release();
			return nullptr;
		}

		CDSLSymbolArray *pdrgpsym = GPOS_NEW(mp) CDSLSymbolArray(mp);
		bool ok = true;
		for (ULONG ulSym = 0; ulSym < syms.size(); ulSym++)
		{
			auto *sym_node = syms[ulSym];
			std::string name = sym_node->getText();
			auto it = bctx.symtab.find(name);
			if (it == bctx.symtab.end())
			{
				EDslSymbolKind esymkind =
					CDSLConstraintKindTable::EsymkindDerivedOutput(edslcon,
															 ulSym);
				if ((EdslconExprListScalarSubquery == edslcon ||
					 EdslconExprListExists == edslcon ||
					 EdslconExprListNotExists == edslcon ||
					 EdslconExprListAny == edslcon ||
					 EdslconExprListAll == edslcon) &&
					1 == ulSym &&
					0 < pdrgpsym->Size())
				{
					esymkind = (*pdrgpsym)[0]->Esymkind();
				}
				if (EdslsymSentinel == esymkind)
				{
					bctx.Fail("constraint references undeclared symbol '" + name +
							  "'");
					ok = false;
					break;
				}
				CDSLSymbol *psym = GPOS_NEW(mp) CDSLSymbol(
					mp, esymkind, name.c_str(), bctx.next_id++, EdslsideTarget);
				pdrgpsymTarget->Append(psym);
				bctx.symtab[name] = psym;
				it = bctx.symtab.find(name);
			}
			it->second->AddRef();  // extra owner: this constraint's sym list
			pdrgpsym->Append(it->second);
		}
		if (!ok)
		{
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if (EdslconPredicateFalse == edslcon &&
			EdslsymPred != (*pdrgpsym)[0]->Esymkind())
		{
			bctx.Fail("PredicateFalse expects a predicate symbol");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if (EdslconPredicateAnd == edslcon)
		{
			for (ULONG ul = 0; ul < pdrgpsym->Size(); ul++)
			{
				if (EdslsymPred != (*pdrgpsym)[ul]->Esymkind())
				{
					bctx.Fail("PredicateAnd expects predicate symbols");
					pdrgpsym->Release();
					pdrgpcon->Release();
					return nullptr;
				}
			}
		}
		if (EdslconPredicateNotTrue == edslcon || EdslconPredicateNot == edslcon)
		{
			for (ULONG ul = 0; ul < pdrgpsym->Size(); ul++)
			{
				if (EdslsymPred != (*pdrgpsym)[ul]->Esymkind())
				{
					bctx.Fail("predicate negation expects predicate symbols");
					pdrgpsym->Release();
					pdrgpcon->Release();
					return nullptr;
				}
			}
		}
		if (EdslconPredicateNullRejecting == edslcon &&
			(EdslsymPred != (*pdrgpsym)[0]->Esymkind() ||
			 EdslsymAttrs != (*pdrgpsym)[1]->Esymkind()))
		{
			bctx.Fail("PredicateNullRejecting expects predicate and attrs symbols");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if ((EdslconPredicateExists == edslcon ||
			 EdslconPredicateNotExists == edslcon) &&
			(EdslsymPred != (*pdrgpsym)[0]->Esymkind() ||
			 EdslsymTable != (*pdrgpsym)[1]->Esymkind()))
		{
			bctx.Fail("existence predicate expects predicate and table symbols");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if ((EdslconPredicateAny == edslcon ||
			 EdslconPredicateAll == edslcon) &&
			(EdslsymPred != (*pdrgpsym)[0]->Esymkind() ||
			 EdslsymPred != (*pdrgpsym)[1]->Esymkind() ||
			 EdslsymAttrs != (*pdrgpsym)[2]->Esymkind() ||
			 EdslsymTable != (*pdrgpsym)[3]->Esymkind()))
		{
			bctx.Fail("quantified predicate expects two predicates, attrs, and table symbols");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if (EdslconPredicateScalarSubquery == edslcon &&
			(EdslsymPred != (*pdrgpsym)[0]->Esymkind() ||
			 EdslsymPred != (*pdrgpsym)[1]->Esymkind() ||
			 EdslsymAttrs != (*pdrgpsym)[2]->Esymkind() ||
			 EdslsymAttrs != (*pdrgpsym)[3]->Esymkind() ||
			 EdslsymAttrs != (*pdrgpsym)[4]->Esymkind() ||
			 EdslsymTable != (*pdrgpsym)[5]->Esymkind()))
		{
			bctx.Fail("PredicateScalarSubquery expects two predicates, three attrs, and a table symbol");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if (EdslconExprListScalarSubquery == edslcon &&
			((EdslsymExpr != (*pdrgpsym)[0]->Esymkind() &&
			  EdslsymFunc != (*pdrgpsym)[0]->Esymkind() &&
			  EdslsymPred != (*pdrgpsym)[0]->Esymkind() &&
			  EdslsymWindow != (*pdrgpsym)[0]->Esymkind()) ||
			 (*pdrgpsym)[0]->Esymkind() != (*pdrgpsym)[1]->Esymkind() ||
			 EdslsymPred != (*pdrgpsym)[2]->Esymkind() ||
			 EdslsymAttrs != (*pdrgpsym)[3]->Esymkind() ||
			 EdslsymAttrs != (*pdrgpsym)[4]->Esymkind() ||
			 EdslsymAttrs != (*pdrgpsym)[5]->Esymkind() ||
			 EdslsymAttrs != (*pdrgpsym)[6]->Esymkind() ||
			 EdslsymTable != (*pdrgpsym)[7]->Esymkind()))
		{
			bctx.Fail("ExprListScalarSubquery expects two equal-kind scalar sequences, a predicate, four attrs, and a table symbol");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if ((EdslconExprListExists == edslcon ||
			 EdslconExprListNotExists == edslcon ||
			 EdslconExprListAny == edslcon ||
			 EdslconExprListAll == edslcon) &&
			((EdslsymExpr != (*pdrgpsym)[0]->Esymkind() &&
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
			 EdslsymTable != (*pdrgpsym)[10]->Esymkind()))
		{
			bctx.Fail("ExprListExists/NotExists/Any/All expects two equal-kind scalar sequences, a marker expression list, attrs/schema metadata, a predicate, four attrs, and a table symbol");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if (EdslconAttrsUnion == edslcon)
		{
			for (ULONG ul = 0; ul < pdrgpsym->Size(); ul++)
			{
				if (EdslsymAttrs != (*pdrgpsym)[ul]->Esymkind())
				{
					bctx.Fail("AttrsUnion expects attrs symbols");
					pdrgpsym->Release();
					pdrgpcon->Release();
					return nullptr;
				}
			}
		}
		if (EdslconSchemaUnion == edslcon &&
			(EdslsymSchema != (*pdrgpsym)[0]->Esymkind() ||
			 EdslsymSchema != (*pdrgpsym)[1]->Esymkind() ||
			 EdslsymAttrs != (*pdrgpsym)[2]->Esymkind()))
		{
			bctx.Fail("SchemaUnion expects output schema, input schema, and attrs symbols");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if (EdslconDepsDisjoint == edslcon &&
			((EdslsymExpr != (*pdrgpsym)[0]->Esymkind() &&
			  EdslsymPred != (*pdrgpsym)[0]->Esymkind() &&
			  EdslsymAttrs != (*pdrgpsym)[0]->Esymkind() &&
			  EdslsymSchema != (*pdrgpsym)[0]->Esymkind() &&
			  EdslsymOrder != (*pdrgpsym)[0]->Esymkind() &&
			  EdslsymWindow != (*pdrgpsym)[0]->Esymkind() &&
			  EdslsymFrame != (*pdrgpsym)[0]->Esymkind()) ||
			 (EdslsymAttrs != (*pdrgpsym)[1]->Esymkind() &&
			  EdslsymSchema != (*pdrgpsym)[1]->Esymkind())))
		{
			bctx.Fail(
				"DepsDisjoint expects dependency-bearing metadata and an attrs/schema domain");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if (EdslconMinimalGrouping == edslcon &&
			(EdslsymAttrs != (*pdrgpsym)[0]->Esymkind() ||
			 EdslsymSchema != (*pdrgpsym)[1]->Esymkind()))
		{
			bctx.Fail("MinimalGrouping expects grouping attrs and aggregate schema symbols");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if (EdslconCorrelationEquality == edslcon &&
			(EdslsymPred != (*pdrgpsym)[0]->Esymkind() ||
			 EdslsymAttrs != (*pdrgpsym)[1]->Esymkind() ||
			 EdslsymAttrs != (*pdrgpsym)[2]->Esymkind()))
		{
			bctx.Fail(
				"CorrelationEquality expects predicate, local attrs, and outer attrs symbols");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		const EDslSymbolKind esymEq =
			EdslconOrderEq == edslcon ? EdslsymOrder
			: (EdslconWindowEq == edslcon ? EdslsymWindow
				 : (EdslconRankEq == edslcon ? EdslsymRank
											 : (EdslconFrameEq == edslcon
													? EdslsymFrame
													: EdslsymSentinel)));
		if (EdslsymSentinel != esymEq &&
			(esymEq != (*pdrgpsym)[0]->Esymkind() ||
			 esymEq != (*pdrgpsym)[1]->Esymkind()))
		{
			bctx.Fail(std::string(CDSLConstraintKindTable::SzName(edslcon)) +
					  " expects two matching window-metadata symbols");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if ((EdslconCumulativeFrame == edslcon ||
			 EdslconFullPartitionFrame == edslcon) &&
			EdslsymFrame != (*pdrgpsym)[0]->Esymkind())
		{
			bctx.Fail("window frame constraint expects a frame symbol");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if (EdslconBoundedRowsFrame == edslcon &&
			(EdslsymFrame != (*pdrgpsym)[0]->Esymkind() ||
			 EdslsymScalar != (*pdrgpsym)[1]->Esymkind() ||
			 EdslsymScalar != (*pdrgpsym)[2]->Esymkind()))
		{
			bctx.Fail("BoundedRowsFrame expects frame and scalar symbols");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if (EdslconRowsFrame == edslcon &&
			(EdslsymFrame != (*pdrgpsym)[0]->Esymkind() ||
			 EdslsymFrameBound != (*pdrgpsym)[1]->Esymkind() ||
			 EdslsymFrameBound != (*pdrgpsym)[2]->Esymkind()))
		{
			bctx.Fail("RowsFrame expects frame-bound symbols");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if (EdslconSliceCompose == edslcon)
		{
			for (ULONG ul = 0; ul < pdrgpsym->Size(); ul++)
			{
				if (EdslsymScalar != (*pdrgpsym)[ul]->Esymkind())
				{
					bctx.Fail("SliceCompose expects six scalar symbols");
					pdrgpsym->Release();
					pdrgpcon->Release();
					return nullptr;
				}
			}
		}
		if ((EdslconScalarOne == edslcon || EdslconScalarZero == edslcon) &&
			EdslsymScalar != (*pdrgpsym)[0]->Esymkind())
		{
			bctx.Fail(std::string(CDSLConstraintKindTable::SzName(edslcon)) +
					  " expects a scalar symbol");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if ((EdslconAttrsEmpty == edslcon ||
			 EdslconAttrsNonEmpty == edslcon) &&
			EdslsymAttrs != (*pdrgpsym)[0]->Esymkind())
		{
			bctx.Fail("AttrsEmpty/AttrsNonEmpty expects an attrs symbol");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if (EdslconPredicateNullSafeEq == edslcon &&
			(EdslsymPred != (*pdrgpsym)[0]->Esymkind() ||
			 EdslsymAttrs != (*pdrgpsym)[1]->Esymkind() ||
			 EdslsymAttrs != (*pdrgpsym)[2]->Esymkind()))
		{
			bctx.Fail("PredicateNullSafeEq expects a predicate and two attrs symbols");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if (EdslconOrderEmpty == edslcon &&
			EdslsymOrder != (*pdrgpsym)[0]->Esymkind())
		{
			bctx.Fail("OrderEmpty expects an order symbol");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if (EdslconRankAttrs == edslcon &&
			(EdslsymAttrs != (*pdrgpsym)[0]->Esymkind() ||
			 EdslsymRank != (*pdrgpsym)[1]->Esymkind()))
		{
			bctx.Fail("RankAttrs expects attrs and rank symbols");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if (EdslconExprNulls == edslcon &&
			(EdslsymExpr != (*pdrgpsym)[0]->Esymkind() ||
			 EdslsymAttrs != (*pdrgpsym)[1]->Esymkind() ||
			 EdslsymAttrs != (*pdrgpsym)[2]->Esymkind()))
		{
			bctx.Fail("ExprNulls expects an expression list and two attrs symbols");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if (EdslconTableShared == edslcon &&
			(EdslsymTable != (*pdrgpsym)[0]->Esymkind() ||
			 EdslsymTable != (*pdrgpsym)[1]->Esymkind()))
		{
			bctx.Fail("TableShared expects two table symbols");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if (EdslconOutputAttrs == edslcon &&
			(EdslsymAttrs != (*pdrgpsym)[0]->Esymkind() ||
			 EdslsymTable != (*pdrgpsym)[1]->Esymkind()))
		{
			bctx.Fail("OutputAttrs expects attrs and table symbols");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if (EdslconSchemaFromAttrs == edslcon &&
			(EdslsymSchema != (*pdrgpsym)[0]->Esymkind() ||
			 EdslsymAttrs != (*pdrgpsym)[1]->Esymkind()))
		{
			bctx.Fail("SchemaFromAttrs expects schema and attrs symbols");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if (EdslconFuncAttrs == edslcon &&
			(EdslsymAttrs != (*pdrgpsym)[0]->Esymkind() ||
			 EdslsymFunc != (*pdrgpsym)[1]->Esymkind()))
		{
			bctx.Fail("FuncAttrs expects attrs and function symbols");
			pdrgpsym->Release();
			pdrgpcon->Release();
			return nullptr;
		}
		if (EdslconPredicateDomainSplit == edslcon)
		{
			const EDslSymbolKind rgExpected[] = {
				EdslsymPred, EdslsymPred, EdslsymPred,
				EdslsymAttrs, EdslsymAttrs, EdslsymAttrs, EdslsymAttrs,
				EdslsymTable, EdslsymTable};
			BOOL fTyped = GPOS_ARRAY_SIZE(rgExpected) == pdrgpsym->Size();
			for (ULONG ul = 0; fTyped && ul < pdrgpsym->Size(); ul++)
			{
				fTyped = rgExpected[ul] == (*pdrgpsym)[ul]->Esymkind();
			}
			if (!fTyped)
			{
				bctx.Fail("PredicateDomainSplit expects three predicates, four attrs, and two tables");
				pdrgpsym->Release();
				pdrgpcon->Release();
				return nullptr;
			}
		}
		pdrgpcon->Append(GPOS_NEW(mp) CDSLConstraint(mp, edslcon, pdrgpsym));
	}
	if (!CDSLExpressionDefinitions::FValidate(pdrgpcon))
	{
		bctx.Fail("invalid or cyclic expression definition graph");
		pdrgpcon->Release();
		return nullptr;
	}
	return pdrgpcon;
}

// Expression bindings match native Boolean trees, SELECT lists and ON slots.
// Input remains an arbitrary relational subtree; these are supported template
// constructors, not a whitelist of rewrite identities.
BOOL
FBindingTree(const CDSLOp *op)
{
	if (EdslopInput == op->Edslop())
	{
		return true;
	}
	const BOOL join = EdslopInnerJoin == op->Edslop() ||
		EdslopLeftJoin == op->Edslop() || EdslopFullJoin == op->Edslop() ||
		EdslopSemiJoin == op->Edslop() || EdslopAntiJoin == op->Edslop();
	if (nullptr == op->Pdrgpsym() ||
		!(join ? 3 == op->Pdrgpsym()->Size() && 2 == op->UlChildren()
			   : 1 == op->UlChildren() &&
				 ((EdslopFilter == op->Edslop() && 2 == op->Pdrgpsym()->Size()) ||
				  (EdslopProj == op->Edslop() && !op->FDistinct() &&
				   (2 == op->Pdrgpsym()->Size() || 3 == op->Pdrgpsym()->Size())))))
	{
		return false;
	}
	for (ULONG i = 0; i < op->UlChildren(); i++)
	{
		if (!FBindingTree((*op)[i]))
			return false;
	}
	return true;
}

BOOL
FDeclareBindings(SBuildCtx &bctx, dsl::DSLRuleParser::ConstraintsContext *ctx,
				 CDSLFragment *source, CDSLFragment *target)
{
	if (nullptr == ctx || ctx->binding().empty())
	{
		if (bctx.has_select_list)
		{
			bctx.Fail("explicit Proj lists require typed expression bindings");
			return false;
		}
		return true;
	}
	if (!FBindingTree(source->PopRoot()) || !FBindingTree(target->PopRoot()))
	{
		bctx.Fail(
			"expression bindings support Input/Filter/plain Proj and complete-predicate Join templates");
		return false;
	}
	// Constructor signatures declare types, not symbol-name prefixes. Source
	// captures precede target terms; references propagate types afterwards.
	std::vector<dsl::DSLRuleParser::BindingContext *> references;
	auto declare = [&](const std::string &name, EDslSymbolKind kind, BOOL match) {
		auto it = bctx.symtab.find(name);
		if (bctx.symtab.end() == it)
		{
			CDSLSymbol *symbol = GPOS_NEW(bctx.mp) CDSLSymbol(
				bctx.mp, kind, name.c_str(), bctx.next_id++,
				match ? EdslsideSource : EdslsideTarget);
			(match ? source : target)->Pdrgpsym()->Append(symbol);
			it = bctx.symtab.emplace(name, symbol).first;
		}
		if (kind != it->second->Esymkind() ||
			(match && EdslsideSource != it->second->Eside()))
		{
			bctx.Fail("invalid expression operand type or source capture of target symbol");
			return false;
		}
		return true;
	};
	for (BOOL match : {true, false})
	{
		for (auto *binding : ctx->binding())
		{
			const BOOL isMatch =
				binding->getStart()->getType() == dsl::DSLRuleParser::ID;
			if (match != isMatch)
			{
				continue;
			}
			auto *call = binding->call();
			if (nullptr == call)
			{
				references.push_back(binding);
				continue;
			}
			const BOOL comparison = "NullSafeEq" == call->ID()->getText();
			const BOOL item = "Item" == call->ID()->getText();
			const BOOL bool_value = "BoolValue" == call->ID()->getText();
			const BOOL case_value = "Case" == call->ID()->getText();
			if (!(("Not" == call->ID()->getText() || "NotTrue" == call->ID()->getText() || bool_value) &&
				  1 == call->SYMBOL().size()) &&
				!(("And" == call->ID()->getText() || "Or" == call->ID()->getText() || comparison) &&
				  2 == call->SYMBOL().size()) && !((item || case_value) && 3 == call->SYMBOL().size()))
			{
				bctx.Fail(
					"unsupported expression constructor or arity (expected Not/NotTrue/And/Or/NullSafeEq/Item/BoolValue/Case)");
				return false;
			}
			if (!declare(binding->SYMBOL(0)->getText(),
				item ? EdslsymExpr : bool_value || case_value ? EdslsymScalar : EdslsymPred, match))
			{
				return false;
			}
			for (ULONG i = 0; i < call->SYMBOL().size(); ++i)
			{
				const auto kind = item
					? (0 == i ? EdslsymScalar : 1 == i ? EdslsymAttrs : EdslsymExpr)
					: case_value ? (0 == i ? EdslsymPred : EdslsymScalar)
					: comparison ? EdslsymAttrs : EdslsymPred;
				if (!declare(call->SYMBOL(i)->getText(), kind, match))
				{
					return false;
				}
			}
		}
	}
	// A reference without a declared endpoint is not implicitly a predicate.
	while (!references.empty())
	{
		const auto before = references.size();
		for (auto it = references.begin(); it != references.end();)
		{
			const auto output = (*it)->SYMBOL(0)->getText();
			const auto input = (*it)->SYMBOL(1)->getText();
			auto known = bctx.symtab.find(output);
			if (bctx.symtab.end() == known)
				known = bctx.symtab.find(input);
			if (bctx.symtab.end() == known)
			{
				++it;
				continue;
			}
			const auto kind = known->second->Esymkind();
			if ((EdslsymPred != kind && EdslsymAttrs != kind &&
				 EdslsymTable != kind && EdslsymSchema != kind && EdslsymExpr != kind && EdslsymScalar != kind) ||
				!declare(output, kind, false) || !declare(input, kind, false))
			{
				bctx.Fail("expression reference requires matching predicate, scalar, attrs, table, schema or expression-list types");
				return false;
			}
			it = references.erase(it);
		}
		if (before == references.size())
		{
			bctx.Fail("expression reference has no declared type");
			return false;
		}
	}
	return true;
}

BOOL
FBuildBindings(SBuildCtx &bctx, dsl::DSLRuleParser::ConstraintsContext *ctx,
			   CDSLFragment *source, CDSLFragment *target, ULONG sourceSymbols,
			   const CDSLConstraintArray *constraints,
			   CDSLExpressionDefinitions *definitions)
{
	if (nullptr == ctx || ctx->binding().empty())
	{
		return true;
	}
	using Definitions = CDSLExpressionDefinitions;
	std::unordered_set<const CDSLSymbol *> available;
	std::unordered_map<const CDSLSymbol *, const CDSLSymbol *> aliases;
	for (ULONG i = 0; i < sourceSymbols; i++)
	{
		available.insert((*source->Pdrgpsym())[i]);
	}
	// Cross-side aliases are bindings. Do not silently interpret constructive
	// legacy constraints as premises for the new oriented expression language.
	for (ULONG i = 0; i < constraints->Size(); i++)
	{
		const CDSLConstraint *con = (*constraints)[i];
		const auto kind = con->Edslcon();
		BOOL sourcePremise = true;
		for (ULONG slot = 0; slot < con->Pdrgpsym()->Size(); slot++)
		{
			sourcePremise &=
				EdslsideSource == (*con->Pdrgpsym())[slot]->Eside() &&
				EdslsymSentinel ==
					CDSLConstraintKindTable::EsymkindDerivedOutput(kind, slot);
		}
		if (sourcePremise)
			continue;  // Checked after capture matching; never produces a value.
		const EDslSymbolKind expected = EdslconTableEq == kind	 ? EdslsymTable
										: EdslconAttrsEq == kind ? EdslsymAttrs
										: EdslconSchemaEq == kind ? EdslsymSchema
										: EdslconExprListEq == kind ? EdslsymExpr
										: EdslconScalarEq == kind ? EdslsymScalar
										: EdslconPredicateEq == kind
											? EdslsymPred
											: EdslsymSentinel;
		if (EdslsymSentinel == expected || 2 != con->Pdrgpsym()->Size())
		{
			bctx.Fail(
				"expression bindings accept source premises and cross-side aliases");
			return false;
		}
		const CDSLSymbol *a = (*con->Pdrgpsym())[0], *b = (*con->Pdrgpsym())[1];
		if (a->Esymkind() != expected || b->Esymkind() != expected ||
			a->Eside() == b->Eside() ||
			!aliases.emplace(EdslsideTarget == a->Eside() ? a : b,
				EdslsideTarget == a->Eside() ? b : a).second)
		{
			bctx.Fail("invalid or duplicate cross-side alias");
			return false;
		}
	}
	for (auto *binding : ctx->binding())
	{
		const BOOL match =
			binding->getStart()->getType() == dsl::DSLRuleParser::ID;
		const auto *output = bctx.symtab.at(binding->SYMBOL(0)->getText());
		auto *call = binding->call();
		const auto *input =
			bctx.symtab.at(nullptr != call ? call->SYMBOL(0)->getText()
										   : binding->SYMBOL(1)->getText());
		if (!match &&
			(EdslsideSource == output->Eside() || aliases.count(output)))
		{
			bctx.Fail(
				"expression construction cannot overwrite a source or alias");
			return false;
		}
		CDSLSymbolArray *symbols = GPOS_NEW(bctx.mp) CDSLSymbolArray(bctx.mp);
		for (const CDSLSymbol *symbol : {output, input})
		{
			const_cast<CDSLSymbol *>(symbol)->AddRef();
			symbols->Append(const_cast<CDSLSymbol *>(symbol));
		}
		for (ULONG i = 1; nullptr != call && i < call->SYMBOL().size(); ++i)
		{
			CDSLSymbol *right = bctx.symtab.at(call->SYMBOL(i)->getText());
			right->AddRef();
			symbols->Append(right);
		}
		const BOOL valid = definitions->FAppendBinding(
			bctx.mp, nullptr == call ? EdslexprRef
				: "And" == call->ID()->getText() ? EdslexprAnd
				: "Or" == call->ID()->getText() ? EdslexprOr
				: "NullSafeEq" == call->ID()->getText() ? EdslexprNullSafeEq
				: "Item" == call->ID()->getText() ? EdslexprItem
				: "BoolValue" == call->ID()->getText() ? EdslexprBoolValue
				: "Case" == call->ID()->getText() ? EdslexprCase
				: "NotTrue" == call->ID()->getText() ? EdslexprNotTrue : EdslexprNot,
			match ? Definitions::EMatch : Definitions::EBuild, symbols);
		symbols->Release();
		if (!valid)
		{
			bctx.Fail("duplicate or cyclic expression binding");
			return false;
		}
	}
	// Dataflow, not declaration order, determines availability. Source matches
	// produce captures; target constructions consume them. Reject disconnected
	// patterns and undefined leaves instead of admitting non-executable rules.
	size_t previous;
	do
	{
		previous = available.size();
		for (const auto &alias : aliases)
		{
			if (available.count(alias.second))
				available.insert(alias.first);
		}
		for (ULONG i = 0; i < definitions->UlDefinitions(); i++)
		{
			const auto *def = definitions->PdefAt(i);
			const BOOL match = Definitions::EMatch == def->Binding();
			BOOL inputsReady = true;
			for (ULONG operand = 0; operand < def->Arity(); operand++)
			{
				const CDSLSymbol *input = def->PsymOperand(operand);
				if (match && available.count(def->PsymOutput()))
					available.insert(input);
				inputsReady &= 0 != available.count(input);
			}
			if (!match && inputsReady)
				available.insert(def->PsymOutput());
		}
	} while (previous != available.size());
	for (const CDSLFragment *fragment : {source, target})
	{
		for (ULONG i = 0; i < fragment->Pdrgpsym()->Size(); i++)
		{
			if (!available.count((*fragment->Pdrgpsym())[i]))
			{
				bctx.Fail(
					"unreachable source pattern or undefined expression input/target");
				return false;
			}
		}
	}
	return true;
}
// Lower surface syntax only. Matching direction, types, scope, constructor
// support and proof/runtime semantics remain owned by the binding pipeline.
std::string
LowerInlineTerm(dsl::DSLRuleParser::TermContext *term, BOOL source,
				std::unordered_set<std::string> &names, ULONG &next,
				std::vector<std::string> &bindings)
{
	if (nullptr != term->SYMBOL())
		return term->SYMBOL()->getText();
	std::string output;
	do
	{
		output = "p" + std::to_string(next++);
	} while (!names.insert(output).second);
	// Source capture declarations follow the pattern root before its children;
	// target definitions follow their operands. Preserve existing named-rule IDs.
	const size_t slot = bindings.size();
	if (source)
		bindings.emplace_back();
	std::string call = term->ID()->getText() + "(";
	for (auto *operand : term->term())
	{
		if (call.back() != '(')
			call += ',';
		call += LowerInlineTerm(operand, source, names, next, bindings);
	}
	call += ')';
	if (source)
		bindings[slot] = call + " := " + output;
	else
		bindings.push_back(output + " := " + call);
	return output;
}

std::string
LowerInlineOp(dsl::DSLRuleParser::OpContext *op, BOOL source,
			  std::unordered_set<std::string> &names, ULONG &next,
			  std::vector<std::string> &bindings)
{
	std::string result = op->ID()->getText();
	if (nullptr != op->STAR())
		result += '*';
	if (nullptr != op->symlist())
	{
		result += '<';
		for (auto *term : op->symlist()->term())
		{
			if (result.back() != '<')
				result += ' ';
			result += LowerInlineTerm(term, source, names, next, bindings);
		}
		result += '>';
	}
	if (!op->op().empty())
	{
		result += '(';
		for (auto *child : op->op())
		{
			if (result.back() != '(')
				result += ',';
			result += LowerInlineOp(child, source, names, next, bindings);
		}
		result += ')';
	}
	return result;
}
}  // namespace

CDSLRule *
CDSLRuleParser::PdslruleParse(CMemoryPool *mp, const CHAR *sz_dsl,
							  const CHAR *sz_verdict, CWStringDynamic *pstrErr)
{
	GPOS_ASSERT(nullptr != sz_dsl);

	SBuildCtx bctx;
	bctx.mp = mp;

	CDSLRule *pdslrule = nullptr;
	std::string fatal;

	try
	{
		antlr4::ANTLRInputStream input(sz_dsl);
		dsl::DSLRuleLexer lexer(&input);
		CCollectingErrorListener lex_errs;
		lexer.removeErrorListeners();
		lexer.addErrorListener(&lex_errs);

		antlr4::CommonTokenStream tokens(&lexer);
		dsl::DSLRuleParser parser(&tokens);
		CCollectingErrorListener parse_errs;
		parser.removeErrorListeners();
		parser.addErrorListener(&parse_errs);

		dsl::DSLRuleParser::Rule_Context *tree = parser.rule_();

		if (lex_errs.m_had_error || parse_errs.m_had_error)
		{
			fatal = "syntax error: " + lex_errs.m_errs +
					(parse_errs.m_errs.empty() ? "" : (" " + parse_errs.m_errs));
		}
		else
		{
			std::unordered_set<std::string> names;
			for (auto *token : tokens.getTokens())
				if (dsl::DSLRuleParser::SYMBOL == token->getType())
					names.insert(token->getText());
			ULONG next = 0;
			std::vector<std::string> bindings;
			const std::string source = LowerInlineOp(
				tree->frag(0)->op(), true, names, next, bindings);
			const size_t sourceBindings = bindings.size();
			const std::string target = LowerInlineOp(
				tree->frag(1)->op(), false, names, next, bindings);
			if (!bindings.empty())
			{
				std::string lowered = source + "|" + target + "|";
				for (size_t i = 0; i <= bindings.size(); i++)
				{
					if (i == sourceBindings && nullptr != tree->constraints())
					{
						if (lowered.back() != '|')
							lowered += ';';
						lowered += tree->constraints()->getText();
					}
					if (i == bindings.size())
						break;
					if (lowered.back() != '|')
						lowered += ';';
					lowered += bindings[i];
				}
				return PdslruleParse(mp, lowered.c_str(), sz_verdict, pstrErr);
			}
			// grammar guarantees exactly two fragments (source, target)
			std::vector<dsl::DSLRuleParser::FragContext *> frags = tree->frag();
			GPOS_ASSERT(2 == frags.size());

			CDSLFragment *pfrag_src =
				PfragBuild(bctx, frags[0], EdslsideSource);
			CDSLFragment *pfrag_tgt = nullptr;
			CDSLConstraintArray *pdrgpcon = nullptr;
			CDSLExpressionDefinitions *definitions = nullptr;
			const ULONG sourceSymbols =
				nullptr == pfrag_src ? 0 : pfrag_src->Pdrgpsym()->Size();

			if (nullptr != pfrag_src)
			{
				pfrag_tgt = PfragBuild(bctx, frags[1], EdslsideTarget);
			}
			if (nullptr != pfrag_tgt &&
				FDeclareBindings(bctx, tree->constraints(), pfrag_src,
								 pfrag_tgt))
			{
				pdrgpcon = PdrgpconBuild(bctx, tree->constraints(),
										 pfrag_tgt->Pdrgpsym());
			}

			if (nullptr != pdrgpcon)
			{
				definitions =
					GPOS_NEW(mp) CDSLExpressionDefinitions(mp, pdrgpcon);
				if (!FBuildBindings(bctx, tree->constraints(), pfrag_src,
									pfrag_tgt, sourceSymbols, pdrgpcon,
									definitions))
				{
					GPOS_DELETE(definitions);
					definitions = nullptr;
					pdrgpcon->Release();
					pdrgpcon = nullptr;
				}
			}
			if (nullptr != pdrgpcon)
			{
				pdslrule =
					GPOS_NEW(mp) CDSLRule(mp, pfrag_src, pfrag_tgt, pdrgpcon,
										  sz_verdict, definitions);
			}
			else
			{
				// unwind whatever succeeded
				if (nullptr != pfrag_tgt)
				{
					pfrag_tgt->Release();
				}
				if (nullptr != pfrag_src)
				{
					pfrag_src->Release();
				}
				fatal = bctx.err.empty() ? "semantic error" : bctx.err;
			}
		}
	}
	catch (const std::exception &ex)
	{
		fatal = std::string("parser exception: ") + ex.what();
	}
	catch (...)
	{
		fatal = "parser exception (unknown)";
	}

	if (nullptr == pdslrule && nullptr != pstrErr)
	{
		pstrErr->Reset();
		pstrErr->AppendCharArray(fatal.empty() ? "unknown parse error"
											   : fatal.c_str());
	}
	return pdslrule;
}
