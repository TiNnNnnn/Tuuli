//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLRule.h
//
//	@doc:
//		In-memory representation of a parsed WeTune rewrite rule — the "template
//		IR". Mirrors WeTune's Substitution / Fragment / Op / Symbol / Constraint
//		object model, but expresses each operator in ORCA's logical-operator
//		vocabulary (COperator::EOperatorId, stored on CDSLOp).
//
//		IMPORTANT — this is a LIGHTWEIGHT template, NOT a live CExpression tree:
//		  * Input<t0> is an opaque table placeholder; ORCA's CLogicalGet demands
//		    a real CTableDescriptor, so we cannot represent it as CLogicalGet.
//		  * placeholder CColRefs would bind to a per-optimization CColumnFactory
//		    (COptCtxt is thread-local, one per optimization); a rule file is
//		    loaded once and reused across many optimizations.
//		Therefore the IR stores operator IDENTITY + DSL SYMBOLS only. Real
//		CColRef / CExpression construction is deferred to the xform Transform()
//		(phase 2), inside a live optimizer context.
//
//		All objects are pool-allocated (GPOS_NEW) and reference-counted.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLRule_H
#define GPOPT_CDSLRule_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "gpos/common/CRefCount.h"
#include "gpos/io/IOstream.h"
#include "gpos/string/CWStringConst.h"

#include "gpopt/dsl/CDSLEnums.h"

namespace gpopt
{
using namespace gpos;

// fwd decl
class CDSLSymbol;
class CDSLOp;
class CDSLConstraint;
class CDSLFragment;
class CDSLRule;

using CDSLSymbolArray = CDynamicPtrArray<CDSLSymbol, CleanupRelease>;
using CDSLOpArray = CDynamicPtrArray<CDSLOp, CleanupRelease>;
using CDSLConstraintArray = CDynamicPtrArray<CDSLConstraint, CleanupRelease>;

//---------------------------------------------------------------------------
//	@class:
//		CDSLSymbol
//
//	@doc:
//		An opaque, identity-based placeholder (WeTune Symbol). Carries its kind
//		(decided positionally by the declaring operator), its printed name
//		(e.g. "t0"), a stable per-rule id, and the side it was first declared
//		on. Identity is by object pointer, exactly as WeTune compares symbols by
//		reference — two occurrences of "t0" on the SAME side are the same
//		CDSLSymbol object (the builder interns by name).
//---------------------------------------------------------------------------
class CDSLSymbol : public CRefCount
{
private:
	const EDslSymbolKind m_esymk;
	CWStringConst *m_pstrName;	 // owned
	const ULONG m_id;
	const EDslSide m_eside;

public:
	CDSLSymbol(const CDSLSymbol &) = delete;

	CDSLSymbol(CMemoryPool *mp, EDslSymbolKind esymk, const CHAR *sz_name,
			   ULONG id, EDslSide eside);

	~CDSLSymbol() override;

	EDslSymbolKind Esymkind() const { return m_esymk; }
	const CWStringConst *PstrName() const { return m_pstrName; }
	ULONG Id() const { return m_id; }
	EDslSide Eside() const { return m_eside; }
};

//---------------------------------------------------------------------------
//	@class:
//		CDSLOp
//
//	@doc:
//		A node in a fragment tree (WeTune Op). Holds the DSL operator kind, the
//		mapped ORCA logical operator id, distinct / sort-direction modifiers,
//		its positional symbols (already validated for count+kind), and its
//		relational children.
//---------------------------------------------------------------------------
class CDSLOp : public CRefCount
{
private:
	const EDslOpKind m_edslop;
	const COperator::EOperatorId m_eopid;	 // mapped ORCA logical op (or Sentinel)
	const BOOL m_fDistinct;					 // Proj* / Union*
	const EDslSortDir m_edslsort;			 // Sort direction
	CDSLSymbolArray *m_pdrgpsym;			 // owned; positional symbols
	CDSLOpArray *m_pdrgpchild;				 // owned; relational children

public:
	CDSLOp(const CDSLOp &) = delete;

	CDSLOp(CMemoryPool *mp, EDslOpKind edslop, BOOL fDistinct,
		   EDslSortDir edslsort, CDSLSymbolArray *pdrgpsym,
		   CDSLOpArray *pdrgpchild);

	~CDSLOp() override;

