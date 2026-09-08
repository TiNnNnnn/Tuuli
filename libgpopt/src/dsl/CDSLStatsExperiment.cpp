//---------------------------------------------------------------------------
// Query-local cardinality experiment configuration.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLStatsExperiment.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include "gpopt/base/CUtils.h"
#include "gpos/io/COstreamString.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CLogicalDynamicGetBase.h"
#include "gpopt/operators/CLogicalGet.h"
#include "gpopt/operators/COperator.h"

using namespace gpopt;

namespace
{
struct SParsedTarget
{
	std::vector<std::string> m_aliases;
	std::string m_fingerprint;
	std::string m_operator;
	DOUBLE m_rows = 0.0;
	BOOL m_has_rows = false;
};

std::string
Trim(const std::string &value)
{
	const size_t begin = value.find_first_not_of(" \t\r\n");
	if (std::string::npos == begin)
	{
		return "";
	}
	const size_t end = value.find_last_not_of(" \t\r\n");
	return value.substr(begin, end - begin + 1);
}

std::string
Unquote(const std::string &value)
{
	if (2 <= value.size() &&
		(('"' == value.front() && '"' == value.back()) ||
		 ('\'' == value.front() && '\'' == value.back())))
	{
		return value.substr(1, value.size() - 2);
	}
	return value;
}

void
Error(CWStringDynamic *errors, ULONG line, const std::string &message)
{
	if (nullptr == errors)
	{
		return;
	}
	errors->AppendFormat(GPOS_WSZ_LIT("line %u: "), line);
	errors->AppendCharArray(message.c_str());
	errors->AppendCharArray("\n");
}

BOOL
KeyValue(const std::string &text, std::string *key, std::string *value)
{
	const size_t colon = text.find(':');
	if (std::string::npos == colon)
	{
		return false;
	}
	*key = Trim(text.substr(0, colon));
	*value = Trim(text.substr(colon + 1));
	return !key->empty();
}

BOOL
ParseRelations(const std::string &value, std::vector<std::string> *aliases)
{
	if (2 > value.size() || '[' != value.front() || ']' != value.back())
	{
		return false;
	}
	std::unordered_set<std::string> seen;
	std::stringstream input(value.substr(1, value.size() - 2));
	std::string item;
	while (std::getline(input, item, ','))
	{
		item = Unquote(Trim(item));
		if (item.empty() || !seen.insert(item).second)
		{
			return false;
		}
		aliases->push_back(item);
	}
	std::sort(aliases->begin(), aliases->end());
	return !aliases->empty();
}

BOOL
ParseFingerprint(std::string value, std::string *fingerprint)
{
	value = Unquote(value);
	if (16 != value.size())
	{
		return false;
	}
	for (CHAR &ch : value)
	{
		if (!std::isxdigit((unsigned char) ch))
		{
			return false;
		}
		ch = (CHAR) std::tolower((unsigned char) ch);
	}
	*fingerprint = value;
	return true;
}

std::string
RelationKey(const std::vector<std::string> &aliases)
{
	std::string key;
	for (const std::string &alias : aliases)
	{
		if (!key.empty())
		{
			key.push_back(',');
		}
		key.append(alias);
	}
	return key;
}

BOOL
ParseRows(const std::string &value, DOUBLE *rows)
{
	errno = 0;
	CHAR *end = nullptr;
	const DOUBLE parsed = std::strtod(value.c_str(), &end);
	if (ERANGE == errno || end == value.c_str() || '\0' != *end ||
		!std::isfinite(parsed) || GPOS_FP_ABS_MIN > parsed ||
		GPOS_FP_ABS_MAX < parsed)
	{
		return false;
	}
	*rows = parsed;
	return true;
}

BOOL
Parse(const CHAR *content, std::string *id,
	  std::vector<SParsedTarget> *targets, BOOL *discover,
	  CWStringDynamic *errors)
{
	std::istringstream input(nullptr == content ? "" : content);
	std::string line;
	SParsedTarget current;
	BOOL in_cardinalities = false;
	BOOL has_current = false;
	BOOL valid = true;
	BOOL has_discover = false;
	ULONG line_no = 0;
	std::unordered_set<std::string> keys;

	auto finish = [&]() {
		if (!has_current)
		{
			return;
		}
		const BOOL relation_selector = !current.m_aliases.empty();
		const BOOL expression_selector = !current.m_fingerprint.empty();
		if (relation_selector == expression_selector || !current.m_has_rows ||
			(expression_selector && current.m_operator.empty()))
		{
			Error(errors, line_no,
				  "each cardinality needs either relations, or expression and operator, plus rows");
			valid = false;
		}
		const std::string selector = relation_selector
			? "relations:" + RelationKey(current.m_aliases)
			: "expression:" + current.m_operator + ":" + current.m_fingerprint;
		if (valid && !keys.insert(selector).second)
		{
			Error(errors, line_no, "duplicate cardinality selector");
			valid = false;
		}
		else if (valid)
		{
			targets->push_back(current);
		}
		current = SParsedTarget();
		has_current = false;
	};

	while (std::getline(input, line))
	{
		++line_no;
		const std::string text = Trim(line);
		if (text.empty() || '#' == text[0])
		{
			continue;
		}
		std::string key;
		std::string value;
		if (0 == text.rfind("- ", 0))
		{
			if (!in_cardinalities)
			{
				Error(errors, line_no, "cardinality entry outside cardinalities");
				valid = false;
				continue;
			}
			finish();
			has_current = true;
			if (!KeyValue(text.substr(2), &key, &value) ||
				!(("relations" == key &&
				   ParseRelations(value, &current.m_aliases)) ||
				  ("expression" == key &&
				   ParseFingerprint(value, &current.m_fingerprint))))
			{
				Error(errors, line_no,
					  "entry must start with relations: [alias, ...] or expression: <16-hex fingerprint>");
				valid = false;
			}
			continue;
		}
		if (!KeyValue(text, &key, &value))
		{
			Error(errors, line_no, "expected key: value");
			valid = false;
			continue;
		}
		if (!in_cardinalities && "experiment" == key && id->empty())
		{
			*id = Unquote(value);
			if (id->empty())
			{
				Error(errors, line_no, "experiment id cannot be empty");
				valid = false;
			}
		}
		else if (!in_cardinalities && "discover" == key && !has_discover)
		{
			has_discover = true;
			if ("true" == value)
			{
				*discover = true;
			}
			else if ("false" != value)
			{
				Error(errors, line_no, "discover must be true or false");
				valid = false;
			}
		}
		else if (!in_cardinalities && "cardinalities" == key && value.empty())
		{
			in_cardinalities = true;
		}
		else if (in_cardinalities && has_current && "rows" == key &&
				 !current.m_has_rows)
		{
			current.m_has_rows = ParseRows(value, &current.m_rows);
			if (!current.m_has_rows)
			{
				Error(errors, line_no, "rows must be a positive finite number");
				valid = false;
			}
		}
		else if (in_cardinalities && has_current && "operator" == key &&
				 current.m_operator.empty())
		{
			current.m_operator = Unquote(value);
			if (current.m_operator.empty())
			{
				Error(errors, line_no, "operator cannot be empty");
				valid = false;
			}
		}
		else
		{
			Error(errors, line_no, "unknown or misplaced field: " + key);
			valid = false;
		}
	}
	finish();
	if (id->empty())
	{
		Error(errors, 0, "missing experiment id");
		valid = false;
	}
	if (!in_cardinalities)
	{
		Error(errors, 0, "missing cardinalities section");
		valid = false;
	}
	return valid;
}

std::string
Alias(const CName &name, CMemoryPool *mp)
{
	CHAR *value = CUtils::CreateMultiByteCharStringFromWCString(
		mp, const_cast<WCHAR *>(name.Pstr()->GetBuffer()));
	std::string result(value);
	GPOS_DELETE_ARRAY(value);
	return result;
}

BOOL
IsScan(const COperator *pop)
{
	return nullptr != dynamic_cast<const CLogicalGet *>(pop) ||
		nullptr != dynamic_cast<const CLogicalDynamicGetBase *>(pop);
}

BOOL
IsSelect(const COperator *pop)
{
	return COperator::EopLogicalSelect == pop->Eopid();
}

BOOL
IsJoin(const COperator *pop)
{
	return COperator::EopLogicalInnerJoin == pop->Eopid();
}

BOOL
IsProject(const COperator *pop)
{
	return COperator::EopLogicalProject == pop->Eopid();
}

void
HashBytes(ULLONG *hash, const CHAR *bytes, size_t length)
{
	for (size_t index = 0; index < length; ++index)
	{
		*hash ^= (BYTE) bytes[index];
		*hash *= 0x100000001b3ULL;
	}
}

std::string
FormatFingerprint(ULLONG hash)
{
	static const CHAR digits[] = "0123456789abcdef";
	CHAR value[17];
	for (ULONG index = 0; index < 16; ++index)
	{
		value[index] = digits[(hash >> ((15 - index) * 4)) & 0x0f];
	}
	value[16] = '\0';
	return value;
}

std::string
Fingerprint(CMemoryPool *mp, const CExpression *expr,
			std::unordered_map<const CExpression *, std::string> *cache)
{
	const auto found = cache->find(expr);
	if (cache->end() != found)
	{
		return found->second;
	}
	ULLONG hash = 0xcbf29ce484222325ULL;
	CWStringDynamic printed(mp);
	COstreamString os(&printed);
	expr->Pop()->OsPrint(os);
	CHAR *operator_text = CUtils::CreateMultiByteCharStringFromWCString(
		mp, const_cast<WCHAR *>(printed.GetBuffer()));
	HashBytes(&hash, operator_text, std::strlen(operator_text));
	GPOS_DELETE_ARRAY(operator_text);
	const CHAR separator = ':';
	for (ULONG child = 0; child < expr->Arity(); ++child)
	{
		HashBytes(&hash, &separator, 1);
		const std::string child_fingerprint =
			Fingerprint(mp, (*expr)[child], cache);
		HashBytes(&hash, child_fingerprint.c_str(), child_fingerprint.size());
	}
	const std::string result = FormatFingerprint(hash);
	cache->emplace(expr, result);
	return result;
}

std::vector<std::string>
Collect(const CExpression *expr, CMemoryPool *mp, BOOL *spj)
{
	const COperator *pop = expr->Pop();
	if (IsScan(pop))
	{
		*spj = true;
		if (const CLogicalGet *get = dynamic_cast<const CLogicalGet *>(pop))
		{
			return {Alias(get->Name(), mp)};
		}
		return {Alias(dynamic_cast<const CLogicalDynamicGetBase *>(pop)->Name(),
					  mp)};
	}

	std::vector<std::string> result;
	ULONG relational_children = 0;
	for (ULONG child = 0; child < expr->Arity(); ++child)
	{
		const CExpression *child_expr = (*expr)[child];
		if (!child_expr->Pop()->FLogical())
		{
			continue;
		}
		++relational_children;
		BOOL child_spj = false;
		std::vector<std::string> child_aliases = Collect(child_expr, mp, &child_spj);
		if (!child_spj)
		{
			*spj = false;
			return {};
		}
		result.insert(result.end(), child_aliases.begin(), child_aliases.end());
	}
	*spj = (IsSelect(pop) || IsProject(pop)) ? 1 == relational_children
										 : IsJoin(pop) && 2 == relational_children;
	if (!*spj)
	{
		return {};
	}
	std::sort(result.begin(), result.end());
	return result;
}

void
Resolve(const CExpression *expr, CMemoryPool *mp,
		std::unordered_map<std::string, ULONG> *target_by_key,
		std::unordered_map<std::string, ULONG> *target_by_expression,
		std::vector<SDSLStatsExperimentTarget> *targets,
		BOOL discover, CWStringDynamic *errors, BOOL *valid,
		std::unordered_map<const CExpression *, std::string> *fingerprints)
{
	for (ULONG child = 0; child < expr->Arity(); ++child)
	{
		if ((*expr)[child]->Pop()->FLogical())
		{
			Resolve((*expr)[child], mp, target_by_key, target_by_expression,
					targets, discover, errors, valid, fingerprints);
		}
	}

	BOOL spj = false;
	std::vector<std::string> aliases = Collect(expr, mp, &spj);
	const COperator *pop = expr->Pop();
	if (spj && !IsProject(pop))
	{
		const std::string key = RelationKey(aliases);
		auto found = target_by_key->find(key);
		if (target_by_key->end() == found && discover)
		{
			const ULONG index = (ULONG) targets->size();
			target_by_key->emplace(key, index);
			targets->push_back({key, "", "", 0.0,
								EdslstatsboundaryScan, nullptr, false});
			found = target_by_key->find(key);
		}
		if (target_by_key->end() != found)
		{
			SDSLStatsExperimentTarget &target = (*targets)[found->second];
			target.m_pop = pop;
			target.m_fingerprint = Fingerprint(mp, expr, fingerprints);
			target.m_operator = pop->SzId();
			target.m_boundary = IsScan(pop)
				? EdslstatsboundaryScan
				: (IsSelect(pop) ? EdslstatsboundarySelect
								 : EdslstatsboundaryJoin);
		}
	}

	const std::string fingerprint = Fingerprint(mp, expr, fingerprints);
	const std::string expression_key =
		std::string(pop->SzId()) + ":" + fingerprint;
	const auto expression = target_by_expression->find(expression_key);
	if (target_by_expression->end() != expression)
	{
		SDSLStatsExperimentTarget &target = (*targets)[expression->second];
		if (nullptr != target.m_pop)
		{
			Error(errors, 0, "expression selector is ambiguous: " + expression_key);
			*valid = false;
		}
		else
		{
			target.m_pop = pop;
			target.m_boundary = EdslstatsboundaryExpression;
		}
	}
}

void
DiscoverExpressions(
	const CExpression *expr, CMemoryPool *mp,
	std::vector<SDSLStatsExperimentTarget> *targets,
	std::unordered_set<const COperator *> *claimed,
	std::unordered_map<const CExpression *, std::string> *fingerprints)
{
	for (ULONG child = 0; child < expr->Arity(); ++child)
	{
		if ((*expr)[child]->Pop()->FLogical())
		{
			DiscoverExpressions((*expr)[child], mp, targets, claimed,
							fingerprints);
		}
	}
	const COperator *pop = expr->Pop();
	if (!claimed->insert(pop).second)
	{
		return;
	}
	targets->push_back(
		{"", Fingerprint(mp, expr, fingerprints), pop->SzId(), 0.0,
		 EdslstatsboundaryExpression, pop, false});
}

EDSLStatsBoundary
Boundary(const COperator *pop)
{
	return IsScan(pop) ? EdslstatsboundaryScan
		: (IsSelect(pop) ? EdslstatsboundarySelect : EdslstatsboundaryJoin);
}
}  // namespace

