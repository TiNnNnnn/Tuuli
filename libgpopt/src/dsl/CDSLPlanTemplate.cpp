//---------------------------------------------------------------------------
// Lossless, addressable ORCA-plan view for DSL rule mining.
//---------------------------------------------------------------------------
#include "gpopt/dsl/CDSLPlanTemplate.h"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <sstream>
#include <unordered_set>
#include <utility>

#include "gpos/common/CAutoRef.h"
#include "gpos/io/COstreamString.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/dsl/CDSLExpressionDefinitions.h"
#include "gpopt/dsl/CDSLExpressionProperties.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CLogicalConstTableGet.h"
#include "gpopt/operators/CLogicalGet.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalSequenceProject.h"
#include "gpopt/operators/CScalarBoolOp.h"
#include "gpopt/operators/CScalarBooleanTest.h"
#include "gpopt/operators/CScalarCmp.h"
#include "gpopt/operators/CScalarCast.h"
#include "gpopt/operators/CScalarConst.h"
#include "gpopt/operators/CScalarFunc.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/search/CGroupExpression.h"
#include "naucrates/base/IDatumInt2.h"
#include "naucrates/base/IDatumInt4.h"
#include "naucrates/base/IDatumInt8.h"
#include "naucrates/base/IDatumBool.h"
#include "naucrates/base/IDatumOid.h"

using namespace gpopt;

