//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLRule.cpp
//
//	@doc:
//		Implementation of the template-IR classes and their canonical printer.
//		OsPrint reproduces WeTune's FragmentUtils.structuralToString /
//		Substitution.toString so that parse -> IR -> print -> parse is stable.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLRule.h"

using namespace gpopt;

// ---------------------------------------------------------------------------
// CDSLSymbol
// ---------------------------------------------------------------------------
CDSLSymbol::CDSLSymbol(CMemoryPool *mp, EDslSymbolKind esymk,
					   const CHAR *sz_name, ULONG id, EDslSide eside)
	: m_esymk(esymk),
	  m_pstrName(GPOS_NEW(mp) CWStringConst(mp, sz_name)),
	  m_id(id),
	  m_eside(eside)
{
	GPOS_ASSERT(EdslsymSentinel != esymk);
}

CDSLSymbol::~CDSLSymbol()
{
	GPOS_DELETE(m_pstrName);
}

// ---------------------------------------------------------------------------
// CDSLOp
// ---------------------------------------------------------------------------
CDSLOp::CDSLOp(CMemoryPool *,  // mp unused: children/syms arrays pre-built
			   EDslOpKind edslop, BOOL fDistinct, EDslSortDir edslsort,
			   CDSLSymbolArray *pdrgpsym, CDSLOpArray *pdrgpchild)
	: m_edslop(edslop),
	  m_eopid(CDSLOpKindTable::Eopid(edslop, fDistinct)),
	  m_fDistinct(fDistinct),
	  m_edslsort(edslsort),
	  m_pdrgpsym(pdrgpsym),
	  m_pdrgpchild(pdrgpchild)
{
	GPOS_ASSERT(nullptr != pdrgpsym);
	GPOS_ASSERT(nullptr != pdrgpchild);
	GPOS_ASSERT(CDSLOpKindTable::UlChildren(edslop) == pdrgpchild->Size());
	GPOS_ASSERT(CDSLOpKindTable::UlSyms(edslop) == pdrgpsym->Size());
}

CDSLOp::~CDSLOp()
{
	m_pdrgpsym->Release();
	m_pdrgpchild->Release();
}

void
CDSLOp::OsPrint(IOstream &os) const
{
	// name + optional '*' + optional Sort direction suffix
	if (EdslopSort == m_edslop && EdslsortAsc == m_edslsort)
	{
		os << "SortAsc";
	}
	else if (EdslopSort == m_edslop && EdslsortDesc == m_edslsort)
	{
		os << "SortDesc";
	}
	else
	{
		os << CDSLOpKindTable::SzName(m_edslop);
		if (m_fDistinct)
		{
			os << "*";
		}
	}

	// <sym sym ...>
	const ULONG ul_syms = m_pdrgpsym->Size();
	if (0 < ul_syms)
	{
		os << "<";
		for (ULONG ul = 0; ul < ul_syms; ul++)
		{
			if (0 < ul)
			{
				os << " ";
			}
			os << (*m_pdrgpsym)[ul]->PstrName()->GetBuffer();
		}
		os << ">";
	}

	// (child,child,...)
	const ULONG ul_children = m_pdrgpchild->Size();
	if (0 < ul_children)
	{
		os << "(";
		for (ULONG ul = 0; ul < ul_children; ul++)
		{
			if (0 < ul)
			{
				os << ",";
			}
			(*m_pdrgpchild)[ul]->OsPrint(os);
		}
		os << ")";
	}
}

// ---------------------------------------------------------------------------
// CDSLConstraint
// ---------------------------------------------------------------------------
CDSLConstraint::CDSLConstraint(CMemoryPool *,  // mp unused
							   EDslConstraintKind edslcon,
							   CDSLSymbolArray *pdrgpsym)
	: m_edslcon(edslcon), m_pdrgpsym(pdrgpsym)
{
	GPOS_ASSERT(nullptr != pdrgpsym);
	GPOS_ASSERT(CDSLConstraintKindTable::UlArity(edslcon) == pdrgpsym->Size());
}

CDSLConstraint::~CDSLConstraint()
{
	m_pdrgpsym->Release();
}

void
CDSLConstraint::OsPrint(IOstream &os) const
{
	os << CDSLConstraintKindTable::SzName(m_edslcon) << "(";
	const ULONG ul_syms = m_pdrgpsym->Size();
	for (ULONG ul = 0; ul < ul_syms; ul++)
	{
		if (0 < ul)
		{
			os << ",";
		}
		os << (*m_pdrgpsym)[ul]->PstrName()->GetBuffer();
	}
	os << ")";
}

// ---------------------------------------------------------------------------
// CDSLFragment
// ---------------------------------------------------------------------------
CDSLFragment::CDSLFragment(CMemoryPool *,  // mp unused
						   CDSLOp *pop_root, CDSLSymbolArray *pdrgpsym)
	: m_pop_root(pop_root), m_pdrgpsym(pdrgpsym)
{
	GPOS_ASSERT(nullptr != pop_root);
	GPOS_ASSERT(nullptr != pdrgpsym);
}

CDSLFragment::~CDSLFragment()
{
	m_pop_root->Release();
	m_pdrgpsym->Release();
}

// ---------------------------------------------------------------------------
// CDSLRule
// ---------------------------------------------------------------------------
CDSLRule::CDSLRule(CMemoryPool *mp, CDSLFragment *pfrag_src,
				   CDSLFragment *pfrag_tgt, CDSLConstraintArray *pdrgpcon,
				   const CHAR *sz_verdict)
	: m_pfrag_src(pfrag_src),
	  m_pfrag_tgt(pfrag_tgt),
	  m_pdrgpcon(pdrgpcon),
	  m_pstr_verdict(nullptr)
{
	GPOS_ASSERT(nullptr != pfrag_src);
	GPOS_ASSERT(nullptr != pfrag_tgt);
	GPOS_ASSERT(nullptr != pdrgpcon);
	if (nullptr != sz_verdict)
	{
		m_pstr_verdict = GPOS_NEW(mp) CWStringConst(mp, sz_verdict);
	}
}

CDSLRule::~CDSLRule()
{
	m_pfrag_src->Release();
	m_pfrag_tgt->Release();
	m_pdrgpcon->Release();
	if (nullptr != m_pstr_verdict)
	{
		GPOS_DELETE(m_pstr_verdict);
	}
}

void
CDSLRule::OsPrint(IOstream &os) const
{
	m_pfrag_src->OsPrint(os);
	os << "|";
	m_pfrag_tgt->OsPrint(os);
	os << "|";
	const ULONG ul_cons = m_pdrgpcon->Size();
	for (ULONG ul = 0; ul < ul_cons; ul++)
	{
		if (0 < ul)
		{
			os << ";";
		}
		(*m_pdrgpcon)[ul]->OsPrint(os);
	}
}
