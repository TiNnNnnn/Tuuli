#!/usr/bin/env python3
"""Build a redacted differential manifest from a WeTune JSONL trace.

The output intentionally excludes rule text, bindings, and source/target plans.
It is therefore suitable for selecting corpus cases without copying a private
rule library into pgORCA.  The original trace should remain in an ignored local
directory when it was produced with private rules.
"""

from __future__ import annotations

import argparse
import json
import sys
from collections import defaultdict
from pathlib import Path
from typing import Any


REWRITTEN_STATUSES = {"applied", "duplicate"}


def read_jsonl(path: Path) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []
    with path.open(encoding="utf-8") as stream:
        for line_number, line in enumerate(stream, 1):
            if not line.strip():
                continue
            try:
                record = json.loads(line)
            except json.JSONDecodeError as error:
                raise ValueError(f"{path}:{line_number}: {error}") from error
            if isinstance(record, dict):
                records.append(record)
    return records


def build_manifest(records: list[dict[str, Any]], maximum: int) -> list[dict[str, Any]]:
    queries: dict[str, str] = {}
    applications: dict[str, list[dict[str, Any]]] = defaultdict(list)

    for record in records:
        case_id = record.get("case_id")
        if not isinstance(case_id, str):
            continue
        if record.get("kind") == "query" and isinstance(record.get("query"), str):
            queries[case_id] = record["query"]
        elif record.get("kind") == "application" and record.get("status") in REWRITTEN_STATUSES:
            applications[case_id].append(record)

    manifest: list[dict[str, Any]] = []
    for case_id in queries:
        case_apps = applications.get(case_id, [])
        if not case_apps:
            continue
        redacted_apps = []
        seen: set[tuple[Any, Any]] = set()
        for application in case_apps:
            identity = (application.get("rule_id"), application.get("status"))
            if identity in seen:
                continue
            seen.add(identity)
            redacted_apps.append({"rule_id": identity[0], "status": identity[1]})
        manifest.append(
            {
                "kind": "differential_case",
                "case_id": case_id,
                "query": queries[case_id],
                "reference_applications": redacted_apps,
            }
        )
        if maximum > 0 and len(manifest) >= maximum:
            break
    return manifest


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("trace", type=Path, help="WeTune TraceRuleChain JSONL")
    parser.add_argument("output", type=Path, help="redacted manifest JSONL")
    parser.add_argument("--max", type=int, default=0, help="maximum triggered cases (0 means all)")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.max < 0:
        print("--max must not be negative", file=sys.stderr)
        return 2
    try:
        manifest = build_manifest(read_jsonl(args.trace), args.max)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with args.output.open("w", encoding="utf-8") as stream:
            for case in manifest:
                stream.write(json.dumps(case, ensure_ascii=False, separators=(",", ":")) + "\n")
    except (OSError, ValueError) as error:
        print(error, file=sys.stderr)
        return 2
    print(f"wrote {len(manifest)} triggered cases to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
