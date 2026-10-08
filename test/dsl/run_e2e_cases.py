#!/usr/bin/env python3
"""Run SQL cases and compare normalized actual output with golden files."""

from __future__ import annotations

from replacement_contract import (validate_replacement_matrix, XFORM_RE, REPLACEMENT_STATES, PROVENANCE_SOURCES)

import argparse
import difflib
import fnmatch
import json
import os
import pathlib
import re
import subprocess
import sys


REPLACEMENT_XFORMS = (
    "CXformSelect2Apply, CXformProject2Apply, CXformSubquery2CorrelatedApply"
)
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
    parser.add_argument("--check-native-instances", action="store_true",
                        help="Kernel-check each expected source route and constructed target (requires RuleSolver/Rocq)")
    parser.add_argument("--disable-xform", action="append", default=[])
    parser.add_argument("--disable-semantic-xforms", type=pathlib.Path,
                        help="Disable semantic rewrites from this build's coverage.json; retain execution preparation")
    return parser.parse_args()


def semantic_xforms_from_audit(runtime: dict[str, object]) -> list[str]:
    if not isinstance(runtime, dict):
        raise ValueError("expected runtime xform audit object")
    entries = runtime.get("xforms", [])
    categories = {"semantic_rewrite", "join_enumeration", "implementation_property"}
    if runtime.get("schema_version") != 2 or not isinstance(entries, list) or not entries:
        raise ValueError("expected nonempty runtime xform audit schema 2")
    names = set()
    semantic = []
    for entry in entries:
        name = entry.get("name") if isinstance(entry, dict) else None
        if (not isinstance(name, str) or not XFORM_RE.fullmatch(name)
                or name in names or entry.get("category") not in categories):
            raise ValueError("invalid, duplicate or unclassified runtime xform")
        names.add(name)
        if entry["category"] == "semantic_rewrite":
            semantic.append(name)
    totals = runtime.get("totals", {})
    if (not isinstance(totals, dict) or not semantic
            or totals.get("native_exploration_xforms") != len(entries)
            or totals.get("semantic_rewrite_xforms") != len(semantic)):
        raise ValueError("incomplete runtime xform audit")
    return sorted(semantic)


def validate_execution_trace(output: str, disabled: list[str]) -> None:
    if "Optimizer: pg_orca" not in output or "Falling back to Postgres" in output:
        raise ValueError("execution baseline requires an ORCA plan, not fallback")
    fired = [name for name in disabled if f"Xform: {name}\n" in output]
    if fired:
        raise ValueError(f"disabled xforms fired: {fired}")


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


def run_sql(args: argparse.Namespace, sql: str, tuples_only: bool = False,
            error_sqlstate: str | None = None) -> str:
    if error_sqlstate is not None and not re.fullmatch(r"[0-9A-Z]{5}", error_sqlstate):
        raise ValueError("error_sqlstate must be a five-character SQLSTATE")
    error_options = ["-v", "VERBOSITY=sqlstate"] if error_sqlstate else []
    process = subprocess.run(
        [*psql_command(args, tuples_only), *error_options, "-c", sql],
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE if tuples_only else subprocess.STDOUT,
        timeout=60,
    )
    diagnostics = (process.stderr or "") if tuples_only else process.stdout
    output = process.stdout + (process.stderr or "")
    if "Failed assertion:" in diagnostics:
        raise RuntimeError(output.rstrip())
    if error_sqlstate is not None:
        if process.returncode != 0 and re.search(
            rf"^ERROR:\s+{error_sqlstate}\s*$", output, re.MULTILINE
        ):
            return f"SQLSTATE {error_sqlstate}"
        raise RuntimeError(f"Expected SQLSTATE {error_sqlstate}, got:\n{output}")
    if process.returncode != 0:
        raise RuntimeError(output.rstrip())
    # COPY CSV represents a single-column NULL row as a bare newline. Keep
    # every row terminator until splitlines(), including trailing NULL rows.
    return process.stdout if tuples_only else process.stdout.rstrip("\n")


def native_setting(enabled: bool) -> str:
    value = "" if enabled else REPLACEMENT_XFORMS
    return f"SET pg_orca.dsl_only_xforms='{value}';"


