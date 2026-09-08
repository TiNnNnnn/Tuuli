//---------------------------------------------------------------------------
// Query-local cardinality experiment configuration.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLStatsExperiment_H
#define GPOPT_CDSLStatsExperiment_H

#include <string>
#include <unordered_map>
#include <vector>

#include "gpos/base.h"
#include "gpos/string/CWStringDynamic.h"

namespace gpopt
{
using namespace gpos;

class CExpression;
class COperator;

enum EDSLStatsBoundary
{
	EdslstatsboundaryScan,
	EdslstatsboundarySelect,
	EdslstatsboundaryJoin,
	EdslstatsboundaryExpression
};

struct SDSLStatsExperimentTarget
{
	std::string m_relations;
	std::string m_fingerprint;
	std::string m_operator;
	DOUBLE m_rows;
	EDSLStatsBoundary m_boundary;
	const COperator *m_pop;
	BOOL m_inject;
};

class CDSLStatsExperimentSnapshot
{
private:
	CMemoryPool *m_mp;
	std::string m_id;
	std::vector<SDSLStatsExperimentTarget> m_targets;
	std::unordered_map<const COperator *, ULONG> m_operator_targets;
	BOOL m_fDiscover;

	explicit CDSLStatsExperimentSnapshot(CMemoryPool *mp)
		: m_mp(mp), m_fDiscover(false)
	{
	}

public:
	CDSLStatsExperimentSnapshot(const CDSLStatsExperimentSnapshot &) = delete;

	static CDSLStatsExperimentSnapshot *PsnapshotLoadBuffer(
		CMemoryPool *mp, const CHAR *content, const CExpression *root,
		CWStringDynamic *errors);
	static CDSLStatsExperimentSnapshot *PsnapshotLoadFile(
		CMemoryPool *mp, const CHAR *path, const CExpression *root,
		CWStringDynamic *errors);
	static std::string Fingerprint(CMemoryPool *mp, const CExpression *expr);

	const SDSLStatsExperimentTarget *Ptarget(const COperator *pop) const;
	const SDSLStatsExperimentTarget *Ptarget(const CExpression *expr) const;
	const CHAR *SzId() const { return m_id.c_str(); }
	ULONG UlTargets() const { return (ULONG) m_targets.size(); }
};

}  // namespace gpopt

#endif  // !GPOPT_CDSLStatsExperiment_H
