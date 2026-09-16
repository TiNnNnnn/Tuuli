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

class CDSLPlanTemplate
{
public:
	CDSLPlanTemplate() = delete;

	static std::vector<const CExpression *> RelationalChildren(
		const CExpression *expr);
	static std::string ExpressionShape(const CExpression *expr);
	static std::string Serialize(CMemoryPool *mp, const CExpression *expr);
	static BOOL FValidateSelection(
		const CExpression *expr, const std::string &root_path,
		const std::vector<std::string> &cut_paths, std::string *error);
	static BOOL FSlice(
		CMemoryPool *mp, CExpression *expr, const std::string &root_path,
		const std::vector<std::string> &cut_paths, std::string *dsl,
		std::string *error);
	static std::string SliceArtifact(
		CMemoryPool *mp, CExpression *expr, const std::string &root_path,
		const std::vector<std::string> &cut_paths);
};
}  // namespace gpopt

#endif  // !GPOPT_CDSLPlanTemplate_H