def disabled_xform_settings(
    expected: dict[str, object], extra_xforms: list[str] | None = None
) -> str:
    statements = []
    xforms = dict.fromkeys([*expected.get("disable_xforms", []), *(extra_xforms or [])])
    for xform in xforms:
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


def experiment_setting(args: argparse.Namespace, expected: dict[str, object]) -> str:
    experiment = expected.get("stats_experiment")
    if experiment is None:
        return "RESET pg_orca.dsl_stats_experiment_path;"
    if (
        not isinstance(experiment, str)
        or pathlib.Path(experiment).name != experiment
    ):
        raise ValueError(f"invalid experiment fixture name: {experiment!r}")
    path = (args.policy_dir / experiment).resolve()
    if not path.is_file():
        raise ValueError(f"experiment fixture not found: {path}")
    escaped = str(path).replace("'", "''")
    return f"SET pg_orca.dsl_stats_experiment_path='{escaped}';"


def run_plan(args: argparse.Namespace, query: str, plan: dict[str, object]) -> str:
    enabled = "on" if plan.get("dsl", True) else "off"
    trace = "on" if plan.get("trace", False) else "off"
    xform_trace = "on" if plan.get("xform_trace", plan.get("trace", False)) else "off"
    edge_budget = int(plan.get("dphyper_edge_budget", 100000))
    pair_budget = int(plan.get("dphyper_pair_budget", 100))
    return run_sql(
        args,
        f"""
LOAD 'pg_orca';
SET pg_orca.enable_orca=on;
SET pg_orca.enable_dsl_rule={enabled};
{policy_setting(args, plan)}
{experiment_setting(args, plan)}
{bool_guc_setting('pg_orca.enable_assert_maxonerow', plan.get('assert_maxonerow'), False)}
{bool_guc_setting('pg_orca.enable_dphyper', plan.get('dphyper'), False)}
{bool_guc_setting('pg_orca.dphyper_shadow', plan.get('dphyper_shadow'), False)}
SET pg_orca.dphyper_edge_budget={edge_budget};
SET pg_orca.dphyper_pair_budget={pair_budget};
{native_setting(bool(plan.get('native', True)))}
{disabled_xform_settings(plan, args.disable_xform)}
SET optimizer_print_xform={xform_trace};
SET optimizer_print_xform_results={xform_trace};
SET pg_orca.trace_dsl_rule={trace};
SET client_min_messages=log;
EXPLAIN (COSTS OFF) {query}
""",
    )


def produced_alternative(output: str, xform: str) -> bool:
    in_xform = False
    in_alternatives = False
    for line in output.splitlines():
        marker = 'TRACE,"Xform: '
        if marker in line:
            traced_xform = line.split(marker, 1)[1].strip()
            in_xform = traced_xform == xform
            in_alternatives = False
            continue
        if in_xform and line.startswith("Alternatives:"):
            in_alternatives = True
        elif in_xform and in_alternatives and line.startswith("0:"):
            return True
    return False


def memo_provenance(output: str) -> list[dict[str, object]]:
    records = []
    for line in output.splitlines():
        marker = 'DSL_TRACE {"kind":"memo_alternative"'
        if marker not in line:
            continue
        record = json.loads(line.split("DSL_TRACE ", 1)[1])
        if record.get("kind") == "memo_alternative":
            records.append(record)
    return records


def source_binding_matches(snapshot: object, patterns: object) -> list[dict[str, object]]:
    if (not isinstance(snapshot, dict) or snapshot.get("scope") != "matched_source_symbols"
            or not isinstance(snapshot.get("symbols"), list)):
        raise ValueError("missing source binding snapshot")
    bindings = snapshot["symbols"]
    if (not isinstance(patterns, list) or any(not isinstance(p, dict)
            or not isinstance(p.get("symbol"), str) for p in patterns)
            or any(not isinstance(b, dict) or not isinstance(b.get("symbol"), str)
                   for b in bindings)
            or len({b["symbol"] for b in bindings}) != len(bindings)):
        raise ValueError("invalid or duplicate source binding symbols")
    return [p for p in patterns if any(
        all(key in b and b[key] == value for key, value in p.items()) for b in bindings)]


