//---------------------------------------------------------------------------
//	MONSOON DSL rule engine
//
//	@filename:
//		CDSLRuleLoader.cpp
//
//	@doc:
//		Rule-library loader. Splits a buffer into lines, parses each rule via
//		CDSLRuleParser, applies the EQ-only trust filter, and collects stats.
//		std::string / std::ifstream are confined to this file.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLRuleLoader.h"

#include <fstream>
#include <sstream>
#include <string>

#include "gpopt/dsl/CDSLRuleParser.h"

using namespace gpopt;

#define GPOPT_DSL_MAX_LOAD_ERRS 8

namespace
{
// Trim ASCII whitespace from both ends.
std::string
Trim(const std::string &s)
{
	size_t b = s.find_first_not_of(" \t\r\n");
	if (std::string::npos == b)
	{
		return "";
	}
	size_t e = s.find_last_not_of(" \t\r\n");
	return s.substr(b, e - b + 1);
}
}  // namespace

CDSLRuleArray *
CDSLRuleLoader::PdrgpdslruleLoadBuffer(CMemoryPool *mp, const CHAR *sz_content,
									   BOOL fEqOnly, SLoadStats *pstats,
									   CWStringDynamic *pstrErrs)
{
	GPOS_ASSERT(nullptr != sz_content);

	CDSLRuleArray *pdrgpdslrule = GPOS_NEW(mp) CDSLRuleArray(mp);
	SLoadStats stats;
	ULONG ul_reported = 0;

	std::istringstream iss(sz_content);
	std::string raw;
	ULONG ul_lineno = 0;
	while (std::getline(iss, raw))
	{
		ul_lineno++;
		std::string line = Trim(raw);
		if (line.empty() || '#' == line[0])
		{
			continue;
		}

		// optional TAB-separated trailing verdict field
		std::string dsl = line;
		std::string verdict;
		size_t tab = line.find('\t');
		if (std::string::npos != tab)
		{
			dsl = Trim(line.substr(0, tab));
			verdict = Trim(line.substr(tab + 1));
		}

		// EQ-only trust filter: a present-but-non-EQ verdict is skipped; an
		// absent verdict is treated as trusted (admitted).
		if (fEqOnly && !verdict.empty() && "EQ" != verdict)
		{
			stats.ul_skipped++;
			continue;
		}

		CWStringDynamic strErr(mp);
		CDSLRule *pdslrule = CDSLRuleParser::PdslruleParse(
			mp, dsl.c_str(), verdict.empty() ? nullptr : verdict.c_str(),
			&strErr);
		if (nullptr == pdslrule)
		{
			stats.ul_failed++;
			if (nullptr != pstrErrs && ul_reported < GPOPT_DSL_MAX_LOAD_ERRS)
			{
				ul_reported++;
				pstrErrs->AppendFormat(GPOS_WSZ_LIT("line %d: "), ul_lineno);
				pstrErrs->Append(&strErr);
				pstrErrs->AppendCharArray("\n");
			}
			continue;
		}
		pdrgpdslrule->Append(pdslrule);
		stats.ul_admitted++;
	}

	if (nullptr != pstats)
	{
		*pstats = stats;
	}
	return pdrgpdslrule;
}

CDSLRuleArray *
CDSLRuleLoader::PdrgpdslruleLoadFile(CMemoryPool *mp, const CHAR *sz_path,
									 BOOL fEqOnly, SLoadStats *pstats,
									 CWStringDynamic *pstrErrs)
{
	GPOS_ASSERT(nullptr != sz_path);

	std::ifstream ifs(sz_path);
	if (!ifs.is_open())
	{
		if (nullptr != pstrErrs)
		{
			pstrErrs->AppendCharArray("cannot open rule file: ");
			pstrErrs->AppendCharArray(sz_path);
		}
		return nullptr;
	}
	std::ostringstream oss;
	oss << ifs.rdbuf();
	std::string content = oss.str();

	return PdrgpdslruleLoadBuffer(mp, content.c_str(), fEqOnly, pstats,
								  pstrErrs);
}
