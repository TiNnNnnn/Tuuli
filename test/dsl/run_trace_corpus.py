#!/usr/bin/env python3
"""Replay a redacted WeTune manifest through pgORCA, one case at a time."""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Any

from compare_rule_traces import compare, read_records


def read_manifest(path: Path) -> list[dict[str, Any]]:
    cases: list[dict[str, Any]] = []
    seen_case_ids: set[str] = set()
    with path.open(encoding="utf-8") as stream:
        for line_number, line in enumerate(stream, 1):
            if not line.strip():
                continue
            try:
                record = json.loads(line)
            except json.JSONDecodeError as error:
                raise ValueError(f"{path}:{line_number}: {error}") from error
            if record.get("kind") != "differential_case":
                continue
            if not isinstance(record.get("case_id"), str) or not isinstance(record.get("query"), str):
                raise ValueError(f"{path}:{line_number}: case_id and query are required")
            if record["case_id"] in seen_case_ids:
                raise ValueError(f"{path}:{line_number}: duplicate case_id {record['case_id']}")
            applications = record.get(
                "expected_applications", record.get("reference_applications")
            )
            if not isinstance(applications, list) or not any(
                isinstance(application, dict) and "rule_id" in application
                for application in applications
            ):
                raise ValueError(
                    f"{path}:{line_number}: at least one expected/reference rule_id is required"
                )
            seen_case_ids.add(record["case_id"])
            cases.append(record)
    return cases


def reference_records(case: dict[str, Any]) -> list[dict[str, Any]]:
    records = []
    applications = case.get("expected_applications", case.get("reference_applications", []))
    for application in applications:
        if not isinstance(application, dict) or "rule_id" not in application:
            continue
        records.append(
            {
                "kind": "application",
                "engine": "wetune",
                "case_id": case["case_id"],
                "rule_id": application["rule_id"],
                "status": application.get("status", "applied"),
            }
        )
    return records


def safe_name(case_id: str) -> str:
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", case_id).strip("._") or "case"


def tail_text(path: Path, maximum: int = 2000) -> str:
    try:
        with path.open("rb") as stream:
            stream.seek(0, os.SEEK_END)
            size = stream.tell()
            stream.seek(max(0, size - maximum * 2))
            return stream.read().decode("utf-8", errors="replace")[-maximum:].strip()
    except OSError:
        return ""


def parse_args() -> argparse.Namespace:
    script_dir = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest", type=Path)
    parser.add_argument("rules", type=Path)
    parser.add_argument("schema", type=Path)
    parser.add_argument("--pg-config", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--trace-runner", type=Path, default=script_dir / "trace_one.sh")
    parser.add_argument("--base-port", type=int, default=55440)
    parser.add_argument(
        "--case",
        dest="case_ids",
        action="append",
        default=[],
        help="run only the selected case_id; may be specified more than once",
    )
    parser.add_argument("--max", type=int, default=0)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.max < 0 or not 1 <= args.base_port <= 65535:
        print("invalid --max or --base-port", file=sys.stderr)
        return 2
    try:
        cases = read_manifest(args.manifest)
    except (OSError, ValueError) as error:
        print(error, file=sys.stderr)
        return 2
    if args.case_ids:
        selected = set(args.case_ids)
        cases = [case for case in cases if case["case_id"] in selected]
        missing = sorted(selected - {case["case_id"] for case in cases})
        if missing:
            print(f"case_id not found: {', '.join(missing)}", file=sys.stderr)
            return 2
    if args.max:
        cases = cases[: args.max]

    rules_path = args.rules.resolve()
    schema_path = args.schema.resolve()
    trace_runner = args.trace_runner.resolve()
    pg_config = args.pg_config.resolve()

    args.output_dir.mkdir(parents=True, exist_ok=True)
    candidate_jsonl = args.output_dir / "pgorca.jsonl"
    case_reports: list[dict[str, Any]] = []
    candidate_records: list[dict[str, Any]] = []

    for index, case in enumerate(cases):
        case_id = case["case_id"]
        stem = safe_name(case_id)
        query_path = args.output_dir / f"{stem}.sql"
        log_path = args.output_dir / f"{stem}.log"
        query_path.write_text(case["query"].rstrip().rstrip(";") + ";\n", encoding="utf-8")

        environment = os.environ.copy()
        environment["PG_CONFIG"] = str(pg_config)
        environment["DSL_TRACE_PORT"] = str(args.base_port + index % 1000)
        environment["DSL_TRACE_ALLOW_EMPTY"] = "1"
        process = subprocess.run(
            [
                str(trace_runner),
                str(rules_path),
                str(schema_path),
                str(query_path.resolve()),
                str(log_path.resolve()),
            ],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=environment,
            check=False,
        )

        if process.returncode != 0:
            detail = tail_text(log_path)
            case_reports.append(
                {
                    "case_id": case_id,
                    "status": "query_error",
                    "returncode": process.returncode,
                    "stderr": process.stderr.strip()[-2000:],
                    "detail": detail,
                }
            )
            continue

        try:
            parsed = read_records(log_path)
        except (OSError, ValueError) as error:
            case_reports.append(
                {"case_id": case_id, "status": "trace_error", "detail": str(error)}
            )
            continue
        for record in parsed:
            record["case_id"] = case_id
        candidate_records.extend(parsed)
        comparison = compare(reference_records(case), parsed, "rule_id")
        case_reports.append(
            {
                "case_id": case_id,
                "status": "passed" if not comparison["missing_rewritten"] else "missing_rewrite",
                "missing_rewritten": comparison["missing_rewritten"],
                "candidate_extra": comparison["candidate_extra"],
                "candidate_application_count": comparison["candidate_application_count"],
            }
        )

    with candidate_jsonl.open("w", encoding="utf-8") as stream:
        for record in candidate_records:
            stream.write(json.dumps(record, ensure_ascii=False, separators=(",", ":")) + "\n")

    failed = sum(case["status"] != "passed" for case in case_reports)
    report = {
        "kind": "corpus_comparison",
        "manifest": str(args.manifest),
        "attempted": len(case_reports),
        "passed": len(case_reports) - failed,
        "failed": failed,
        "cases": case_reports,
    }
    report_path = args.output_dir / "report.json"
    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