def bind_source_captures(snapshot: object, manifest: object) -> dict[str, dict[str, object]]:
    """Join one native occurrence to emitted capture terms, not a lowering certificate."""
    source_binding_matches(snapshot, [])
    if snapshot.get("complete") is not True:
        raise ValueError("source binding snapshot is incomplete")
    if (not isinstance(manifest, dict) or manifest.get("scope") != "rule_source_captures"
            or not isinstance(manifest.get("captures"), list) or not manifest["captures"]):
        raise ValueError("missing source capture manifest")

    def paths(entry: dict[str, object]) -> list[str]:
        locations = entry.get("source_paths")
        if (not isinstance(locations, list) or not locations
                or any(not isinstance(p, str) or not re.fullmatch(
                    r"r(?:/\d+)*/[tapsfneowmbrhvc]:\d+(?:/[A-Za-z][A-Za-z0-9_]*:\d+)*", p)
                       for p in locations) or len(set(locations)) != len(locations)):
            raise ValueError("invalid or duplicate source capture paths")
        return locations

    by_path = {}
    for binding in snapshot["symbols"]:
        if (binding.get("bound") is not True or binding.get("unsupported_payload")
                or not isinstance(binding.get("kind"), str)
                or not re.fullmatch(r"[tapsfneowmbrhvc]", binding["kind"])
                or not any(key in binding for key in ("expression", "expressions", "columns"))
                or "expression" in binding and not isinstance(binding["expression"], dict)
                or "columns" in binding and not isinstance(binding["columns"], list)
                or "expressions" in binding and (not isinstance(binding["expressions"], list)
                    or any(not isinstance(e, dict) for e in binding["expressions"]))):
            raise ValueError("source capture has no complete native binding")
        for path in paths(binding):
            if path in by_path:
                raise ValueError("source capture path has multiple owners")
            by_path[path] = binding
    result = {}
    owners = set()
    for capture in manifest["captures"]:
        if (not isinstance(capture, dict) or not isinstance(capture.get("term"), str)
                or not capture["term"] or capture["term"] in result):
            raise ValueError("invalid or duplicate kernel capture term")
        locations = paths(capture)
        binding = by_path.get(locations[0])
        if (binding is None or binding.get("kind") != capture.get("kind")
                or any(by_path.get(path) is not binding for path in locations)
                or set(binding["source_paths"]) != set(locations)):
            raise ValueError("kernel capture does not identify the same native source")
        if id(binding) in owners:
            raise ValueError("native source has multiple kernel capture owners")
        owners.add(id(binding))
        result[capture["term"]] = binding
    return result


