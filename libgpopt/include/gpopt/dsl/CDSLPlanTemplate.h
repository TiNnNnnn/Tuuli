//---------------------------------------------------------------------------
// Lossless, addressable ORCA-plan view for DSL rule mining.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLPlanTemplate_H
#define GPOPT_CDSLPlanTemplate_H

#include <string>
#include <vector>

#include "gpos/base.h"

namespace gpopt
{
using namespace gpos;

class CExpression;
class CDSLRule;
class CDSLModel;

class CDSLPlanTemplate
{
public:
	CDSLPlanTemplate() = delete;

	static std::vector<const CExpression *> RelationalChildren(
		const CExpression *expr);
	static std::string ExpressionShape(const CExpression *expr);
	// Complete source trees only. Derives column metadata; Memo callers must
	// pass a detached copy. Exported column sets are not ordered row layouts
	// and do not certify source admissibility or rewrite equivalence.
	static std::string Serialize(CMemoryPool *mp, const CExpression *expr);
	// Observe an existing production model, never rematch a generated template.
	// Copies expressions before deriving properties; completeness is not proof.
	// source_paths use DSL child routes, kind/ordinal ports and match operand edges,
	// not native-plan paths or hidden symbol names.
	static std::string MatchedSourceBindings(
		const CDSLRule *rule, const CDSLModel *model);
	static BOOL FValidateSelection(
		const CExpression *expr, const std::string &root_path,
		const std::vector<std::string> &cut_paths, std::string *error);
	static BOOL FSlice(
		CMemoryPool *mp, CExpression *expr, const std::string &root_path,
		const std::vector<std::string> &cut_paths, std::string *dsl,
		std::string *error, std::string *source_bindings = nullptr);
	// Optional exact matcher bindings, not proof or source admissibility.
	static std::string SliceArtifact(
		CMemoryPool *mp, CExpression *expr, const std::string &root_path,
		const std::vector<std::string> &cut_paths, BOOL source_bindings = false);
};
}  // namespace gpopt

#endif  // !GPOPT_CDSLPlanTemplate_H