CDSLStatsExperimentSnapshot *
CDSLStatsExperimentSnapshot::PsnapshotLoadBuffer(CMemoryPool *mp,
											  const CHAR *content,
											  const CExpression *root,
											  CWStringDynamic *errors)
{
	std::string id;
	std::vector<SParsedTarget> parsed;
	BOOL discover = false;
	if (nullptr == root || !Parse(content, &id, &parsed, &discover, errors))
	{
		return nullptr;
	}

	CDSLStatsExperimentSnapshot *snapshot =
		GPOS_NEW(mp) CDSLStatsExperimentSnapshot(mp);
	snapshot->m_id = id;
	snapshot->m_fDiscover = discover;
	std::unordered_map<std::string, ULONG> target_by_key;
	std::unordered_map<std::string, ULONG> target_by_expression;
	for (const SParsedTarget &entry : parsed)
	{
		const ULONG index = (ULONG) snapshot->m_targets.size();
		if (!entry.m_aliases.empty())
		{
			const std::string key = RelationKey(entry.m_aliases);
			target_by_key.emplace(key, index);
			snapshot->m_targets.push_back(
				{key, "", "", entry.m_rows, EdslstatsboundaryScan,
				 nullptr, true});
		}
		else
		{
			const std::string key =
				entry.m_operator + ":" + entry.m_fingerprint;
			target_by_expression.emplace(key, index);
			snapshot->m_targets.push_back(
				{"", entry.m_fingerprint, entry.m_operator, entry.m_rows,
				 EdslstatsboundaryExpression, nullptr, true});
		}
	}
	BOOL valid = true;
	std::unordered_map<const CExpression *, std::string> fingerprints;
	Resolve(root, mp, &target_by_key, &target_by_expression,
			&snapshot->m_targets, discover, errors, &valid, &fingerprints);
	if (!valid)
	{
		GPOS_DELETE(snapshot);
		return nullptr;
	}
	if (discover)
	{
		std::unordered_set<const COperator *> claimed;
		for (const SDSLStatsExperimentTarget &target : snapshot->m_targets)
		{
			if (nullptr != target.m_pop)
			{
				claimed.insert(target.m_pop);
			}
		}
		DiscoverExpressions(root, mp, &snapshot->m_targets, &claimed,
							&fingerprints);
	}
	for (ULONG index = 0; index < snapshot->m_targets.size(); ++index)
	{
		SDSLStatsExperimentTarget &target = snapshot->m_targets[index];
		if (nullptr == target.m_pop)
		{
			Error(errors, 0, target.m_relations.empty()
				? "expression not found: " + target.m_operator + ":" +
					  target.m_fingerprint
				: "relation set not found in an SPJ region: " +
					  target.m_relations);
			GPOS_DELETE(snapshot);
			return nullptr;
		}
		if (!snapshot->m_operator_targets.emplace(target.m_pop, index).second)
		{
			Error(errors, 0, "multiple selectors resolve to the same expression");
			GPOS_DELETE(snapshot);
			return nullptr;
		}
	}
	return snapshot;
}

