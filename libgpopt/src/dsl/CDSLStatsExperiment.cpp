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
#include <iomanip>
#include <limits>
#include <sstream>
#include <unordered_set>

#include "gpopt/base/CUtils.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CDrvdPropRelational.h"
#include "gpopt/base/CColRefTable.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLPlanTemplate.h"
#include "gpos/io/COstreamString.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CLogicalDynamicGetBase.h"
#include "gpopt/operators/CLogicalGet.h"
#include "gpopt/operators/CLogicalIndexGet.h"
#include "gpopt/operators/CLogicalBitmapTableGet.h"
#include "gpopt/operators/CLogicalSetOp.h"
#include "gpopt/operators/COperator.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CScalarBoolOp.h"
#include "gpopt/operators/CScalarBooleanTest.h"
#include "gpopt/operators/CScalarConst.h"
#include "naucrates/base/IDatumInt2.h"
#include "naucrates/base/IDatumInt4.h"
#include "naucrates/base/IDatumInt8.h"
#include "naucrates/base/IDatumBool.h"
#include "naucrates/base/IDatumOid.h"
#include "gpopt/search/CGroup.h"
#include "gpopt/search/CGroupExpression.h"
#include "naucrates/statistics/IStatistics.h"
#include "naucrates/statistics/CStatistics.h"
#include "naucrates/md/CMDIdGPDB.h"

using namespace gpopt;

namespace
{
std::string Fingerprint(CMemoryPool *mp, const CExpression *expr,
	std::unordered_map<const CExpression *, std::string> *cache);

// Read stored semantic enums only; no metadata/statistics derivation or
// debug-text parsing. A category is not full operator/function identity.
const CHAR *ScalarKind(const COperator *op)
{
	if (const auto *cmp = dynamic_cast<const CScalarCmp *>(op))
	{
		switch (cmp->ParseCmpType())
		{
			case IMDType::EcmptEq: return "eq";
			case IMDType::EcmptNEq: return "neq";
			case IMDType::EcmptL: return "lt";
			case IMDType::EcmptLEq: return "le";
			case IMDType::EcmptG: return "gt";
			case IMDType::EcmptGEq: return "ge";
			case IMDType::EcmptIDF: return "distinct";
			case IMDType::EcmptOther: return "other";
		}
	}
	if (const auto *boolean = dynamic_cast<const CScalarBoolOp *>(op))
	{
		switch (boolean->Eboolop())
		{
			case CScalarBoolOp::EboolopAnd: return "and";
			case CScalarBoolOp::EboolopOr: return "or";
			case CScalarBoolOp::EboolopNot: return "not";
			default: break;
		}
	}
	if (const auto *test = dynamic_cast<const CScalarBooleanTest *>(op))
	{
		switch (test->Ebt())
		{
			case CScalarBooleanTest::EbtIsTrue: return "is_true";
			case CScalarBooleanTest::EbtIsNotTrue: return "is_not_true";
			case CScalarBooleanTest::EbtIsFalse: return "is_false";
			case CScalarBooleanTest::EbtIsNotFalse: return "is_not_false";
			case CScalarBooleanTest::EbtIsUnknown: return "is_unknown";
			case CScalarBooleanTest::EbtIsNotUnknown: return "is_not_unknown";
			default: break;
		}
	}
	return nullptr;
}

void ConstantContext(std::ostream &out, gpnaucrates::IDatum *datum)
{
	using namespace gpnaucrates;
	const CHAR *kinds[] = {"int2", "int4", "int8", "bool", "oid", "generic"};
	static_assert(GPOS_ARRAY_SIZE(kinds) == IMDType::EtiGeneric + 1, "datum kinds changed");
	const auto type = datum->GetDatumType();
	const BOOL known_type = IMDType::EtiInt2 <= type && type < IMDType::EtiGeneric;
	const BOOL is_null = datum->IsNull();
	out << ",\"constant\":{\"kind\":\"" << (known_type ? kinds[type] : "generic")
		<< "\",\"is_null\":" << (is_null ? "true" : "false")
		<< ",\"value_observed\":" << (is_null || known_type ? "true" : "false")
		<< ",\"value\":";
	// Generic datum statistics mappings can be lossy (or hashes). They are
	// never a substitute for a typed SQL literal, including for non-null values.
	if (is_null || !known_type)
		out << "null";
	else
	{
		switch (type)
		{
			case IMDType::EtiInt2: out << dynamic_cast<IDatumInt2 *>(datum)->Value(); break;
			case IMDType::EtiInt4: out << dynamic_cast<IDatumInt4 *>(datum)->Value(); break;
			case IMDType::EtiInt8: out << dynamic_cast<IDatumInt8 *>(datum)->Value(); break;
			case IMDType::EtiBool: out << (dynamic_cast<IDatumBool *>(datum)->GetValue() ? "true" : "false"); break;
			case IMDType::EtiOid: out << dynamic_cast<IDatumOid *>(datum)->OidValue(); break;
			default: break;
		}
	}
	out << "}";
}

void ColumnReference(std::ostream &out, const CColRef *column, ULONG slot)
{
	const CColRefTable *base = dynamic_cast<const CColRefTable *>(column);
	const CMDIdGPDB *relation = nullptr == base ? nullptr
		: dynamic_cast<const CMDIdGPDB *>(column->GetMdidTable());
	out << "{\"slot\":" << slot << ",\"kind\":\""
		<< (nullptr == base ? "computed" : "table") << "\",\"attribute_number\":";
	if (nullptr != base)
		out << base->AttrNum();
	else
		out << "null";
	out << ",\"relation_oid\":";
	if (nullptr != relation && 0 != relation->Oid() &&
		(IMDId::EmdidRel == relation->MdidType() || IMDId::EmdidGeneral == relation->MdidType()))
		out << relation->Oid();
	else
		out << "null";
	out << "}";
}
}

