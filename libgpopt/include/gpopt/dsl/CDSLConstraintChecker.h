//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLConstraintChecker.h
//
//	@doc:
//		Stage ② of the three-stage rewrite (WeTune Model.checkConstraints): given
//		a rule and the CDSLModel populated by the matcher, verify the rule's
//		STRUCTURAL constraints against the bound artifacts + live metadata. A rule
//		fires only if every constraint holds (WeTune early-aborts on the first
//		failure; we do the same).
//
//		Which constraints run here (doc §10 mapping table):
//		  AttrsSub(a,x)  : columns bound to <a> are a subset of a table/subtree's
//		                   output columns or of a bound schema's columns
//		  Unique(t,a)    : columns bound to <a> form a key of <t>'s subtree
//		                   -> DeriveKeyCollection()->FKey(pcrs)
//		  NotNull(t,a)   : columns bound to <a> are all non-nullable in <t>
//		                   -> DeriveNotNullColumns + ContainsAll
//		  Reference(t0,a0,t1,a1) : a0 references a1 via a foreign key
//		                   -> IMDRelation::ForeignKeyAt.  NOTE: in this ORCA build
//		                   ForeignKeyAt is only populated by CMDRelationGPDB from a
//		                   live relcache; the programmatic test fixture carries no
//		                   FK metadata, so Reference checking is implemented
//		                   defensively but exercised only against real relations
//		                   (test-migrated once base B / live PG is wired — doc §1).
//
//		Equality-class constraints also gate SOURCE matches. If both symbols are
//		bound, their artifacts must be semantically compatible (same relation MDId,
//		same underlying columns, or matching scalar trees). Target-side symbols are
//		unbound during matching and remain aliases consumed by the instantiator.
//
//		The bound <t> artifact is a relational CExpression subtree; the bound <a>
//		artifact is a CColRefArray. Metadata is reached via
//		COptCtxt::PoctxtFromTLS()->Pmda() when needed.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLConstraintChecker_H
#define GPOPT_CDSLConstraintChecker_H

#include "gpos/base.h"

#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CDSLConstraintChecker
//
//	@doc:
//		Verifies a rule's structural constraints against a populated model.
//		Stateless apart from the transient pool; construct per check.
//---------------------------------------------------------------------------
class CDSLConstraintChecker
{
private:
	CMemoryPool *m_mp;

	// dispatch one constraint; returns true if it holds (or is a no-op class
	// constraint handled elsewhere).
	BOOL FCheckOne(const CDSLRule *prule, const CDSLConstraint *pcon,
				   const CDSLModel *pmodel) const;

	// *Eq: check two source-side bindings when both are present; an unbound
	// target-side symbol is deferred to instantiation alias resolution.
	BOOL FCheckEquality(const CDSLRule *prule, const CDSLConstraint *pcon,
					const CDSLModel *pmodel) const;

	// AttrsSub(a,x): x is a table/subtree or schema symbol
	BOOL FCheckAttrsSub(const CDSLConstraint *pcon,
						const CDSLModel *pmodel) const;

	// Unique(t,a): cols(<a>) is a key of <t>'s subtree
	BOOL FCheckUnique(const CDSLConstraint *pcon,
					  const CDSLModel *pmodel) const;

	// NotNull(t,a): cols(<a>) all non-nullable in <t>'s subtree
	BOOL FCheckNotNull(const CDSLConstraint *pcon,
					   const CDSLModel *pmodel) const;

	// Reference(t0,a0,t1,a1): a0 -> a1 foreign key (see header note)
	BOOL FCheckReference(const CDSLConstraint *pcon,
						 const CDSLModel *pmodel) const;

	// build a CColRefSet from the CColRefArray bound to an attrs symbol; NULL if
	// the symbol is unbound. Caller owns the set (Release).
	CColRefSet *PcrsFromAttrsSym(const CDSLSymbol *psymAttrs,
								 const CDSLModel *pmodel) const;

public:
	CDSLConstraintChecker(const CDSLConstraintChecker &) = delete;

	explicit CDSLConstraintChecker(CMemoryPool *mp) : m_mp(mp)
	{
		GPOS_ASSERT(nullptr != mp);
	}

	// Check every constraint of prule against pmodel; true iff all hold. When
	// requested, report (without transferring ownership) the first failed
	// constraint and its zero-based position for verbose differential traces.
	BOOL FCheck(const CDSLRule *prule, const CDSLModel *pmodel,
				const CDSLConstraint **ppconFailed = nullptr,
				ULONG *pulFailed = nullptr) const;
};
}  // namespace gpopt

#endif	// !GPOPT_CDSLConstraintChecker_H