CDSLStatsExperimentSnapshot *
CDSLStatsExperimentSnapshot::PsnapshotLoadFile(CMemoryPool *mp,
											const CHAR *path,
											const CExpression *root,
											CWStringDynamic *errors)
{
	std::ifstream input(path);
	if (!input)
	{
		if (nullptr != errors)
		{
			errors->AppendCharArray("cannot open DSL stats experiment file: ");
			errors->AppendCharArray(path);
			errors->AppendCharArray("\n");
		}
		return nullptr;
	}
	std::ostringstream content;
	content << input.rdbuf();
	return PsnapshotLoadBuffer(mp, content.str().c_str(), root, errors);
}

const SDSLStatsExperimentTarget *
CDSLStatsExperimentSnapshot::Ptarget(const COperator *pop) const
{
	const auto found = m_operator_targets.find(pop);
	return m_operator_targets.end() == found ? nullptr : &m_targets[found->second];
}

std::string
CDSLStatsExperimentSnapshot::Fingerprint(CMemoryPool *mp,
									 const CExpression *expr)
{
	std::unordered_map<const CExpression *, std::string> cache;
	return ::Fingerprint(mp, expr, &cache);
}

const SDSLStatsExperimentTarget *
CDSLStatsExperimentSnapshot::Ptarget(const CExpression *expr) const
{
	if (nullptr == expr)
	{
		return nullptr;
	}
	if (const SDSLStatsExperimentTarget *target = Ptarget(expr->Pop()))
	{
		return target;
	}
	// Standalone join candidates (notably DPHyper candidates costed before Memo
	// insertion) have fresh operator objects. Scan and Select targets deliberately
	// require the resolved operator so nested filters with the same relset are not
	// injected twice.
	if (!IsJoin(expr->Pop()))
	{
		return nullptr;
	}
	BOOL spj = false;
	std::vector<std::string> aliases = Collect(expr, m_mp, &spj);
	if (!spj)
	{
		return nullptr;
	}
	const std::string key = RelationKey(aliases);
	for (const SDSLStatsExperimentTarget &target : m_targets)
	{
		if (target.m_inject && target.m_relations == key &&
			target.m_boundary == Boundary(expr->Pop()))
		{
			return &target;
		}
	}
	return nullptr;
}