def source_route_occurrences(records: list[dict[str, object]]) -> list[dict[str, object]]:
    """Pair ordered production observations, not source lowering or rewrite certificates.

    A pre-view route tree and a post-evaluation model are distinct objects. Their
    coherent trace identity permits checking them together, not assuming equality.
    """
    active = None
    occurrences = []
    for record in records:
        kind = record.get("kind")
        if kind == "rule_route":
            active = record
        elif kind == "experiment_outcome" or (kind == "rule_route_outcome" and active
                and record.get("route_sequence") == active.get("route_sequence")):
            active = None
        elif kind == "rule_candidate" and active:
            context = active.get("route_input_context")
            binding = record.get("binding_context")
            if (not isinstance(context, dict) or "plan_template" not in context
                    or not isinstance(binding, dict) or "source_bindings" not in binding
                    or record.get("status") not in ("ready_cbo", "ready_rbo")):
                continue
            if active.get("context_resolution_errors") or record.get("context_resolution_errors"):
                raise ValueError("unresolved source route context")
            if (context.get("capture") != "before_evaluation"
                    or context.get("scope") != "source_before_match_view"
                    or binding.get("capture") != "after_evaluation"):
                raise ValueError("source route has the wrong capture phase")
            if (type(active.get("route_sequence")) is not int or active["route_sequence"] < 1
                    or type(record.get("sequence")) is not int or record["sequence"] < 1
                    or context.get("template_route") != active["route_sequence"]
                    or type(context.get("template_route")) is not int
                    or record.get("evaluated") is not True):
                raise ValueError("source route has no evaluated occurrence identity")
            if (not isinstance(active.get("experiment"), str) or not active["experiment"]
                    or active["experiment"] != record.get("experiment")
                    or any(type(active.get(key)) is not int or active[key] < 0
                           or type(record.get(key)) is not int or active[key] != record[key]
                           for key in ("group", "group_expression", "memo_version"))):
                raise ValueError("source route and candidate belong to different Memo states")
            fingerprint = context.get("template_fingerprint")
            input_context = record.get("input_context")
            if (not isinstance(fingerprint, str) or not re.fullmatch(r"[0-9a-f]{16}", fingerprint)
                    or fingerprint != record.get("source_fingerprint")
                    or not isinstance(input_context, dict)
                    or not isinstance(input_context.get("root"), dict)
                    or fingerprint != input_context["root"].get("reference_key")):
                raise ValueError("source route does not identify the candidate input")
            if (not isinstance(record.get("rule_hash"), str) or not record["rule_hash"]
                    or record["rule_hash"] != binding.get("rule_hash")):
                raise ValueError("source route bindings belong to another rule")
            source_binding_matches(binding["source_bindings"], [])
            snapshot = context["plan_template"]
            if (binding["source_bindings"].get("complete") is not True
                    or not isinstance(snapshot, dict) or snapshot.get("complete") is not True
                    or snapshot.get("schema") != "pgorca.dsl.plan-template.v1"
                    or not isinstance(snapshot.get("nodes"), list) or not snapshot["nodes"]
                    or any(not isinstance(n, dict) or not isinstance(n.get("orca_operator"), str)
                           for n in snapshot["nodes"])):
                raise ValueError("source route has an incomplete tree or binding model")
            occurrence = {"rule_hash": record["rule_hash"],
                "route_sequence": active["route_sequence"], "candidate_sequence": record["sequence"],
                "operators": [n["orca_operator"] for n in snapshot["nodes"]],
                "snapshot": snapshot, "bindings": binding["source_bindings"]}
            if "target_context" in binding:
                target = binding["target_context"]
                if (not isinstance(target, dict) or target.get("capture") != "after_instantiation"
                        or target.get("scope") != "constructed_rule_target"
                        or not isinstance(target.get("fingerprint"), str)
                        or not re.fullmatch(r"[0-9a-f]{16}", target["fingerprint"])
                        or target["fingerprint"] != record.get("target_fingerprint")):
                    raise ValueError("source route does not identify the constructed target")
                actual_target = target.get("plan_template")
                if (not isinstance(actual_target, dict) or actual_target.get("complete") is not True
                        or actual_target.get("schema") != "pgorca.dsl.plan-template.v1"
                        or not isinstance(actual_target.get("nodes"), list) or not actual_target["nodes"]
                        or any(not isinstance(n, dict) or not isinstance(n.get("orca_operator"), str)
                               for n in actual_target["nodes"])):
                    raise ValueError("source route has an incomplete target tree")
                occurrence.update(target_snapshot=actual_target,
                    target_operators=[n["orca_operator"] for n in actual_target["nodes"]])
            occurrences.append(occurrence)
    return occurrences


def native_rule_texts(output: str) -> dict[str, str]:
    """Use ORCA's canonical printer, never hash/parse raw fixture lines."""
    rules = {}
    for text in re.findall(r"^Rule: ([^\r\n]+)$", output, re.MULTILINE):
        identity = 0xcbf29ce484222325
        for byte in text.encode("ascii"):
            identity = ((identity ^ byte) * 0x100000001b3) & ((1 << 64) - 1)
        key = f"{identity:016x}"
        if key in rules and rules[key] != text:
            raise ValueError("canonical rule identity collision in trace")
        rules[key] = text
    return rules


