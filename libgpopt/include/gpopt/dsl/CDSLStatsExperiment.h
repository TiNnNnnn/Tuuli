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
class CDSLRule;
class CDSLModel;

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

// An unbound request, not an observed or successfully injected statistic.
struct SDSLStatsExperimentRequest
{
	std::vector<std::string> m_aliases;
	std::string m_fingerprint;
	std::string m_operator;
	DOUBLE m_rows;
};

class CDSLStatsExperimentSnapshot
{
private:
	CMemoryPool *m_mp;
	std::string m_id;
	std::vector<SDSLStatsExperimentTarget> m_targets;
	std::unordered_map<const COperator *, ULONG> m_operator_targets;
	BOOL m_fDiscover;
	ULONG m_template_route = 0;
	std::string m_template_fingerprint;
	std::string m_template_root;
	std::vector<std::string> m_template_cuts;

	explicit CDSLStatsExperimentSnapshot(CMemoryPool *mp)
		: m_mp(mp), m_fDiscover(false)
	{
	}

public:
	CDSLStatsExperimentSnapshot(const CDSLStatsExperimentSnapshot &) = delete;

	// Same parser as runtime loading; no expression, metadata or statistics needed.
	// Failed parses leave the output arguments unchanged.
	static BOOL FParseRequests(const CHAR *content, std::string *id,
		std::vector<SDSLStatsExperimentRequest> *requests, BOOL *discover,
		CWStringDynamic *errors);

	static CDSLStatsExperimentSnapshot *PsnapshotLoadBuffer(
		CMemoryPool *mp, const CHAR *content, const CExpression *root,
		CWStringDynamic *errors);
	static CDSLStatsExperimentSnapshot *PsnapshotLoadFile(
		CMemoryPool *mp, const CHAR *path, const CExpression *root,
		CWStringDynamic *errors);
	static std::string Fingerprint(CMemoryPool *mp, const CExpression *expr);
	// Cached properties only: never derive statistics for instrumentation.
	static std::string InputContext(const CExpression *expr, CMemoryPool *mp = nullptr,
		BOOL query_input = false);
	// Lightweight lossless tree for every CBO routing occurrence.
	static std::string RouteContext(const CExpression *expr,
		const CDSLStatsExperimentSnapshot *snapshot = nullptr, ULONG sequence = 0);
	static std::string ExpressionShape(const CExpression *expr);
	static std::string BindingContext(const CDSLRule *rule, const CDSLModel *model);
	// Bounded log records transport the entire JSON value, including large trees.
	static std::vector<std::string> ContextRecords(ULONG id, const CHAR *field,
		const std::string &value);

	const SDSLStatsExperimentTarget *Ptarget(const COperator *pop) const;
	// Original request ordinal for this resolved operator, excluding discovery.
	BOOL FRequestIndex(const COperator *pop, ULONG *index) const;
	const SDSLStatsExperimentTarget *Ptarget(const CExpression *expr) const;
	const CHAR *SzId() const { return m_id.c_str(); }
	BOOL FHasTemplateSelection() const { return !m_template_root.empty(); }
	ULONG UlTemplateRoute() const { return m_template_route; }
	std::string TemplateSelectionArtifact(CExpression *root) const;
	ULONG UlTargets() const { return (ULONG) m_targets.size(); }
	const std::vector<SDSLStatsExperimentTarget> &Targets() const
	{
		return m_targets;
	}
};

}  // namespace gpopt

#endif  // !GPOPT_CDSLStatsExperiment_H
