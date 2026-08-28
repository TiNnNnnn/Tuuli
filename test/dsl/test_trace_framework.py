#!/usr/bin/env python3
"""Self-tests for the redacted WeTune/pgORCA differential framework."""

from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from build_reference_manifest import build_manifest
from compare_rule_traces import compare, read_records
from import_wetune_workloads import postgres_schema, schema_catalog
from run_dphyper_stability import imported_cases, parse_dphyper_events, summarize
from run_e2e_cases import bool_guc_setting, disabled_xform_settings
from run_trace_corpus import (
    alignment_summary,
    failed_query_status,
    orca_fallback_reason,
    parameter_count,
    parse_args,
    read_manifest,
    reference_records,
    render_trace_query,
    rule_count,
    trace_metrics,
    validate_rule_ids,
)


class TraceFrameworkTest(unittest.TestCase):
    def test_e2e_can_preserve_extension_guc_defaults(self) -> None:
        self.assertEqual(
            bool_guc_setting("pg_orca.enable_dphyper", "default", False),
            "RESET pg_orca.enable_dphyper;",
        )

    def test_dphyper_stability_preserves_imported_statement_ids(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            cases = Path(directory) / "cases.sql"
            cases.write_text("SELECT 1\n\nSELECT 3\n", encoding="utf-8")

            self.assertEqual(
                imported_cases("app", cases),
                [("app:1", "SELECT 1"), ("app:3", "SELECT 3")],
            )

    def test_dphyper_stability_parses_region_events(self) -> None:
        events = parse_dphyper_events(
            'TRACE,"DPHyper: status=applied group=4 nodes=5 '
            'enumeration=simplified mode=replacement",\n'
            'TRACE,"DPHyper: status=fallback reason=pair_budget '
            'owner=greedy_nary",\n'
        )

        self.assertEqual(
            events,
            [
                {
                    "status": "applied",
                    "group": 4,
                    "nodes": 5,
                    "enumeration": "simplified",
                    "mode": "replacement",
                },
                {
                    "status": "fallback",
                    "reason": "pair_budget",
                    "owner": "greedy_nary",
                },
            ],
        )

    def test_dphyper_stability_separates_regression_and_native_fallback(self) -> None:
        records = [
            {
                "case_id": "app:1",
                "native": {"status": "ok", "elapsed_ms": 10.0},
                "replacement": {
                    "status": "ok",
                    "elapsed_ms": 12.0,
                    "dphyper_events": [
                        {"status": "applied", "enumeration": "simplified"},
                        {
                            "status": "fallback",
                            "reason": "pair_budget",
                            "owner": "greedy_nary",
                        },
                    ],
                },
            },
            {
                "case_id": "app:2",
                "native": {"status": "ok", "elapsed_ms": 10.0},
                "replacement": {
                    "status": "query_error",
                    "elapsed_ms": 11.0,
                    "dphyper_events": [],
                },
            },
            {
                "case_id": "app:3",
                "native": {"status": "ok", "elapsed_ms": 10.0},
                "replacement": {
                    "status": "statement_timeout",
                    "elapsed_ms": 60000.0,
                    "dphyper_events": [],
                },
            },
        ]

        summary = summarize("app", records)

        self.assertEqual(summary["replacement_regressions"], ["app:2"])
        self.assertEqual(summary["replacement_timeouts"], ["app:3"])
        self.assertEqual(summary["dphyper_applied_queries"], 1)
        self.assertEqual(summary["dphyper_simplified_queries"], 1)
        self.assertEqual(summary["fallback_reasons"], {"pair_budget": 1})
        self.assertEqual(summary["greedy_fallback_events"], 1)
        self.assertEqual(summary["dpv2_fallback_events"], 0)

    def test_corpus_replay_uses_bounded_dphyper_defaults(self) -> None:
        with patch.object(
            sys,
            "argv",
            [
                "run_trace_corpus.py",
                "manifest.jsonl",
                "rules.txt",
                "schema.sql",
                "--pg-config",
                "/tmp/pg_config",
                "--output-dir",
                "/tmp/output",
            ],
        ):
            args = parse_args()

        self.assertEqual(args.dphyper, "off")
        self.assertEqual(args.dphyper_shadow, "on")
        self.assertEqual(args.dphyper_pair_budget, 100)
        self.assertEqual(args.dphyper_edge_budget, 100000)
        self.assertIsNone(args.policy)
        self.assertFalse(args.strict_trigger_set)

    def test_statement_timeout_has_its_own_failure_class(self) -> None:
        self.assertEqual(
            failed_query_status("ERROR: canceling statement due to statement timeout"),
            "timeout",
        )
        self.assertEqual(failed_query_status("ERROR: bad query"), "query_error")

    def test_trace_metrics_uses_cumulative_rule_maxima(self) -> None:
        records = [
            {
                "kind": "memo_summary",
                "groups": 10,
                "duplicate_groups": 1,
                "group_expressions": 20,
            },
            {
                "kind": "rule_summary",
                "rule_id": 2,
                "binding_attempts": 5,
                "generated_alternatives": 1,
                "duplicate_alternatives": 2,
                "budget_exhausted": 0,
                "budget_skipped": 0,
            },
            {
                "kind": "memo_summary",
                "groups": 12,
                "duplicate_groups": 3,
                "group_expressions": 18,
            },
            {
                "kind": "rule_summary",
                "rule_id": 2,
                "binding_attempts": 8,
                "generated_alternatives": 2,
                "duplicate_alternatives": 2,
                "budget_exhausted": 1,
                "budget_skipped": 4,
            },
            {
                "kind": "rule_summary",
                "rule_id": 6,
                "binding_attempts": 3,
                "generated_alternatives": 1,
                "duplicate_alternatives": 0,
                "budget_exhausted": 0,
                "budget_skipped": 0,
            },
        ]

        self.assertEqual(
            trace_metrics(records),
            {
                "memo_stages": 2,
                "peak_groups": 12,
                "peak_duplicate_groups": 3,
                "peak_group_expressions": 20,
                "rules_attempted": 2,
                "binding_attempts": 11,
                "generated_alternatives": 3,
                "duplicate_alternatives": 2,
                "budget_exhausted": 1,
                "budget_skipped": 4,
            },
        )

    def test_alignment_summary_separates_subset_and_exact_sets(self) -> None:
        cases = [
            {
                "missing_rewritten": [],
				"missing_capable": [],
                "inconclusive_budget": [],
                "candidate_extra": [],
            },
            {
                "missing_rewritten": [],
				"missing_capable": [],
                "inconclusive_budget": [],
                "candidate_extra": [17],
            },
            {
                "missing_rewritten": [6],
				"missing_capable": [6],
                "inconclusive_budget": [],
                "candidate_extra": [17],
            },
        ]
        records = [
            {"kind": "application", "status": "applied_rbo"},
            {"kind": "application", "status": "duplicate"},
        ]

        self.assertEqual(
            alignment_summary(cases, records),
            {
                "comparable": 3,
                "subset_aligned": 2,
				"capability_subset_aligned": 2,
                "exact_trigger_set": 1,
                "missing_rule_distribution": {6: 1},
				"missing_capability_distribution": {6: 1},
                "extra_rule_distribution": {17: 2},
                "application_status_distribution": {
                    "applied_rbo": 1,
                    "duplicate": 1,
                },
            },
        )

    def test_manifest_rule_ids_must_exist_in_supplied_rule_set(self) -> None:
        cases = [
            {
                "expected_applications": [
                    {"rule_id": 2, "status": "applied"},
                    {"rule_id": 5, "status": "applied"},
                    {"rule_id": 6, "status": "applied"},
                ]
            }
        ]
        with tempfile.TemporaryDirectory() as directory:
            rules = Path(directory) / "rules.txt"
            rules.write_text(
                "# comment\n"
                "source|target|\n"
                "\n"
                "source2|target2|\n"
                "source3|target3|\tNEQ\n"
                "source4|target4|\tEQ\n",
                encoding="utf-8",
            )

            self.assertEqual(rule_count(rules), 3)
            with self.assertRaisesRegex(ValueError, "5.*generated together"):
                validate_rule_ids(cases, rules)

    def test_e2e_can_force_a_physical_xform_path_without_result_rows(self) -> None:
        self.assertEqual(
            disabled_xform_settings(
                {"disable_xforms": ["CXformImplementLeftAntiSemiJoin"]}
            ),
            "DO $dsl$ BEGIN PERFORM disable_xform("
            "'CXformImplementLeftAntiSemiJoin'); END $dsl$;",
        )

    def test_e2e_rejects_unsafe_xform_names(self) -> None:
        with self.assertRaisesRegex(ValueError, "invalid xform name"):
            disabled_xform_settings({"disable_xforms": ["x'); DROP TABLE t; --"]})

    def test_postgres_unique_indexes_become_global_key_metadata(self) -> None:
        schema = (
            "CREATE TABLE t(a integer, b integer); "
            "CREATE UNIQUE INDEX t_ab_idx ON t(a, b); "
            "CREATE UNIQUE INDEX t_partial_idx ON t(a) WHERE b > 0;"
        )

        converted = postgres_schema(schema)
        _, unique_keys = schema_catalog(converted)

        self.assertIn("ADD CONSTRAINT t_ab_idx UNIQUE (a, b)", converted)
        self.assertNotIn("t_partial_idx", converted)
        self.assertEqual(unique_keys["t"], {("a", "b")})

    def test_reads_compact_trace_from_postgresql_log(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            trace = Path(directory) / "pgorca.log"
            trace.write_text(
                "unrelated PostgreSQL output\n"
                'LOG:  DSL_TRACE {"kind":"application","engine":"pgorca",'
                '"rule_id":8,"status":"applied","binding_count":3}\n',
                encoding="utf-8",
            )

            self.assertEqual(
                read_records(trace),
                [
                    {
                        "kind": "application",
                        "engine": "pgorca",
                        "rule_id": 8,
                        "status": "applied",
                        "binding_count": 3,
                    }
                ],
            )

    def test_match_without_rewrite_is_reported_missing(self) -> None:
        reference = [{"kind": "application", "rule_id": 8, "status": "applied"}]
        candidate = [
            {
                "kind": "application",
                "rule_id": 8,
                "status": "constraint_rejected",
            }
        ]

        report = compare(reference, candidate, "rule_id")

        self.assertEqual(report["shared_matched"], [8])
        self.assertEqual(report["missing_rewritten"], [8])

    def test_equivalent_duplicate_satisfies_reference_rewrite(self) -> None:
        reference = [{"kind": "application", "rule_id": 8, "status": "applied"}]
        candidate = [
            {"kind": "application", "rule_id": 8, "status": "duplicate"}
        ]

        report = compare(reference, candidate, "rule_id")

        self.assertEqual(report["shared_matched"], [8])
        self.assertEqual(report["shared_rewritten"], [8])
        self.assertEqual(report["missing_rewritten"], [])

    def test_bottom_up_rbo_application_satisfies_reference_rewrite(self) -> None:
        reference = [{"kind": "application", "rule_id": 8, "status": "applied"}]
        candidate = [
            {"kind": "application", "rule_id": 8, "status": "applied_rbo"}
        ]

        report = compare(reference, candidate, "rule_id")

        self.assertEqual(report["shared_matched"], [8])
        self.assertEqual(report["shared_rewritten"], [8])
        self.assertEqual(report["missing_rewritten"], [])

    def test_losing_rbo_alternative_is_capability_not_replacement(self) -> None:
        reference = [{"kind": "application", "rule_id": 8, "status": "applied"}]
        candidate = [
            {"kind": "application", "rule_id": 8, "status": "applicable_rbo"}
        ]

        report = compare(reference, candidate, "rule_id")

        self.assertEqual(report["missing_rewritten"], [8])
        self.assertEqual(report["shared_capable"], [8])
        self.assertEqual(report["missing_capable"], [])

    def test_budget_exhausted_is_matched_but_inconclusive(self) -> None:
        reference = [{"kind": "application", "rule_id": 8, "status": "applied"}]
        candidate = [
            {"kind": "application", "rule_id": 8, "status": "budget_exhausted"}
        ]

        report = compare(reference, candidate, "rule_id")

        self.assertEqual(report["missing_matched"], [])
        self.assertEqual(report["inconclusive_budget"], [8])
        self.assertEqual(report["missing_rewritten"], [])

    def test_budget_skipped_is_not_a_match_but_is_inconclusive(self) -> None:
        reference = [{"kind": "application", "rule_id": 8, "status": "applied"}]
        candidate = [
            {"kind": "application", "rule_id": 8, "status": "budget_skipped"}
        ]

        report = compare(reference, candidate, "rule_id")

        self.assertEqual(report["missing_matched"], [8])
        self.assertEqual(report["inconclusive_budget"], [8])
        self.assertEqual(report["missing_rewritten"], [])

    def test_cascades_budget_makes_downstream_missing_rule_inconclusive(self) -> None:
        reference = [
            {"kind": "application", "rule_id": 8, "status": "applied"},
            {"kind": "application", "rule_id": 9, "status": "applied"},
        ]
        candidate = [
            {"kind": "application", "rule_id": 8, "status": "budget_skipped"}
        ]

        report = compare(reference, candidate, "rule_id")

        self.assertEqual(report["inconclusive_budget"], [8, 9])
        self.assertEqual(report["missing_rewritten"], [])

    def test_rule_summary_budget_also_marks_search_inconclusive(self) -> None:
        reference = [{"kind": "application", "rule_id": 9, "status": "applied"}]
        candidate = [
            {
                "kind": "rule_summary",
                "rule_id": 8,
                "budget_exhausted": 0,
                "budget_skipped": 12,
            }
        ]

        report = compare(reference, candidate, "rule_id")

        self.assertTrue(report["search_budget_limited"])
        self.assertEqual(report["candidate_budget_limited"], [8])
        self.assertEqual(report["inconclusive_budget"], [9])
        self.assertEqual(report["missing_rewritten"], [])

    def test_search_summaries_do_not_change_rule_alignment(self) -> None:
        reference = [{"kind": "application", "rule_id": 8, "status": "applied"}]
        candidate = [
            {"kind": "application", "rule_id": 8, "status": "applied"},
            {
                "kind": "memo_summary",
                "groups": 12,
                "group_expressions": 18,
            },
            {
                "kind": "rule_summary",
                "rule_id": 8,
                "binding_attempts": 7,
                "generated_alternatives": 1,
            },
        ]

        report = compare(reference, candidate, "rule_id")

        self.assertEqual(report["shared_rewritten"], [8])
        self.assertEqual(report["candidate_extra"], [])

    def test_cascades_extra_rewrite_does_not_hide_missing_reference(self) -> None:
        reference = [
            {"kind": "application", "rule_id": 8, "status": "applied"},
            {"kind": "application", "rule_id": 9, "status": "applied"},
        ]
        candidate = [
            {"kind": "application", "rule_id": 8, "status": "applied"},
            {"kind": "application", "rule_id": 42, "status": "applied"},
        ]

        report = compare(reference, candidate, "rule_id")

        self.assertEqual(report["missing_rewritten"], [9])
        self.assertEqual(report["candidate_extra"], [42])

    def test_manifest_redacts_rule_and_plan_material(self) -> None:
        records = [
            {
                "kind": "query",
                "case_id": "case:1",
                "query": "SELECT 1",
                "schema": "PRIVATE_SCHEMA",
            },
            {
                "kind": "application",
                "case_id": "case:1",
                "rule_id": 17,
                "status": "applied",
                "rule": "PRIVATE_RULE",
                "bindings": {"PRIVATE_BINDING": "value"},
                "source": "PRIVATE_SOURCE_PLAN",
                "target": "PRIVATE_TARGET_PLAN",
            },
        ]

        manifest = build_manifest(records, maximum=0)
        rendered = json.dumps(manifest)

        self.assertEqual(
            manifest,
            [
                {
                    "kind": "differential_case",
                    "case_id": "case:1",
                    "query": "SELECT 1",
                    "reference_applications": [{"rule_id": 17, "status": "applied"}],
                }
            ],
        )
        for secret in (
            "PRIVATE_SCHEMA",
            "PRIVATE_RULE",
            "PRIVATE_BINDING",
            "PRIVATE_SOURCE_PLAN",
            "PRIVATE_TARGET_PLAN",
        ):
            self.assertNotIn(secret, rendered)

    def test_manifest_can_replace_query_dialect_and_case_prefix(self) -> None:
        records = [
            {
                "kind": "query",
                "case_id": "source.sql:2",
                "query": "SELECT `i` FROM `t`",
            },
            {
                "kind": "application",
                "case_id": "source.sql:2",
                "rule_id": 7,
                "status": "applied",
            },
        ]

        manifest = build_manifest(
            records,
            maximum=0,
            case_prefix="sample",
            translated_queries=["", 'SELECT "i" FROM "t"'],
        )

        self.assertEqual(manifest[0]["case_id"], "sample:2")
        self.assertEqual(manifest[0]["query"], 'SELECT "i" FROM "t"')

    def test_public_and_private_manifest_shapes_share_the_same_oracle(self) -> None:
        cases = [
            {
                "kind": "differential_case",
                "case_id": "public",
                "query": "SELECT 1",
                "expected_applications": [{"rule_id": 3, "status": "applied"}],
            },
            {
                "kind": "differential_case",
                "case_id": "private",
                "query": "SELECT 2",
                "reference_applications": [{"rule_id": 4, "status": "duplicate"}],
            },
        ]
        with tempfile.TemporaryDirectory() as directory:
            manifest_path = Path(directory) / "cases.jsonl"
            manifest_path.write_text(
                "".join(json.dumps(case) + "\n" for case in cases), encoding="utf-8"
            )
            loaded = read_manifest(manifest_path)

        self.assertEqual(reference_records(loaded[0])[0]["rule_id"], 3)
        self.assertEqual(reference_records(loaded[1])[0]["rule_id"], 4)
        self.assertEqual(reference_records(loaded[1])[0]["status"], "duplicate")

    def test_manifest_without_an_oracle_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            manifest_path = Path(directory) / "empty-oracle.jsonl"
            manifest_path.write_text(
                json.dumps(
                    {
                        "kind": "differential_case",
                        "case_id": "false-positive",
                        "query": "SELECT 1",
                        "expected_applications": [],
                    }
                )
                + "\n",
                encoding="utf-8",
            )

            with self.assertRaisesRegex(ValueError, "at least one expected/reference"):
                read_manifest(manifest_path)

    def test_parameter_count_ignores_literals_identifiers_and_comments(self) -> None:
        query = (
            "SELECT '$9', \"$8\" FROM t WHERE a = $2 "
            "/* $7 */ AND b = $1 -- $6\n"
        )

        self.assertEqual(parameter_count(query), 2)

    def test_parameterized_trace_uses_explain_generic_plan(self) -> None:
        rendered = render_trace_query("SELECT * FROM t WHERE a = $1 LIMIT $2")

        self.assertEqual(
            rendered,
            "EXPLAIN (GENERIC_PLAN, COSTS OFF)\n"
            "SELECT * FROM t WHERE a = $1 LIMIT $2;\n",
        )

    def test_plain_trace_query_is_explained_directly(self) -> None:
        self.assertEqual(
            render_trace_query("SELECT * FROM t;"),
            "EXPLAIN (COSTS OFF)\nSELECT * FROM t;\n",
        )

    def test_orca_fallback_is_not_counted_as_missing_dsl_rewrite(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / "case.log"
            log.write_text(
                "NOTICE: Falling back to Postgres-based planner because GPORCA "
                "does not support the following feature: DISTINCT ON\n",
                encoding="utf-8",
            )

            self.assertEqual(
                orca_fallback_reason(log),
                "Falling back to Postgres-based planner because GPORCA does not "
                "support the following feature: DISTINCT ON",
            )

    def test_internal_orca_fallback_without_standard_notice_is_detected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / "case.log"
            log.write_text(
                '2026-01-01,THD000,ERROR,"Query-to-DXL Translation failed",\n'
                " Seq Scan on t\n",
                encoding="utf-8",
            )

            self.assertEqual(
                orca_fallback_reason(log), "Query-to-DXL Translation failed"
            )

    def test_orca_plan_marker_wins_when_no_fallback_was_logged(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / "case.log"
            log.write_text("Result\n Optimizer: pg_orca\n", encoding="utf-8")

            self.assertIsNone(orca_fallback_reason(log))


if __name__ == "__main__":
    unittest.main()