def check_native_routes(expected: dict[str, object], output: str, destination: pathlib.Path) -> int:
    """Check all occurrences selected by the expectation, including their actual target."""
    patterns = expected.get("rule_source_routes", [])
    if not patterns:
        return 0
    observation = expected.get("native_observation", "exact_tree")
    if observation not in ("exact_tree", "all_source_outputs"):
        raise ValueError("unknown native instance observation")
    # Reuse the regular expectation validator and the maintained trace reader.
    if actual_plan({"rule_source_routes": patterns}, output)["rule_source_routes"] != patterns:
        raise ValueError("native instance check requires every expected source route")
    import ml_orca_test_support
    from ml_orca.collect.run_workload_comparison import trace_records
    sys.path.insert(0, str(pathlib.Path(os.environ["WETUNE_HOME"]) / "formalsql"))
    from native_instance import check_native_instance
    rules = native_rule_texts(output)
    checked = 0
    for occurrence in source_route_occurrences(trace_records(output)):
        if not any(all(occurrence.get(key) == value for key, value in pattern.items())
                   for pattern in patterns):
            continue
        if "target_snapshot" not in occurrence:
            raise ValueError("native instance check requires the same occurrence's constructed target")
        rule = rules.get(occurrence["rule_hash"])
        if rule is None:
            raise ValueError("native instance check requires the hash-matched canonical rule text")
        path = destination / f"route-{occurrence['route_sequence']}-candidate-{occurrence['candidate_sequence']}"
        path.mkdir(parents=True, exist_ok=True)
        (path / "occurrence.json").write_text(canonical(occurrence), encoding="utf-8")
        check_native_instance(rule, occurrence["snapshot"], occurrence["target_snapshot"],
                              lambda manifest: bind_source_captures(occurrence["bindings"], manifest), path,
                              observe_compute=observation == "all_source_outputs")
        checked += 1
    if not checked:
        raise ValueError("native instance check captured no matching occurrences")
    return checked


def actual_plan(expected: dict[str, object], output: str) -> dict[str, object]:
    actual = {
        key: expected[key]
        for key in (
            "name", "dsl", "xform_trace", "dphyper", "dphyper_edge_budget",
            "dphyper_pair_budget", "dphyper_shadow", "native", "trace",
            "disable_xforms", "policy", "stats_experiment", "assert_maxonerow", "native_observation"
        )
        if key in expected
    }
    if "contains" in expected:
        actual["contains"] = [text for text in expected["contains"] if text in output]
    if "not_contains" in expected:
        actual["not_contains"] = [
            text for text in expected["not_contains"] if text not in output
        ]
    if "rule_edges" in expected:
        import ml_orca_test_support  # Reuse versioned edge/batch decoding.
        from ml_orca.collect.run_workload_comparison import trace_records
        patterns = expected["rule_edges"]
        if not isinstance(patterns, list) or any(
            not isinstance(edge, dict) or not {"src_rule", "dst_rule"} <= edge.keys()
            for edge in patterns
        ):
            raise ValueError("rule_edges requires source/destination rule patterns")
        edges = [row for row in trace_records(output) if row.get("kind") == "rule_edge"]
        actual["rule_edges"] = [pattern for pattern in patterns if any(
            all(key in edge and edge[key] == value for key, value in pattern.items())
            for edge in edges
        )]
    if "alternative" in expected:
        actual["alternative"] = [
            xform
            for xform in expected["alternative"]
            if produced_alternative(output, xform)
        ]
    if "joins" in expected:
        actual["joins"] = len(JOIN_RE.findall(output))
    if "plan_slice" in expected or "source_binding_matches" in expected:
        import ml_orca_test_support  # Locate the maintained trace reader.
        from ml_orca.collect.run_workload_comparison import trace_records
        contexts = [row["value"]["input_context"] for row in trace_records(output)
                    if row.get("kind") == "candidate_context" and row.get("field") == "query_input_context"]
        if len(contexts) != 1:
            raise ValueError("plan-slice check requires exactly one complete query input context")
        sliced = contexts[0].get("plan_slice")
        if "plan_slice" in expected:
            actual["plan_slice"] = sliced
        if "source_binding_matches" in expected:
            if not isinstance(sliced, dict) or sliced.get("status") != "ok":
                raise ValueError("source binding checks require a successful plan slice")
            actual["source_binding_matches"] = source_binding_matches(
                sliced.get("source_bindings"), expected["source_binding_matches"])
    if "rule_source_routes" in expected:
        import ml_orca_test_support
        from ml_orca.collect.run_workload_comparison import trace_records
        patterns = expected["rule_source_routes"]
        if (not isinstance(patterns, list) or any(not isinstance(p, dict)
                or not isinstance(p.get("rule_hash"), str) or not p["rule_hash"]
                or not isinstance(p.get("operators"), list) or not p["operators"]
                or any(not isinstance(op, str) for op in p["operators"])
                or "route_sequence" in p and (type(p["route_sequence"]) is not int
                    or p["route_sequence"] < 1) for p in patterns)):
            raise ValueError("source route checks require a rule identity and ordered operators")
        captures = source_route_occurrences(trace_records(output))
        actual["rule_source_routes"] = [p for p in patterns if any(
            all(key in capture and capture[key] == value for key, value in p.items())
            for capture in captures)]
    if "rule_binding_matches" in expected:
        import ml_orca_test_support
        from ml_orca.collect.run_workload_comparison import trace_records
        patterns = expected["rule_binding_matches"]
        if not isinstance(patterns, list) or any(not isinstance(p, dict)
                or not isinstance(p.get("rule_hash"), str) or not p["rule_hash"]
                or not isinstance(p.get("symbols"), list) for p in patterns):
            raise ValueError("rule binding checks require a rule identity and symbol patterns")
        for pattern in patterns:
            source_binding_matches({"scope": "matched_source_symbols", "symbols": []}, pattern["symbols"])
        contexts = []
        for event in trace_records(output):
            context = event.get("binding_context")
            if (event.get("kind") != "rule_candidate" or not isinstance(context, dict)
                    or "source_bindings" not in context):
                continue
            if context.get("rule_hash") != event.get("rule_hash"):
                raise ValueError("source bindings belong to another rule")
            source_binding_matches(context["source_bindings"], [])
            contexts.append(context)
        # All requested captures must come from ONE occurrence, not the union
        # of multiple matches of the same rule in different Memo contexts.
        actual["rule_binding_matches"] = [p for p in patterns if any(
            c["rule_hash"] == p["rule_hash"] and
            c["source_bindings"].get("complete") is True and
            source_binding_matches(c["source_bindings"], p["symbols"]) == p["symbols"]
            for c in contexts)]
    if "provenance" in expected:
        records = memo_provenance(output)
        sources = {record.get("source") for record in records}
        origins = {record.get("origin") for record in records}
        contract = expected["provenance"]
        actual["provenance"] = {
            "required_sources": [
                source
                for source in contract.get("required_sources", [])
                if source in sources
            ],
            "required_origins": [
                origin
                for origin in contract.get("required_origins", [])
                if origin in origins
            ],
            "forbidden_origins": [
                origin
                for origin in contract.get("forbidden_origins", [])
                if origin not in origins
            ],
        }
    return actual


