//---------------------------------------------------------------------------
// MONSOON DSL corpus capability auditor.
//
// Usage: pgorca_rule_audit <rules-directory> [output-directory]
//---------------------------------------------------------------------------
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "gpos/_api.h"
#include "gpos/common/CMainArgs.h"
#include "gpos/io/COstreamString.h"
#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/string/CWStringDynamic.h"

#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/init.h"
#include "gpopt/search/CJobJoinEnumeration.h"
#include "gpopt/xforms/CXformFactory.h"
#include "naucrates/init.h"

using namespace gpos;
using namespace gpopt;
namespace fs = std::filesystem;

namespace
{
struct SFeatureStats
{
	unsigned long source_occurrences = 0;
	unsigned long target_occurrences = 0;
	BOOL matcher_supported = false;
	BOOL instantiator_supported = false;
};

struct SRuleRecord
{
	unsigned long id = 0;
	std::string file;
	unsigned long line = 0;
	std::string dsl;
	std::string verdict;
	std::string status;
	std::string source_root;
	std::string target_root;
	std::string parse_error;
	std::set<std::string> reasons;
	std::set<std::string> source_ops;
	std::set<std::string> target_ops;
	std::set<std::string> constraints;
	std::vector<std::string> native_xforms;
};

struct SXformRecord
{
	unsigned long id = 0;
	std::string name;
	std::string category;
	std::string replacement_owner;
	std::string source_operator;
	std::vector<unsigned long> source_root_candidate_rule_ids;
};

BOOL
FImplementationPropertyExploration(CXform::EXformId id)
{
	switch (id)
	{
		// Logical access-path generation retained beside physical implementation.
		case CXform::ExfSelect2IndexGet:
		case CXform::ExfSelect2DynamicIndexGet:
		case CXform::ExfSelect2BitmapBoolOp:
		case CXform::ExfSelect2DynamicBitmapBoolOp:
		case CXform::ExfJoin2BitmapIndexGetApply:
		case CXform::ExfJoin2IndexGetApply:
		case CXform::ExfExpandDynamicGetWithForeignPartitions:
		case CXform::ExfLimit2IndexGet:
		case CXform::ExfMinMax2IndexGet:
		case CXform::ExfMinMax2IndexOnlyGet:
		case CXform::ExfSelect2IndexOnlyGet:
		case CXform::ExfSelect2DynamicIndexOnlyGet:
		case CXform::ExfLimit2IndexOnlyGet:
		case CXform::ExfSemiJoin2IndexGetApply:
		// Mandatory DML/distributed physical preparation.
		case CXform::ExfInsert2DML:
		case CXform::ExfDelete2DML:
		case CXform::ExfUpdate2DML:
		case CXform::ExfSplitLimit:
		case CXform::ExfSplitGbAgg:
		case CXform::ExfSplitGbAggDedup:
		case CXform::ExfSplitDQA:
		case CXform::ExfGbAggWithMDQA2Join:
		case CXform::ExfImplementFullOuterMergeJoin:
		case CXform::ExfSplitWindowFunc:
			return true;
		default:
			return false;
	}
}

struct SAudit
{
	std::string rules_dir;
	std::string output_dir;
	std::vector<SRuleRecord> rules;
	std::vector<SXformRecord> xforms;
	std::map<std::string, SFeatureStats> operators;
	std::map<std::string, unsigned long> constraints;
	std::map<std::string, unsigned long> reason_counts;
	unsigned long physical_lines = 0;
	unsigned long candidates = 0;
	unsigned long parsed = 0;
	unsigned long supported = 0;
	unsigned long unsupported = 0;
	unsigned long parse_failed = 0;
	unsigned long skipped_non_eq = 0;
	unsigned long semantic_xforms = 0;
	unsigned long join_enumeration_xforms = 0;
	unsigned long implementation_property_xforms = 0;
	std::string fatal_error;
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
Narrow(const CWStringDynamic &value)
{
	std::string result;
	const WCHAR *buffer = value.GetBuffer();
	for (ULONG ul = 0; L'\0' != buffer[ul]; ul++)
	{
		const WCHAR wc = buffer[ul];
		result.push_back(wc >= 0 && wc <= 0x7f ? static_cast<char>(wc) : '?');
	}
	return result;
}

std::string
JsonEscape(const std::string &value)
{
	std::ostringstream out;
	for (unsigned char ch : value)
	{
		switch (ch)
		{
			case '"':
				out << "\\\"";
				break;
			case '\\':
				out << "\\\\";
				break;
			case '\n':
				out << "\\n";
				break;
			case '\r':
				out << "\\r";
				break;
			case '\t':
				out << "\\t";
				break;
			default:
				if (ch < 0x20)
				{
					const char hex[] = "0123456789abcdef";
					out << "\\u00" << hex[ch >> 4] << hex[ch & 0xf];
				}
				else
				{
					out << static_cast<char>(ch);
				}
		}
	}
	return out.str();
}

std::string
CsvEscape(const std::string &value)
{
	if (std::string::npos == value.find_first_of(",\"\r\n"))
	{
		return value;
	}
	std::string result = "\"";
	for (char ch : value)
	{
		if ('"' == ch)
		{
			result += "\"\"";
		}
		else
		{
			result += ch;
		}
	}
	result += '"';
	return result;
}

std::string
Join(const std::vector<std::string> &values, const char *separator)
{
	std::ostringstream out;
	for (size_t i = 0; i < values.size(); i++)
	{
		if (0 < i)
		{
			out << separator;
		}
		out << values[i];
	}
	return out.str();
}

std::string
OpName(const CDSLOp *op)
{
	std::string name = CDSLOpKindTable::SzName(op->Edslop());
	if (EdslopAgg == op->Edslop() &&
		EdslaggfuncUnknown != op->Edslaggfunc())
	{
		const CHAR *agg_name =
			CDSLOpKindTable::SzAggFuncName(op->Edslaggfunc());
		if (nullptr != agg_name)
		{
			name += "_";
			name += agg_name;
		}
	}
	if (op->FDistinct())
	{
		name += "*";
	}
	if (EdslopSort == op->Edslop())
	{
		if (EdslsortAsc == op->Edslsort())
		{
			name = "SortAsc";
		}
		else if (EdslsortDesc == op->Edslsort())
		{
			name = "SortDesc";
		}
	}
	return name;
}

void
CollectOps(const CDSLOp *op, BOOL source, SRuleRecord *record, SAudit *audit)
{
	const std::string name = OpName(op);
	SFeatureStats &stats = audit->operators[name];
	stats.matcher_supported =
		CDSLOpKindTable::FMatcherSupported(op->Edslop());
	stats.instantiator_supported =
		CDSLOpKindTable::FInstantiatorSupported(op->Edslop());
	if (source)
	{
		stats.source_occurrences++;
		record->source_ops.insert(name);
		if (!stats.matcher_supported)
		{
			record->reasons.insert("matcher.unsupported_operator." + name);
		}
	}
	else
	{
		stats.target_occurrences++;
		record->target_ops.insert(name);
		if (!stats.instantiator_supported)
		{
			record->reasons.insert("instantiator.unsupported_operator." + name);
		}
	}

	for (ULONG ul = 0; ul < op->UlChildren(); ul++)
	{
		CollectOps((*op)[ul], source, record, audit);
	}
}

std::vector<COperator::EOperatorId>
SourceRootOperatorIds(const CDSLOp *root)
{
	std::vector<COperator::EOperatorId> ids;
	const COperator::EOperatorId primary = root->Eopid();
	if (COperator::EopSentinel != primary)
	{
		ids.push_back(primary);
	}
	if (EdslopAgg == root->Edslop() ||
		CDSLOpKindTable::FHasPreUnnestRepresentation(root->Edslop()))
	{
		ids.push_back(COperator::EopLogicalSelect);
	}
	std::sort(ids.begin(), ids.end());
	ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
	return ids;
}

std::vector<std::string>
NativeXformCandidates(const CDSLOp *root)
{
	std::vector<std::string> result;
	const std::vector<COperator::EOperatorId> root_ids =
		SourceRootOperatorIds(root);
	CXformFactory *factory = CXformFactory::Pxff();
	if (nullptr == factory)
	{
		return result;
	}

	for (ULONG ul = 0; ul < CXform::ExfSentinel; ul++)
	{
		const CXform::EXformId id = static_cast<CXform::EXformId>(ul);
		if (!factory->IsXformIdUsed(id))
		{
			continue;
		}
		CXform *xform = factory->Pxf(id);
		if (nullptr == xform || xform->FImplementation() ||
			CXform::FPGORCAExploration(id))
		{
			continue;
		}
		CExpression *pattern = xform->PexprPattern();
		if (nullptr != pattern &&
			root_ids.end() != std::find(root_ids.begin(), root_ids.end(),
								 pattern->Pop()->Eopid()))
		{
			result.emplace_back(xform->SzId());
		}
	}
	std::sort(result.begin(), result.end());
	result.erase(std::unique(result.begin(), result.end()), result.end());
	return result;
}

void
AnalyzeRule(CDSLRule *rule, SRuleRecord *record, SAudit *audit)
{
	CDSLOp *source_root = rule->PfragSrc()->PopRoot();
	CDSLOp *target_root = rule->PfragTgt()->PopRoot();
	record->source_root = OpName(source_root);
	record->target_root = OpName(target_root);

	if (!CDSLOpKindTable::FSourceRootDispatchSupported(
			source_root->Edslop(), source_root->FDistinct()))
	{
		record->reasons.insert("dispatch.unsupported_root." +
							   record->source_root);
	}
	CollectOps(source_root, true, record, audit);
	CollectOps(target_root, false, record, audit);

	CDSLConstraintArray *constraints = rule->Pdrgpcon();
	for (ULONG ul = 0; ul < constraints->Size(); ul++)
	{
		const CDSLConstraint *constraint = (*constraints)[ul];
		const std::string name =
			CDSLConstraintKindTable::SzName(constraint->Edslcon());
		record->constraints.insert(name);
		audit->constraints[name]++;
		if (!CDSLConstraintKindTable::FCheckerSupported(
				constraint->Edslcon()))
		{
			record->reasons.insert("checker.unsupported_constraint." + name);
		}
	}

	if (record->reasons.empty())
	{
		record->status = "supported_static";
		record->native_xforms = NativeXformCandidates(source_root);
		audit->supported++;
	}
	else
	{
		record->status = "unsupported_static";
		audit->unsupported++;
		for (const std::string &reason : record->reasons)
		{
			audit->reason_counts[reason]++;
		}
	}
}

void
CollectNativeXforms(SAudit *audit)
{
	CXformFactory *factory = CXformFactory::Pxff();
	GPOS_ASSERT(nullptr != factory);
	for (ULONG ul = 0; ul < CXform::ExfSentinel; ul++)
	{
		const CXform::EXformId id = static_cast<CXform::EXformId>(ul);
		if (!factory->IsXformIdUsed(id) || CXform::FPGORCAExploration(id))
		{
			continue;
		}
		CXform *xform = factory->Pxf(id);
		if (nullptr == xform || !xform->FExploration())
		{
			continue;
		}

		SXformRecord record;
		record.id = ul;
		record.name = xform->SzId();
		CExpression *pattern = xform->PexprPattern();
		if (nullptr != pattern && nullptr != pattern->Pop())
		{
			record.source_operator = pattern->Pop()->SzId();
		}
		if (CJobJoinEnumeration::FNativeJoinEnumerationXform(id))
		{
			record.category = "join_enumeration";
			record.replacement_owner =
				CJobJoinEnumeration::FReplacesNativeXform(id) ? "dphyper"
														 : "native";
			audit->join_enumeration_xforms++;
		}
		else if (FImplementationPropertyExploration(id))
		{
			record.category = "implementation_property";
			record.replacement_owner = "cascades";
			audit->implementation_property_xforms++;
		}
		else
		{
			record.category = "semantic_rewrite";
			record.replacement_owner = "native";
			audit->semantic_xforms++;
		}

		for (const SRuleRecord &rule : audit->rules)
		{
			if (rule.native_xforms.end() !=
				std::find(rule.native_xforms.begin(), rule.native_xforms.end(),
						  record.name))
			{
				record.source_root_candidate_rule_ids.push_back(rule.id);
			}
		}
		audit->xforms.push_back(std::move(record));
	}
}

void
WriteStringArray(std::ostream &out, const std::set<std::string> &values)
{
	out << '[';
	bool first = true;
	for (const std::string &value : values)
	{
		out << (first ? "" : ",") << '"' << JsonEscape(value) << '"';
		first = false;
	}
	out << ']';
}

void
WriteStringArray(std::ostream &out, const std::vector<std::string> &values)
{
	out << '[';
	for (size_t i = 0; i < values.size(); i++)
	{
		out << (0 == i ? "" : ",") << '"' << JsonEscape(values[i]) << '"';
	}
	out << ']';
}

bool
WriteReports(const SAudit &audit)
{
	std::error_code ec;
	fs::create_directories(audit.output_dir, ec);
	if (ec)
	{
		std::cerr << "cannot create output directory: " << ec.message()
				  << std::endl;
		return false;
	}

	std::ofstream json(fs::path(audit.output_dir) / "coverage.json");
	std::ofstream unsupported(fs::path(audit.output_dir) /
						  "unsupported_features.csv");
	std::ofstream candidates(fs::path(audit.output_dir) /
						 "replacement_candidates.csv");
	if (!json || !unsupported || !candidates)
	{
		std::cerr << "cannot open one or more report files" << std::endl;
		return false;
	}

	json << "{\n  \"schema_version\":2,\n  \"rules_dir\":\""
		 << JsonEscape(audit.rules_dir) << "\",\n  \"totals\":{";
	json << "\"physical_lines\":" << audit.physical_lines
		 << ",\"candidates\":" << audit.candidates
		 << ",\"parsed\":" << audit.parsed
		 << ",\"supported_static\":" << audit.supported
		 << ",\"unsupported_static\":" << audit.unsupported
		 << ",\"parse_failed\":" << audit.parse_failed
		 << ",\"skipped_non_eq\":" << audit.skipped_non_eq
		 << ",\"native_exploration_xforms\":" << audit.xforms.size()
		 << ",\"semantic_rewrite_xforms\":" << audit.semantic_xforms
		 << ",\"join_enumeration_xforms\":"
		 << audit.join_enumeration_xforms
		 << ",\"implementation_property_xforms\":"
		 << audit.implementation_property_xforms << "},\n";

	json << "  \"xforms\":[";
	bool first = true;
	for (const SXformRecord &xform : audit.xforms)
	{
		json << (first ? "\n" : ",\n") << "    {\"id\":" << xform.id
			 << ",\"name\":\"" << JsonEscape(xform.name)
			 << "\",\"category\":\"" << xform.category
			 << "\",\"replacement_owner\":\"" << xform.replacement_owner
			 << "\",\"source_operator\":\""
			 << JsonEscape(xform.source_operator)
			 << "\",\"source_root_candidate_rule_ids\":[";
		for (size_t i = 0; i < xform.source_root_candidate_rule_ids.size(); i++)
		{
			json << (0 == i ? "" : ",")
				 << xform.source_root_candidate_rule_ids[i];
		}
		json << "]}";
		first = false;
	}
	json << (first ? "" : "\n  ") << "],\n";

	json << "  \"operators\":[";
	first = true;
	for (const auto &entry : audit.operators)
	{
		json << (first ? "\n" : ",\n") << "    {\"name\":\""
			 << JsonEscape(entry.first) << "\",\"source_occurrences\":"
			 << entry.second.source_occurrences << ",\"target_occurrences\":"
			 << entry.second.target_occurrences << ",\"matcher_supported\":"
			 << (entry.second.matcher_supported ? "true" : "false")
			 << ",\"instantiator_supported\":"
			 << (entry.second.instantiator_supported ? "true" : "false") << '}';
		first = false;
	}
	json << (first ? "" : "\n  ") << "],\n  \"constraints\":[";
	first = true;
	for (const auto &entry : audit.constraints)
	{
		json << (first ? "\n" : ",\n") << "    {\"name\":\""
			 << JsonEscape(entry.first) << "\",\"occurrences\":" << entry.second
			 << ",\"checker_supported\":true}";
		first = false;
	}
	json << (first ? "" : "\n  ") << "],\n  \"failure_reasons\":[";
	first = true;
	for (const auto &entry : audit.reason_counts)
	{
		json << (first ? "\n" : ",\n") << "    {\"reason\":\""
			 << JsonEscape(entry.first) << "\",\"affected_rules\":"
			 << entry.second << '}';
		first = false;
	}
	json << (first ? "" : "\n  ") << "],\n  \"rules\":[";
	first = true;
	for (const SRuleRecord &rule : audit.rules)
	{
		json << (first ? "\n" : ",\n") << "    {\"id\":" << rule.id
			 << ",\"file\":\"" << JsonEscape(rule.file) << "\",\"line\":"
			 << rule.line << ",\"status\":\"" << rule.status
			 << "\",\"source_root\":\"" << JsonEscape(rule.source_root)
			 << "\",\"target_root\":\"" << JsonEscape(rule.target_root)
			 << "\",\"verdict\":\"" << JsonEscape(rule.verdict)
			 << "\",\"parse_error\":\"" << JsonEscape(rule.parse_error)
			 << "\",\"source_ops\":";
		WriteStringArray(json, rule.source_ops);
		json << ",\"target_ops\":";
		WriteStringArray(json, rule.target_ops);
		json << ",\"constraints\":";
		WriteStringArray(json, rule.constraints);
		json << ",\"reasons\":";
		WriteStringArray(json, rule.reasons);
		json << ",\"native_xform_candidates\":";
		WriteStringArray(json, rule.native_xforms);
		json << ",\"dsl\":\"" << JsonEscape(rule.dsl) << "\"}";
		first = false;
	}
	json << (first ? "" : "\n  ") << "]\n}\n";

	unsupported << "stage,feature_type,feature,reason_code,affected_rules\n";
	for (const auto &entry : audit.reason_counts)
	{
		const std::string &reason = entry.first;
		const size_t first_dot = reason.find('.');
		const size_t second_dot = reason.find('.', first_dot + 1);
		const std::string stage = reason.substr(0, first_dot);
		const std::string reason_kind =
			reason.substr(first_dot + 1, second_dot - first_dot - 1);
		const std::string feature = reason.substr(second_dot + 1);
		const std::string feature_type =
			std::string::npos != reason_kind.find("operator") ||
					"unsupported_root" == reason_kind
				? "operator"
				: "constraint";
		unsupported << CsvEscape(stage) << ',' << CsvEscape(feature_type) << ','
					<< CsvEscape(feature) << ',' << CsvEscape(reason) << ','
					<< entry.second << '\n';
	}

	candidates << "rule_id,file,line,source_root,target_root,"
				   "native_xform_candidates,verification_status\n";
	for (const SRuleRecord &rule : audit.rules)
	{
		if ("supported_static" != rule.status)
		{
			continue;
		}
		candidates << rule.id << ',' << CsvEscape(rule.file) << ',' << rule.line
				   << ',' << CsvEscape(rule.source_root) << ','
				   << CsvEscape(rule.target_root) << ','
				   << CsvEscape(Join(rule.native_xforms, ";"))
				   << ",needs_runtime_replacement_test\n";
	}
	return true;
}

void
AuditFiles(CMemoryPool *mp, SAudit *audit)
{
	std::error_code ec;
	if (!fs::is_directory(audit->rules_dir, ec))
	{
		audit->fatal_error = "rules path is not a readable directory";
		return;
	}

	std::vector<fs::path> files;
	for (fs::recursive_directory_iterator it(audit->rules_dir, ec), end;
		 !ec && it != end; it.increment(ec))
	{
		if (it->is_regular_file())
		{
			files.push_back(it->path());
		}
	}
	if (ec)
	{
		audit->fatal_error = "cannot enumerate rules directory: " + ec.message();
		return;
	}
	std::sort(files.begin(), files.end());

	unsigned long next_id = 1;
	for (const fs::path &path : files)
	{
		std::ifstream input(path);
		if (!input)
		{
			audit->fatal_error = "cannot read rule file: " + path.string();
			return;
		}
		std::string raw;
		unsigned long line_number = 0;
		while (std::getline(input, raw))
		{
			line_number++;
			audit->physical_lines++;
			std::string line = Trim(raw);
			if (line.empty() || '#' == line[0])
			{
				continue;
			}

			SRuleRecord record;
			record.id = next_id++;
			record.file = fs::relative(path, audit->rules_dir, ec).generic_string();
			if (ec)
			{
				record.file = path.filename().generic_string();
				ec.clear();
			}
			record.line = line_number;
			audit->candidates++;

			const size_t tab = line.find('\t');
			record.dsl = Trim(line.substr(0, tab));
			if (std::string::npos != tab)
			{
				record.verdict = Trim(line.substr(tab + 1));
			}
			if (!record.verdict.empty() && "EQ" != record.verdict)
			{
				record.status = "skipped_non_eq";
				audit->skipped_non_eq++;
				audit->rules.push_back(std::move(record));
				continue;
			}

			CWStringDynamic parse_error(mp);
			CDSLRule *rule = CDSLRuleParser::PdslruleParse(
				mp, record.dsl.c_str(),
				record.verdict.empty() ? nullptr : record.verdict.c_str(),
				&parse_error);
			if (nullptr == rule)
			{
				record.status = "parse_failed";
				record.parse_error = Narrow(parse_error);
				audit->parse_failed++;
				audit->rules.push_back(std::move(record));
				continue;
			}

			audit->parsed++;
			AnalyzeRule(rule, &record, audit);
			rule->Release();
			audit->rules.push_back(std::move(record));
		}
	}
}

void *
RunAudit(void *argument)
{
	SAudit *audit = static_cast<SAudit *>(argument);
	CAutoMemoryPool amp;
	AuditFiles(amp.Pmp(), audit);
	if (audit->fatal_error.empty())
	{
		CollectNativeXforms(audit);
	}
	return nullptr;
}
}  // namespace

int
main(int argc, char **argv)
{
	if (argc < 2 || argc > 3)
	{
		std::cerr << "usage: pgorca_rule_audit <rules-directory> "
					 "[output-directory]" << std::endl;
		return 2;
	}

	SAudit audit;
	audit.rules_dir = fs::absolute(argv[1]).lexically_normal().string();
	audit.output_dir =
		fs::absolute(argc == 3 ? argv[2] : "rule-audit").lexically_normal().string();

	struct gpos_init_params gpos_params = {nullptr};
	gpos_init(&gpos_params);
	gpdxl_init();
	gpopt_init();

	gpos_exec_params params;
	params.func = RunAudit;
	params.arg = &audit;
	params.result = nullptr;
	params.stack_start = &params;
	params.error_buffer = nullptr;
	params.error_buffer_size = -1;
	params.abort_requested = nullptr;
	const INT framework_status = gpos_exec(&params);

	gpopt_terminate();
	gpdxl_terminate();
	gpos_terminate();

	if (0 != framework_status)
	{
		std::cerr << "audit aborted inside the GPOS task, status="
				  << framework_status << std::endl;
		return framework_status;
	}
	if (!audit.fatal_error.empty())
	{
		std::cerr << audit.fatal_error << std::endl;
		return 1;
	}
	if (!WriteReports(audit))
	{
		return 1;
	}

	std::cout << "DSL rule audit complete: candidates=" << audit.candidates
			  << " parsed=" << audit.parsed
			  << " supported_static=" << audit.supported
			  << " unsupported_static=" << audit.unsupported
			  << " parse_failed=" << audit.parse_failed
			  << " skipped_non_eq=" << audit.skipped_non_eq
			  << " native_exploration=" << audit.xforms.size()
			  << " semantic_rewrite=" << audit.semantic_xforms
			  << " join_enumeration=" << audit.join_enumeration_xforms
			  << " implementation_property="
			  << audit.implementation_property_xforms << std::endl;
	std::cout << "Reports: " << audit.output_dir << std::endl;
	return 0;
}