namespace
{
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

std::string
JsonString(const std::string &value)
{
	std::ostringstream out;
	out << '"';
	for (const unsigned char ch : value)
	{
		switch (ch)
		{
			case '"': out << "\\\""; break;
			case '\\': out << "\\\\"; break;
			case '\b': out << "\\b"; break;
			case '\f': out << "\\f"; break;
			case '\n': out << "\\n"; break;
			case '\r': out << "\\r"; break;
			case '\t': out << "\\t"; break;
			default:
				if (ch < 0x20)
					out << "\\u00" << "0123456789abcdef"[ch >> 4]
						<< "0123456789abcdef"[ch & 15];
				else
					out << ch;
		}
	}
	out << '"';
	return out.str();
}

std::string
OperatorText(CMemoryPool *mp, const CExpression *expr)
{
	CWStringDynamic printed(mp);
	COstreamString os(&printed);
	expr->Pop()->OsPrint(os);
	CHAR *text = CUtils::CreateMultiByteCharStringFromWCString(
		mp, const_cast<WCHAR *>(printed.GetBuffer()));
	std::string result(text);
	GPOS_DELETE_ARRAY(text);
	return result;
}

std::string
MetadataId(CMemoryPool *mp, IMDId *id)
{
	CHAR *text = CUtils::CreateMultiByteCharStringFromWCString(
		mp, const_cast<WCHAR *>(id->GetBuffer()));
	std::string result = JsonString(text);
	GPOS_DELETE_ARRAY(text);
	return result;
}

void
AppendColumn(CMemoryPool *mp, std::ostringstream *out, const CColRef *column)
{
	*out << "{\"id\":" << column->Id() << ",\"type\":"
		<< MetadataId(mp, column->RetrieveType()->MDId())
		<< ",\"typmod\":" << column->TypeModifier() << '}';
}

void
AppendColumns(CMemoryPool *mp, std::ostringstream *out, const CColRefSet *columns)
{
	*out << '[';
	CColRefSetIter iter(*columns);
	BOOL first = true;
	while (iter.Advance())
	{
		const CColRef *column = iter.Pcr();
		if (!first)
			*out << ',';
		first = false;
		AppendColumn(mp, out, column);
	}
	*out << ']';
}

void
AppendColumns(CMemoryPool *mp, std::ostringstream *out, const CColRefArray *columns)
{
	*out << '[';
	for (ULONG i = 0; i < columns->Size(); ++i)
	{
		if (i) *out << ',';
		AppendColumn(mp, out, (*columns)[i]);
	}
	*out << ']';
}

// These are native metadata facts, not a certificate of row layout or rule
// equivalence. Column sets have no SELECT-list order; ProjectElement paths do.
void
AppendColumnFacts(CMemoryPool *mp, std::ostringstream *out, const CExpression *expr)
{
	if (!expr->Pop()->FLogical() && !expr->Pop()->FScalar())
		return;
	// Property derivation populates caches without changing the source tree.
	CExpression *derived = const_cast<CExpression *>(expr);
	*out << ",\"column_facts\":{";
	if (expr->Pop()->FLogical())
	{
		*out << "\"output\":";
		AppendColumns(mp, out, derived->DeriveOutputColumns());
		*out << ",\"outer\":";
		AppendColumns(mp, out, derived->DeriveOuterReferences());
		// A column set is not a row layout. Preserve a declared leaf layout
		// only; derived operators require their own ordered lowering contract.
		const CColRefArray *layout = nullptr;
		if (const auto *get = dynamic_cast<const CLogicalGet *>(expr->Pop()))
		{
			layout = get->PdrgpcrOutput();
			*out << ",\"relation_mdid\":" << MetadataId(mp, get->Ptabdesc()->MDId());
		}
		else if (const auto *values = dynamic_cast<const CLogicalConstTableGet *>(expr->Pop()))
		{
			layout = values->PdrgpcrOutput();
			*out << ",\"constant_rows\":[";
			const auto *rows = values->Pdrgpdrgpdatum();
			for (ULONG i = 0; i < rows->Size(); ++i)
			{
				GPOS_CHECK_ABORT;
				if (i) *out << ',';
				*out << '[';
				const auto *row = (*rows)[i];
				for (ULONG j = 0; j < row->Size(); ++j)
				{
					if (j) *out << ',';
					IDatum *datum = (*row)[j];
					*out << "{\"type\":" << MetadataId(mp, datum->MDId())
						<< ",\"typmod\":" << datum->TypeModifier();
					ConstantContext(*out, datum);
					*out << '}';
				}
				*out << ']';
			}
			*out << ']';
		}
		*out << ",\"output_layout\":";
		if (nullptr != layout)
			AppendColumns(mp, out, layout);
		else
			*out << "null";
	}
	else
	{
		const CScalar *scalar = CScalar::PopConvert(expr->Pop());
		*out << "\"used\":";
		AppendColumns(mp, out, derived->DeriveUsedColumns());
		*out << ",\"defined\":";
		AppendColumns(mp, out, derived->DeriveDefinedColumns());
		if (CDSLMatchView::FScalarValue(expr))
		{
			*out << ",\"value_type\":" << MetadataId(mp, scalar->MdidType())
				<< ",\"value_typmod\":" << CDSLMatchView::ScalarValueTypeModifier(expr);
		}
		// Names are display text, not catalog identity. Argument types/order
		// remain on the existing child paths; IDs alone certify no semantics.
		if (nullptr != scalar->MdIdOp())
			*out << ",\"operator_mdid\":" << MetadataId(mp, scalar->MdIdOp());
		if (COperator::EopScalarFunc == expr->Pop()->Eopid())
		{
			const auto *function = CScalarFunc::PopConvert(expr->Pop());
			*out << ",\"function_mdid\":" << MetadataId(mp, function->FuncMdId())
				<< ",\"function_format\":" << function->FuncFormat()
				<< ",\"function_variadic\":" << (function->IsFuncVariadic() ? "true" : "false");
		}
		else if (COperator::EopScalarCast == expr->Pop()->Eopid())
		{
			const auto *cast = CScalarCast::PopConvert(expr->Pop());
			*out << ",\"function_mdid\":" << MetadataId(mp, cast->FuncMdId())
				<< ",\"binary_coercible\":" << (cast->IsBinaryCoercible() ? "true" : "false");
		}
	}
	*out << '}';
}

void
AppendExpressionTree(CMemoryPool *mp, std::ostringstream *out,
	const CExpression *root, const std::string &root_path)
{
	std::vector<std::pair<const CExpression *, std::string>> pending{{root, root_path}};
	BOOL first = true;
	while (!pending.empty())
	{
		const auto current = pending.back();
		pending.pop_back();
		if (!first)
			*out << ",";
		first = false;
		*out << "{\"path\":" << JsonString(current.second)
			<< ",\"operator\":" << JsonString(current.first->Pop()->SzId())
			<< ",\"operator_text\":" << JsonString(OperatorText(mp, current.first))
			<< ",\"arity\":" << current.first->Arity();
		CDSLPlanTemplate::AppendScalarContext(*out, current.first->Pop());
		AppendColumnFacts(mp, out, current.first);
		*out << '}';
		for (ULONG i = current.first->Arity(); i > 0; --i)
			pending.emplace_back((*current.first)[i - 1], current.second + "/" +
				std::to_string(i - 1));
	}
}

void
AppendOnce(std::vector<std::string> *values, const std::string &value)
{
	if (values->end() == std::find(values->begin(), values->end(), value))
		values->push_back(value);
}

std::vector<std::string>
DSLViews(const CExpression *expr)
{
	std::vector<std::string> views;
	const COperator::EOperatorId eopid = expr->Pop()->Eopid();
	for (ULONG op = EdslopInnerJoin; op < EdslopSentinel; ++op)
	{
		const EDslOpKind kind = static_cast<EDslOpKind>(op);
		if (!CDSLOpKindTable::FMatcherSupported(kind))
			continue;
		if (CDSLOpKindTable::Eopid(kind, false) == eopid)
			AppendOnce(&views, CDSLOpKindTable::SzName(kind));
		if (CDSLOpKindTable::Eopid(kind, true) == eopid &&
			CDSLOpKindTable::Eopid(kind, false) != eopid)
			AppendOnce(&views, std::string(CDSLOpKindTable::SzName(kind)) + "*");
	}
	if (COperator::EopLogicalSelect == eopid)
	{
		std::vector<const CExpression *> pending;
		for (ULONG i = 0; i < expr->Arity(); ++i)
			if ((*expr)[i]->Pop()->FScalar())
				pending.push_back((*expr)[i]);
		while (!pending.empty())
		{
			const CExpression *scalar = pending.back();
			pending.pop_back();
			switch (scalar->Pop()->Eopid())
			{
				case COperator::EopScalarSubqueryExists:
					AppendOnce(&views, "Exists");
					break;
				case COperator::EopScalarSubqueryNotExists:
					AppendOnce(&views, "NotExists");
					break;
				case COperator::EopScalarSubqueryAny:
					AppendOnce(&views, "InSubFilter");
					AppendOnce(&views, "Any");
					break;
				case COperator::EopScalarSubqueryAll:
					AppendOnce(&views, "All");
					break;
				default:
					break;
			}
			for (ULONG i = 0; i < scalar->Arity(); ++i)
				pending.push_back((*scalar)[i]);
		}
	}
	if (views.empty() && CDSLPlanTemplate::RelationalChildren(expr).empty())
		views.push_back("Input");
	return views;
}

BOOL
FPathWithin(const std::string &path, const std::string &root)
{
	return path == root ||
		(path.size() > root.size() && 0 == path.compare(0, root.size(), root) &&
		 '/' == path[root.size()]);
}

std::unordered_set<std::string>
RelationalPaths(const CExpression *expr)
{
	std::unordered_set<std::string> paths;
	std::vector<std::pair<const CExpression *, std::string>> pending{{expr, "r"}};
	while (!pending.empty())
	{
		const auto current = pending.back();
		pending.pop_back();
		paths.insert(current.second);
		const auto children = CDSLPlanTemplate::RelationalChildren(current.first);
		for (ULONG i = children.size(); i > 0; --i)
			pending.emplace_back(children[i - 1], current.second + "/" +
				std::to_string(i - 1));
	}
	return paths;
}

const CExpression *
PexprAtPath(const CExpression *expr, const std::string &path)
{
	if ("r" == path)
		return expr;
	if (path.size() < 3 || 0 != path.compare(0, 2, "r/"))
		return nullptr;
	size_t begin = 2;
	while (begin < path.size())
	{
		const size_t end = path.find('/', begin);
		const std::string part = path.substr(begin, end - begin);
		if (part.empty() || part.find_first_not_of("0123456789") != std::string::npos)
			return nullptr;
		const auto children = CDSLPlanTemplate::RelationalChildren(expr);
		const unsigned long index = std::strtoul(part.c_str(), nullptr, 10);
		if (index >= children.size())
			return nullptr;
		expr = children[index];
		if (std::string::npos == end)
			return expr;
		begin = end + 1;
	}
	return nullptr;
}

EDslOpKind
CanonicalKind(const CExpression *expr, BOOL *distinct)
{
	*distinct = false;
	switch (expr->Pop()->Eopid())
	{
	case COperator::EopLogicalSelect: return EdslopFilter;
		case COperator::EopLogicalProject: return EdslopCompute;
		case COperator::EopLogicalGbAgg:
			if (2 == expr->Arity() && 0 == (*expr)[1]->Arity() &&
				COperator::EgbaggtypeGlobal ==
					CLogicalGbAgg::PopConvert(expr->Pop())->Egbaggtype())
			{
				*distinct = true;
				return EdslopProj;
			}
			return EdslopAgg;
		case COperator::EopLogicalGbAggDeduplicate: return EdslopAgg;
		case COperator::EopLogicalSequenceProject:
			return CLogicalSequenceProject::PopConvert(expr->Pop())->FHasFrameSpecs()
				? EdslopWindowFrame : EdslopWindowRows;
		case COperator::EopLogicalInnerJoin: return EdslopInnerJoin;
		case COperator::EopLogicalLeftOuterJoin: return EdslopLeftJoin;
		case COperator::EopLogicalFullOuterJoin: return EdslopFullJoin;
		case COperator::EopLogicalLeftSemiJoin: return EdslopSemiJoin;
		case COperator::EopLogicalLeftSemiApply:
		case COperator::EopLogicalLeftSemiCorrelatedApply: return EdslopSemiApply;
		case COperator::EopLogicalLeftAntiSemiJoin: return EdslopAntiJoin;
		case COperator::EopLogicalLeftAntiSemiApply:
		case COperator::EopLogicalLeftAntiSemiCorrelatedApply: return EdslopAntiApply;
		case COperator::EopLogicalLeftAntiSemiJoinNotIn: return EdslopAntiJoinNotIn;
		case COperator::EopLogicalLeftAntiSemiApplyNotIn: return EdslopAntiApplyNotIn;
		case COperator::EopLogicalInnerApply:
		case COperator::EopLogicalInnerCorrelatedApply: return EdslopInnerApply;
		case COperator::EopLogicalLeftOuterApply:
		case COperator::EopLogicalLeftOuterCorrelatedApply: return EdslopLeftOuterApply;
		case COperator::EopLogicalUnion:
			*distinct = true;
			return EdslopUnion;
		case COperator::EopLogicalUnionAll: return EdslopUnion;
		case COperator::EopLogicalIntersect:
			*distinct = true;
			return EdslopIntersect;
		case COperator::EopLogicalIntersectAll: return EdslopIntersect;
		case COperator::EopLogicalDifference:
			*distinct = true;
			return EdslopExcept;
		case COperator::EopLogicalDifferenceAll: return EdslopExcept;
		case COperator::EopLogicalMaxOneRow: return EdslopMaxOneRow;
		case COperator::EopLogicalCTEAnchor: return EdslopCTEAnchor;
		default: return EdslopSentinel;
	}
}

CDSLOp *
PopInput(CMemoryPool *mp, ULONG *symbol_counts, ULONG *symbol_id)
{
	CDSLSymbolArray *symbols = GPOS_NEW(mp) CDSLSymbolArray(mp);
	const std::string name = "t" + std::to_string(symbol_counts[EdslsymTable]++);
	symbols->Append(GPOS_NEW(mp) CDSLSymbol(
		mp, EdslsymTable, name.c_str(), (*symbol_id)++, EdslsideSource));
	return GPOS_NEW(mp) CDSLOp(mp, EdslopInput, false, EdslsortNone,
		EdslaggfuncUnknown, symbols, GPOS_NEW(mp) CDSLOpArray(mp));
}

CDSLOp *
PopOperator(CMemoryPool *mp, EDslOpKind kind, BOOL distinct,
	EDslSortDir sort, CDSLOpArray *children, ULONG *symbol_counts,
	ULONG *symbol_id)
{
	CDSLSymbolArray *symbols = GPOS_NEW(mp) CDSLSymbolArray(mp);
	// Inner/outer/full Join have several compatible wire forms.  The complete
	// predicate form is the only one that losslessly describes both equality-only
	// and residual predicates without inspecting their shape.
	const BOOL predicate_join = EdslopInnerJoin == kind || EdslopLeftJoin == kind ||
		EdslopFullJoin == kind;
	const EDslSymbolKind join_symbols[] = {
		EdslsymPred, EdslsymAttrs, EdslsymAttrs};
	const ULONG symbol_count = predicate_join ? GPOS_ARRAY_SIZE(join_symbols)
		: CDSLOpKindTable::UlSyms(kind);
	for (ULONG i = 0; i < symbol_count; ++i)
	{
		const EDslSymbolKind symbol_kind = predicate_join
			? join_symbols[i]
			: (EdslopSort == kind && EdslsortSpec == sort
				? EdslsymOrder
				: CDSLOpKindTable::EsymkindAt(kind, i));
		const std::string name(1, CDSLOpKindTable::WcSymPrefix(symbol_kind));
		const std::string indexed = name + std::to_string(symbol_counts[symbol_kind]++);
		symbols->Append(GPOS_NEW(mp) CDSLSymbol(mp, symbol_kind,
			indexed.c_str(), (*symbol_id)++, EdslsideSource));
	}
	return GPOS_NEW(mp) CDSLOp(mp, kind, distinct, sort,
		EdslaggfuncUnknown, symbols, children);
}

CDSLOp *
PopSlice(CMemoryPool *mp, const CExpression *expr, const std::string &path,
	const std::unordered_set<std::string> &cuts,
	std::unordered_set<std::string> *used_cuts, ULONG *symbol_counts,
	ULONG *symbol_id, std::string *error)
{
	if (cuts.end() != cuts.find(path))
	{
		used_cuts->insert(path);
		return PopInput(mp, symbol_counts, symbol_id);
	}
	const auto relational = CDSLPlanTemplate::RelationalChildren(expr);
	if (relational.empty())
		return PopInput(mp, symbol_counts, symbol_id);
	if (COperator::EopLogicalLimit == expr->Pop()->Eopid())
	{
		CDSLMatchView::SOrderLimit view;
		if (1 != relational.size() ||
			!CDSLMatchView::FOrderLimit(const_cast<CExpression *>(expr), &view))
		{
			*error = "no canonical DSL view for " + path + " (" +
				expr->Pop()->SzId() + ")";
			return nullptr;
		}
		CDSLOp *child = PopSlice(mp, relational[0], path + "/0", cuts,
			used_cuts, symbol_counts, symbol_id, error);
		if (nullptr == child)
			return nullptr;

		if (!view.m_pos->IsEmpty())
		{
			// Capture the complete order, including NULL placement and comparator
			// identity. SortBy also composes with typed expression bindings.
			CDSLOpArray *sort_children = GPOS_NEW(mp) CDSLOpArray(mp);
			sort_children->Append(child);
			child = PopOperator(mp, EdslopSort, false, EdslsortSpec,
				sort_children, symbol_counts, symbol_id);
		}
		if (!view.m_fHasLimit)
		{
			if (!view.m_pos->IsEmpty())
				return child;
			child->Release();
			*error = "empty LogicalLimit has no DSL view at " + path;
			return nullptr;
		}
		CDSLOpArray *limit_children = GPOS_NEW(mp) CDSLOpArray(mp);
		limit_children->Append(child);
		return PopOperator(mp, EdslopLimit, false, EdslsortNone,
			limit_children, symbol_counts, symbol_id);
	}
	if (COperator::EopLogicalSequenceProject == expr->Pop()->Eopid())
	{
		if (1 != relational.size())
		{
			*error = "canonical DSL arity mismatch at " + path;
			return nullptr;
		}
		CDSLOp *child = PopSlice(mp, relational[0], path + "/0", cuts,
			used_cuts, symbol_counts, symbol_id, error);
		if (nullptr == child)
			return nullptr;
		CDSLOpArray *children = GPOS_NEW(mp) CDSLOpArray(mp);
		children->Append(child);
		BOOL distinct = false;
		const EDslOpKind kind = CanonicalKind(expr, &distinct);
		return PopOperator(mp, kind, false, EdslsortNone, children,
			symbol_counts, symbol_id);
	}
	if ((COperator::EopLogicalUnion == expr->Pop()->Eopid() ||
		 COperator::EopLogicalUnionAll == expr->Pop()->Eopid()) &&
		relational.size() > 2)
	{
		std::vector<CDSLOp *> branches;
		for (ULONG i = 0; i < relational.size(); ++i)
		{
			CDSLOp *branch = PopSlice(mp, relational[i], path + "/" +
				std::to_string(i), cuts, used_cuts, symbol_counts, symbol_id, error);
			if (nullptr == branch)
			{
				for (CDSLOp *built : branches)
					built->Release();
				return nullptr;
			}
			branches.push_back(branch);
		}
		CDSLOp *right = branches.back();
		for (ULONG i = branches.size() - 1; i > 0; --i)
		{
			CDSLOpArray *children = GPOS_NEW(mp) CDSLOpArray(mp);
			children->Append(branches[i - 1]);
			children->Append(right);
			right = PopOperator(mp, EdslopUnion,
				1 == i && COperator::EopLogicalUnion == expr->Pop()->Eopid(),
				EdslsortNone, children, symbol_counts, symbol_id);
		}
		return right;
	}
	if (COperator::EopLogicalSelect == expr->Pop()->Eopid() &&
		relational.size() > 1)
	{
		const EDslOpKind candidates[] = {
			EdslopExists, EdslopNotExists, EdslopAny, EdslopAll};
		for (const EDslOpKind kind : candidates)
		{
			ULONG local_counts[EdslsymSentinel];
			for (ULONG i = 0; i < EdslsymSentinel; ++i)
				local_counts[i] = symbol_counts[i];
			ULONG local_id = *symbol_id;
			std::unordered_set<std::string> local_cuts = *used_cuts;
			CDSLOpArray *children = GPOS_NEW(mp) CDSLOpArray(mp);
			for (ULONG i = 0; i < 2; ++i)
			{
				CDSLOp *child = PopSlice(mp, relational[i], path + "/" +
					std::to_string(i), cuts, &local_cuts, local_counts,
					&local_id, error);
				if (nullptr == child)
				{
					children->Release();
					return nullptr;
				}
				children->Append(child);
			}
			CDSLOp *candidate = PopOperator(mp, kind, false, EdslsortNone,
				children, local_counts, &local_id);
			CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
			CDSLMatcher matcher(mp);
			const BOOL matched = matcher.FMatch(candidate,
				const_cast<CExpression *>(expr), model);
			model->Release();
			if (matched)
			{
				for (ULONG i = 0; i < EdslsymSentinel; ++i)
					symbol_counts[i] = local_counts[i];
				*symbol_id = local_id;
				*used_cuts = std::move(local_cuts);
				error->clear();
				return candidate;
			}
			candidate->Release();
		}
		// A subquery under a Boolean expression need not be a relational
		// semi/anti filter. Fall through to the scalar-aware Filter template;
		// FSlice still rejects unrepresented cuts and validates every capture.
	}
	BOOL distinct = false;
	const EDslOpKind kind = CanonicalKind(expr, &distinct);
	if (EdslopSentinel == kind)
	{
		*error = "no canonical DSL view for " + path + " (" + expr->Pop()->SzId() + ")";
		return nullptr;
	}
	const ULONG child_count = CDSLOpKindTable::UlChildren(kind);
	if (relational.size() < child_count ||
		((EdslopUnion == kind || EdslopIntersect == kind || EdslopExcept == kind) &&
		 relational.size() != child_count))
	{
		*error = "canonical DSL arity mismatch at " + path;
		return nullptr;
	}
	CDSLOpArray *children = GPOS_NEW(mp) CDSLOpArray(mp);
	for (ULONG i = 0; i < child_count; ++i)
	{
		CDSLOp *child = PopSlice(mp, relational[i], path + "/" +
			std::to_string(i), cuts, used_cuts, symbol_counts, symbol_id, error);
		if (nullptr == child)
		{
			children->Release();
			return nullptr;
		}
		children->Append(child);
	}
	return PopOperator(mp, kind, distinct, EdslsortNone, children,
		symbol_counts, symbol_id);
}

std::string
SymbolText(CMemoryPool *mp, const CDSLSymbol *symbol)
{
	CHAR *text = CUtils::CreateMultiByteCharStringFromWCString(
		mp, const_cast<WCHAR *>(symbol->PstrName()->GetBuffer()));
	const std::string result(text);
	GPOS_DELETE_ARRAY(text);
	return result;
}

std::string
TemplateText(CMemoryPool *mp, const CDSLOp *op, BOOL print_children = true)
{
	CWStringDynamic printed(mp);
	COstreamString os(&printed);
	op->OsPrint(os, print_children);
	CHAR *text = CUtils::CreateMultiByteCharStringFromWCString(
		mp, const_cast<WCHAR *>(printed.GetBuffer()));
	const std::string result(text);
	GPOS_DELETE_ARRAY(text);
	return result;
}

std::string
ValueTemplate(CMemoryPool *mp, const CExpression *expr, ULONG *symbol_counts,
	BOOL *expanded);

std::string
PredicateTemplate(CMemoryPool *mp, const CExpression *expr, ULONG *symbol_counts,
	BOOL *expanded)
{
	GPOS_CHECK_STACK_SIZE;
	const BOOL negated_exists = COperator::EopScalarSubqueryNotExists == expr->Pop()->Eopid();
	if ((COperator::EopScalarSubqueryAny == expr->Pop()->Eopid() ||
		 COperator::EopScalarSubqueryAll == expr->Pop()->Eopid()) && 2 == expr->Arity())
	{
		*expanded = true;
		const std::string head = "c" + std::to_string(symbol_counts[EdslsymCompareHead]++);
		const std::string arguments = "Args(" + ValueTemplate(mp, (*expr)[1], symbol_counts, expanded) + ",Args())";
		const std::string output = "a" + std::to_string(symbol_counts[EdslsymAttrs]++);
		const std::string query = "t" + std::to_string(symbol_counts[EdslsymTable]++);
		return std::string(COperator::EopScalarSubqueryAny == expr->Pop()->Eopid() ? "Any(" : "All(") +
			head + ',' + arguments + ',' + output + ',' + query + ')';
	}
	if ((COperator::EopScalarSubqueryExists == expr->Pop()->Eopid() || negated_exists) &&
		1 == expr->Arity() && (*expr)[0]->Pop()->FLogical())
	{
		*expanded = true;
		const std::string exists = "Exists(t" + std::to_string(symbol_counts[EdslsymTable]++) + ')';
		return negated_exists ? "Not(" + exists + ')' : exists;
	}
	if (((COperator::EopScalarIf == expr->Pop()->Eopid() && 3 == expr->Arity()) ||
		(COperator::EopScalarIdent == expr->Pop()->Eopid() && 0 == expr->Arity()) ||
		(COperator::EopScalarSubquery == expr->Pop()->Eopid() && 1 == expr->Arity()) ||
		CDSLMatchView::FScalarCall(expr)) &&
		IMDType::EtiBool == COptCtxt::PoctxtFromTLS()->Pmda()->RetrieveType(
			CScalar::PopConvert(expr->Pop())->MdidType())->GetDatumType())
	{
		*expanded = true;
		return "ValueBool(" + ValueTemplate(mp, expr, symbol_counts, expanded) + ')';
	}
	CColRefArray *left = nullptr;
	CColRefArray *right = nullptr;
	if (CDSLMatchView::FNullSafeEqColumns(mp, expr, &left, &right))
	{
		left->Release();
		right->Release();
		*expanded = true;
		const ULONG first = symbol_counts[EdslsymAttrs]++;
		const ULONG second = symbol_counts[EdslsymAttrs]++;
		return "NullSafeEq(a" + std::to_string(first) + ",a" +
			std::to_string(second) + ')';
	}
	const BOOL is_not = 1 == expr->Arity() &&
		CUtils::FScalarBoolOp(const_cast<CExpression *>(expr), CScalarBoolOp::EboolopNot);
	const BOOL is_not_true = 1 == expr->Arity() &&
		COperator::EopScalarBooleanTest == expr->Pop()->Eopid() &&
		CScalarBooleanTest::EbtIsNotTrue == CScalarBooleanTest::PopConvert(expr->Pop())->Ebt();
	const BOOL is_and = 2 == expr->Arity() &&
		CUtils::FScalarBoolOp(const_cast<CExpression *>(expr), CScalarBoolOp::EboolopAnd);
	const BOOL is_or = 2 == expr->Arity() &&
		CUtils::FScalarBoolOp(const_cast<CExpression *>(expr), CScalarBoolOp::EboolopOr);
	if (!is_not && !is_not_true && !is_and && !is_or)
		return "p" + std::to_string(symbol_counts[EdslsymPred]++);
	*expanded = true;
	std::string result = is_not ? "Not(" : is_not_true ? "NotTrue(" : is_and ? "And(" : "Or(";
	for (ULONG i = 0; i < expr->Arity(); ++i)
	{
		if (i)
			result += ',';
		result += PredicateTemplate(mp, (*expr)[i], symbol_counts, expanded);
	}
	return result + ')';
}

std::string
ValueTemplate(CMemoryPool *mp, const CExpression *expr, ULONG *symbol_counts,
	BOOL *expanded)
{
	GPOS_CHECK_STACK_SIZE;
	if (COperator::EopScalarIdent == expr->Pop()->Eopid() && 0 == expr->Arity())
	{
		*expanded = true;
		return "Column(a" + std::to_string(symbol_counts[EdslsymAttrs]++) + ')';
	}
	if (COperator::EopScalarSubquery == expr->Pop()->Eopid() && 1 == expr->Arity())
	{
		*expanded = true;
		const std::string output = "a" + std::to_string(symbol_counts[EdslsymAttrs]++);
		return "Subquery(" + output + ",t" + std::to_string(symbol_counts[EdslsymTable]++) + ')';
	}
	if (CDSLMatchView::FScalarCall(expr))
	{
		*expanded = true;
		const std::string head = "h" + std::to_string(symbol_counts[EdslsymCallHead]++);
		std::vector<std::string> values;
		for (ULONG i = 0; i < expr->Arity(); ++i)
			values.push_back(ValueTemplate(mp, (*expr)[i], symbol_counts, expanded));
		std::string arguments = "Args()";
		for (auto it = values.rbegin(); it != values.rend(); ++it)
			arguments = "Args(" + *it + ',' + arguments + ')';
		return "Call(" + head + ',' + arguments + ')';
	}
	if (COperator::EopScalarIf == expr->Pop()->Eopid() && 3 == expr->Arity())
	{
		// Keep the ordered, lazy arms explicit. Unknown leaves remain captures.
		std::string result = "Case(" + PredicateTemplate(mp, (*expr)[0], symbol_counts, expanded);
		for (ULONG i = 1; i < 3; ++i)
			result += ',' + ValueTemplate(mp, (*expr)[i], symbol_counts, expanded);
		return result + ')';
	}
	const auto *type = COptCtxt::PoctxtFromTLS()->Pmda()->RetrieveType(
		CScalar::PopConvert(expr->Pop())->MdidType());
	if (IMDType::EtiBool == type->GetDatumType())
		return "BoolValue(" + PredicateTemplate(mp, expr, symbol_counts, expanded) + ')';
	return "n" + std::to_string(symbol_counts[EdslsymScalar]++);
}

// Follow the selected relational frontier, never inspect inside an Input cut.
// Unsupported scalar subtrees stay opaque occurrences, not guessed semantics.
BOOL
FExpressionTemplate(CMemoryPool *mp, const CDSLOp *op,
	const CExpression *expr, ULONG *symbol_counts, BOOL *expanded, std::string *text)
{
	GPOS_CHECK_STACK_SIZE;
	if (EdslopInput == op->Edslop())
	{
		*text = TemplateText(mp, op);
		return true;
	}
	if (COperator::EopLogicalLimit == expr->Pop()->Eopid())
	{
		CDSLMatchView::SOrderLimit view;
		if (!CDSLMatchView::FOrderLimit(const_cast<CExpression *>(expr), &view))
			return false;
		const CDSLOp *child = op;
		std::string prefix;
		ULONG wrappers = 0;
		// Consume exactly this fused node's wrappers, not a nested native Limit.
		for (const EDslOpKind kind : {EdslopLimit, EdslopSort})
		{
			if (kind == EdslopLimit ? !view.m_fHasLimit : view.m_pos->IsEmpty())
				continue;
			if (child->Edslop() != kind || child->UlChildren() != 1)
				return false;
			prefix += TemplateText(mp, child, false) + "(";
			child = (*child)[0];
			++wrappers;
		}
		if (0 == wrappers)
			return false;
		std::string input;
		if (!FExpressionTemplate(mp, child, view.m_pexprChild, symbol_counts, expanded, &input))
			input = TemplateText(mp, child);
		*text = prefix + input + std::string(wrappers, ')');
		return true;
	}
	const EDslOpKind kind = op->Edslop();
	const BOOL apply = EdslopInnerApply == kind || EdslopLeftOuterApply == kind ||
		EdslopSemiApply == kind || EdslopAntiApply == kind;
	const BOOL join = EdslopInnerJoin == kind || EdslopLeftJoin == kind ||
		EdslopFullJoin == kind || EdslopSemiJoin == kind || EdslopAntiJoin == kind || apply;
	const BOOL project = EdslopCompute == kind;
	const BOOL expand_scalar = join || project || EdslopFilter == kind;
	BOOL distinct = false;
	if (CanonicalKind(expr, &distinct) != kind)
		return false;
	const ULONG children = op->UlChildren();
	if (expand_scalar && children + 1 != expr->Arity())
		return false;
	if (!expand_scalar)
	{
		// Only traverse a one-to-one native/canonical frontier. Fused or folded
		// views need their own mapping; do not infer it from matching op names.
		const auto relational = CDSLPlanTemplate::RelationalChildren(expr);
		if (relational.size() != children)
			return false;
		for (ULONG i = 0; i < children; ++i)
			if (relational[i] != (*expr)[i])
				return false;
	}
	if (project && (COperator::EopScalarProjectList != (*expr)[1]->Pop()->Eopid() ||
		(*expr)[1]->DeriveHasNonScalarFunction()))
		return false;
	BOOL local = false;
	if (expand_scalar)
	{
		CColRefSet *available = GPOS_NEW(mp) CColRefSet(mp);
		for (ULONG i = 0; i < children; ++i)
			available->Union((*expr)[i]->DeriveOutputColumns());
		local = available->ContainsAll((*expr)[children]->DeriveUsedColumns());
		available->Release();
		if (!local && EdslopFilter != kind && !project)
			return false;
	}
	std::string inputs;
	for (ULONG i = 0; i < children; ++i)
	{
		std::string child;
		// Missing scalar expansion for a child is not a reason to hide the
		// parent's expressions. Keep its canonical relational view intact;
		// FSlice validates the complete mixed template with the real matcher.
		if (!FExpressionTemplate(mp, (*op)[i], (*expr)[i], symbol_counts, expanded, &child))
			child = TemplateText(mp, (*op)[i]);
		if (i)
			inputs += ',';
		inputs += child;
	}
	if (!expand_scalar)
	{
		*text = TemplateText(mp, op, false);
		if (0 < children)
			*text += "(" + inputs + ")";
		return true;
	}
	if (project)
	{
		// LogicalProject appends definitions while retaining its input columns.
		// Expanding its values must preserve Compute, not turn it into SELECT.
		// Outer references are valid; FSlice's production matcher checks fresh
		// outputs and sibling independence, and captures the original scope.
		// Unsupported value internals remain typed scalar captures, not guesses.
		std::string list;
		for (ULONG i = 0; i < (*expr)[1]->Arity(); ++i)
		{
			const CExpression *item = (*(*expr)[1])[i];
			if (COperator::EopScalarProjectElement != item->Pop()->Eopid() || 1 != item->Arity())
				return false;
			const CExpression *value = (*item)[0];
			list += "Item(" + ValueTemplate(mp, value, symbol_counts, expanded);
			list += ",a" + std::to_string(symbol_counts[EdslsymAttrs]++) + ',';
		}
		// Keep the slice width-polymorphic: this capture binds any remaining
		// items. A caller can close the list explicitly with Item().
		list += SymbolText(mp, (*op->Pdrgpsym())[0]);
		list.append((*expr)[1]->Arity(), ')');
		*text = "Compute<" + list + " " + SymbolText(mp, (*op->Pdrgpsym())[1]) + " " +
			SymbolText(mp, (*op->Pdrgpsym())[2]) + ">(" + inputs + ")";
		*expanded = true;
		return true;
	}
	*text = std::string(CDSLOpKindTable::SzName(kind)) + "<" +
		PredicateTemplate(mp, (*expr)[children], symbol_counts, expanded);
	const ULONG dependencies = apply ? 3 : EdslopFilter == kind && !local ? 2 : children;
	for (ULONG i = 1; i <= dependencies; ++i)
		*text += " " + SymbolText(mp, (*op->Pdrgpsym())[i]);
	*text += ">(" + inputs + ")";
	return true;
}

// Snapshot the successful production model before its symbols/carriers die.
// A routed binding may contain a group-bound leaf rather than a complete tree.
// Do not derive properties on that leaf or populate the live Memo's caches.
BOOL
AppendBoundExpression(CMemoryPool *mp, std::ostringstream *out, const CExpression *expression)
{
	std::vector<const CExpression *> pending{expression};
	while (!pending.empty())
	{
		GPOS_CHECK_ABORT;
		const CExpression *node = pending.back();
		pending.pop_back();
		if (node->Pop()->FPattern() || (nullptr != node->Pgexpr() &&
			node->Arity() != node->Pgexpr()->Arity()))
		{
			*out << "null";
			return false;
		}
		for (ULONG i = 0; i < node->Arity(); ++i) pending.push_back((*node)[i]);
	}
	CAutoRef<UlongToColRefMap> columns(GPOS_NEW(mp) UlongToColRefMap(mp));
	CAutoRef<CExpression> copy(expression->PexprCopyWithRemappedColumns(mp, columns.Value(), false));
	*out << CDSLPlanTemplate::Serialize(mp, copy.Value());
	return true;
}

// Address source captures using the already-validated definition graph.
using SourcePaths = std::map<const CDSLSymbol *, std::vector<std::string>>;

void
CollectCapturePaths(const CDSLSymbol *symbol, const std::string &path,
	const CDSLExpressionDefinitions *definitions, SourcePaths *paths)
{
	GPOS_CHECK_ABORT;
	(*paths)[symbol].push_back(path);
	const auto *definition = nullptr == definitions ? nullptr : definitions->Pdef(symbol);
	if (nullptr == definition || CDSLExpressionDefinitions::EMatch != definition->Binding())
		return;
	for (ULONG i = 0; i < definition->Arity(); ++i)
		CollectCapturePaths(definition->PsymOperand(i), path + '/' +
			CDSLExpressionDefinitions::SzBindingName(definition->Edslexpr()) + ':' +
			std::to_string(i), definitions, paths);
}

void
CollectSourcePaths(const CDSLOp *op, const std::string &path,
	const CDSLExpressionDefinitions *definitions, SourcePaths *paths)
{
	ULONG ordinals[EdslsymSentinel] = {};
	for (ULONG i = 0; i < op->Pdrgpsym()->Size(); ++i)
	{
		const CDSLSymbol *symbol = (*op->Pdrgpsym())[i];
		CollectCapturePaths(symbol, path + '/' + CDSLOpKindTable::WcSymPrefix(symbol->Esymkind()) +
			':' + std::to_string(ordinals[symbol->Esymkind()]++), definitions, paths);
	}
	for (ULONG i = 0; i < op->UlChildren(); ++i)
		CollectSourcePaths((*op)[i], path + '/' + std::to_string(i), definitions, paths);
}

// Column arrays retain binding order, unlike the sets in plan column_facts.
// Unsupported payloads remain explicit; this does not validate a lowering.
std::string
SourceBindings(CMemoryPool *mp, const std::vector<const CDSLSymbol *> &symbols,
	const CDSLModel *model, const SourcePaths &paths)
{
	std::ostringstream out;
	BOOL complete = true;
	out << "{\"scope\":\"matched_source_symbols\",\"symbols\":[";
	for (ULONG i = 0; i < symbols.size(); ++i)
	{
		GPOS_CHECK_ABORT;
		const CDSLSymbol *symbol = symbols[i];
		CRefCount *value = model->PvalLookup(symbol);
		if (i) out << ',';
		out << "{\"symbol\":" << JsonString(SymbolText(mp, symbol))
			<< ",\"kind\":\"" << CDSLOpKindTable::WcSymPrefix(symbol->Esymkind())
			<< "\",\"bound\":" << (nullptr == value ? "false" : "true");
		CExpression *expression = dynamic_cast<CExpression *>(value);
		if (nullptr != expression)
		{
			out << ",\"expression\":";
			complete &= AppendBoundExpression(mp, &out, expression);
		}
		else if (nullptr != value && (EdslsymAttrs == symbol->Esymkind() ||
			EdslsymSchema == symbol->Esymkind() || EdslsymRank == symbol->Esymkind()))
		{
			out << ",\"columns\":";
			AppendColumns(mp, &out, static_cast<CColRefArray *>(value));
		}
		else if (nullptr != value && (EdslsymFunc == symbol->Esymkind() ||
			EdslsymValueList == symbol->Esymkind()))
		{
			const auto *expressions = static_cast<CExpressionArray *>(value);
			out << ",\"expressions\":[";
			for (ULONG j = 0; j < expressions->Size(); ++j)
			{
				if (j) out << ',';
				complete &= AppendBoundExpression(mp, &out, (*expressions)[j]);
			}
			out << ']';
		}
		else
		{
			// ponytail: order/frame/context payloads need their own lowering contract.
			out << ",\"unsupported_payload\":true";
			complete = false;
		}
		const auto locations = paths.find(symbol);
		out << ",\"source_paths\":[";
		if (locations != paths.end())
			for (ULONG j = 0; j < locations->second.size(); ++j)
				out << (j ? "," : "") << JsonString(locations->second[j]);
		else
			complete = false;
		out << ']';
		out << '}';
	}
	out << "],\"complete\":" << (complete ? "true" : "false") << '}';
	return out.str();
}
}  // namespace

