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
from run_trace_corpus import read_manifest, reference_records


class TraceFrameworkTest(unittest.TestCase):
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


if __name__ == "__main__":
    unittest.main()
