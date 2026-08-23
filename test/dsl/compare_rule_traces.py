#!/usr/bin/env python3
"""Compare WeTune and pgORCA rule-application traces.

Both plain JSONL emitted by WeTune's TraceRuleChain and PostgreSQL log output
containing `DSL_TRACE {...}` records are accepted. The comparison intentionally
does not require the same chain order or final plan: Cascades may discover more
applications than WeTune's bottom-up optimizer.
"""

from __future__ import annotations

import argparse
import json
import sys
from collections import Counter
from pathlib import Path
from typing import Any, Iterable


def read_records(path: Path) -> list[dict[str, Any]]:
    decoder = json.JSONDecoder()
    records: list[dict[str, Any]] = []
    with path.open(encoding="utf-8", errors="replace") as stream:
        for line_number, line in enumerate(stream, 1):
            marker = line.find("DSL_TRACE ")
            payload = line[marker + len("DSL_TRACE ") :] if marker >= 0 else line.lstrip()
            if not payload.startswith("{"):
                continue
            try:
                record, _ = decoder.raw_decode(payload)
            except json.JSONDecodeError as error:
                raise ValueError(f"{path}:{line_number}: invalid trace JSON: {error}") from error
            if isinstance(record, dict):
                records.append(record)
    return records


def applications(records: Iterable[dict[str, Any]]) -> list[dict[str, Any]]:
    return [record for record in records if record.get("kind") == "application"]


def keys_of(records: Iterable[dict[str, Any]], key: str, statuses: set[str]) -> Counter[Any]:
    keys: Counter[Any] = Counter()
    for record in records:
        if record.get("status") not in statuses:
            continue
        value = record.get(key)
        if value is not None:
            keys[value] += 1
    return keys


def sorted_keys(values: Iterable[Any]) -> list[Any]:
    return sorted(values, key=lambda value: (str(type(value)), str(value)))


def compare(reference: list[dict[str, Any]], candidate: list[dict[str, Any]], key: str) -> dict[str, Any]:
    reference_apps = applications(reference)
    candidate_apps = applications(candidate)

    # A WeTune application record is emitted only after full structural match.
    reference_matched = keys_of(
        reference_apps,
        key,
        {"applied", "duplicate", "instantiation_failed", "normalization_failed"},
    )
    reference_rewritten = keys_of(reference_apps, key, {"applied", "duplicate"})

    # pgORCA logs every bucket candidate, including structural match rejection.
    candidate_matched = keys_of(
        candidate_apps,
        key,
        {
            "constraint_rejected",
            "instantiate_rejected",
            "applied",
            "duplicate",
        },
    )
    candidate_rewritten = keys_of(candidate_apps, key, {"applied", "duplicate"})

    reference_matched_set = set(reference_matched)
    reference_rewritten_set = set(reference_rewritten)
    candidate_matched_set = set(candidate_matched)
    candidate_rewritten_set = set(candidate_rewritten)

    return {
        "key": key,
        "reference_application_count": len(reference_apps),
        "candidate_application_count": len(candidate_apps),
        "reference_matched": sorted_keys(reference_matched_set),
        "candidate_matched": sorted_keys(candidate_matched_set),
        "shared_matched": sorted_keys(reference_matched_set & candidate_matched_set),
        "missing_matched": sorted_keys(reference_matched_set - candidate_matched_set),
        "reference_rewritten": sorted_keys(reference_rewritten_set),
        "candidate_rewritten": sorted_keys(candidate_rewritten_set),
        "shared_rewritten": sorted_keys(reference_rewritten_set & candidate_rewritten_set),
        "missing_rewritten": sorted_keys(reference_rewritten_set - candidate_rewritten_set),
        "candidate_extra": sorted_keys(candidate_rewritten_set - reference_rewritten_set),
        "reference_rewrite_counts": dict(reference_rewritten),
        "candidate_rewrite_counts": dict(candidate_rewritten),
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("reference", type=Path, help="WeTune TraceRuleChain JSONL")
    parser.add_argument("candidate", type=Path, help="pgORCA JSONL or raw PostgreSQL trace")
    parser.add_argument(
        "--key",
        choices=("rule_id", "rule"),
        default="rule_id",
        help="identity used to correlate rules (default: physical rule line)",
    )
    parser.add_argument("--strict-extra", action="store_true", help="also reject pgORCA-only rewrites")
    parser.add_argument("--output", type=Path, help="write the JSON report to this file")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        report = compare(read_records(args.reference), read_records(args.candidate), args.key)
    except (OSError, ValueError) as error:
        print(error, file=sys.stderr)
        return 2

    rendered = json.dumps(report, ensure_ascii=False, indent=2, sort_keys=True)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered + "\n", encoding="utf-8")
    print(rendered)

    if report["missing_rewritten"]:
        return 1
    if args.strict_extra and report["candidate_extra"]:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
