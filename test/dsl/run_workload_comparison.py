#!/usr/bin/env python3
"""Compare native ORCA with DSL+DPHyper on fixed optimizer workloads."""

from __future__ import annotations

import argparse
import difflib
import fnmatch
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time
from typing import Any

from build_xform_replacement_inventory import merge_inventory, read_matrices
from run_dphyper_stability import parse_dphyper_events


SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_WORKLOADS = SCRIPT_DIR / "workloads"
XFORM_RE = re.compile(r"CXform[A-Za-z0-9_]+")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--pg-config", required=True)
    parser.add_argument("--audit-bin", required=True)
    parser.add_argument("--workload", action="append", choices=("tpch", "tpcds", "job", "sqlstorm"))
    parser.add_argument("-t", "--test", action="append", default=[])
    parser.add_argument("--workload-dir", type=Path, default=DEFAULT_WORKLOADS)
    parser.add_argument("--rule-file", type=Path, default=SCRIPT_DIR / "rules/orca_replacements.rules")
    parser.add_argument("--policy-file", type=Path)
    parser.add_argument("--output", type=Path, default=SCRIPT_DIR.parent.parent / "build/dsl-workloads")
    parser.add_argument("--port", type=int, default=60460)
    parser.add_argument("--timeout", type=int, default=60)
    parser.add_argument("--strict", action="store_true")
    return parser.parse_args()


def run(command: list[str], **kwargs: Any) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, check=False, text=True, **kwargs)


def psql(
    binary: Path,
    socket: Path,
    port: int,
    database: str,
    sql: str,
    timeout: int,
) -> tuple[str, str, int, float]:
    start = time.perf_counter()
    try:
        result = run(
            [
                str(binary), "-X", "-qAt", "-v", "ON_ERROR_STOP=1",
                "-h", str(socket), "-p", str(port), "-d", database,
            ],
            input=f"SET statement_timeout='{timeout * 1000}ms';\n{sql}",
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout + 5,
        )
        return result.stdout, result.stderr, result.returncode, 1000 * (time.perf_counter() - start)
    except subprocess.TimeoutExpired as error:
        stdout = error.stdout.decode() if isinstance(error.stdout, bytes) else (error.stdout or "")
        stderr = error.stderr.decode() if isinstance(error.stderr, bytes) else (error.stderr or "")
        return stdout, stderr + "\nTIMEOUT", 124, 1000 * (time.perf_counter() - start)


def settings(mode: str, semantic_xforms: list[str], policy_file: Path | None) -> str:
    replacement = mode == "replacement"
    disabled = "\n".join(
        f"DO $dsl$ BEGIN PERFORM disable_xform('{name}'); END $dsl$;"
        for name in semantic_xforms
    ) if replacement else ""
    policy = "RESET pg_orca.dsl_rule_policy_path;"
    if replacement and policy_file is not None:
        path = str(policy_file.resolve()).replace("'", "''")
        policy = f"SET pg_orca.dsl_rule_policy_path='{path}';"
    return f"""
LOAD 'pg_orca';
SET pg_orca.enable_orca=on;
SET pg_orca.enable_dsl_rule={'on' if replacement else 'off'};
{policy}
SET pg_orca.enable_dphyper={'on' if replacement else 'off'};
SET pg_orca.dphyper_shadow=off;
SET pg_orca.enable_assert_maxonerow=off;
SET pg_orca.dsl_only_xforms='';
SET pg_orca.trace_fallback=on;
{disabled}
"""


def trace_settings(mode: str, semantic_xforms: list[str], policy_file: Path | None) -> str:
    native_xform_trace = "on" if mode == "native" else "off"
    return settings(mode, semantic_xforms, policy_file) + """
SET optimizer_print_xform={xform_trace};
SET optimizer_print_xform_results={xform_trace};
SET pg_orca.trace_dsl_rule=on;
SET client_min_messages=log;
""".format(xform_trace=native_xform_trace)


def trace_records(text: str) -> list[dict[str, Any]]:
    records = []
    for line in text.splitlines():
        if "DSL_TRACE {" not in line:
            continue
        payload = line.split("DSL_TRACE ", 1)[1]
        try:
            records.append(json.loads(payload[: payload.rfind("}") + 1]))
        except (ValueError, json.JSONDecodeError):
            pass
    return records


def error_summary(text: str) -> str:
    return "\n".join(
        line
        for line in text.splitlines()
        if line.startswith(("ERROR:", "FATAL:"))
        or ',ERROR,"' in line
        or line.startswith("INFO:  pg_orca: falling back")
        or (line.startswith("DETAIL:") and "Failed assertion" in line)
        or line == "TIMEOUT"
    )