std::string
CDSLStatsExperimentSnapshot::ExpressionShape(const CExpression *expr)
{
	return CDSLPlanTemplate::ExpressionShape(expr);
}

std::string
CDSLStatsExperimentSnapshot::BindingContext(const CDSLRule *rule, const CDSLModel *model)
{
	const CDSLSymbolArray *symbols = rule->PfragSrc()->Pdrgpsym();
	std::string entries;
	ULONG total = 0, retained = 0;
	BOOL full = false;
	for (ULONG i = 0; i < symbols->Size(); ++i)
	{
		const CDSLSymbol *symbol = (*symbols)[i];
		const BOOL table = EdslsymTable == symbol->Esymkind();
		if (!table && EdslsymPred != symbol->Esymkind())
			continue;
		++total;
		if (full)
			continue;
		const CExpression *bound = nullptr == model ? nullptr
			: table ? model->PexprTable(symbol) : model->PexprPred(symbol);
		std::ostringstream entry;
		entry << std::setprecision(17);
		entry << "{\"symbol_index\":" << i << ",\"kind\":\""
			<< (table ? "table" : "predicate") << "\",\"bound\":"
			<< (nullptr == bound ? "false" : "true")
			<< ",\"derived\":" << (nullptr != model && model->FDerivedBinding(symbol) ? "true" : "false")
			<< ",\"shape\":" << (nullptr == bound ? "null" : ExpressionShape(bound));
		if (table)
		{
			const CGroupExpression *gexpr =
				nullptr == bound ? nullptr : bound->Pgexpr();
			entry << ",\"memo_group\":";
			if (nullptr != gexpr)
				entry << gexpr->Pgroup()->Id();
			else
				entry << "null";
			entry << ",\"memo_group_expression\":";
			if (nullptr != gexpr)
				entry << gexpr->Id();
			else
				entry << "null";
			const gpnaucrates::IStatistics *stats = nullptr == bound ? nullptr : bound->Pstats();
			const CHAR *origin = nullptr == stats ? "missing" : "expression";
			if (nullptr == stats && nullptr != gexpr)
			{
				stats = gexpr->Pgroup()->Pstats();
				if (nullptr != stats)
					origin = "memo_group";
			}
			entry << ",\"stats_source\":\"" << origin << "\",\"rows\":";
			if (nullptr != stats && std::isfinite(stats->Rows().Get()))
				entry << stats->Rows().Get();
			else
				entry << "null";
		}
		entry << "}";
		if (entries.size() + entry.str().size() + 1 > 2048)
		{
			full = true;
			continue;
		}
		if (retained++)
			entries += ",";
		entries += entry.str();
	}
	return "{\"capture\":\"after_evaluation\",\"scope\":\"source_table_predicate_symbols\","
		"\"symbols\":[" + entries + "],\"total_symbols\":" + std::to_string(total) +
		",\"omitted_symbols\":" + std::to_string(total - retained) + "}";
}

