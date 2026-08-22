#!/usr/bin/env python3
"""Self-tests for the redacted WeTune/pgORCA differential framework."""

from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from build_reference_manifest import build_manifest
from compare_rule_traces import compare, read_records
from import_wetune_workloads import postgres_schema, schema_catalog
from run_trace_corpus import (
    orca_fallback_reason,
    parameter_count,
    read_manifest,
    reference_records,
    render_trace_query,
)


class TraceFrameworkTest(unittest.TestCase):
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

    def test_parameterized_trace_uses_a_generic_prepared_plan(self) -> None:
        rendered = render_trace_query("SELECT * FROM t WHERE a = $1 LIMIT $2")

        self.assertIn("PREPARE dsl_trace_case AS", rendered)
        self.assertIn("EXECUTE dsl_trace_case(NULL, NULL)", rendered)
        self.assertTrue(rendered.endswith("DEALLOCATE dsl_trace_case;\n"))

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
