#!/usr/bin/env python3
"""Merge the runtime native-xform registry with causal E2E evidence."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any

from run_e2e_cases import validate_replacement_matrix


STATUS_RANK = {
    "unassessed": 0,
    "expressible": 1,
    "verified_partial_replacement": 2,
    "verified_full_replacement": 3,
}


def read_matrices(expect_dir: Path) -> list[dict[str, Any]]:
    matrices = []
    for path in sorted(expect_dir.glob("*.expect")):
        expectation = json.loads(path.read_text(encoding="utf-8"))
        if "replacement" not in expectation:
            continue
        validate_replacement_matrix(expectation)
        replacement = expectation["replacement"]
        status = replacement.get("status")
        if status not in STATUS_RANK:
            raise ValueError(f"{path}: invalid replacement status: {status!r}")
        rules = replacement.get("dsl_rules", [])
        if (
            not isinstance(rules, list)
            or any(not isinstance(rule_id, int) or rule_id <= 0 for rule_id in rules)
        ):
            raise ValueError(f"{path}: replacement.dsl_rules must be positive ids")
        rule_hashes = replacement.get("dsl_rule_hashes", [])
        if (
            not isinstance(rule_hashes, list)
            or any(
                not isinstance(rule_hash, str)
                or re.fullmatch(r"[0-9a-f]{16}", rule_hash) is None
                for rule_hash in rule_hashes
            )
        ):
            raise ValueError(
                f"{path}: replacement.dsl_rule_hashes must be 16-digit lowercase hex"
            )
        if not rules and not rule_hashes:
            raise ValueError(
                f"{path}: replacement must identify at least one DSL rule"
            )
        scope = replacement.get("scope")
        if not isinstance(scope, str) or not scope.strip():
            raise ValueError(f"{path}: replacement.scope must be non-empty")
        matrices.append(
            {
                "case": path.stem,
                "xforms": replacement["xforms"],
                "scope": scope,
                "status": status,
                "dsl_rules": rules,
                "dsl_rule_hashes": rule_hashes,
            }
        )
    return matrices


def merge_inventory(
    runtime: dict[str, Any], matrices: list[dict[str, Any]]
) -> dict[str, Any]:
    xforms = runtime.get("xforms")
    if not isinstance(xforms, list):
        raise ValueError("runtime audit has no xforms array")
    by_name = {entry.get("name"): entry for entry in xforms}
    if len(by_name) != len(xforms) or None in by_name:
        raise ValueError("runtime audit contains duplicate or unnamed xforms")

    result_xforms = []
    for source in xforms:
        entry = dict(source)
        category = entry.get("category")
        if category == "join_enumeration":
            entry["replacement_status"] = (
                "replaced_by_dphyper"
                if entry.get("replacement_owner") == "dphyper"
                else "retained_native_gap"
            )
        elif category == "implementation_property":
            entry["replacement_status"] = "retained_in_cascades"
        else:
            entry["replacement_status"] = "unassessed"
        entry["evidence"] = []
        result_xforms.append(entry)
    result_by_name = {entry["name"]: entry for entry in result_xforms}

    for matrix in matrices:
        for name in matrix["xforms"]:
            if name not in result_by_name:
                raise ValueError(
                    f"{matrix['case']}: replacement target is not a native "
                    f"exploration xform: {name}"
                )
            entry = result_by_name[name]
            if entry["category"] != "semantic_rewrite":
                raise ValueError(
                    f"{matrix['case']}: cannot attach DSL replacement evidence "
                    f"to {entry['category']} xform {name}"
                )
            evidence = {
                key: matrix.get(key, [] if key.startswith("dsl_rule") else None)
                for key in (
                    "case", "scope", "status", "dsl_rules", "dsl_rule_hashes"
                )
            }
            entry["evidence"].append(evidence)
            if STATUS_RANK[matrix["status"]] > STATUS_RANK[
                entry["replacement_status"]
            ]:
                entry["replacement_status"] = matrix["status"]

    summary = {
        "native_exploration_xforms": len(result_xforms),
        "semantic_rewrite_xforms": sum(
            entry["category"] == "semantic_rewrite" for entry in result_xforms
        ),
        "join_enumeration_xforms": sum(
            entry["category"] == "join_enumeration" for entry in result_xforms
        ),
        "implementation_property_xforms": sum(
            entry["category"] == "implementation_property"
            for entry in result_xforms
        ),
        "verified_partial_xforms": sum(
            entry["replacement_status"] == "verified_partial_replacement"
            for entry in result_xforms
        ),
        "verified_full_xforms": sum(
            entry["replacement_status"] == "verified_full_replacement"
            for entry in result_xforms
        ),
        "causal_matrix_cases": len(matrices),
    }
    return {
        "schema_version": 1,
        "summary": summary,
        "xform_sets": [matrix for matrix in matrices if len(matrix["xforms"]) > 1],
        "xforms": result_xforms,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("runtime_audit", type=Path)
    parser.add_argument("expect_dir", type=Path)
    parser.add_argument("output", type=Path)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    runtime = json.loads(args.runtime_audit.read_text(encoding="utf-8"))
    inventory = merge_inventory(runtime, read_matrices(args.expect_dir))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(inventory, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    print(json.dumps(inventory["summary"], ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
