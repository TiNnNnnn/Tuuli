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
#include "gpopt/dsl/CDSLExpressionDefinitions.h"

#include "gpos/io/COstreamString.h"
#include "gpos/string/CWStringDynamic.h"

using namespace gpopt;

namespace
{
void
ComputeRuleIdentity(const BYTE *pbytes, ULONG ulLength, CHAR szIdentity[17])
{
	// Byte-at-a-time FNV-1a is deliberately used here instead of the faster
	// native-word HashByteArray(): canonical identities must not depend on host
	// endianness and are computed only once when a rule is loaded.
	const ULLONG ullFnvOffset = 0xcbf29ce484222325ULL;
	const ULLONG ullFnvPrime = 0x100000001b3ULL;
	ULLONG ullHash = ullFnvOffset;
	for (ULONG ul = 0; ul < ulLength; ++ul)
	{
		ullHash ^= (ULLONG) pbytes[ul];
		ullHash *= ullFnvPrime;
	}

	static const CHAR szDigits[] = "0123456789abcdef";
	for (ULONG ul = 0; ul < 16; ++ul)
	{
		const ULONG ulShift = (15 - ul) * 4;
		szIdentity[ul] = szDigits[(ullHash >> ulShift) & 0x0f];
	}
	szIdentity[16] = '\0';
}
}  // namespace

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
			   EDslAggFuncKind edslaggfunc,
			   CDSLSymbolArray *pdrgpsym, CDSLOpArray *pdrgpchild)
	: m_edslop(edslop),
	  m_eopid(CDSLOpKindTable::Eopid(edslop, fDistinct)),
	  m_fDistinct(fDistinct),
	  m_edslsort(edslsort),
	  m_edslaggfunc(edslaggfunc),
	  m_pdrgpsym(pdrgpsym),
	  m_pdrgpchild(pdrgpchild)
{
	GPOS_ASSERT(nullptr != pdrgpsym);
	GPOS_ASSERT(nullptr != pdrgpchild);
	GPOS_ASSERT(CDSLOpKindTable::UlChildren(edslop) == pdrgpchild->Size());
	GPOS_ASSERT(CDSLOpKindTable::UlSyms(edslop) == pdrgpsym->Size() ||
				(EdslopAgg == edslop && 5 == pdrgpsym->Size()) ||
				((EdslopInnerJoin == edslop || EdslopLeftJoin == edslop ||
				  EdslopFullJoin == edslop) &&
				 (2 == pdrgpsym->Size() || 3 == pdrgpsym->Size() ||
				  4 == pdrgpsym->Size() ||
				  5 == pdrgpsym->Size())) ||
				(EdslopExists == edslop && 3 == pdrgpsym->Size()) ||
				(EdslopFilter == edslop && 2 == pdrgpsym->Size()) ||
				(EdslopInSubFilter == edslop && 1 == pdrgpsym->Size()) ||
				((EdslopUnion == edslop || EdslopIntersect == edslop ||
				  EdslopExcept == edslop) &&
				 (0 == pdrgpsym->Size() || 2 == pdrgpsym->Size())) ||
				(EdslopAntiJoinNotIn == edslop && 3 == pdrgpsym->Size()));
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
	else if (EdslopAgg == m_edslop &&
			 EdslaggfuncUnknown != m_edslaggfunc)
	{
		os << "Agg_" << CDSLOpKindTable::SzAggFuncName(m_edslaggfunc);
		if (m_fDistinct)
		{
			os << "*";
		}
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
	  m_pexprdefs(nullptr),
	  m_pstr_verdict(nullptr),
	  m_ul_source_line(0),
	  m_sz_identity{0}
{
	GPOS_ASSERT(nullptr != pfrag_src);
	GPOS_ASSERT(nullptr != pfrag_tgt);
	GPOS_ASSERT(nullptr != pdrgpcon);
	m_pexprdefs = GPOS_NEW(mp) CDSLExpressionDefinitions(mp, pdrgpcon);
	if (nullptr != sz_verdict)
	{
		m_pstr_verdict = GPOS_NEW(mp) CWStringConst(mp, sz_verdict);
	}

	// The grammar admits ASCII identifiers only, and the canonical printer emits
	// ASCII punctuation/operator names.  Hash those canonical bytes rather than
	// wchar_t storage, whose width and byte order differ between platforms.
	CWStringDynamic strCanonical(mp);
	COstreamString os(&strCanonical);
	OsPrint(os);
	const ULONG ulLength = strCanonical.Length();
	BYTE *pbytes = GPOS_NEW_ARRAY(mp, BYTE, ulLength);
	const WCHAR *wszCanonical = strCanonical.GetBuffer();
	for (ULONG ul = 0; ul < ulLength; ++ul)
	{
		GPOS_ASSERT(0 <= wszCanonical[ul] && 0x7f >= wszCanonical[ul]);
		pbytes[ul] = (BYTE) wszCanonical[ul];
	}
	ComputeRuleIdentity(pbytes, ulLength, m_sz_identity);
	GPOS_DELETE_ARRAY(pbytes);
}

CDSLRule::~CDSLRule()
{
	GPOS_DELETE(m_pexprdefs);
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