std::string
CDSLPlanTemplate::SerializeCaptured(CMemoryPool *mp, const CExpression *expr)
{
	GPOS_ASSERT(nullptr != mp && nullptr != expr);
	std::ostringstream out;
	AppendBoundExpression(mp, &out, expr);
	return out.str();
}

std::string
CDSLPlanTemplate::MatchedSourceBindings(const CDSLRule *rule, const CDSLModel *model)
{
	GPOS_ASSERT(nullptr != rule && nullptr != model);
	std::vector<const CDSLSymbol *> symbols;
	for (ULONG i = 0; i < rule->PfragSrc()->Pdrgpsym()->Size(); ++i)
		symbols.push_back((*rule->PfragSrc()->Pdrgpsym())[i]);
	SourcePaths paths;
	CollectSourcePaths(rule->PfragSrc()->PopRoot(), "r", rule->Pexprdefs(), &paths);
	return SourceBindings(model->Pmp(), symbols, model, paths);
}

std::vector<const CExpression *>
CDSLPlanTemplate::RelationalChildren(const CExpression *expr)
{
	std::vector<const CExpression *> result;
	std::vector<const CExpression *> pending;
	for (ULONG child = expr->Arity(); child > 0; --child)
		pending.push_back((*expr)[child - 1]);
	while (!pending.empty())
	{
		const CExpression *input = pending.back();
		pending.pop_back();
		if (input->Pop()->FLogical())
		{
			result.push_back(input);
			continue;
		}
		for (ULONG child = input->Arity(); child > 0; --child)
			pending.push_back((*input)[child - 1]);
	}
	return result;
}