std::string
CDSLStatsExperimentSnapshot::InputContext(const CExpression *expr, CMemoryPool *mp,
	BOOL query_input)
{
	std::ostringstream out;
	out << std::setprecision(17);
	std::unordered_map<const CExpression *, std::string> fingerprints;
	const auto node = [&](const CExpression *input)
	{
		const gpnaucrates::IStatistics *stats = input->Pstats();
		const CHAR *origin = nullptr == stats ? "missing" : "expression";
		if (nullptr == stats && nullptr != input->Pgexpr())
		{
			stats = input->Pgexpr()->Pgroup()->Pstats();
			if (nullptr != stats)
				origin = "memo_group";
		}
		out << "{\"operator\":\"" << input->Pop()->SzId()
			<< "\",\"arity\":" << input->Arity()
			<< ",\"stats_source\":\"" << origin << "\",\"rows\":";
		if (nullptr != stats && std::isfinite(stats->Rows().Get()))
			out << stats->Rows().Get();
		else
			out << "null";
		out << ",\"empty\":";
		if (nullptr != stats)
			out << (stats->IsEmpty() ? "true" : "false");
		else
			out << "null";
		const CGroup *group = nullptr == input->Pgexpr() ? nullptr : input->Pgexpr()->Pgroup();
		out << ",\"memo_group_expressions\":";
		if (nullptr != group)
			out << group->UlGExprs();
		else
			out << "null";
		out << ",\"memo_state\":";
		if (nullptr != group)
		{
			const CGroup *stats_owner = group;
			while (nullptr != stats_owner->PgroupDuplicate())
				stats_owner = stats_owner->PgroupDuplicate();
			out << "{\"group\":" << group->Id()
				<< ",\"statistics_owner_group\":" << stats_owner->Id()
				<< ",\"group_expression\":" << input->Pgexpr()->Id()
				<< ",\"group_explored\":" << (group->FExplored() ? "true" : "false")
				<< ",\"group_implemented\":" << (group->FImplemented() ? "true" : "false")
				<< ",\"expression_explored\":" << (input->Pgexpr()->FExplored() ? "true" : "false")
				<< ",\"expression_implemented\":" << (input->Pgexpr()->FImplemented() ? "true" : "false") << "}";
		}
		else
			out << "null";
		out << ",\"logical_properties\":";
		// Read only detached, complete Memo properties. Instrumentation must not
		// trigger property/statistics derivation or change rule scheduling.
		CDrvdProp *props = nullptr == group ? nullptr : group->Pdp();
		if (nullptr != props && props->IsComplete() && props->Ept() == CDrvdProp::EptRelational)
		{
			const CDrvdPropRelational *rel = CDrvdPropRelational::GetRelationalProperties(props);
			const CKeyCollection *keys = rel->GetKeyCollection();
			out << "{\"source\":\"complete_memo_group\",\"output_columns\":"
				<< rel->GetOutputColumns()->Size() << ",\"outer_columns\":"
				<< rel->GetOuterReferences()->Size() << ",\"not_null_columns\":"
				<< rel->GetNotNullColumns()->Size() << ",\"key_count\":"
				<< (nullptr == keys ? 0 : keys->Keys()) << ",\"join_depth\":"
				<< rel->GetJoinDepth() << "}";
		}
		else
			out << "null";
		// A query-local provenance key, not a query-independent model feature.
		if (nullptr != mp)
			out << ",\"reference_key\":\"" << ::Fingerprint(mp, input, &fingerprints) << "\"";
		out << "}";
	};
	out << "{";
	COptCtxt *context = COptCtxt::PoctxtFromTLS();
	if (nullptr != context && context->FHasDSLStatsExperiment())
		out << "\"stats_lifecycle_sequence\":" << context->UlDSLStatsLifecycleEvents() << ",";
	out << (query_input
		? "\"capture\":\"before_memo_initialization\",\"scope\":\"query_after_preprocessing\","
		: "\"capture\":\"before_evaluation\",\"scope\":\"source_before_match_view\",")
		<< "\"source_shape\":"
		<< ExpressionShape(expr) << ",\"root\":";
	node(expr);
	out << ",\"children\":[";
	ULONG total = 0;
	// Bound each compact JSON line; omitted children are counted, not hidden.
	const ULONG limit = 8;
	for (ULONG i = 0; i < expr->Arity(); ++i)
	{
		const CExpression *child = (*expr)[i];
		if (child->Pop()->FScalar())
			continue;
		if (total < limit)
		{
			if (0 < total)
				out << ",";
			out << "{\"position\":" << i << ",\"node\":";
			node(child);
			out << "}";
		}
		++total;
	}
	out << "],\"relational_children\":" << total
		<< ",\"omitted_children\":" << (total > limit ? total - limit : 0);
	if (query_input)
	{
		out << ",\"plan_template\":" << CDSLPlanTemplate::Serialize(mp, expr);
		const CDSLStatsExperimentSnapshot *snapshot = nullptr == context ? nullptr
			: context->PDSLStatsExperimentSnapshot();
		if (nullptr != snapshot && 0 != snapshot->UlTemplateRoute())
			out << ",\"template_route\":" << snapshot->UlTemplateRoute();
		else if (nullptr != snapshot && snapshot->FHasTemplateSelection())
			out << ",\"plan_slice\":"
				<< snapshot->TemplateSelectionArtifact(const_cast<CExpression *>(expr));
	}
	// Ordered operator/arity prefix distinguishes trees with the same histogram.
	// The transport is chunked, not the tree: every consumed binding path must
	// remain addressable. Still only read cached root/direct-child properties.
	const CDSLStatsExperimentSnapshot *experiment = nullptr == context ? nullptr
		: context->PDSLStatsExperimentSnapshot();
	out << ",\"source_tree\":{\"request_binding\":";
	if (nullptr != experiment)
		out << "\"resolved_operator_only\"";
	else
		out << "null";
	out << ",\"nodes\":[";
	std::vector<const CExpression *> pending{expr};
	std::unordered_map<const CColRef *, ULONG> column_slots;
	std::vector<const CLogicalSetOp *> setops;
	// Canonicalize only references present in this tree before serializing
	// their declarations. Get outputs can precede their scalar consumers.
	ULONG visited = 0;
	while (!pending.empty())
	{
		if (0 == visited++ % 256)
			GPOS_CHECK_ABORT;
		const CExpression *input = pending.back();
		pending.pop_back();
		if (COperator::EopScalarIdent == input->Pop()->Eopid())
			column_slots.emplace(CScalarIdent::PopConvert(input->Pop())->Pcr(), column_slots.size());
		const auto *setop = dynamic_cast<const CLogicalSetOp *>(input->Pop());
		if (nullptr != setop)
			setops.push_back(setop);
		for (ULONG i = input->Arity(); i > 0; --i)
			pending.push_back((*input)[i - 1]);
	}
	// Keep existing scalar-reference slots stable. Register positional set-op
	// columns even when no ScalarIdent happens to mention them in this tree.
	std::vector<const CColRef *> additional_columns;
	auto register_columns = [&](const CColRefArray *columns) {
		for (ULONG i = 0; i < columns->Size(); ++i)
			if (column_slots.emplace((*columns)[i], column_slots.size()).second)
				additional_columns.push_back((*columns)[i]);
	};
	for (const CLogicalSetOp *setop : setops)
	{
		GPOS_CHECK_ABORT;
		register_columns(setop->PdrgpcrOutput());
		for (ULONG i = 0; i < setop->PdrgpdrgpcrInput()->Size(); ++i)
			register_columns((*setop->PdrgpdrgpcrInput())[i]);
	}
	auto column_array = [&](const CColRefArray *columns) {
		out << "[";
		for (ULONG i = 0; i < columns->Size(); ++i)
			out << (i ? "," : "") << column_slots.at((*columns)[i]);
		out << "]";
	};
	pending.push_back(expr);
	ULONG retained = 0;
	while (!pending.empty())
	{
		if (0 == retained % 256)
			GPOS_CHECK_ABORT;
		const CExpression *input = pending.back();
		pending.pop_back();
		if (retained++)
			out << ",";
		out << "{\"operator\":\"" << input->Pop()->SzId()
			<< "\",\"arity\":" << input->Arity() << ",\"relation_oid\":";
		// Read the existing descriptor only. The OID joins a same-database
		// catalog snapshot; it is neither a cardinality nor a model feature.
		const CTableDescriptor *table = CLogical::PtabdescFromTableGet(input->Pop());
		const CMDIdGPDB *mdid = nullptr == table ? nullptr
			: dynamic_cast<const CMDIdGPDB *>(table->MDId());
		if (nullptr != mdid &&
			(IMDId::EmdidRel == mdid->MdidType() || IMDId::EmdidGeneral == mdid->MdidType()) &&
			0 != mdid->Oid())
			out << mdid->Oid();
		else
			out << "null";
		// Lookup the already-resolved request; do not rebind, derive statistics,
		// or infer absence of indirect effects through equivalent Memo groups.
		ULONG request_index = 0;
		out << ",\"request_index\":";
		if (nullptr != experiment && experiment->FRequestIndex(input->Pop(), &request_index))
			out << request_index;
		else
			out << "null";
		if (const CHAR *kind = ScalarKind(input->Pop()))
			out << ",\"scalar_kind\":\"" << kind << "\"";
		if (COperator::EopScalarConst == input->Pop()->Eopid())
			ConstantContext(out, CScalarConst::PopConvert(input->Pop())->GetDatum());
		if (COperator::EopScalarIdent == input->Pop()->Eopid())
		{
			// Declared identity only: a base colref can also be reused as a
			// set-op output. Never treat its catalog statistics as derived stats.
			const CColRef *column = CScalarIdent::PopConvert(input->Pop())->Pcr();
			out << ",\"column_ref\":";
			ColumnReference(out, column, column_slots.at(column));
		}
		const auto *setop = dynamic_cast<const CLogicalSetOp *>(input->Pop());
		if (nullptr != setop)
		{
			out << ",\"setop_columns\":{\"output_slots\":";
			column_array(setop->PdrgpcrOutput());
			out << ",\"input_slots\":[";
			for (ULONG i = 0; i < setop->PdrgpdrgpcrInput()->Size(); ++i)
			{
				if (i) out << ",";
				column_array((*setop->PdrgpdrgpcrInput())[i]);
			}
			out << "]}";
		}
		const CLogicalGet *get = dynamic_cast<const CLogicalGet *>(input->Pop());
		const CLogicalDynamicGetBase *dynamic_get =
			dynamic_cast<const CLogicalDynamicGetBase *>(input->Pop());
		const CLogicalIndexGet *index_get = dynamic_cast<const CLogicalIndexGet *>(input->Pop());
		const CLogicalBitmapTableGet *bitmap_get =
			dynamic_cast<const CLogicalBitmapTableGet *>(input->Pop());
		const CColRefArray *outputs = nullptr != get ? get->PdrgpcrOutput()
			: nullptr != dynamic_get ? dynamic_get->PdrgpcrOutput()
			: nullptr != index_get ? index_get->PdrgpcrOutput()
			: nullptr != bitmap_get ? bitmap_get->PdrgpcrOutput() : nullptr;
		if (nullptr != outputs)
		{
			out << ",\"referenced_output_slots\":[";
			BOOL first = true;
			for (ULONG i = 0; i < outputs->Size(); ++i)
			{
				auto found = column_slots.find((*outputs)[i]);
				if (found != column_slots.end())
				{
					out << (first ? "" : ",") << found->second;
					first = false;
				}
			}
			out << "]";
		}
		if (COperator::EopScalarProjectElement == input->Pop()->Eopid())
		{
			const CColRef *column = CScalarProjectElement::PopConvert(input->Pop())->Pcr();
			auto found = column_slots.find(column);
			out << ",\"referenced_definition_slot\":";
			if (found != column_slots.end())
				out << found->second;
			else
				out << "null";
		}
		out << "}";
		for (ULONG i = input->Arity(); i > 0; --i)
			pending.push_back((*input)[i - 1]);
	}
	out << "],\"additional_column_refs\":[";
	for (ULONG i = 0; i < additional_columns.size(); ++i)
	{
		if (i) out << ",";
		ColumnReference(out, additional_columns[i], column_slots.at(additional_columns[i]));
	}
	out << "],\"complete\":" << (pending.empty() ? "true" : "false") << "}}";
	return out.str();
}