def run_orca_rows(
    args: argparse.Namespace,
    query: str,
    expected: dict[str, object],
) -> str:
    query = query.rstrip().removesuffix(";")
    return run_sql(
        args,
        f"""
LOAD 'pg_orca';
SET pg_orca.enable_orca=on;
SET pg_orca.enable_dsl_rule={'on' if expected.get('dsl', True) else 'off'};
{policy_setting(args, expected)}
{experiment_setting(args, expected)}
{bool_guc_setting('pg_orca.enable_assert_maxonerow', expected.get('assert_maxonerow'), False)}
{bool_guc_setting('pg_orca.enable_dphyper', expected.get('dphyper'), False)}
{bool_guc_setting('pg_orca.dphyper_shadow', expected.get('dphyper_shadow'), False)}
SET pg_orca.dphyper_edge_budget={int(expected.get('dphyper_edge_budget', 100000))};
SET pg_orca.dphyper_pair_budget={int(expected.get('dphyper_pair_budget', 100))};
{native_setting(bool(expected.get('native', True)))}
{disabled_xform_settings(expected, args.disable_xform)}
SET client_min_messages=log;
COPY ({query}) TO STDOUT WITH (FORMAT csv);
""",
        tuples_only=True,
        error_sqlstate=expected.get("error_sqlstate"),
    )