std::string
CDSLPlanTemplate::ExpressionShape(const CExpression *expr)
{
	std::map<std::string, ULONG> operators;
	std::vector<std::pair<const CExpression *, ULONG>> pending{{expr, 1}};
	ULONG nodes = 0, scalar = 0, patterns = 0, depth = 0;
	while (!pending.empty() && nodes < 4096)
	{
		const auto entry = pending.back();
		const std::string name(entry.first->Pop()->SzId());
		if (16 == operators.size() && operators.end() == operators.find(name))
			break;
		pending.pop_back();
		++operators[name];
		++nodes;
		scalar += entry.first->Pop()->FScalar() ? 1 : 0;
		patterns += entry.first->Pop()->FPattern() ? 1 : 0;
		depth = std::max(depth, entry.second);
		for (ULONG i = entry.first->Arity(); i > 0; --i)
			pending.emplace_back((*entry.first)[i - 1], entry.second + 1);
	}
	std::ostringstream out;
	out << "{\"complete\":" << (pending.empty() ? "true" : "false")
		<< ",\"nodes\":" << nodes << ",\"scalar_nodes\":" << scalar
		<< ",\"pattern_nodes\":" << patterns << ",\"depth\":" << depth
		<< ",\"operators\":{";
	BOOL first = true;
	for (const auto &entry : operators)
	{
		if (!first)
			out << ",";
		first = false;
		out << "\"" << entry.first << "\":" << entry.second;
	}
	out << "}}";
	return out.str();
}