def produced_xforms(text: str) -> list[str]:
    result = []
    current = None
    alternatives = False
    for line in text.splitlines():
        if 'TRACE,"Xform: ' in line:
            current = line.split('TRACE,"Xform: ', 1)[1].strip()
            alternatives = False
        elif current and line.startswith("Alternatives:"):
            alternatives = True
        elif current and alternatives and line.startswith("0:"):
            result.append(current)
            current = None
    return result


def plan_tree(stdout: str) -> dict[str, Any] | None:
    try:
        value = json.loads(stdout)
        return value[0]["Plan"]
    except (KeyError, IndexError, TypeError, json.JSONDecodeError):
        return None


def optimizer_name(stdout: str) -> str | None:
    try:
        value = json.loads(stdout)
        return value[0].get("Optimizer", "postgres")
    except (IndexError, TypeError, json.JSONDecodeError):
        return None


def walk_plan(plan: dict[str, Any] | None) -> list[dict[str, Any]]:
    if plan is None:
        return []
    return [plan, *(node for child in plan.get("Plans", []) for node in walk_plan(child))]


def plan_difference(left: dict[str, Any] | None, right: dict[str, Any] | None) -> str:
    if left is None or right is None:
        return "plan_error"
    if left == right:
        return "identical"
    left_nodes = [node.get("Node Type") for node in walk_plan(left)]
    right_nodes = [node.get("Node Type") for node in walk_plan(right)]
    left_relations = [
        (node.get("Relation Name"), node.get("Alias"))
        for node in walk_plan(left)
        if node.get("Relation Name")
    ]
    right_relations = [
        (node.get("Relation Name"), node.get("Alias"))
        for node in walk_plan(right)
        if node.get("Relation Name")
    ]
    relation_key = lambda item: (item[0] or "", item[1] or "")
    if left_relations != right_relations and sorted(
        left_relations, key=relation_key
    ) == sorted(right_relations, key=relation_key):
        return "join_order"
    if left_nodes == right_nodes:
        return "expression_or_property"
    return "physical_shape"


def compact(sequence: list[str]) -> list[str]:
    return [item for index, item in enumerate(sequence) if index == 0 or sequence[index - 1] != item]


