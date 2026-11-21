#ifndef GPOPT_CDSLPendingCTEs_H
#define GPOPT_CDSLPendingCTEs_H

#include <vector>

#include "gpos/base.h"

namespace gpopt
{
// Only registrations created by this candidate may be discarded.
struct CDSLPendingCTEs
{
	std::vector<gpos::ULONG> ids;

	CDSLPendingCTEs() = default;
	CDSLPendingCTEs(const CDSLPendingCTEs &) = delete;
	CDSLPendingCTEs &operator=(const CDSLPendingCTEs &) = delete;
	~CDSLPendingCTEs();
};
}  // namespace gpopt

#endif
