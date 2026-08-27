#!/usr/bin/env python3
"""Run SQL cases and compare normalized actual output with golden files."""

from __future__ import annotations

import argparse
import difflib
import fnmatch
import json
import pathlib
import re
import subprocess
import sys


REPLACEMENT_XFORMS = "CXformSelect2Apply, CXformProject2Apply"
JOIN_RE = re.compile(
    r"^\s*(?:->\s*)?(?:(?:(?:Hash|Merge)(?:\s+(?:Left|Right|Full|Semi|Anti))?\s+Join)"
    r"|(?:Nested Loop(?:\s+(?:Left|Right|Full|Semi|Anti)\s+Join)?))",
    re.MULTILINE,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--psql", required=True)
    parser.add_argument("--host", required=True)
    parser.add_argument("--port", required=True)
    parser.add_argument("--sql-dir", type=pathlib.Path, required=True)
    parser.add_argument("--expect-dir", type=pathlib.Path, required=True)
    parser.add_argument("--policy-dir", type=pathlib.Path, required=True)
    parser.add_argument("--result-dir", type=pathlib.Path, required=True)
    parser.add_argument("--diff-dir", type=pathlib.Path, required=True)
    parser.add_argument("--artifact-dir", type=pathlib.Path, required=True)
    parser.add_argument("--cases")
    return parser.parse_args()


def psql_command(args: argparse.Namespace, tuples_only: bool = False) -> list[str]:
    command = [
        args.psql,
        "-X",
        "-v",
        "ON_ERROR_STOP=1",
        "-h",
        args.host,
        "-p",
        args.port,
        "-d",
        "postgres",
        "-q",
    ]
    if tuples_only:
        command.append("-At")
    return command


def run_sql(args: argparse.Namespace, sql: str, tuples_only: bool = False) -> str:
    process = subprocess.run(
        [*psql_command(args, tuples_only), "-c", sql],
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if process.returncode != 0:
        raise RuntimeError(process.stdout.rstrip())
    return process.stdout.rstrip("\n")


def native_setting(enabled: bool) -> str:
    value = "" if enabled else REPLACEMENT_XFORMS
    return f"SET pg_orca.dsl_only_xforms='{value}';"


def disabled_xform_settings(expected: dict[str, object]) -> str:
    statements = []
    for xform in expected.get("disable_xforms", []):
        if not isinstance(xform, str) or not re.fullmatch(r"[A-Za-z0-9_]+", xform):
            raise ValueError(f"invalid xform name: {xform!r}")
        # DO avoids adding the disable_xform() result row to COPY output while
        # retaining the setting in this psql session.
        statements.append(
            f"DO $dsl$ BEGIN PERFORM disable_xform('{xform}'); END $dsl$;"
        )
    return "\n".join(statements)


def bool_guc_setting(name: str, value: object, fallback: bool) -> str:
    if value == "default":
        return f"RESET {name};"
    enabled = fallback if value is None else bool(value)
    return f"SET {name}={'on' if enabled else 'off'};"


def policy_setting(args: argparse.Namespace, expected: dict[str, object]) -> str:
    policy = expected.get("policy")
    if policy is None:
        return "RESET pg_orca.dsl_rule_policy_path;"
    if not isinstance(policy, str) or pathlib.Path(policy).name != policy:
        raise ValueError(f"invalid policy fixture name: {policy!r}")
    path = (args.policy_dir / policy).resolve()
    if not path.is_file():
        raise ValueError(f"policy fixture not found: {path}")
    escaped = str(path).replace("'", "''")
    return f"SET pg_orca.dsl_rule_policy_path='{escaped}';"


def run_plan(args: argparse.Namespace, query: str, plan: dict[str, object]) -> str:
    enabled = "on" if plan.get("dsl", True) else "off"
    trace = "on" if plan.get("trace", False) else "off"
    edge_budget = int(plan.get("dphyper_edge_budget", 100000))
    pair_budget = int(plan.get("dphyper_pair_budget", 100))
    return run_sql(
        args,
        f"""
LOAD 'pg_orca';
SET pg_orca.enable_orca=on;
SET pg_orca.enable_dsl_rule={enabled};
{policy_setting(args, plan)}
{bool_guc_setting('pg_orca.enable_dphyper', plan.get('dphyper'), False)}
{bool_guc_setting('pg_orca.dphyper_shadow', plan.get('dphyper_shadow'), True)}
SET pg_orca.dphyper_edge_budget={edge_budget};
SET pg_orca.dphyper_pair_budget={pair_budget};
{native_setting(bool(plan.get('native', True)))}
{disabled_xform_settings(plan)}
SET optimizer_print_xform={trace};
SET optimizer_print_xform_results={trace};
SET pg_orca.trace_dsl_rule={trace};
SET client_min_messages=log;
EXPLAIN (COSTS OFF) {query}
""",
    )


def produced_alternative(output: str, xform: str) -> bool:
    in_xform = False
    in_alternatives = False
    for line in output.splitlines():
        if f"Xform: {xform}" in line:
            in_xform = True
            in_alternatives = False
            continue
        if in_xform and 'TRACE,"Xform:' in line:
            in_xform = False
        elif in_xform and line.startswith("Alternatives:"):
            in_alternatives = True
        elif in_xform and in_alternatives and line.startswith("0:"):
            return True
    return False


def actual_plan(expected: dict[str, object], output: str) -> dict[str, object]:
    actual = {
        key: expected[key]
        for key in (
            "name", "dsl", "dphyper", "dphyper_edge_budget",
            "dphyper_pair_budget", "dphyper_shadow", "native", "trace",
            "disable_xforms", "policy"
        )
        if key in expected
    }
    if "contains" in expected:
        actual["contains"] = [text for text in expected["contains"] if text in output]
    if "not_contains" in expected:
        actual["not_contains"] = [
            text for text in expected["not_contains"] if text not in output
        ]
    if "alternative" in expected:
        actual["alternative"] = [
            xform
            for xform in expected["alternative"]
            if produced_alternative(output, xform)
        ]
    if "joins" in expected:
        actual["joins"] = len(JOIN_RE.findall(output))
    return actual


def actual_rows(
    args: argparse.Namespace,
    query: str,
    expected: dict[str, object],
) -> dict[str, object]:
    query = query.rstrip().removesuffix(";")
    dsl_rows = run_sql(
        args,
        f"""
LOAD 'pg_orca';
SET pg_orca.enable_orca=on;
SET pg_orca.enable_dsl_rule=on;
{policy_setting(args, expected)}
SET pg_orca.enable_dphyper={'on' if expected.get('dphyper', False) else 'off'};
SET pg_orca.dphyper_shadow={'on' if expected.get('dphyper_shadow', True) else 'off'};
SET pg_orca.dphyper_edge_budget={int(expected.get('dphyper_edge_budget', 100000))};
SET pg_orca.dphyper_pair_budget={int(expected.get('dphyper_pair_budget', 100))};
{native_setting(bool(expected.get('native', True)))}
{disabled_xform_settings(expected)}
COPY ({query}) TO STDOUT WITH (FORMAT csv);
""",
        tuples_only=True,
    )
    postgres_rows = run_sql(
        args,
        f"""
LOAD 'pg_orca';
SET pg_orca.enable_orca=off;
COPY ({query}) TO STDOUT WITH (FORMAT csv);
""",
        tuples_only=True,
    )
    actual = {
        key: expected[key]
        for key in (
            "dphyper", "dphyper_shadow", "dphyper_edge_budget",
            "dphyper_pair_budget", "native", "disable_xforms", "policy"
        )
        if key in expected
    }
    actual["output"] = dsl_rows.splitlines()
    actual["postgres_output"] = postgres_rows.splitlines()
    return actual


def canonical(value: dict[str, object]) -> str:
    return json.dumps(value, indent=2, ensure_ascii=False) + "\n"


def selected(case_name: str, patterns: list[str]) -> bool:
    return any(fnmatch.fnmatchcase(case_name, pattern) for pattern in patterns)


def main() -> int:
    args = parse_args()
    patterns = args.cases.split(",") if args.cases else ["*"]
    sql_files = [
        path
        for path in sorted(args.sql_dir.glob("*.sql"))
        if not path.name.startswith("_") and selected(path.stem, patterns)
    ]
    args.result_dir.mkdir(parents=True, exist_ok=True)
    args.diff_dir.mkdir(parents=True, exist_ok=True)
    args.artifact_dir.mkdir(parents=True, exist_ok=True)

    failed = []
    for sql_path in sql_files:
        case_name = sql_path.stem
        expect_path = args.expect_dir / f"{case_name}.expect"
        expected_text = expect_path.read_text(encoding="utf-8")
        expectation = json.loads(expected_text)
        query = sql_path.read_text(encoding="utf-8").strip()
        actual = {
            key: expectation[key]
            for key in ("description",)
            if key in expectation
        }
        actual["plans"] = []

        for plan in expectation.get("plans", []):
            output = run_plan(args, query, plan)
            artifact = args.artifact_dir / f"{case_name}.{plan['name']}.plan"
            artifact.write_text(output + "\n", encoding="utf-8")
            actual["plans"].append(actual_plan(plan, output))
        if "rows" in expectation:
            actual["rows"] = actual_rows(args, query, expectation["rows"])

        actual_text = canonical(actual)
        result_path = args.result_dir / f"{case_name}.output"
        diff_path = args.diff_dir / f"{case_name}.diff"
        result_path.write_text(actual_text, encoding="utf-8")
        if actual_text == expected_text:
            if diff_path.exists():
                diff_path.unlink()
            print(f"ok {case_name}")
        else:
            diff = difflib.unified_diff(
                expected_text.splitlines(keepends=True),
                actual_text.splitlines(keepends=True),
                fromfile=str(expect_path),
                tofile=str(result_path),
            )
            diff_path.write_text("".join(diff), encoding="utf-8")
            failed.append(case_name)
            print(f"not ok {case_name}: {diff_path}")

    if failed:
        print(f"DSL E2E failed: {len(failed)} case(s)", file=sys.stderr)
        return 1
    print(f"DSL E2E passed: {len(sql_files)} SQL/expect cases")
    return 0


if __name__ == "__main__":
    sys.exit(main())