def inventory(
    args: argparse.Namespace,
) -> tuple[list[str], dict[str, set[str]], set[str], set[str]]:
    audit_dir = args.output / "audit"
    result = run(
        [str(Path(args.audit_bin).resolve()), str((SCRIPT_DIR / "rules").resolve()), str(audit_dir.resolve())],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if result.returncode:
        raise RuntimeError(result.stdout)
    runtime = json.loads((audit_dir / "coverage.json").read_text(encoding="utf-8"))
    merged = merge_inventory(runtime, read_matrices(SCRIPT_DIR / "e2e/expect"))
    semantic = sorted(entry["name"] for entry in merged["xforms"] if entry["category"] == "semantic_rewrite")
    semantic_set = set(semantic)
    joins = {
        entry["name"]
        for entry in merged["xforms"]
        if entry["category"] == "join_enumeration"
    }
    hashes: dict[str, set[str]] = {}
    for entry in merged["xforms"]:
        for evidence in entry["evidence"]:
            for rule_hash in evidence["dsl_rule_hashes"]:
                hashes.setdefault(rule_hash, set()).add(entry["name"])
    return semantic, hashes, semantic_set, joins


def compare_query(
    args: argparse.Namespace,
    psql_bin: Path,
    socket: Path,
    database: str,
    workload: str,
    query_path: Path,
    semantic_xforms: list[str],
    hash_xforms: dict[str, set[str]],
    semantic_xform_set: set[str],
    join_xforms: set[str],
) -> dict[str, Any]:
    query = query_path.read_text(encoding="utf-8").strip().removesuffix(";")
    artifact = args.output / workload / query_path.stem
    artifact.mkdir(parents=True, exist_ok=True)
    modes: dict[str, dict[str, Any]] = {}
    for mode in ("native", "replacement"):
        base = settings(mode, semantic_xforms, args.policy_file)
        plan_out, plan_err, plan_rc, plan_ms = psql(
            psql_bin, socket, args.port, database,
            trace_settings(mode, semantic_xforms, args.policy_file)
            + f"\nEXPLAIN (COSTS OFF, FORMAT JSON) {query};",
            args.timeout,
        )
        rows_out, rows_err, rows_rc, rows_ms = psql(
            psql_bin, socket, args.port, database,
            base + f"\nSET client_min_messages=warning;\nCOPY ({query}) TO STDOUT WITH (FORMAT csv);",
            args.timeout,
        )
        (artifact / f"{mode}.plan.json").write_text(plan_out, encoding="utf-8")
        trace = plan_err
        (artifact / f"{mode}.trace").write_text(trace, encoding="utf-8")
        records = trace_records(trace)
        optimizer = optimizer_name(plan_out)
        modes[mode] = {
            "plan_rc": plan_rc,
            "rows_rc": rows_rc,
            "plan_ms": round(plan_ms, 3),
            "rows_ms": round(rows_ms, 3),
            "rows_hash": hashlib.sha256(rows_out.encode()).hexdigest(),
            "error": "\n".join(
                summary
                for summary in (
                    error_summary(plan_err),
                    error_summary(rows_err) if rows_rc else "",
                )
                if summary
            ),
            "optimizer": optimizer,
            "fallback": plan_rc == 0 and optimizer == "postgres",
            "plan": plan_tree(plan_out),
            "produced_xforms": produced_xforms(trace),
            "applied_rule_hashes": [
                record["rule_hash"] for record in records
                if record.get("status") == "applied" and "rule_hash" in record
            ],
            "dphyper_events": parse_dphyper_events(trace),
            "native_memo_origins": sorted({
                str(record.get("origin")) for record in records
                if record.get("kind") == "memo_alternative" and record.get("source") == "native"
            }),
        }

    produced = modes["native"]["produced_xforms"]
    native_sequence = [name for name in produced if name in semantic_xform_set]
    native_join_sequence = [name for name in produced if name in join_xforms]
    applied_hashes = modes["replacement"]["applied_rule_hashes"]
    mapped_sequence = [
        "|".join(sorted(hash_xforms[rule_hash]))
        for rule_hash in applied_hashes if rule_hash in hash_xforms
    ]
    native_set = set(native_sequence)
    mapped_set = {name for rule_hash in applied_hashes for name in hash_xforms.get(rule_hash, ())}
    replacement_origins = set(modes["replacement"]["native_memo_origins"])
    forbidden_origins = sorted(replacement_origins & set(semantic_xforms))
    plan_kind = plan_difference(modes["native"]["plan"], modes["replacement"]["plan"])
    outcome_equal = all(
        modes["replacement"][key] == modes["native"][key]
        for key in ("plan_rc", "rows_rc")
    )
    rows_equal = (
        modes["native"]["rows_rc"] == 0
        and modes["replacement"]["rows_rc"] == 0
        and modes["native"]["rows_hash"] == modes["replacement"]["rows_hash"]
    )
    native_orca = modes["native"]["optimizer"] == "pg_orca"
    replacement_orca = modes["replacement"]["optimizer"] == "pg_orca"
    if native_orca and replacement_orca:
        coverage_status = "comparable"
    elif native_orca:
        coverage_status = "replacement_failed"
    elif replacement_orca:
        coverage_status = "replacement_only"
    else:
        coverage_status = "native_unsupported"
    trigger_set_equal = native_set == mapped_set
    trigger_order_equal = compact(native_sequence) == compact(mapped_sequence)
    dphyper_applied = any(
        event.get("status") == "applied"
        for event in modes["replacement"]["dphyper_events"]
    )
    join_enumeration_replaced = not native_join_sequence or dphyper_applied
    explanations = []
    if not trigger_set_equal:
        explanations.append("different_semantic_search_path")
    elif not trigger_order_equal:
        explanations.append("atomic_DSL_rules_change_Cascades_order")
    if native_join_sequence or dphyper_applied:
        explanations.append("DPHyper_join_enumeration")
    if plan_kind != "identical":
        explanations.append(f"final_plan_{plan_kind}")
    if forbidden_origins:
        explanations.append("disabled_native_semantic_origin_leaked")
    result = {
        "workload": workload,
        "query": query_path.name,
        "outcome_equal": outcome_equal,
        "rows_equal": rows_equal,
        "coverage_status": coverage_status,
        "trigger_set_equal": trigger_set_equal,
        "trigger_order_equal": trigger_order_equal,
        "join_enumeration_replaced": join_enumeration_replaced,
        "plan_comparison": plan_kind,
        "native_trigger_sequence": native_sequence,
        "native_join_trigger_sequence": native_join_sequence,
        "replacement_rule_sequence": applied_hashes,
        "replacement_mapped_sequence": mapped_sequence,
        "missing_native_triggers": sorted(native_set - mapped_set),
        "extra_mapped_triggers": sorted(mapped_set - native_set),
        "forbidden_native_origins": forbidden_origins,
        "explanations": explanations,
        "modes": modes,
    }
    for mode in modes.values():
        mode.pop("plan")
    (artifact / "comparison.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return result


def selected(workload: str, query: Path, patterns: list[str]) -> bool:
    key = f"{workload}/{query.stem}"
    return not patterns or any(fnmatch.fnmatchcase(key, pattern) for pattern in patterns)


def main() -> int:
    args = parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    pg_bindir = Path(subprocess.check_output([args.pg_config, "--bindir"], text=True).strip())
    semantic_xforms, hash_xforms, semantic_xform_set, join_xforms = inventory(args)
    workloads = args.workload or ["tpch", "tpcds", "job", "sqlstorm"]
    with tempfile.TemporaryDirectory(prefix="pgorca-workloads.", dir="/tmp") as temporary:
        root = Path(temporary)
        data = root / "data"
        socket = root / "socket"
        socket.mkdir()
        init = run([str(pg_bindir / "initdb"), "-D", str(data), "--no-locale", "--encoding=UTF8"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if init.returncode:
            raise RuntimeError(init.stdout)
        env = {**os.environ, "MONSOON_DSL_RULES": str(args.rule_file.resolve())}
        log = args.output / "postgresql.log"
        start = run([
            str(pg_bindir / "pg_ctl"), "-D", str(data), "-l", str(log),
            "-o", f"-c listen_addresses='' -c logging_collector=off -k {socket} -p {args.port}", "start",
        ], env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if start.returncode:
            raise RuntimeError(start.stdout)
        results = []
        try:
            for workload in workloads:
                database = f"dsl_{workload}"
                created = run([str(pg_bindir / "createdb"), "-h", str(socket), "-p", str(args.port), database], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
                if created.returncode:
                    raise RuntimeError(created.stdout)
                schema = args.workload_dir / workload / "schema.sql"
                loaded = run([
                    str(pg_bindir / "psql"), "-X", "-v", "ON_ERROR_STOP=1",
                    "-h", str(socket), "-p", str(args.port), "-d", database,
                    "-c", "CREATE EXTENSION pg_orca;", "-f", str(schema),
                ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
                if loaded.returncode:
                    raise RuntimeError(f"{workload} schema failed:\n{loaded.stdout}")
                queries = sorted((args.workload_dir / workload / "sql").glob("*.sql"))
                for query in queries:
                    if not selected(workload, query, args.test):
                        continue
                    result = compare_query(
                        args, pg_bindir / "psql", socket, database, workload, query,
                        semantic_xforms, hash_xforms, semantic_xform_set,
                        join_xforms,
                    )
                    results.append(result)
                    print(
                        f"{workload}/{query.stem}: outcome={result['outcome_equal']} "
                        f"triggers={result['trigger_set_equal']}/{result['trigger_order_equal']} "
                        f"plan={result['plan_comparison']} "
                        f"coverage={result['coverage_status']}",
                        flush=True,
                    )
        finally:
            run([str(pg_bindir / "pg_ctl"), "-D", str(data), "stop", "-m", "fast"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    summary = {
        "queries": len(results),
        "outcome_equal": sum(result["outcome_equal"] for result in results),
        "rows_equal": sum(result["rows_equal"] for result in results),
        "trigger_set_equal": sum(result["trigger_set_equal"] for result in results),
        "trigger_order_equal": sum(result["trigger_order_equal"] for result in results),
        "join_enumeration_replaced": sum(
            result["join_enumeration_replaced"] for result in results
        ),
        "plan_identical": sum(result["plan_comparison"] == "identical" for result in results),
        "coverage_status": {
            status: sum(result["coverage_status"] == status for result in results)
            for status in (
                "comparable",
                "replacement_failed",
                "replacement_only",
                "native_unsupported",
            )
        },
        "comparable": {
            "queries": sum(
                result["coverage_status"] == "comparable" for result in results
            ),
            "trigger_set_equal": sum(
                result["coverage_status"] == "comparable"
                and result["trigger_set_equal"]
                for result in results
            ),
            "trigger_order_equal": sum(
                result["coverage_status"] == "comparable"
                and result["trigger_order_equal"]
                for result in results
            ),
            "plan_identical": sum(
                result["coverage_status"] == "comparable"
                and result["plan_comparison"] == "identical"
                for result in results
            ),
        },
        "plan_difference_reasons": {
            reason: sum(result["plan_comparison"] == reason for result in results)
            for reason in ("join_order", "expression_or_property", "physical_shape", "plan_error")
        },
    }
    (args.output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    failed = any(
        result["coverage_status"] == "replacement_failed"
        or not result["outcome_equal"]
        or (
            result["coverage_status"] == "comparable"
            and not result["rows_equal"]
        )
        or not result["join_enumeration_replaced"]
        or result["forbidden_native_origins"]
        or (args.strict and (not result["trigger_set_equal"] or not result["trigger_order_equal"] or result["plan_comparison"] != "identical"))
        for result in results
    )
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