void
CDSLPlanTemplate::AppendScalarContext(std::ostream &out, const COperator *op)
{
	if (const CHAR *kind = ScalarKind(op))
		out << ",\"scalar_kind\":\"" << kind << "\"";
	if (const auto *constant = dynamic_cast<const CScalarConst *>(op))
		ConstantContext(out, constant->GetDatum());
}

std::string
CDSLPlanTemplate::Serialize(CMemoryPool *mp, const CExpression *expr)
{
	GPOS_ASSERT(nullptr != mp && nullptr != expr);
	std::ostringstream out;
	out << "{\"schema\":\"pgorca.dsl.plan-template.v1\","
		   "\"path_semantics\":\"ordered_relational_children\","
		   "\"view_semantics\":\"dispatcher_candidates_validated_at_slice\","
		   "\"placeholder\":\"Input\",\"nodes\":[";
	std::vector<std::pair<const CExpression *, std::string>> pending{{expr, "r"}};
	BOOL first_node = true;
	while (!pending.empty())
	{
		GPOS_CHECK_ABORT;
		const auto current = pending.back();
		pending.pop_back();
		const CExpression *node = current.first;
		const std::string &path = current.second;
		const auto relational = RelationalChildren(node);
		const auto views = DSLViews(node);
		if (!first_node)
			out << ",";
		first_node = false;
		out << "{\"path\":" << JsonString(path) << ",\"orca_operator\":"
			<< JsonString(node->Pop()->SzId()) << ",\"operator_text\":"
			<< JsonString(OperatorText(mp, node)) << ",\"dsl_views\":[";
		for (ULONG i = 0; i < views.size(); ++i)
		{
			if (i)
				out << ",";
			out << "\"" << views[i] << "\"";
		}
		out << "],\"requires_cut\":" << (views.empty() ? "true" : "false")
			<< ",\"relational_children\":[";
		for (ULONG i = 0; i < relational.size(); ++i)
		{
			if (i)
				out << ",";
			out << "\"" << path << "/" << i << "\"";
		}
		out << "],\"scalar_children\":[";
		BOOL first_scalar = true;
		for (ULONG i = 0; i < node->Arity(); ++i)
		{
			const CExpression *child = (*node)[i];
			if (!child->Pop()->FScalar())
				continue;
			if (!first_scalar)
				out << ",";
			first_scalar = false;
			out << "{\"position\":" << i << ",\"operator\":"
				<< JsonString(child->Pop()->SzId()) << ",\"shape\":"
				<< ExpressionShape(child) << ",\"tree\":[";
			AppendExpressionTree(mp, &out, child, "s" + std::to_string(i));
			out << "]}";
		}
		out << ']';
		AppendScalarContext(out, node->Pop());
		AppendColumnFacts(mp, &out, node);
		out << '}';
		for (ULONG i = relational.size(); i > 0; --i)
			pending.emplace_back(relational[i - 1], path + "/" + std::to_string(i - 1));
	}
	// One root scan, not one recursive scan per exported node. Unknown is null;
	// this retained-scope footprint is not a source/transport certificate.
	out << "],\"frame_reads\":";
	CColRefSet *frame = dslproperties::PcrsFrameReads(mp, const_cast<CExpression *>(expr));
	if (nullptr == frame)
		out << "null";
	else
	{
		AppendColumns(mp, &out, frame);
		frame->Release();
	}
	out << ",\"complete\":true}";
	return out.str();
}