	EDslOpKind Edslop() const { return m_edslop; }
	COperator::EOperatorId Eopid() const { return m_eopid; }
	BOOL FDistinct() const { return m_fDistinct; }
	EDslSortDir Edslsort() const { return m_edslsort; }
	CDSLSymbolArray *Pdrgpsym() const { return m_pdrgpsym; }
	CDSLOpArray *Pdrgpchild() const { return m_pdrgpchild; }

	ULONG UlChildren() const { return m_pdrgpchild->Size(); }
	CDSLOp *operator[](ULONG ul) const { return (*m_pdrgpchild)[ul]; }

	// append the canonical DSL text of this subtree to os (round-trip)
	void OsPrint(IOstream &os) const;
};

//---------------------------------------------------------------------------
//	@class:
//		CDSLConstraint
//
//	@doc:
//		A constraint (WeTune Constraint): a kind plus its symbol arguments,
//		already validated for arity.
//---------------------------------------------------------------------------
class CDSLConstraint : public CRefCount
{
private:
	const EDslConstraintKind m_edslcon;
	CDSLSymbolArray *m_pdrgpsym;   // owned; arity already checked

public:
	CDSLConstraint(const CDSLConstraint &) = delete;

	CDSLConstraint(CMemoryPool *mp, EDslConstraintKind edslcon,
				   CDSLSymbolArray *pdrgpsym);

	~CDSLConstraint() override;

	EDslConstraintKind Edslcon() const { return m_edslcon; }
	CDSLSymbolArray *Pdrgpsym() const { return m_pdrgpsym; }

	void OsPrint(IOstream &os) const;
};

//---------------------------------------------------------------------------
//	@class:
//		CDSLFragment
//
//	@doc:
//		One side of a rule (WeTune Fragment): a root operator plus the flat list
//		of symbols declared within it (in declaration order, for round-trip and
//		for the engine to enumerate).
//---------------------------------------------------------------------------
class CDSLFragment : public CRefCount
{
private:
	CDSLOp *m_pop_root;			   // owned
	CDSLSymbolArray *m_pdrgpsym;   // owned; symbols declared in this fragment

public:
	CDSLFragment(const CDSLFragment &) = delete;

	CDSLFragment(CMemoryPool *mp, CDSLOp *pop_root, CDSLSymbolArray *pdrgpsym);

	~CDSLFragment() override;

	CDSLOp *PopRoot() const { return m_pop_root; }
	CDSLSymbolArray *Pdrgpsym() const { return m_pdrgpsym; }

	void OsPrint(IOstream &os) const { m_pop_root->OsPrint(os); }
};

//---------------------------------------------------------------------------
//	@class:
//		CDSLRule
//
//	@doc:
//		A complete rewrite rule (WeTune Substitution): source + target fragments
//		sharing one symbol namespace, plus constraints. Optionally carries proof
//		metadata (verdict/backend/proof-time) so the loader can admit EQ-only
//		rules. Immutable after construction.
//---------------------------------------------------------------------------
class CDSLRule : public CRefCount
{
private:
	CDSLFragment *m_pfrag_src;			 // owned
	CDSLFragment *m_pfrag_tgt;			 // owned
	CDSLConstraintArray *m_pdrgpcon;	 // owned
	CWStringConst *m_pstr_verdict;		 // owned; may be NULL (e.g. "EQ")

public:
	CDSLRule(const CDSLRule &) = delete;

	CDSLRule(CMemoryPool *mp, CDSLFragment *pfrag_src, CDSLFragment *pfrag_tgt,
			 CDSLConstraintArray *pdrgpcon, const CHAR *sz_verdict);

	~CDSLRule() override;

	CDSLFragment *PfragSrc() const { return m_pfrag_src; }
	CDSLFragment *PfragTgt() const { return m_pfrag_tgt; }
	CDSLConstraintArray *Pdrgpcon() const { return m_pdrgpcon; }
	const CWStringConst *PstrVerdict() const { return m_pstr_verdict; }

	// ORCA logical op at the source root — the bucket key the engine dispatches
	// on (which shell xform owns this rule). EopSentinel for Input/subquery/etc.
	COperator::EOperatorId EopidSrcRoot() const
	{
		return m_pfrag_src->PopRoot()->Eopid();
	}

	// canonical "<source>|<target>|<constraints>" (round-trip)
	void OsPrint(IOstream &os) const;
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLRule_H
