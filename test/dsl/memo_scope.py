"""Bound the enclosing row columns of an inserted logical expression in one final Memo."""

from collections import defaultdict


# Apply evaluates its right relational child with the current left row.
_APPLY = {
    "CLogicalInnerApply", "CLogicalInnerCorrelatedApply",
    "CLogicalLeftOuterApply", "CLogicalLeftOuterCorrelatedApply",
    "CLogicalLeftSemiApply", "CLogicalLeftSemiCorrelatedApply",
    "CLogicalLeftSemiApplyIn", "CLogicalLeftSemiCorrelatedApplyIn",
    "CLogicalLeftAntiSemiApply", "CLogicalLeftAntiSemiCorrelatedApply",
    "CLogicalLeftAntiSemiApplyNotIn", "CLogicalLeftAntiSemiCorrelatedApplyNotIn",
}


def memo_frame_columns(records, rule_hash, candidate_sequence=None, *, require_root_frame=False):
    def of(kind):
        return [r for r in records if r["kind"] == kind]

    # A merged-away group may be omitted only when every live expression
    # is accounted for and every port points to a group in this snapshot.
    summaries = of("memo_summary")
    if len(summaries) != 1:
        raise ValueError("expected exactly one final Memo")
    summary = summaries[0]
    scopes = of("memo_scope_group")
    groups = {r["group"]: r for r in scopes}
    roots = [r["group"] for r in scopes if r["root"] is True]
    count = sum(len(r["alternatives"]) + r["physical"] for r in scopes)
    if (len(roots) != 1 or len(groups) != len(scopes) or len(groups) + summary["duplicate_groups"] != summary["groups"]
            or count != summary["group_expressions"] or any(r["stage"] != summary["stage"] for r in scopes)):
        raise ValueError("incomplete final Memo scope inventory")

    alternatives = {}
    logical = set()
    parents = defaultdict(list)
    producers = defaultdict(set)
    for group, record in groups.items():
        for expr in record["alternatives"]:
            key = (group, expr["id"])
            op = expr["operator"]
            if not op.startswith(("CLogical", "CScalar")):
                raise ValueError("unknown logical or scalar Memo operator")
            if key in alternatives or any(child not in groups for child in expr["children"]):
                raise ValueError("duplicate expression or missing child group")
            alternatives[key] = op
            if "CTE" in op:
                if (op not in ("CLogicalCTEAnchor", "CLogicalCTEConsumer", "CLogicalCTEProducer")
                        or type(expr.get("cte_id")) is not int or expr["cte_id"] < 0):
                    raise ValueError("missing or unknown CTE binding")
                if op == "CLogicalCTEProducer":
                    producers[expr["cte_id"]].add(group)
            if op.startswith("CLogical"):
                logical.add((*key, op))
            for port, child in enumerate(expr["children"]):
                parents[child].append((group, op, port, expr["children"]))
    inventory = of("memo_alternative")
    if (len(inventory) != len(logical) or any(r["stage"] != summary["stage"] for r in inventory)
            or {(r["group"], r["group_expression"], r["operator"]) for r in inventory} != logical):
        raise ValueError("logical alternatives disagree with final Memo scope records")

    reachable = set()
    pending = roots.copy()
    while pending:
        group = pending.pop()
        if group in reachable:
            continue
        reachable.add(group)
        for expr in groups[group]["alternatives"]:
            if expr["operator"] in ("CLogicalCTEAnchor", "CLogicalCTEConsumer"):
                definitions = producers.get(expr["cte_id"])
                if not definitions:
                    raise ValueError("reachable CTE has no producer in the final Memo")
                # Keep every possible definition path. Crossing one still needs
                # a proof of its enclosing frames and is rejected on the ascent.
                for producer in definitions:
                    parents[producer].append((group, expr["operator"], None, ()))
                    pending.append(producer)
            pending.extend(expr["children"])

    matches = [r for r in of("rule_candidate_outcome") if r.get("status") == "memo_inserted"
               and r["rule_hash"] == rule_hash
               and (candidate_sequence is None or r["candidate_sequence"] == candidate_sequence)]
    if not matches:
        raise ValueError("no inserted candidate for requested rule")
    candidates_by_id = defaultdict(list)
    for r in of("rule_candidate"):
        candidates_by_id[r["experiment"], r["sequence"]].append(r)
    pending = []
    for match in matches:
        group = match["group"]
        candidates = candidates_by_id[match["experiment"], match["candidate_sequence"]]
        if (len(candidates) != 1 or candidates[0]["rule_hash"] != rule_hash
                or candidates[0]["placement"] != "cbo" or candidates[0]["group"] != group
                or group not in reachable
                or not alternatives.get((group, candidates[0]["group_expression"]), "").startswith("CLogical")
                or not alternatives.get((group, match["group_expression"]), "").startswith("CLogical")):
            raise ValueError("cannot identify inserted candidate in final Memo")
        pending.append(group)

    columns = {}
    seen = set()
    while pending:
        child = pending.pop()
        if child in seen:
            continue
        seen.add(child)
        for parent, op, port, children in parents[child]:
            if parent not in reachable:
                continue
            frame = None
            if op in ("CLogicalInnerJoin", "CLogicalLeftOuterJoin", "CLogicalRightOuterJoin") and port in (0, 1):
                frame = children[1 - port]
            elif (op in _APPLY or op in ("CLogicalProject", "CLogicalSelect")) and port == 1:
                frame = children[0]
            elif not (
                (port == 0 and (op in _APPLY or op in ("CLogicalProject", "CLogicalSelect", "CLogicalLimit", "CLogicalGbAgg")))
                or (port == 0 and op in ("CScalarSubquery", "CScalarSubqueryAny", "CScalarSubqueryAll",
                                         "CScalarSubqueryExists", "CScalarSubqueryNotExists", "CScalarProjectElement"))
                or op in ("CScalarProjectList", "CScalarCmp")
            ):
                raise ValueError(f"unsupported enclosing scope at {op} child {port}")
            if frame is not None:
                # Even a zero-column input can introduce a row frame.
                if require_root_frame:
                    raise ValueError("candidate may enter a different frame from the Memo root")
                sibling = groups[frame]["output"]
                if sibling is None:
                    raise ValueError("missing enclosing row schema")
                for column in sibling:
                    if columns.setdefault(column["id"], column) != column:
                        raise ValueError("conflicting sibling column metadata")
            pending.append(parent)
    return [columns[i] for i in sorted(columns)]