BOOL
CDSLPlanTemplate::FValidateSelection(
	const CExpression *expr, const std::string &root_path,
	const std::vector<std::string> &cut_paths, std::string *error)
{
	GPOS_ASSERT(nullptr != expr);
	const auto paths = RelationalPaths(expr);
	if (paths.end() == paths.find(root_path))
	{
		if (nullptr != error)
			*error = "root path does not exist";
		return false;
	}
	std::unordered_set<std::string> cuts;
	for (const std::string &cut : cut_paths)
	{
		if (paths.end() == paths.find(cut))
		{
			if (nullptr != error)
				*error = "cut path does not exist: " + cut;
			return false;
		}
		if (!FPathWithin(cut, root_path))
		{
			if (nullptr != error)
				*error = "cut path is outside selected root: " + cut;
			return false;
		}
		if (!cuts.insert(cut).second)
		{
			if (nullptr != error)
				*error = "duplicate cut path: " + cut;
			return false;
		}
	}
	for (const std::string &left : cuts)
		for (const std::string &right : cuts)
			if (left != right && FPathWithin(left, right))
			{
				if (nullptr != error)
					*error = "cut paths must form an antichain";
				return false;
			}
	if (nullptr != error)
		error->clear();
	return true;
}

BOOL
CDSLPlanTemplate::FSlice(
	CMemoryPool *mp, CExpression *expr, const std::string &root_path,
	const std::vector<std::string> &cut_paths, std::string *dsl,
	std::string *error, std::string *source_bindings)
{
	GPOS_ASSERT(nullptr != mp && nullptr != expr && nullptr != dsl && nullptr != error);
	if (nullptr != source_bindings) source_bindings->clear();
	if (!FValidateSelection(expr, root_path, cut_paths, error))
		return false;
	CExpression *root = const_cast<CExpression *>(PexprAtPath(expr, root_path));
	GPOS_ASSERT(nullptr != root);
	std::unordered_set<std::string> cuts(cut_paths.begin(), cut_paths.end());
	std::unordered_set<std::string> used_cuts;
	ULONG symbol_counts[EdslsymSentinel] = {};
	ULONG symbol_id = 0;
	CDSLOp *source = PopSlice(mp, root, root_path, cuts, &used_cuts,
		symbol_counts, &symbol_id, error);
	if (nullptr == source)
		return false;
	if (used_cuts.size() != cuts.size())
	{
		source->Release();
		*error = "a cut path is not represented by the canonical DSL view";
		return false;
	}
	std::string expression_template;
	BOOL expanded = false;
	if (FExpressionTemplate(mp, source, root, symbol_counts,
		&expanded, &expression_template) && expanded)
	{
		// A temporary carrier rule exercises the real parser and matcher. It is
		// not an equivalence claim, is never registered and is never instantiated.
		const CDSLOp *input = source;
		while (0 != input->UlChildren())
			input = (*input)[0];
		GPOS_ASSERT(EdslopInput == input->Edslop());
		const std::string target = "t" + std::to_string(symbol_counts[EdslsymTable]);
		const std::string carrier = expression_template + "|Input<" + target +
			">|" + target + " := " + SymbolText(mp, (*input->Pdrgpsym())[0]);
		CWStringDynamic parse_error(mp);
		CDSLRule *rule = CDSLRuleParser::PdslruleParse(mp, carrier.c_str(), nullptr, &parse_error);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher matcher(mp, rule);
		const BOOL matched = nullptr != rule &&
			matcher.FMatch(rule->PfragSrc()->PopRoot(), root, model);
		if (matched && nullptr != source_bindings)
		{
			*source_bindings = MatchedSourceBindings(rule, model);
		}
		model->Release();
		CRefCount::SafeRelease(rule);
		source->Release();
		if (!matched)
		{
			*error = "expression template failed production parser/matcher validation";
			return false;
		}
		*dsl = expression_template;
		error->clear();
		return true;
	}
	CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
	CDSLMatcher matcher(mp);
	const BOOL matched = matcher.FMatch(source, root, model);
	if (matched && nullptr != source_bindings)
	{
		std::vector<const CDSLSymbol *> symbols;
		std::vector<const CDSLOp *> pending{source};
		while (!pending.empty())
		{
			const CDSLOp *node = pending.back();
			pending.pop_back();
			for (ULONG i = 0; i < node->Pdrgpsym()->Size(); ++i)
				symbols.push_back((*node->Pdrgpsym())[i]);
			for (ULONG i = node->UlChildren(); i > 0; --i) pending.push_back((*node)[i - 1]);
		}
		SourcePaths paths;
		CollectSourcePaths(source, "r", nullptr, &paths);
		*source_bindings = SourceBindings(mp, symbols, model, paths);
	}
	model->Release();
	if (!matched)
	{
		source->Release();
		*error = "canonical DSL view does not match selected source";
		return false;
	}
	*dsl = TemplateText(mp, source);
	source->Release();
	error->clear();
	return true;
}

std::string
CDSLPlanTemplate::SliceArtifact(
	CMemoryPool *mp, CExpression *expr, const std::string &root_path,
	const std::vector<std::string> &cut_paths, BOOL source_bindings)
{
	std::string dsl, error, bindings;
	const BOOL ok = FSlice(mp, expr, root_path, cut_paths, &dsl, &error,
		source_bindings ? &bindings : nullptr);
	std::ostringstream out;
	out << "{\"schema\":\"pgorca.dsl.plan-slice.v1\",\"root_path\":"
		<< JsonString(root_path) << ",\"cut_paths\":[";
	for (ULONG i = 0; i < cut_paths.size(); ++i)
	{
		if (i)
			out << ",";
		out << JsonString(cut_paths[i]);
	}
	out << "],\"status\":\"" << (ok ? "ok" : "error") << "\",";
	if (ok)
		out << "\"source_template\":" << JsonString(dsl) << ",\"error\":null";
	else
		out << "\"source_template\":null,\"error\":" << JsonString(error);
	if (source_bindings) out << ",\"source_bindings\":" << (ok ? bindings : "null");
	out << '}';
	return out.str();
}