std::string
CDSLStatsExperimentSnapshot::RouteContext(const CExpression *expr,
	const CDSLStatsExperimentSnapshot *snapshot, ULONG sequence)
{
	GPOS_ASSERT(nullptr != expr);
	std::ostringstream out;
	out << "{\"capture\":\"before_evaluation\","
		   "\"scope\":\"source_before_match_view\","
		   "\"source_tree\":{\"encoding\":\"preorder_operator_arity\",\"nodes\":\"";
	std::vector<const CExpression *> pending{expr};
	ULONG nodes = 0;
	while (!pending.empty())
	{
		if (0 == nodes % 256)
			GPOS_CHECK_ABORT;
		const CExpression *input = pending.back();
		pending.pop_back();
		if (nodes++)
			out << " ";
		out << input->Pop()->SzId() << "/" << input->Arity();
		for (ULONG child = input->Arity(); child > 0; --child)
			pending.push_back((*input)[child - 1]);
	}
	out << "\",\"complete\":true},"
		   "\"relational_tree\":{\"encoding\":\"preorder_operator_arity\",\"nodes\":\"";
	pending = {expr};
	nodes = 0;
	while (!pending.empty())
	{
		if (0 == nodes % 256)
			GPOS_CHECK_ABORT;
		const CExpression *input = pending.back();
		pending.pop_back();
		const std::vector<const CExpression *> relational_children =
			CDSLPlanTemplate::RelationalChildren(input);
		if (nodes++)
			out << " ";
		out << input->Pop()->SzId() << "/" << relational_children.size();
		for (ULONG child = relational_children.size(); child > 0; --child)
			pending.push_back(relational_children[child - 1]);
	}
	out << "\",\"complete\":true}";
	if (nullptr != snapshot && 0 != sequence && sequence == snapshot->m_template_route)
	{
		const std::string fingerprint = Fingerprint(snapshot->m_mp, expr);
		out << ",\"template_route\":" << sequence
			<< ",\"template_fingerprint\":\"" << fingerprint << "\"";
		// A route is an instantiated binding, not the entire Memo. Do not pretend
		// a depth-limited, group-bound leaf is a complete standalone plan.
		BOOL complete = true;
		pending = {expr};
		while (!pending.empty())
		{
			GPOS_CHECK_ABORT;
			const CExpression *input = pending.back();
			pending.pop_back();
			if (nullptr != input->Pgexpr() && input->Arity() != input->Pgexpr()->Arity())
				complete = false;
			for (ULONG child = 0; child < input->Arity(); ++child)
				pending.push_back((*input)[child]);
		}
		if (!complete)
			out << ",\"template_error\":\"incomplete route binding\"";
		else if (!snapshot->m_template_fingerprint.empty() &&
				 snapshot->m_template_fingerprint != fingerprint)
			out << ",\"template_error\":\"route fingerprint mismatch\"";
		else
		{
			out << ",\"plan_template\":" << CDSLPlanTemplate::Serialize(snapshot->m_mp, expr);
			if (snapshot->FHasTemplateSelection())
			{
				// The production matcher derives properties. Validate on a detached
				// tree so observing a route cannot populate live Memo properties.
				UlongToColRefMap *columns = GPOS_NEW(snapshot->m_mp) UlongToColRefMap(snapshot->m_mp);
				CExpression *copy = expr->PexprCopyWithRemappedColumns(snapshot->m_mp, columns, false);
				columns->Release();
				out << ",\"plan_slice\":" << snapshot->TemplateSelectionArtifact(copy);
				copy->Release();
			}
		}
	}
	out << "}";
	return out.str();
}

