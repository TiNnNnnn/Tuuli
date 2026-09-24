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

#include "gpos/io/COstreamString.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/dsl/CDSLEnums.h"
#include "gpopt/dsl/CDSLMatchView.h"
#include "gpopt/dsl/CDSLMatcher.h"
#include "gpopt/dsl/CDSLModel.h"
#include "gpopt/dsl/CDSLRule.h"
#include "gpopt/dsl/CDSLRuleParser.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalSequenceProject.h"
#include "gpopt/operators/CScalarBoolOp.h"
#include "gpopt/operators/CScalarBooleanTest.h"
#include "gpopt/operators/CScalarProjectElement.h"

using namespace gpopt;

namespace
{
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
			<< ",\"arity\":" << current.first->Arity() << "}";
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
		case COperator::EopLogicalInnerJoin: return EdslopInnerJoin;
		case COperator::EopLogicalLeftOuterJoin: return EdslopLeftJoin;
		case COperator::EopLogicalFullOuterJoin: return EdslopFullJoin;
		case COperator::EopLogicalLeftSemiJoin: return EdslopSemiJoin;
		case COperator::EopLogicalLeftSemiApply: return EdslopSemiApply;
		case COperator::EopLogicalLeftAntiSemiJoin: return EdslopAntiJoin;
		case COperator::EopLogicalLeftAntiSemiApply: return EdslopAntiApply;
		case COperator::EopLogicalLeftAntiSemiJoinNotIn: return EdslopAntiJoinNotIn;
		case COperator::EopLogicalLeftAntiSemiApplyNotIn: return EdslopAntiApplyNotIn;
		case COperator::EopLogicalInnerApply: return EdslopInnerApply;
		case COperator::EopLogicalLeftOuterApply: return EdslopLeftOuterApply;
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
			EDslSortDir direction =
				CDSLMatchView::EdslsortDefault(view.m_pos);
			if (EdslsortNone == direction)
				direction = EdslsortSpec;
			CDSLOpArray *sort_children = GPOS_NEW(mp) CDSLOpArray(mp);
			sort_children->Append(child);
			child = PopOperator(mp, EdslopSort, false, direction,
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
		const EDslOpKind kind =
			CLogicalSequenceProject::PopConvert(expr->Pop())->FHasFrameSpecs()
			? EdslopWindowFrame
			: EdslopWindowRows;
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
		*error = "no canonical subquery DSL view for " + path;
		return nullptr;
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
ValueTemplate(CMemoryPool *mp, const CExpression *expr, ULONG *symbol_counts,
	BOOL *expanded);

std::string
PredicateTemplate(CMemoryPool *mp, const CExpression *expr, ULONG *symbol_counts,
	BOOL *expanded)
{
	GPOS_CHECK_STACK_SIZE;
	if (((COperator::EopScalarIf == expr->Pop()->Eopid() && 3 == expr->Arity()) ||
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
	const CExpression *expr, ULONG *symbol_counts, BOOL *expanded, std::string *text,
	std::string *input)
{
	GPOS_CHECK_STACK_SIZE;
	if (EdslopInput == op->Edslop())
	{
		*input = SymbolText(mp, (*op->Pdrgpsym())[0]);
		*text = "Input<" + *input + ">";
		return true;
	}
	const EDslOpKind kind = op->Edslop();
	const BOOL join = EdslopInnerJoin == kind || EdslopLeftJoin == kind ||
		EdslopFullJoin == kind || EdslopSemiJoin == kind || EdslopAntiJoin == kind;
	const BOOL project = EdslopCompute == kind;
	BOOL distinct = false;
	if ((!join && !project && EdslopFilter != kind) || CanonicalKind(expr, &distinct) != kind)
		return false;
	const ULONG children = join ? 2 : 1;
	if (children + 1 != expr->Arity() || (*expr)[children]->DeriveHasSubquery())
		return false;
	if (project && (COperator::EopScalarProjectList != (*expr)[1]->Pop()->Eopid() ||
		(*expr)[1]->DeriveHasNonScalarFunction()))
		return false;
	CColRefSet *available = GPOS_NEW(mp) CColRefSet(mp);
	for (ULONG i = 0; i < children; ++i)
		available->Union((*expr)[i]->DeriveOutputColumns());
	const BOOL local = available->ContainsAll((*expr)[children]->DeriveUsedColumns());
	available->Release();
	if (!local)
		return false;
	std::string inputs;
	for (ULONG i = 0; i < children; ++i)
	{
		std::string child;
		if (!FExpressionTemplate(mp, (*op)[i], (*expr)[i], symbol_counts, expanded, &child, input))
			return false;
		if (i)
			inputs += ',';
		inputs += child;
	}
	if (project)
	{
		// Only independent SELECT items belong to Proj: the local dependency
		// check above excludes sequential LET references to earlier definitions.
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
		// The last capture binds the remaining list (empty in this instance),
		// rather than inventing an empty-list constructor or fixing query width.
		list += SymbolText(mp, (*op->Pdrgpsym())[0]);
		list.append((*expr)[1]->Arity(), ')');
		*text = "Proj<" + SymbolText(mp, (*op->Pdrgpsym())[1]) + " " +
			SymbolText(mp, (*op->Pdrgpsym())[2]) + " " + list + ">(" + inputs + ")";
		*expanded = true;
		return true;
	}
	*text = std::string(CDSLOpKindTable::SzName(kind)) + "<" +
		PredicateTemplate(mp, (*expr)[children], symbol_counts, expanded);
	for (ULONG i = 1; i <= children; ++i)
		*text += " " + SymbolText(mp, (*op->Pdrgpsym())[i]);
	*text += ">(" + inputs + ")";
	return true;
}
}  // namespace

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
		out << "]}";
		for (ULONG i = relational.size(); i > 0; --i)
			pending.emplace_back(relational[i - 1], path + "/" + std::to_string(i - 1));
	}
	out << "],\"complete\":true}";
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
	std::string *error)
{
	GPOS_ASSERT(nullptr != mp && nullptr != expr && nullptr != dsl && nullptr != error);
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
	std::string expression_template, input;
	BOOL expanded = false;
	if (FExpressionTemplate(mp, source, root, symbol_counts,
		&expanded, &expression_template, &input) && expanded)
	{
		// A temporary carrier rule exercises the real parser and matcher. It is
		// not an equivalence claim, is never registered and is never instantiated.
		const std::string target = "t" + std::to_string(symbol_counts[EdslsymTable]);
		const std::string carrier = expression_template + "|Input<" + target +
			">|" + target + " := " + input;
		CWStringDynamic parse_error(mp);
		CDSLRule *rule = CDSLRuleParser::PdslruleParse(mp, carrier.c_str(), nullptr, &parse_error);
		CDSLModel *model = GPOS_NEW(mp) CDSLModel(mp);
		CDSLMatcher matcher(mp, rule);
		const BOOL matched = nullptr != rule &&
			matcher.FMatch(rule->PfragSrc()->PopRoot(), root, model);
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
	model->Release();
	if (!matched)
	{
		source->Release();
		*error = "canonical DSL view does not match selected source";
		return false;
	}
	CWStringDynamic printed(mp);
	COstreamString os(&printed);
	source->OsPrint(os);
	CHAR *text = CUtils::CreateMultiByteCharStringFromWCString(
		mp, const_cast<WCHAR *>(printed.GetBuffer()));
	*dsl = text;
	GPOS_DELETE_ARRAY(text);
	source->Release();
	error->clear();
	return true;
}

std::string
CDSLPlanTemplate::SliceArtifact(
	CMemoryPool *mp, CExpression *expr, const std::string &root_path,
	const std::vector<std::string> &cut_paths)
{
	std::string dsl, error;
	const BOOL ok = FSlice(mp, expr, root_path, cut_paths, &dsl, &error);
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
		out << "\"source_template\":" << JsonString(dsl) << ",\"error\":null}";
	else
		out << "\"source_template\":null,\"error\":" << JsonString(error) << "}";
	return out.str();
}
