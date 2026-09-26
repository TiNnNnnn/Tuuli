"""E2E dependency assertions must survive trace serialization changes."""

import json
import unittest

from run_e2e_cases import actual_plan


class RuleEdgeExpectationTest(unittest.TestCase):
    def test_plain_and_batched_edges_have_the_same_contract(self):
        edge = {"scheduler": "cbo", "src_rule": "a", "dst_rule": "b",
                "target_path": "r", "dst_binding_path": "r",
                "path_kind": "instantiated_expression", "candidate_status": "ready_cbo"}
        plain = {"kind": "rule_edge", "engine": "pgorca", **edge}
        batch = {"kind": "rule_edge_batch", "engine": "pgorca", "scheduler": "cbo",
                 "schema_version": 2, "edges": [
                     ["a", "b", "r", "r", "ready_cbo", 0, 0, 1, 19, 2,
                      "memo_consumes", "memo_inserted"]]}
        for record in (plain, batch):
            output = "LOG: DSL_TRACE " + json.dumps(record)
            expected = {"rule_edges": [edge]}
            self.assertEqual(expected, actual_plan(expected, output))
            for key, wrong in (("src_rule", "c"), ("dst_rule", "c"),
                               ("target_path", "r/0"), ("dst_binding_path", "r/0"),
                               ("candidate_status", "constraint_rejected")):
                self.assertEqual({"rule_edges": []}, actual_plan(
                    {"rule_edges": [{**edge, key: wrong}]}, output))
        self.assertEqual({"rule_edges": []}, actual_plan({"rule_edges": [edge]}, ""))

    def test_malformed_patterns_and_batches_are_not_silently_accepted(self):
        for patterns in ({}, [{}], [{"src_rule": "a"}], [None]):
            with self.assertRaises(ValueError):
                actual_plan({"rule_edges": patterns}, "")
        with self.assertRaises(ValueError):
            actual_plan({"rule_edges": [{"src_rule": "a", "dst_rule": "b"}]},
                        'DSL_TRACE {"kind":"rule_edge_batch","engine":"pgorca",'
                        '"scheduler":"cbo","schema_version":999,"edges":[]}')


if __name__ == "__main__":
    unittest.main()