std::vector<std::string>
CDSLStatsExperimentSnapshot::ContextRecords(ULONG id, const CHAR *field,
	const std::string &value)
{
	GPOS_ASSERT(0 < id && nullptr != field && !value.empty());
	GPOS_ASSERT(0 == std::strcmp(field, "input_context") ||
		0 == std::strcmp(field, "binding_context") ||
		0 == std::strcmp(field, "query_input_context") ||
		0 == std::strcmp(field, "route_input_context"));
	const std::string identity = ",\"engine\":\"pgorca\",\"context_id\":" +
		std::to_string(id) + ",\"field\":\"" + field + "\"";
	const size_t chunk_bytes = 2048;
	if (value.size() <= chunk_bytes)
		return {"DSL_TRACE {\"kind\":\"candidate_context\"" + identity +
			",\"value\":" + value + "}"};
	std::vector<std::string> records;
	const size_t parts = (value.size() + chunk_bytes - 1) / chunk_bytes;
	const CHAR *hex = "0123456789abcdef";
	for (size_t part = 0; part < parts; ++part)
	{
		GPOS_CHECK_ABORT;
		std::string fragment;
		const size_t end = std::min(value.size(), (part + 1) * chunk_bytes);
		for (size_t i = part * chunk_bytes; i < end; ++i)
		{
			const unsigned char byte = static_cast<unsigned char>(value[i]);
			fragment.push_back(hex[byte >> 4]);
			fragment.push_back(hex[byte & 15]);
		}
		// Hex transports arbitrary UTF-8 boundaries and quotes without relying on
		// the logger's multibyte conversion or on an escaped fragment being JSON.
		records.push_back("DSL_TRACE {\"kind\":\"candidate_context_fragment\"" + identity +
			",\"encoding\":\"utf8_hex\",\"part\":" + std::to_string(part) +
			",\"parts\":" + std::to_string(parts) + ",\"total_bytes\":" +
			std::to_string(value.size()) + ",\"fragment\":\"" + fragment + "\"}");
	}
	return records;
}

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
ParsePaths(const std::string &value, std::vector<std::string> *paths)
{
	if (2 > value.size() || '[' != value.front() || ']' != value.back())
		return false;
	std::unordered_set<std::string> seen;
	std::stringstream input(value.substr(1, value.size() - 2));
	std::string item;
	while (std::getline(input, item, ','))
	{
		item = Unquote(Trim(item));
		if (item.empty())
			continue;
		if (!seen.insert(item).second)
			return false;
		paths->push_back(item);
	}
	return true;
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
		!std::isfinite(parsed) || CStatistics::MinRows.Get() > parsed ||
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
	  std::string *template_root, std::vector<std::string> *template_cuts,
	  ULONG *template_route, std::string *template_fingerprint,
	  CWStringDynamic *errors)
{
	std::istringstream input(nullptr == content ? "" : content);
	std::string line;
	SParsedTarget current;
	BOOL in_cardinalities = false;
	BOOL has_current = false;
	BOOL valid = true;
	BOOL has_discover = false;
	BOOL has_template_cuts = false;
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
		else if (!in_cardinalities && "template_route" == key && 0 == *template_route)
		{
			errno = 0;
			CHAR *end = nullptr;
			const unsigned long long route = std::strtoull(value.c_str(), &end, 10);
			if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos ||
				errno || *end || 0 == route || route > std::numeric_limits<ULONG>::max())
			{
				Error(errors, line_no, "template_route must be a positive integer");
				valid = false;
			}
			else
				*template_route = (ULONG) route;
		}
		else if (!in_cardinalities && "template_fingerprint" == key && template_fingerprint->empty())
		{
			if (!ParseFingerprint(value, template_fingerprint))
			{
				Error(errors, line_no, "template_fingerprint must be 16 hexadecimal digits");
				valid = false;
			}
		}
		else if (!in_cardinalities && "template_root" == key &&
				 template_root->empty())
		{
			*template_root = Unquote(value);
			if (template_root->empty())
			{
				Error(errors, line_no, "template_root cannot be empty");
				valid = false;
			}
		}
		else if (!in_cardinalities && "template_cuts" == key &&
				 !has_template_cuts)
		{
			has_template_cuts = true;
			if (!ParsePaths(value, template_cuts))
			{
				Error(errors, line_no, "template_cuts must be a list of unique paths");
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
				Error(errors, line_no, "rows must be finite and at least 1 (ORCA statistics minimum)");
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
	if (has_template_cuts && template_root->empty())
	{
		Error(errors, 0, "template_cuts requires template_root");
		valid = false;
	}
	if ((!template_fingerprint->empty() && 0 == *template_route) ||
		(0 != *template_route && !template_root->empty() && template_fingerprint->empty()))
	{
		Error(errors, 0, "route template selection requires template_route and template_fingerprint");
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

BOOL
CDSLStatsExperimentSnapshot::FParseRequests(const CHAR *content, std::string *id,
	std::vector<SDSLStatsExperimentRequest> *requests, BOOL *discover,
	CWStringDynamic *errors)
{
	std::string parsed_id;
	std::vector<SParsedTarget> parsed;
	BOOL parsed_discover = false;
	std::string template_root;
	std::vector<std::string> template_cuts;
	ULONG template_route = 0;
	std::string template_fingerprint;
	if (!Parse(content, &parsed_id, &parsed, &parsed_discover,
			   &template_root, &template_cuts, &template_route, &template_fingerprint, errors))
		return false;
	std::vector<SDSLStatsExperimentRequest> result;
	for (const SParsedTarget &entry : parsed)
		result.push_back({entry.m_aliases, entry.m_fingerprint,
			entry.m_operator, entry.m_rows});
	*id = std::move(parsed_id);
	*requests = std::move(result);
	*discover = parsed_discover;
	return true;
}

CDSLStatsExperimentSnapshot *
CDSLStatsExperimentSnapshot::PsnapshotLoadBuffer(CMemoryPool *mp,
											  const CHAR *content,
											  const CExpression *root,
											  CWStringDynamic *errors)
{
	std::string id;
	std::vector<SParsedTarget> parsed;
	BOOL discover = false;
	std::string template_root;
	std::vector<std::string> template_cuts;
	ULONG template_route = 0;
	std::string template_fingerprint;
	if (nullptr == root || !Parse(content, &id, &parsed, &discover,
							 &template_root, &template_cuts, &template_route, &template_fingerprint, errors))
	{
		return nullptr;
	}

	CDSLStatsExperimentSnapshot *snapshot =
		GPOS_NEW(mp) CDSLStatsExperimentSnapshot(mp);
	snapshot->m_id = id;
	snapshot->m_fDiscover = discover;
	snapshot->m_template_route = template_route;
	snapshot->m_template_fingerprint = std::move(template_fingerprint);
	if (!template_root.empty())
	{
		std::string selection_error;
		if (0 == template_route && !CDSLPlanTemplate::FValidateSelection(
				root, template_root, template_cuts, &selection_error))
		{
			Error(errors, 0, selection_error);
			GPOS_DELETE(snapshot);
			return nullptr;
		}
		snapshot->m_template_root = std::move(template_root);
		snapshot->m_template_cuts = std::move(template_cuts);
	}
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
			if (target.m_inject && nullptr != target.m_pop)
				claimed.insert(target.m_pop);
		}
		// Explicit injections supersede discovery-only observations. Two
		// explicit selectors for one node still fail the uniqueness check below.
		auto &targets = snapshot->m_targets;
		targets.erase(std::remove_if(targets.begin(), targets.end(),
			[&claimed](const SDSLStatsExperimentTarget &target) {
				return !target.m_inject && claimed.count(target.m_pop) != 0;
			}), targets.end());
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

std::string
CDSLStatsExperimentSnapshot::TemplateSelectionArtifact(CExpression *root) const
{
	GPOS_ASSERT(FHasTemplateSelection() && nullptr != root);
	return CDSLPlanTemplate::SliceArtifact(
		m_mp, root, m_template_root, m_template_cuts);
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

BOOL
CDSLStatsExperimentSnapshot::FRequestIndex(const COperator *pop, ULONG *index) const
{
	const auto found = m_operator_targets.find(pop);
	if (found == m_operator_targets.end() || !m_targets[found->second].m_inject)
		return false;
	// Declared requests precede all entries appended by discovery.
	*index = found->second;
	return true;
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
