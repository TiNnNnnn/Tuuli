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

#include "antlr4-runtime.h"

#include "DSLRuleLexer.h"
#include "DSLRuleParser.h"

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
				   dsl::DSLRuleParser::SymlistContext *symlist_ctx,
				   EDslSide eside, CDSLSymbolArray *pdrgpsym_frag)
{
	CMemoryPool *mp = bctx.mp;
	const ULONG ul_expected = CDSLOpKindTable::UlSyms(edslop);
	const ULONG ul_given =
		(nullptr == symlist_ctx) ? 0 : (ULONG) symlist_ctx->SYMBOL().size();

	// MONSOON's checked-in rule corpus predates aggregateOutputAttrs and uses
	// Agg<groupByAttrs aggregateAttrs aggFunc schema havingPred> (5 symbols).
	// Current SQLSolver adds aggregateOutputAttrs as the third symbol (6 total).
	// Accept both wire formats; the matcher/instantiator infer legacy aggregate
	// output columns from schema - groupByAttrs.
	const BOOL fLegacyAgg = EdslopAgg == edslop && 5 == ul_given;
	// Join always binds equality keys. It may additionally bind the complete
	// output (<a s>), the non-equality residual predicate and its dependencies
	// (<p a a>), or both. Keep every historical form wire-compatible.
	const BOOL fJoin =
		EdslopInnerJoin == edslop || EdslopLeftJoin == edslop;
	const BOOL fCompatibleJoin = fJoin &&
		(2 == ul_given || 4 == ul_given || 5 == ul_given || 7 == ul_given);
	// Existing WeTune corpora declare no Union symbols. The extended
	// Union<a s> form exposes the ordered full-row output so a later operator can
	// reference it (for example, full-row dedup above UnionAll).
	const BOOL fLegacyUnion = EdslopUnion == edslop && 0 == ul_given;
	if (ul_given != ul_expected && !fLegacyAgg && !fCompatibleJoin &&
		!fLegacyUnion)
	{
		std::ostringstream os;
		os << "operator " << CDSLOpKindTable::SzName(edslop) << " expects "
		   << (EdslopAgg == edslop
				   ? "5 or 6"
				   : (fJoin
						  ? "2, 4, 5, or 7"
						  : (EdslopUnion == edslop
								 ? "0 or 2"
								 : std::to_string(ul_expected))))
		   << " symbol(s) in <...>, got " << ul_given;
		bctx.Fail(os.str());
		return nullptr;
	}

	CDSLSymbolArray *pdrgpsym = GPOS_NEW(mp) CDSLSymbolArray(mp);
	for (ULONG ul = 0; ul < ul_given; ul++)
	{
		std::string name = symlist_ctx->SYMBOL(ul)->getText();
		if (bctx.symtab.find(name) != bctx.symtab.end())
		{
			// redeclaration (incl. cross-side reuse) — WeTune BiMap collision
			bctx.Fail("value already present: symbol '" + name +
					  "' declared more than once");
			pdrgpsym->Release();
			return nullptr;
		}
		EDslSymbolKind esymk;
		if (fLegacyAgg && 2 <= ul)
		{
			// Current schema is [a,a,a,f,s,p]; removing aggregateOutputAttrs
			// yields the legacy [a,a,f,s,p] layout.
			esymk = CDSLOpKindTable::EsymkindAt(edslop, ul + 1);
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
		bctx, edslop, op_ctx->symlist(), eside, pdrgpsym_frag);
	if (nullptr == pdrgpsym)
	{
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

// Build the constraint list. Constraints only REFERENCE declared symbols.
CDSLConstraintArray *
PdrgpconBuild(SBuildCtx &bctx,
			  dsl::DSLRuleParser::ConstraintsContext *cons_ctx)
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
		EDslConstraintKind edslcon = CDSLConstraintKindTable::Parse(cname.c_str());
		if (EdslconSentinel == edslcon)
		{
			bctx.Fail("unknown constraint: " + cname);
			pdrgpcon->Release();
			return nullptr;
		}
		std::vector<antlr4::tree::TerminalNode *> syms = con_ctx->SYMBOL();
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
		for (auto *sym_node : syms)
		{
			std::string name = sym_node->getText();
			auto it = bctx.symtab.find(name);
			if (it == bctx.symtab.end())
			{
				bctx.Fail("constraint references undeclared symbol '" + name +
						  "'");
				ok = false;
				break;
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
		pdrgpcon->Append(GPOS_NEW(mp) CDSLConstraint(mp, edslcon, pdrgpsym));
	}
	return pdrgpcon;
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
			// grammar guarantees exactly two fragments (source, target)
			std::vector<dsl::DSLRuleParser::FragContext *> frags = tree->frag();
			GPOS_ASSERT(2 == frags.size());

			CDSLFragment *pfrag_src =
				PfragBuild(bctx, frags[0], EdslsideSource);
			CDSLFragment *pfrag_tgt = nullptr;
			CDSLConstraintArray *pdrgpcon = nullptr;

			if (nullptr != pfrag_src)
			{
				pfrag_tgt = PfragBuild(bctx, frags[1], EdslsideTarget);
			}
			if (nullptr != pfrag_tgt)
			{
				pdrgpcon = PdrgpconBuild(bctx, tree->constraints());
			}

			if (nullptr != pdrgpcon)
			{
				pdslrule = GPOS_NEW(mp)
					CDSLRule(mp, pfrag_src, pfrag_tgt, pdrgpcon, sz_verdict);
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
