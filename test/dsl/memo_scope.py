"""Bound the enclosing row columns of an inserted logical expression in one final Memo."""

from collections import defaultdict


def memo_frame_columns(records, rule_hash, candidate_sequence=None):
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
    for group, record in groups.items():
        for expr in record["alternatives"]:
            key = (group, expr["id"])
            op = expr["operator"]
            if not op.startswith(("CLogical", "CScalar")):
                raise ValueError("unknown logical or scalar Memo operator")
            if key in alternatives or any(child not in groups for child in expr["children"]):
                raise ValueError("duplicate expression or missing child group")
            alternatives[key] = op
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
            if "CTE" in expr["operator"]:
                raise ValueError("scope across CTE producer and consumer requires their binding")
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
            if op in ("CLogicalInnerJoin", "CLogicalLeftOuterJoin", "CLogicalRightOuterJoin") and port < 2:
                sibling = groups[children[1 - port]]["output"]
                if sibling is None:
                    raise ValueError("missing sibling row schema")
                for column in sibling:
                    if columns.setdefault(column["id"], column) != column:
                        raise ValueError("conflicting sibling column metadata")
            elif op not in ("CLogicalProject", "CLogicalSelect", "CLogicalLimit") or port != 0:
                raise ValueError(f"unsupported enclosing scope at {op} child {port}")
            pending.append(parent)
    return [columns[i] for i in sorted(columns)]