def actual_rows(
    args: argparse.Namespace,
    query: str,
    expected: dict[str, object],
    plans: list[dict[str, object]] | None = None,
) -> dict[str, object]:
    query = query.rstrip().removesuffix(";")
    by_name = {plan["name"]: plan for plan in (plans or [])}
    plan_outputs = expected.get("plan_outputs")
    if plan_outputs is not None and (
        not isinstance(plan_outputs, dict)
        or not plan_outputs
        or any(name not in by_name or not isinstance(rows, list)
               or any(not isinstance(row, str) for row in rows)
               for name, rows in plan_outputs.items())
    ):
        raise ValueError("rows.plan_outputs requires existing plan names and CSV row lists")
    dsl_rows = run_orca_rows(args, query, expected)
    postgres_rows = run_sql(
        args,
        f"""
LOAD 'pg_orca';
SET pg_orca.enable_orca=off;
COPY ({query}) TO STDOUT WITH (FORMAT csv);
""",
        tuples_only=True,
        error_sqlstate=expected.get("error_sqlstate"),
    )
    actual = {
        key: expected[key]
        for key in (
            "dsl", "dphyper", "dphyper_shadow", "dphyper_edge_budget",
            "dphyper_pair_budget", "native", "disable_xforms", "policy",
            "assert_maxonerow", "stats_experiment", "error_sqlstate"
        )
        if key in expected
    }
    actual["output"] = dsl_rows.splitlines()
    actual["postgres_output"] = postgres_rows.splitlines()
    if "off_output" in expected:
        actual["off_output"] = run_orca_rows(args, query, {**expected, "dsl": False}).splitlines()
    if plan_outputs is not None:
        # Execute the state itself: inheriting rows.disable_xforms would turn
        # the native/shadow baselines into another native-off execution.
        actual["plan_outputs"] = {
            name: run_orca_rows(args, query, {
                **by_name[name], **({"error_sqlstate": expected["error_sqlstate"]}
                                  if "error_sqlstate" in expected else {}),
            }).splitlines()
            for name in plan_outputs
        }
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
    if not sql_files:
        raise ValueError("no E2E cases selected")
    if args.disable_semantic_xforms:
        runtime = json.loads(args.disable_semantic_xforms.read_text(encoding="utf-8"))
        semantic = semantic_xforms_from_audit(runtime)
        args.disable_xform = sorted(set(args.disable_xform) | set(semantic))
        (args.artifact_dir / "execution-policy.json").write_text(canonical({
            "runtime_xforms": runtime["xforms"],
            "disabled_xforms": args.disable_xform,
        }), encoding="utf-8")

    failed = []
    native_instances = 0
    for sql_path in sql_files:
        case_name = sql_path.stem
        expect_path = args.expect_dir / f"{case_name}.expect"
        expected_text = expect_path.read_text(encoding="utf-8")
        expectation = json.loads(expected_text)
        validate_replacement_matrix(expectation)
        query = sql_path.read_text(encoding="utf-8").strip()
        actual = {
            key: expectation[key]
            for key in ("description", "replacement")
            if key in expectation
        }
        actual["plans"] = []

        for plan in expectation.get("plans", []):
            if args.disable_semantic_xforms and not plan.get("xform_trace", plan.get("trace", False)):
                raise ValueError("execution baseline requires xform trace in every plan state")
            output = run_plan(args, query, plan)
            artifact = args.artifact_dir / f"{case_name}.{plan['name']}.plan"
            artifact.write_text(output + "\n", encoding="utf-8")
            if args.check_native_instances:
                native_instances += check_native_routes(plan, output,
                    args.artifact_dir / f"{case_name}.{plan['name']}.native-instances")
            if args.disable_semantic_xforms:
                validate_execution_trace(output, args.disable_xform)
            actual["plans"].append(actual_plan(plan, output))
        if "rows" in expectation:
            actual["rows"] = actual_rows(args, query, expectation["rows"], expectation.get("plans"))

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

    if args.check_native_instances and not native_instances:
        raise ValueError("requested native checks but selected cases have no expected source routes")
    if native_instances:
        print(f"Conditional native kernel checks: {native_instances}; not deployment certificates")
    if failed:
        print(f"DSL E2E failed: {len(failed)} case(s)", file=sys.stderr)
        return 1
    print(f"DSL E2E passed: {len(sql_files)} SQL/expect cases")
    return 0


if __name__ == "__main__":
    sys.exit(main())
