//---------------------------------------------------------------------------
// Query-local provenance emitted by DSL target instantiation.
//---------------------------------------------------------------------------
#ifndef GPOPT_CDSLProvenance_H
#define GPOPT_CDSLProvenance_H

#include <string>
#include <vector>

namespace gpopt
{
struct SDSLTargetInputOrigin
{
	std::string m_template_path;
	std::string m_expression_path;
};

using CDSLTargetInputOriginArray = std::vector<SDSLTargetInputOrigin>;
}  // namespace gpopt

#endif  // !GPOPT_CDSLProvenance_H
