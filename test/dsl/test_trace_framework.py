#!/usr/bin/env python3
"""Self-tests for the redacted WeTune/pgORCA differential framework."""

from __future__ import annotations

import json
import math
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import MagicMock, patch

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from build_reference_manifest import build_manifest
from instantiate_query_workload import instantiate, instantiate_relations, write_workload as write_parameter_workload
from generate_stats_sweep import discovery_targets, geometric_factors, sweep_points, write_sweep, sampled_input_target, write_cohort_sweeps
from plot_stats_sweep import measured_points, timing_points
from profile_rule_candidates import (candidate_evidence, STAGES, binding_shape_features, cohort_candidate_evidence,
                                     family_design_weights, injection_order, sweep_candidate_evidence, cbo_contribution,
                                     parameter_candidate_evidence, candidate_state, state_coverage, cost_evidence,
                                     cost_lifecycle_evidence, search_check_evidence, search_contribution, target_root_costs,
                                     observed_rule_edges, binding_origin_evidence)
from profile_query_cohort import (build_cohort, cohort_results, query_features, rule_distribution,
                                  local_input_records, placement_evidence, incremental_cohort, stratified_cohort)
from profile_corpus_attempts import select_cases, summarize_trace, coverage, export_workload
from run_workload_comparison import parse_args as parse_workload_args
from profile_rule_conditions import condition_bins, query_cells, aggregate_cells, rule_effects
from profile_rule_pair import paired_evidence
from profile_data_scale import scale_setups
from compare_rule_curves import model_check, transport_check, additive_response_check, clipped_affine_check, summarize as summarize_curve
from evaluate_rule_dimensions import (fit_dimensions, score_dimensions, pooled_cells,
                                      template_cells, validate as validate_dimensions)
from export_rule_examples import render_example, decode_records as decode_rule_examples, summarize_examples
from build_xform_replacement_inventory import (
    audit_memo_provenance,
    merge_inventory,
)
from compare_rule_traces import compare, read_records
from import_wetune_workloads import postgres_schema, schema_catalog, translate_query
from merge_rule_graph import merge_graph, read_trace_inputs, render_dot
from replacement_rule_classification import audit_rule_file, audit_rule_text
from run_dphyper_stability import imported_cases, parse_dphyper_events, summarize
from run_e2e_cases import (
    actual_rows,
    actual_plan,
    bool_guc_setting,
    disabled_xform_settings,
    memo_provenance,
    produced_alternative,
    validate_replacement_matrix,
)
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
from run_workload_comparison import (
    artifact_snapshot,
    select_workload_queries,
    collect_cardinalities,
    collect_postgres_oracle,
    annotate_input_stats_reference,
    load_input_stats_reference,
    distribution,
    dsl_observability,
    error_summary,
    experiment_distribution_summary,
    experiment_observations,
    experiment_run_status,
    explain_times,
    optimization_time,
    optimizer_name,
    plan_difference,
    profile_comparability,
    psql,
    run_profile_timing,
    run_mode,
    copy_result_summary,
    result_semantics,
    results_equal,
    produced_xforms,
    server_failure,
    settings,
    timing_summary,
    timing_plan,
    timing_schedule,
    trace_records,
    trace_settings,
    write_profile_policies,
)


class TraceFrameworkTest(unittest.TestCase):
    def test_clipped_affine_uses_training_active_branch_and_keeps_extrapolation(self) -> None:
        result = clipped_affine_check([1, 2, 4, 6], [0, 0, -2, -4], [3, 8], [-1, -6])
        self.assertTrue(result['identified'])
        self.assertEqual(result['threshold'], 2)
        self.assertEqual(result['predictions'], [-1, -6])
        self.assertEqual(result['mae'], 0)
        self.assertEqual(result['extrapolated'], [False, True])
        changed = clipped_affine_check([1, 2, 4, 6], [0, 0, -2, -4], [3, 8], [99, 99])
        self.assertEqual(changed['predictions'], result['predictions'])
        self.assertFalse(clipped_affine_check([1, 2, 3], [0, 0, 0], [4], [0])['identified'])
        with self.assertRaisesRegex(ValueError, 'positive'):
            clipped_affine_check([1, 2], [1, -1], [3], [-2])

    def test_joint_response_is_predicted_from_axes_not_fitted_to_it(self) -> None:
        xs = [[1, 1], [2, 1], [1, 2], [2, 2]]
        result = additive_response_check(xs, [1, 2, 2, 4], [1, 1])
        point, = result['joint_points']
        self.assertEqual(point['predicted'], 3)
        self.assertEqual(point['interaction_residual'], 1)
        self.assertEqual(result['mae'], 1)
        self.assertEqual(additive_response_check(xs, [1, 2, 2, 99], [1, 1])['joint_points'][0]['predicted'], 3)
        self.assertEqual(additive_response_check(xs, [2, 3, 3, 4], [1, 1])['mae'], 0)
        missing = additive_response_check([xs[0], xs[1], xs[3]], [1, 2, 4], [1, 1])
        self.assertEqual(missing['joint_points'][0]['missing_axes'], [[1, 2]])
        self.assertIsNone(missing['mae'])
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            additive_response_check(xs + [xs[0]], [1, 2, 2, 4, 1], [1, 1])
        with self.assertRaisesRegex(ValueError, 'finite'):
            additive_response_check(xs, [1, 2, 2, float('nan')], [1, 1])

    def test_workload_selection_cannot_silently_run_zero_queries(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            sql = root / "app" / "sql"
            sql.mkdir(parents=True)
            query = sql / "1.sql"
            query.write_text("SELECT 1")
            self.assertEqual(select_workload_queries(root, ["app", "empty"], ["app/1"]),
                             {"app": [query], "empty": []})
            with self.assertRaisesRegex(ValueError, "no queries selected"):
                select_workload_queries(root, ["app"], ["1"])

    def test_artifact_snapshot_tracks_content_and_missing_files(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "engine.so"
            path.write_bytes(b"first")
            before = artifact_snapshot({"engine": path})
            self.assertEqual(before, artifact_snapshot({"engine": path}))
            self.assertEqual(before["engine"]["size"], 5)
            path.write_bytes(b"other")
            self.assertNotEqual(before, artifact_snapshot({"engine": path}))
            path.unlink()
            self.assertIn("error", artifact_snapshot({"engine": path})["engine"])

    def test_e2e_result_rows_use_the_requested_cardinality_experiment(self) -> None:
        args = SimpleNamespace(policy_dir=SCRIPT_DIR / "rules", disable_xform=[])
        expected = {"stats_experiment": "stats_noop_experiment.yaml"}
        with patch("run_e2e_cases.run_sql", return_value="410\n") as run:
            result = actual_rows(args, "SELECT COUNT(*) FROM sample", expected)
        self.assertEqual(result["stats_experiment"], expected["stats_experiment"])
        dsl_sql = run.call_args_list[0].args[1]
        self.assertIn("SET pg_orca.dsl_stats_experiment_path=", dsl_sql)
        self.assertIn("stats_noop_experiment.yaml", dsl_sql)
        self.assertNotIn("dsl_stats_experiment_path", run.call_args_list[1].args[1])
        self.assertEqual(result["output"], result["postgres_output"])

    def test_transfer_models_are_frozen_before_target_observations(self) -> None:
        result = transport_check([1, 2, 4], [3, 5, 9], [1, 3, 8], [3, 7, 17])
        self.assertEqual(result['mae']['affine_rows'], 0)
        self.assertEqual(result['extrapolated'], [False, False, True])
        changed = transport_check([1, 2, 4], [3, 5, 9], [1, 3, 8], [30, 70, 170])
        self.assertEqual(changed['predictions'], result['predictions'])
        self.assertEqual(changed['source_fit'], result['source_fit'])
        self.assertGreater(changed['mae']['affine_rows'], 0)
        with self.assertRaises(ValueError):
            transport_check([1, 2, 4], [3, 5, 9], [1], [])

    def test_result_comparison_uses_bags_only_for_unordered_queries(self) -> None:
        self.assertEqual(result_semantics('SELECT a FROM t'), 'bag')
        self.assertEqual(result_semantics('SELECT a FROM t ORDER BY a'), 'ordered')
        self.assertEqual(result_semantics('SELECT a FROM t UNION SELECT a FROM u ORDER BY 1'), 'ordered')
        self.assertEqual(result_semantics('SELECT a FROM (SELECT a FROM t ORDER BY a) s'), 'bag')
        self.assertEqual(result_semantics('SELECT 1; SELECT 2'), 'ordered')
        self.assertEqual(result_semantics("SELECT 'unterminated"), 'ordered')
        self.assertEqual(result_semantics('SELECT 1 /*'), 'ordered')
        a = {'rows_rc': 0, 'rows_hash': 'first', 'rows_bag_hash': 'same', 'result_semantics': 'bag'}
        b = {**a, 'rows_hash': 'second'}
        self.assertTrue(results_equal(a, b))
        self.assertFalse(results_equal(a, {**b, 'rows_bag_hash': 'different'}))
        self.assertFalse(results_equal(a, {**b, 'rows_rc': 1}))
        self.assertFalse(results_equal({**a, 'result_semantics': 'ordered'}, {**b, 'result_semantics': 'ordered'}))
        self.assertFalse(results_equal(a, {**b, 'result_semantics': 'ordered'}))
        self.assertFalse(results_equal(a, {**b, 'rows_bag_hash': None}))

    def test_template_dimensions_share_features_across_rule_identities(self) -> None:
        features = {'source_nodes': 3, 'constraint_count': 5}
        graph = {'nodes': [{'rule_hash': h, 'template_features': features} for h in ('r', 's')]}
        cells = [{'axis': axis, 'band': 8, 'rule_hash': h, 'complete': True, 'evaluated': 1,
                  'case_id': 'a:1', 'dataset': 'a'} for h in ('r', 's') for axis in ('nodes', 'scalar_nodes')]
        result = pooled_cells(template_cells(cells, graph))
        self.assertEqual(len(result), 3)
        self.assertTrue(all(c['evaluated'] == 2 for c in result))
        self.assertEqual(next(c['band'] for c in result if c['axis'] == 'template_joint'), (2, 4, 8))
        two_apps = result + [{**c, 'case_id': 'b:1', 'dataset': 'b'} for c in result]
        self.assertEqual(validate_dimensions(two_apps), validate_dimensions(json.loads(json.dumps(two_apps))))
        with self.assertRaises(ValueError):
            template_cells(cells, {'nodes': graph['nodes'][:1]})
        with self.assertRaises(ValueError):
            template_cells(cells, {'nodes': [{'rule_hash': 'r'}]})

    def test_generic_dimensions_are_pre_evaluation_and_keep_zero_distinct(self) -> None:
        row = {'evaluated': True, 'input_context': {'capture': 'before_evaluation',
               'scope': 'source_before_match_view', 'source_shape': {'complete': True,
               'pattern_nodes': 0, 'nodes': 6, 'scalar_nodes': 4, 'depth': 3},
               'root': {'stats_source': 'memo_group', 'rows': 0}}}
        bins, _ = condition_bins(row)
        self.assertEqual(bins['relational_nodes'], 2)
        self.assertEqual(bins['scalar_nodes'], 4)
        self.assertEqual(bins['root_rows'], 0)
        row['input_context']['root']['stats_source'] = 'missing'
        self.assertIsNone(condition_bins(row)[0]['root_rows'])
        row['input_context']['capture'] = 'after_evaluation'
        self.assertTrue(all(v is None for v in condition_bins(row)[0].values()))

    def test_dimension_validation_excludes_application_and_rule_identity(self) -> None:
        def cell(app, query, band, count, positives):
            return {'case_id': query, 'dataset': app, 'axis': 'nodes', 'band': band,
                    'complete': True, 'rule_hash': 'ignored', 'evaluated': count,
                    'ready_cbo': positives, 'match_log_sum': positives,
                    'match_log_square_sum': positives}
        data = [cell(app, app+':1', band, 1, band-1) for app in ('a', 'b') for band in (1, 2)]
        pooled = pooled_cells(data)
        for c in data:
            c['rule_hash'] = 'a-different-rule'
        self.assertEqual(pooled_cells(data), pooled)
        for result in validate_dimensions(pooled):
            self.assertEqual(result['baseline_mse'], .25)
            self.assertEqual(result['dimension_mse'], 0)
            self.assertEqual(result['known_fraction'], 1)
        mixed = [cell(app, app+':1', 1, 2, 1) for app in ('a', 'b')]
        for result in validate_dimensions(pooled_cells(mixed)):
            self.assertEqual(result['dimension_mse'], .25)  # Not zero error on cell means.
        unequal = [cell('a', 'a:1', 1, 99, 0), cell('a', 'a:2', 2, 1, 1), cell('b', 'b:1', 3, 1, 0)]
        for result in validate_dimensions(pooled_cells(unequal)):
            heldout = next(f for f in result['folds'] if f['heldout_application'] == 'b')['queries'][0]
            self.assertEqual(heldout['baseline_mse'], .25)  # Query equal, not 1/100 positive.
            self.assertEqual(heldout['dimension_mse'], .25)  # Unseen band falls back to training only.
            self.assertEqual(heldout['known_fraction'], 0)

    def test_frozen_dimensions_do_not_refit_or_hide_rare_positives(self) -> None:
        def cell(query, band, n, positives):
            return {'case_id': query, 'dataset': query.split(':')[0], 'axis': 'nodes',
                    'band': band, 'evaluated': n, 'ready_cbo': positives,
                    'match_log_sum': positives, 'match_log_square_sum': positives}
        frozen = json.loads(json.dumps(fit_dimensions([
            cell('train:1', 1, 99, 0), cell('train:2', 2, 1, 1)])))
        snapshot = json.dumps(frozen, sort_keys=True)
        # Unknown and missing bins fall back to the query-equal training mean (1/2).
        test = [cell('test:1', 4, 99, 0), cell('test:1', None, 1, 1)]
        scored = next(r for r in score_dimensions(frozen, test) if r['target'] == 'ready')
        self.assertEqual(scored['dimension_mse'], .25)
        self.assertEqual(scored['known_fraction'], 0)
        self.assertEqual(scored['positive_rate'], .01)
        self.assertEqual(scored['dimension_positive_brier'], .25)
        self.assertEqual(scored['dimension_negative_brier'], .25)
        score_dimensions(frozen, [cell('test:1', 1, 100, 100)])
        self.assertEqual(json.dumps(frozen, sort_keys=True), snapshot)
        with self.assertRaises(ValueError):
            score_dimensions(frozen, [cell('train:1', 1, 1, 0)])
        negative = next(r for r in score_dimensions(frozen, [cell('test:1', 1, 100, 0)])
                        if r['target'] == 'ready')
        self.assertIsNone(negative['dimension_positive_brier'])

    def test_curve_models_and_failed_designs(self) -> None:
        xs = [1, 2, 4, 8]
        self.assertEqual(model_check(xs, [3]*4)['leave_one_scale_out_mae']['constant'], 0)
        self.assertAlmostEqual(model_check(xs, [2*x+3 for x in xs])['leave_one_scale_out_mae']['affine_rows'], 0)
        with self.assertRaises(ValueError):
            model_check([1, 1, 2], [1, 2, 3])
        with self.assertRaises(ValueError):
            model_check([1, 2, 3], [1, float('nan'), 3])
        report = {'manifest': {'scales': xs, 'rule_hash': 'r', 'timing_repeats': 7},
                  'points': [{'rows': x, 'runner_returncode': 1} for x in xs]}
        result = summarize_curve(report)
        self.assertFalse(result['complete_design'])
        self.assertIsNone(result['execution_model'])
        self.assertIsNone(result['search_constant'])
        self.assertEqual(len(result['points']), 4)
        # Plotting remains optional for framework CI; verify the failed-data domain directly.
        from profile_data_scale import render
        axes = MagicMock()
        axes.flat = [MagicMock() for _ in range(4)]
        plotting = MagicMock()
        plotting.subplots.return_value = (MagicMock(), axes)
        with tempfile.TemporaryDirectory() as directory, patch('profile_data_scale.chinese_plotting', return_value=plotting):
            render(report, Path(directory), None)
        for ax in axes.flat:
            ax.set_xlim.assert_called_once_with(min(xs)/1.2, max(xs)*1.2)

    def test_curve_search_stages_preserve_ready_without_new_root(self) -> None:
        row = {'delta': {'attempts': 9, 'memo_expressions': 0}, 'search': {'complete': True,
               'delta': {'cost:costed': -2}}, 'pairs': [], 'target_attempts': 4,
               'target_ready': 1, 'target_root_inserted': 0, 'rule_deltas': {
                   'r': {'attempts': 4, 'ready_cbo': 1, 'memo_duplicate': 1}}}
        report = {'manifest': {'scales': [10], 'rule_hash': 'r', 'timing_repeats': 7},
                  'points': [{'rows': 10, 'runner_returncode': 0, 'evidence': {'runs': [row]}}]}
        point = summarize_curve(report)['points'][0]
        self.assertEqual(point['target_attempts'], 4)
        self.assertEqual(point['target_ready'], 1)
        self.assertEqual(point['target_root_inserted'], 0)
        self.assertEqual(point['target_stages']['memo_duplicate'], 1)
        self.assertEqual(point['physical_search_delta']['cost:costed'], -2)
        row['pairs'] = [{'delta_planning_ms': 1, 'delta_execution_ms': 0,
                         'delta_plan_plus_execution_ms': 1} for _ in range(7)]
        self.assertIsNone(summarize_curve(report)['execution_model'])
        row['search']['complete'] = False
        invalid = summarize_curve(report)['points'][0]
        self.assertFalse(invalid['audited'])
        self.assertIsNone(invalid['target_stages'])
        self.assertIsNone(invalid['physical_search_delta'])

    def test_data_scale_template_is_explicit_and_preserves_sql(self) -> None:
        template = 'INSERT INTO t SELECT i FROM generate_series(1, ${rows}) g(i);\nANALYZE;'
        setups = scale_setups(template, [1250, 2500])
        self.assertEqual(setups[1250], template.replace('${rows}', '1250'))
        self.assertEqual(setups[2500], template.replace('${rows}', '2500'))
        for invalid in ([], [0], [-1], [True], [1, 1]):
            with self.assertRaises(ValueError):
                scale_setups(template, invalid)
        with self.assertRaises(ValueError):
            scale_setups('SELECT 1', [10])
        with self.assertRaises(KeyError):
            scale_setups(template + ' SELECT ${unknown}', [10])

    def test_pair_report_reuses_audit_and_does_not_fabricate_cardinality(self) -> None:
        scenario = {'stats_experiment': '/tmp/observation.yaml', 'arms': {
            'off': {'experiment_outcomes': [{'stats_targets': []}]},
            'cbo': {'experiment_outcomes': [{'stats_targets': []}]}}}
        comparison = {'rule_profile': {'rule_hash': 'r', 'scenarios': [scenario]}}
        rows = [{'pairs': [{'delta_planning_ms': 2, 'delta_execution_ms': -3},
                           {'delta_planning_ms': None, 'delta_execution_ms': None}]}]
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'comparison.json'
            path.write_text(json.dumps(comparison))
            with patch('profile_rule_pair.cbo_contribution', return_value=rows) as audit:
                result = paired_evidence(path)
                manifest = audit.call_args.args[0]
                self.assertEqual(manifest['targets'], [])
                self.assertEqual(manifest['points'][0]['factors'], [])
                self.assertEqual(result['runs'][0]['pairs'][0]['delta_plan_plus_execution_ms'], -1)
                self.assertIsNone(result['runs'][0]['pairs'][1]['delta_plan_plus_execution_ms'])
            scenario['arms']['cbo']['experiment_outcomes'][0]['stats_targets'] = [{'requested_rows': 100}]
            path.write_text(json.dumps(comparison))
            with self.assertRaisesRegex(ValueError, 'sweep reporter'):
                paired_evidence(path)

    def test_exported_corpus_uses_normal_workload_comparator(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'corpus/app').mkdir(parents=True)
            schema = b'CREATE TABLE t (k int);\n'
            (root / 'corpus/app/schema.sql').write_bytes(schema)
            cases = [{'dataset': 'app', 'cases': [{'case_id': 'app:7', 'query': 'SELECT count(*) FROM t;'}]}]
            export_workload(cases, root / 'corpus', root / 'export')
            self.assertEqual((root / 'export/app/schema.sql').read_bytes(), schema)
            self.assertEqual((root / 'export/app/sql/7.sql').read_text(), cases[0]['cases'][0]['query'])
            with self.assertRaises(FileExistsError):
                export_workload(cases, root / 'corpus', root / 'export')
            base = b'CREATE TABLE t (k int PRIMARY KEY, v int);\n'
            patched = base + b'\n-- WeTune schema patches\nALTER TABLE t ALTER COLUMN v SET NOT NULL;\n'
            (root / 'corpus/app/schema.sql').write_bytes(patched)
            export_workload(cases, root / 'corpus', root / 'base', 'base')
            export_workload(cases, root / 'corpus', root / 'aligned')
            self.assertEqual((root / 'base/app/schema.sql').read_bytes(), base)
            self.assertEqual((root / 'aligned/app/schema.sql').read_bytes(), patched)
            self.assertEqual((root / 'corpus/app/schema.sql').read_bytes(), patched)
            with self.assertRaisesRegex(ValueError, 'schema domain'):
                export_workload(cases, root / 'corpus', root / 'bad', 'unknown')
        argv = ['runner', '--pg-config', '/bin/pg_config', '--audit-bin', '/bin/audit', '--workload', 'app']
        with patch.object(sys, 'argv', argv):
            self.assertEqual(parse_workload_args().workload, ['app'])
        with patch.object(sys, 'argv', argv[:-1] + ['../app']), patch('sys.stderr'):
            with self.assertRaises(SystemExit):
                parse_workload_args()

    def test_rule_effects_keep_work_local_benefits_and_unknowns_separate(self) -> None:
        common = {'experiment': 'e', 'group': 1, 'group_expression': 9, 'optimization_context': 0,
                  'optimization_request': 0, 'search_stage': 0, 'memo_version': 10,
                  'preceding_rule_candidates': 2, 'status': 'costed', 'cost_kind': 'computed',
                  'operator': 'CPhysicalTableScan', 'origin_group': 1, 'origin_expression': 7}
        costs = [{**common, 'sequence': 1, 'cost': 10, 'origin_group': 2},
                 {**common, 'sequence': 2, 'cost': 5, 'context_best_cost_at_event': 10},
                 {**common, 'sequence': 3, 'cost': 12, 'context_best_cost_at_event': 5}]
        lifecycle = [{'status': 'retained_new', 'candidate_sequence': 1},
                     {'status': 'best_updated', 'candidate_sequence': 1},
                     {'status': 'retained_new', 'candidate_sequence': 2},
                     {'status': 'best_updated', 'candidate_sequence': 2, 'previous_candidate_sequence': 1},
                     {'status': 'discarded', 'candidate_sequence': 3, 'previous_candidate_sequence': 2},
                     {'status': 'selected_plan', 'candidate_sequence': 2, 'operator': 'CPhysicalTableScan',
                      'cost': 5, 'plan_node': 1, 'parent_plan_node': 0}]
        lifecycle = [{**e, 'sequence': i, 'experiment': 'e', 'group': 1, 'optimization_context': 0}
                     for i, e in enumerate(lifecycle, 1)]
        run = {'plan_rc': 0, 'rows_rc': 0, 'optimizer': 'pg_orca', 'cost_events': costs,
               'cost_lifecycle_events': lifecycle, 'experiment_outcomes': [{'experiment': 'e', 'stats_targets': [],
               'cost_trace_version': 1, 'cost_candidates': 3, 'rule_candidates': 2,
               'cost_lifecycle_version': 1, 'cost_lifecycle_events': 6}]}
        rejected = {'rule_hash': 'r', 'status': 'match_rejected', 'match_us': 2, 'constraint_us': 0,
                    'instantiate_us': 0, 'direct_insertions': None, 'memo_outcome': None}
        ready = {**rejected, 'status': 'ready_cbo', 'instantiate_us': 7, 'direct_insertions': 3,
                 'memo_outcome': {'status': 'memo_inserted', 'group': 1, 'group_expression': 7}}
        audit = {'complete': True, 'exclusions': [], 'rows': [rejected, ready]}
        result = rule_effects(run, audit, [{'src_rule': 'r', 'dst_rule': 'other'}])
        self.assertTrue(result['complete'], result['exclusions'])
        row = result['rules'][0]
        self.assertEqual(row['evaluation_us'], 11)
        self.assertEqual(row['nonready_evaluation_us'], 2)
        self.assertEqual(row['cost_sequences'], [2, 3])  # Same expression ID in another group is not owned.
        self.assertEqual([c['status'] for c in row['comparisons']], ['improved', 'worse'])
        self.assertEqual(row['comparisons'][0]['relative_reduction'], .5)
        self.assertTrue(row['comparisons'][0]['became_best'])
        self.assertFalse(row['comparisons'][1]['became_best'])
        self.assertEqual(row['lifecycle_status_counts']['selected_plan'], 1)
        self.assertEqual(row['outgoing_consumptions'], {'other': 1})
        del costs[1]['context_best_cost_at_event']
        comparison = rule_effects(run, audit, [])['rules'][0]['comparisons'][0]
        self.assertEqual(comparison['status'], 'no_incumbent')
        self.assertIsNone(comparison['relative_reduction'])
        costs[1]['context_best_cost_at_event'] = 5 + 1e-10
        self.assertEqual(rule_effects(run, audit, [])['rules'][0]['comparisons'][0]['status'],
                         'equal_at_trace_precision')
        lifecycle.pop()
        partial = rule_effects(run, audit, [])
        self.assertFalse(partial['complete'])
        self.assertEqual(partial['rules'], [])  # Incomplete evidence is not an observed zero benefit.

    def test_rule_condition_rates_balance_queries_and_preserve_unknowns(self) -> None:
        rejected = {'rule_hash': 'r', 'status': 'match_rejected', 'evaluated': True,
                    'match_us': 1, 'constraint_us': 0, 'instantiate_us': 0,
                    'input_context': {'capture': 'before_evaluation', 'scope': 'source_before_match_view',
                                      'root': {'stats_source': 'missing', 'rows': None},
                                      'source_shape': {'complete': True, 'pattern_nodes': 0, 'nodes': 7, 'depth': 3}}}
        ready = {**rejected, 'status': 'ready_cbo', 'instantiate_us': 20, 'direct_insertions': 3}
        a, available = query_cells([rejected] * 99, 'a:1', 'a', True)
        b, _ = query_cells([ready], 'b:1', 'b', True)
        partial, _ = query_cells([ready], 'c:1', 'c', False)
        points = aggregate_cells(a + b + partial)
        point = next(p for p in points if p['complete'] and p['axis'] == 'nodes')
        self.assertEqual(point['band'], 4)
        self.assertEqual(point['metrics']['ready_rate']['query_mean'], .5)
        self.assertEqual(point['metrics']['ready_rate']['pooled'], .01)
        self.assertEqual(point['metrics']['construction_us']['query_mean'], 20)
        self.assertEqual(point['metrics']['construction_us']['queries'], 1)
        self.assertEqual(point['metrics']['memo_growth']['query_mean'], 1.5)
        self.assertEqual(available['root_rows_available'], 0)
        unknown = {**rejected, 'input_context': {**rejected['input_context'],
                    'source_shape': {'complete': False, 'nodes': 7, 'depth': 3, 'pattern_nodes': 0}}}
        self.assertIsNone(condition_bins(unknown)[0]['nodes'])
        after = {**rejected, 'input_context': {**rejected['input_context'], 'capture': 'after_evaluation'}}
        self.assertIsNone(condition_bins(after)[0]['nodes'])
        unresolved, _ = query_cells([{**ready, 'direct_insertions': None}], 'd:1', 'd', False)
        self.assertIsNone(aggregate_cells(unresolved)[0]['metrics']['memo_growth'])

    def test_mysql_function_lowering_preserves_date_and_aggregate_structure(self) -> None:
        sql, _ = translate_query("SELECT datediff(a,b), to_days(a), "
                                 "group_concat(DISTINCT id SEPARATOR ';'), group_concat(id ORDER BY id), "
                                 "concat('2019-07-08', ' 00:00:00'), concat(a, NULL), ?", "mysql")
        self.assertIn('(CAST("a" AS DATE) - CAST("b" AS DATE))', sql)
        self.assertIn("CAST('0001-01-01' AS DATE) + 366", sql)
        self.assertIn('STRING_AGG(DISTINCT CAST("id" AS TEXT)', sql)
        self.assertIn('ORDER BY "id"', sql)
        self.assertIn("'2019-07-08 00:00:00'", sql)
        self.assertIn('NULL', sql)
        self.assertIn('$1', sql)
        self.assertNotIn('0000-01-01', sql)
        postgres, _ = translate_query("SELECT age(a,b)", "postgres")
        self.assertIn('AGE', postgres)
        with self.assertRaisesRegex(ValueError, 'typed order normalization'):
            translate_query('SELECT group_concat(DISTINCT id ORDER BY id)', 'mysql')
        source = "SELECT DATEDIFF('2007-12-31 23:59:59','2007-12-30') AS delta, " \
                 "TO_DAYS('2007-10-07') AS day_no, TO_DAYS(NULL) IS NULL AS null_day, " \
                 "DATEDIFF('2000-02-28','2000-03-01') AS leap_delta, " \
                 "GROUP_CONCAT(k ORDER BY k SEPARATOR ';') AS ids FROM dsl_eq_left " \
                 "WHERE CONCAT('2019-07-08',' 00:00:00') = '2019-07-08 00:00:00'"
        self.assertEqual(translate_query(source, 'mysql')[0] + ';\n',
                         (SCRIPT_DIR / 'e2e/sql/mysql_function_translation.sql').read_text())

    def test_plan_comparison_ignores_execution_metrics_but_keeps_optimizer_choices(self) -> None:
        baseline = {"Node Type": "Hash Join", "Actual Rows": 20, "Shared Hit Blocks": 3,
                    "Hash Cond": "a.id = b.id", "Plans": [{"Node Type": "Hash", "Hash Buckets": 1024,
                        "Original Hash Buckets": 1024, "Peak Memory Usage": 10,
                        "Plans": [{"Node Type": "Seq Scan", "Relation Name": "b"}]}]}
        changed = json.loads(json.dumps(baseline))
        changed.update({"Actual Rows": 30, "Shared Hit Blocks": 8})
        changed["Plans"][0].update({"Hash Buckets": 2048, "Original Hash Buckets": 2048, "Peak Memory Usage": 20})
        self.assertEqual(plan_difference(baseline, changed), "identical")
        self.assertEqual(baseline["Plans"][0]["Hash Buckets"], 1024)
        changed["Hash Cond"] = "a.other = b.id"
        self.assertEqual(plan_difference(baseline, changed), "expression_or_property")
        changed["Node Type"] = "Nested Loop"
        self.assertEqual(plan_difference(baseline, changed), "physical_shape")
        self.assertEqual(plan_difference(None, changed), "plan_error")

    def test_search_checks_use_actual_bounds_and_validate_prior_cost_references(self) -> None:
        candidate = {"experiment": "e", "sequence": 1, "group": 1, "group_expression": 2,
                     "optimization_context": 0, "optimization_request": 0, "search_stage": 0,
                     "memo_version": 10, "preceding_rule_candidates": 0,
                     "status": "costed", "cost_kind": "computed", "cost": 10}
        check = {"experiment": "e", "sequence": 1, "check": "prune", "status": "pruned",
                 "preceding_rule_candidates": 0, "preceding_cost_candidates": 1,
                 "incumbent_candidate": 1, "incumbent_cost": 10, "lower_bound": 12}
        run = {"optimizer": "pg_orca", "plan_rc": 0, "rows_rc": 0, "cost_events": [candidate],
               "search_checks": [check], "experiment_outcomes": [{"experiment": "e", "stats_targets": [],
                   "cost_trace_version": 1, "cost_candidates": 1, "rule_candidates": 0,
                   "search_check_version": 1, "search_checks": 1}]}
        report = search_check_evidence(run)
        self.assertTrue(report["complete"], report["exclusions"])
        self.assertEqual(report["margins"][0]["relative_margin"], 0.2)
        check["status"] = "bound_not_worse"
        self.assertIn("contradictory_pruning_comparison", search_check_evidence(run)["exclusions"])
        check["lower_bound"] = 10
        self.assertTrue(search_check_evidence(run)["margins"][0]["near_display_precision"])
        check["incumbent_candidate"] = 2
        self.assertIn("unresolved_search_check_cost_reference", search_check_evidence(run)["exclusions"])
        check["incumbent_candidate"] = 1
        check["incumbent_cost"] = None
        self.assertIn("invalid_pruning_comparison", search_check_evidence(run)["exclusions"])
        check.update(incumbent_cost=0, lower_bound=0)
        candidate["cost"] = 0
        self.assertIsNone(search_check_evidence(run)["margins"][0]["relative_margin"])
        run["search_checks"] = []
        self.assertIn("search_check_count_mismatch", search_check_evidence(run)["exclusions"])

    def test_cost_lifecycle_tracks_exact_retention_replacement_and_selected_identity(self) -> None:
        candidate = {"experiment": "e", "group": 1, "group_expression": 2, "operator": "CPhysicalScan",
                     "optimization_context": 0, "optimization_request": 0, "search_stage": 0,
                     "memo_version": 10, "preceding_rule_candidates": 0, "status": "costed", "cost_kind": "computed"}
        lifecycle = [
            {"status": "retained_new", "candidate_sequence": 1, "previous_candidate_sequence": 0},
            {"status": "best_updated", "candidate_sequence": 1, "previous_candidate_sequence": 0},
            {"status": "retained_replacement", "candidate_sequence": 2, "previous_candidate_sequence": 1},
            {"status": "best_updated", "candidate_sequence": 2, "previous_candidate_sequence": 1},
            {"status": "selected_plan", "candidate_sequence": 2, "operator": "CPhysicalScan", "cost": 3,
             "plan_node": 1, "parent_plan_node": 0}]
        events = [{**e, "sequence": i + 1, "experiment": "e", "group": 1, "optimization_context": 0}
                  for i, e in enumerate(lifecycle)]
        run = {"optimizer": "pg_orca", "plan_rc": 0, "rows_rc": 0, "cost_lifecycle_events": events,
               "cost_events": [{**candidate, "sequence": 1, "cost": 5}, {**candidate, "sequence": 2, "cost": 3}],
               "experiment_outcomes": [{"experiment": "e", "stats_targets": [], "cost_trace_version": 1,
                    "cost_candidates": 2, "rule_candidates": 0, "cost_lifecycle_version": 1, "cost_lifecycle_events": 5}]}
        report = cost_lifecycle_evidence(run)
        self.assertTrue(report["complete"], report["exclusions"])
        self.assertEqual(report["ever_best_candidates"], 2)
        self.assertEqual(report["selected_distinct_candidates"], 1)
        self.assertEqual(report["retained_without_observed_replacement"], 1)
        child = {"group": 1, "optimization_context": 0, "cost_candidate_sequence": 1}
        run["cost_events"][1]["child_contexts"] = [child]
        self.assertEqual(len(cost_lifecycle_evidence(run)["cost_child_links"]), 1)
        child["cost_candidate_sequence"] = 99
        self.assertIn("unresolved_costed_child_reference", cost_lifecycle_evidence(run)["exclusions"])
        run["cost_events"][1].pop("child_contexts")
        events[-1]["cost"] = None
        self.assertIn("selected_candidate_mismatch", cost_lifecycle_evidence(run)["exclusions"])
        events[-1]["cost"] = 3
        events[-1]["candidate_sequence"] = 1
        self.assertIn("selected_candidate_not_retained_and_costed", cost_lifecycle_evidence(run)["exclusions"])
        events[-1]["candidate_sequence"] = 0  # Unsupported extraction must not guess a candidate.
        self.assertIn("unresolved_cost_lifecycle_reference", cost_lifecycle_evidence(run)["exclusions"])
        events[-1]["candidate_sequence"] = 2
        events[3]["previous_candidate_sequence"] = 0
        self.assertIn("inconsistent_best_update", cost_lifecycle_evidence(run)["exclusions"])
        run["cost_lifecycle_events"] = events[:2]
        self.assertIn("missing_retention_decisions", cost_lifecycle_evidence(run)["exclusions"])
        self.assertIn("cost_lifecycle_count_mismatch", cost_lifecycle_evidence(run)["exclusions"])
        self.assertIn("missing_selected_physical_plan", cost_lifecycle_evidence(run)["exclusions"])

    def test_cost_event_audit_separates_computed_cost_pruning_and_incomplete_traces(self) -> None:
        event = {"kind": "cost_candidate", "experiment": "e", "sequence": 1, "group": 4,
                 "group_expression": 2, "optimization_context": 0, "optimization_request": 0,
                 "search_stage": 0, "memo_version": 10, "preceding_rule_candidates": 3,
                 "status": "costed", "cost": 4.5, "cost_kind": "computed", "rows": 0}
        run = {"optimizer": "pg_orca", "plan_rc": 0, "rows_rc": 0,
               "experiment_outcomes": [{"experiment": "e", "stats_targets": [], "cost_trace_version": 1,
                                        "cost_candidates": 1, "rule_candidates": 3}],
               "cost_events": [event], "stats_events": [{"group": 4}]}
        report = cost_evidence(run)
        self.assertTrue(report["complete"], report["exclusions"])
        self.assertEqual(report["costed_with_rows"], 1)
        self.assertEqual(report["events_in_observed_stats_group"], 1)
        event.update(status="pruned", cost_kind="lower_bound")
        self.assertTrue(cost_evidence(run)["complete"])
        self.assertEqual(cost_evidence(run)["costed_with_rows"], 0)
        event["cost_kind"] = "computed"
        self.assertIn("invalid_cost_or_bound", cost_evidence(run)["exclusions"])
        event.update(cost_kind="lower_bound", preceding_rule_candidates=4, sequence=2)
        self.assertIn("invalid_candidate_watermark", cost_evidence(run)["exclusions"])
        self.assertIn("cost_sequence_gap_or_duplicate", cost_evidence(run)["exclusions"])
        run["cost_events"] = []
        self.assertIn("cost_count_mismatch", cost_evidence(run)["exclusions"])
        run["experiment_outcomes"][0].pop("cost_trace_version")
        self.assertIn("cost_trace_version_missing", cost_evidence(run)["exclusions"])
        run.update(plan_rc=124)
        self.assertIn("unsuccessful_experiment", cost_evidence(run)["exclusions"])

    def test_candidate_state_keeps_time_provenance_and_missing_statistics_separate(self) -> None:
        node = {"operator": "CLogicalGet", "arity": 0, "stats_source": "memo_group",
                "rows": 0, "empty": True, "reference_key": "query-local"}
        row = {"evaluated": True, "group": 5, "memo_version": 10, "status": "ready_cbo",
               "binding_context": {"symbols": ["post-evaluation"]},
               "input_context": {"capture": "before_evaluation", "scope": "source_before_match_view",
                   "root": node, "children": [{"position": 1, "node": node}],
                   "relational_children": 1, "omitted_children": 0,
                   "source_shape": {"complete": True, "pattern_nodes": 0}}}
        state = candidate_state(row)
        self.assertTrue(state["features"]["root"]["rows_available"])
        self.assertEqual(state["features"]["root"]["rows"], 0)
        self.assertNotIn("reference_key", state["features"]["root"])
        node.update(memo_group_expressions=7, logical_properties={"source": "complete_memo_group",
                    "output_columns": 2, "outer_columns": 0, "not_null_columns": 1,
                    "key_count": 1, "join_depth": 1, "ignored_column_id": 99})
        state = candidate_state(row)
        self.assertEqual(state["features"]["root"]["memo_group_expressions"], 7)
        self.assertEqual(state["features"]["root"]["logical_properties"]["key_count"], 1)
        self.assertNotIn("ignored_column_id", state["features"]["root"]["logical_properties"])
        changed = {**row, "group": 99, "memo_version": 100, "status": "match_rejected", "binding_context": {}}
        self.assertEqual(candidate_state(changed)["features"], state["features"])
        self.assertNotEqual(candidate_state(changed)["provenance"], state["provenance"])
        self.assertEqual(state_coverage([row])["all_relational_child_rows_available"], 1)
        row["input_context"]["omitted_children"] = 1
        self.assertEqual(state_coverage([row])["all_relational_child_rows_available"], 0)
        node["stats_source"] = "missing"
        self.assertIsNone(candidate_state(row)["features"]["root"]["rows"])
        node["stats_source"] = "expression"
        for invalid in (None, -1, float("nan"), float("inf"), True):
            node["rows"] = invalid
            self.assertFalse(candidate_state(row)["features"]["root"]["rows_available"])
        node["rows"] = 42
        for change in ({"evaluated": False}, {"context_resolution_errors": ["input_context"]},
                       {"input_context": {**row["input_context"], "capture": "after_evaluation"}}):
            invalid = {**row, **change}
            self.assertFalse(candidate_state(invalid)["captured"])
            self.assertIsNone(candidate_state(invalid)["features"]["root"]["rows"])
        self.assertEqual(state_coverage([])["captured"], 0)

    def test_shape_features_separate_pre_input_from_bound_predicate_and_unknowns(self) -> None:
        shape = {"complete": True, "pattern_nodes": 0, "nodes": 3, "depth": 2,
                 "operators": {"CScalarSubqueryExists": 1}}
        predicate = {"kind": "predicate", "bound": True, "derived": False,
                     "shape": {**shape, "operators": {"CScalarCmp": 1}}}
        table = {"kind": "table", "bound": True, "derived": False, "shape": shape}
        row = {"input_context": {"capture": "before_evaluation", "source_shape": shape},
               "binding_context": {"capture": "after_evaluation", "scope": "source_table_predicate_symbols",
                                   "omitted_symbols": 0, "symbols": [predicate, table]}}
        self.assertEqual(binding_shape_features(row), {"source_subqueries": "CScalarSubqueryExists",
            "bound_predicate_subqueries": "no_subquery", "bound_predicate_structure": "nonconstant_shape",
            "bound_table_nodes": 3, "bound_table_depth": 2})
        predicate["shape"]["operators"] = {"CScalarConst": 1}
        self.assertEqual(binding_shape_features(row)["bound_predicate_structure"], "constant_only")
        predicate["shape"]["complete"] = False
        self.assertEqual(binding_shape_features(row)["bound_predicate_structure"], "unknown")
        predicate["shape"]["complete"] = True
        shape["pattern_nodes"] = 1
        self.assertEqual(binding_shape_features(row)["source_subqueries"], "unknown")
        self.assertIsNone(binding_shape_features(row)["bound_table_nodes"])
        shape["pattern_nodes"] = 0
        shape["complete"] = False
        self.assertEqual(binding_shape_features(row)["source_subqueries"], "unknown")
        predicate["derived"] = True
        self.assertEqual(binding_shape_features(row)["bound_predicate_subqueries"], "derived")
        self.assertEqual(binding_shape_features(row)["bound_predicate_structure"], "unknown")
        predicate["bound"] = False
        self.assertEqual(binding_shape_features(row)["bound_predicate_subqueries"], "unbound")
        row["binding_context"]["omitted_symbols"] = 1
        self.assertEqual(binding_shape_features(row)["bound_predicate_subqueries"], "unknown")
        self.assertEqual(binding_shape_features({})["source_subqueries"], "unknown")

    def test_candidate_context_encoding_is_lossless_and_missing_references_visible(self) -> None:
        definition = {"kind": "candidate_context", "field": "input_context", "context_id": 1,
                      "value": {"capture": "before_evaluation", "root": {"rows": None}}}
        candidate = {"kind": "rule_candidate", "input_context_id": 1}
        encode = lambda rows: "\n".join("DSL_TRACE " + json.dumps(r) for r in rows)
        decoded = trace_records(encode([definition, candidate, candidate]))
        self.assertEqual(decoded[1]["input_context"], definition["value"])
        # Buffered GPOS entries and per-entry PostgreSQL LOG messages must
        # decode identically, including unrelated messages between entries.
        streamed = '\nWARNING: unrelated message\n'.join(
            'LOG:  2026-09-09,THD000,TRACE,"DSL_TRACE ' + json.dumps(row) + '",'
            for row in [definition, candidate, candidate])
        self.assertEqual(trace_records(streamed), decoded)
        decoded[1]["input_context"]["root"]["rows"] = 3
        self.assertIsNone(decoded[2]["input_context"]["root"]["rows"])
        self.assertEqual(trace_records(encode([candidate]))[0]["context_resolution_errors"], ["input_context"])
        changed = {**definition, "value": {"root": {"rows": 42}}}
        with self.assertRaisesRegex(ValueError, "conflicting"):
            trace_records(encode([definition, changed]))
        decoded = trace_records(encode([definition, candidate, {"kind": "experiment_outcome"}, changed, candidate]))
        self.assertEqual(decoded[-1]["input_context"]["root"]["rows"], 42)

    def test_candidate_audit_checks_full_denominators_and_exact_insertion_interval(self) -> None:
        rejected = {"kind": "rule_candidate", "experiment": "e", "sequence": 1,
                    "placement": "cbo", "rule_hash": "r", "status": "match_rejected",
                    "evaluated": True, "input_context": {"capture": "before_evaluation"},
                    "match_us": 2, "constraint_us": 0, "instantiate_us": 0}
        ready = {**rejected, "sequence": 2, "status": "ready_cbo", "constraint_us": 3, "instantiate_us": 4}
        outcome = {"kind": "rule_candidate_outcome", "experiment": "e", "candidate_sequence": 2,
                   "rule_hash": "r", "status": "memo_inserted", "memo_version_before": 4,
                   "memo_version_at_insertion_start": 10, "memo_version_after": 12}
        counters = {field: 0 for field, _, _ in STAGES.values()}
        counters.update(binding_attempts=2, match_rejected=1, generated_alternatives=1,
                        match_us=4, constraint_us=3, instantiate_us=4)
        run = {"plan_rc": 0, "rows_rc": 0, "optimizer": "pg_orca",
               "experiment_outcomes": [{"experiment": "e", "stats_targets": [],
                                        "candidate_trace_version": 2, "rule_candidates": 2}],
               "candidate_events": [rejected, ready, outcome], "dsl_observability": {"rules": {"r": counters}}}
        report = candidate_evidence(run)
        self.assertTrue(report["complete"], report["exclusions"])
        self.assertEqual(report["attempts"], 2)
        self.assertEqual(report["rows"][1]["direct_insertions"], 2)  # Not 12 - 4.
        records = [rejected, ready, outcome,
                   {"kind": "rule_summary", "rule_hash": "r", **counters},
                   {"kind": "experiment_outcome", **run["experiment_outcomes"][0]}]
        text = "\n".join("DSL_TRACE " + json.dumps(r) for r in records)
        observed = summarize_trace(text, 0, None)
        self.assertTrue(observed["complete"], observed["exclusions"])
        self.assertEqual(observed["attempts"], 2)  # No selected-plan event is required.
        self.assertEqual(observed["statuses"]["match_rejected"], 1)
        partial = summarize_trace(text, 3, "fallback")
        self.assertFalse(partial["complete"])
        self.assertEqual(partial["attempts"], 2)  # Failed-query evidence is not discarded.
        self.assertEqual(coverage([observed, partial])["attempts"], 4)
        self.assertEqual(coverage([observed])["attempted_rules"], ["r"])
        run["candidate_events"] = [ready, outcome]
        self.assertFalse(candidate_evidence(run)["complete"])
        run["candidate_events"] = [rejected, ready]
        self.assertIn("unresolved_ready_candidate", candidate_evidence(run)["exclusions"])
        run["candidate_events"].append({**outcome, "rule_hash": "other"})
        with self.assertRaisesRegex(ValueError, "mismatched"):
            candidate_evidence(run)
        run["candidate_events"] = [rejected, ready, outcome]
        counters["match_us"] += 1
        self.assertIn("rule_timing_mismatch:r:match_us", candidate_evidence(run)["exclusions"])
        counters["match_us"] -= 1
        run["experiment_outcomes"][0]["candidate_trace_version"] = 1
        self.assertIn("candidate_trace_version_missing", candidate_evidence(run)["exclusions"])

    def test_corpus_sampling_keeps_duplicate_sql_and_line_identity(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            app = root / "app"
            app.mkdir()
            (app / "schema.sql").write_text("CREATE TABLE t (a int);")
            (app / "cases.sql").write_text("SELECT 1\n\nSELECT 1\nSELECT 2\n")
            sampled = select_cases(root, 2, 7)
            self.assertEqual(sampled, select_cases(root, 2, 7))
            self.assertEqual(sampled[0]["population"], 3)
            rest = select_cases(root, 2, 7, offset=2)[0]["cases"]
            self.assertFalse({c["case_id"] for c in rest} & {c["case_id"] for c in sampled[0]["cases"]})
            full = select_cases(root, 0, 7)[0]["cases"]
            self.assertEqual({c["case_id"] for c in full}, {"app:1", "app:3", "app:4"})
            self.assertEqual(sum(c["query"] == "SELECT 1" for c in full), 2)

    def test_workload_applied_sequence_excludes_candidate_events_not_repeated_applications(self) -> None:
        application = {"kind": "application", "status": "applied_rbo", "rule_hash": "target"}
        cost = {"kind": "cost_candidate", "status": "costed", "group": 7, "optimization_context": 2}
        lifecycle = {"kind": "cost_lifecycle", "status": "selected_plan", "candidate_sequence": 1}
        check = {"kind": "search_check", "check": "properties", "status": "accepted"}
        edge = {"kind": "rule_edge", "src_rule": "r", "dst_rule": "s"}
        records = [application, {**application, "kind": "rule_candidate"}, application, cost, lifecycle, check, edge]
        trace = "\n".join("DSL_TRACE " + json.dumps(r) for r in records)
        args = SimpleNamespace(profile_rule="target", port=1, timeout=60)
        with tempfile.TemporaryDirectory() as temporary, patch("run_workload_comparison.psql") as execute:
            execute.side_effect = [("[]", trace, 0, 1), ("1\n", "", 0, 1)]
            result = run_mode(args, Path("psql"), Path("/tmp"), "db", "SELECT 1", Path(temporary),
                              "cbo", "replacement", [], None)
        self.assertEqual(result["applied_rule_hashes"], ["target", "target"])
        self.assertEqual(result["cost_events"], [cost])
        self.assertEqual(result["cost_lifecycle_events"], [lifecycle])
        self.assertEqual(result["search_checks"], [check])
        self.assertEqual(result["rule_edges"], [edge])
        self.assertTrue(all(call.kwargs["retry_on_server_failure"] is False for call in execute.call_args_list))

    def test_workload_result_count_uses_csv_records_and_preserves_failures(self) -> None:
        args = SimpleNamespace(profile_rule="target", port=1, timeout=60)
        for output, plan_rc, rows_rc, expected in (
                ("", 0, 0, 0), ("0,,\n", 0, 0, 1),
                ('"first\nsecond",1\n"",2\n', 0, 0, 2),
                ('"' + 'x' * 150000 + '"\n', 0, 0, 1),
                ("\n", 0, 0, 1), ("partial", 0, 1, None), ("", 1, 1, None)):
            with self.subTest(output=output, plan_rc=plan_rc, rows_rc=rows_rc), \
                    tempfile.TemporaryDirectory() as temporary, patch("run_workload_comparison.psql") as execute:
                message = "COPY ERROR" if rows_rc else ""
                execute.side_effect = [("[]", "", plan_rc, 1), (output, message, rows_rc, 1)]
                result = run_mode(args, Path("psql"), Path("/tmp"), "db", "SELECT 1",
                                  Path(temporary), "cbo", "replacement", [], None)
                self.assertEqual(result["rows_count"], expected)
                self.assertEqual(execute.call_count, 1 if plan_rc else 2)
                self.assertEqual((Path(temporary) / "cbo.rows.csv").read_text(), "" if plan_rc else output)
                self.assertEqual((Path(temporary) / "cbo.rows.stderr").read_text(), "" if plan_rc else message)
                if expected is None:
                    self.assertIsNone(result["rows_bag_hash"])

    def test_named_sql_parameters_are_values_and_keep_all_instances(self) -> None:
        sql = "SELECT :v, :v, :flag, :n"
        rendered = instantiate(sql, {"v": "x'; DROP TABLE t;--", "flag": True, "n": None})
        import sqlglot
        tree = sqlglot.parse_one(rendered, read="postgres")
        self.assertEqual(tree.expressions[0].this, "x'; DROP TABLE t;--")
        self.assertEqual(tree.expressions[0], tree.expressions[1])
        for query, values in ((sql, {"v": 1}), ("SELECT :v", {"v": 1, "extra": 2}),
                              ("SELECT :v", {"v": [1]}), ("SELECT :v", {"v": float("nan")}),
                              ("SELECT :v; SELECT 2", {"v": 1}), ("SELECT :v, $1", {"v": 1}),
                              ("SELECT * FROM :v", {"v": "t"})):
            with self.assertRaises(ValueError):
                instantiate(query, values)
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "tpch").mkdir()
            (root / "tpch/schema.sql").write_text("CREATE TABLE t(x int);")
            (root / "template.sql").write_text("SELECT * FROM t WHERE x >= :v")
            spec = {"schema_version": 1, "workload": "tpch", "parameter_design": {"method": "test"},
                    "templates": [{"id": "t", "sql_file": "template.sql", "cases": [
                        {"id": "one", "parameters": {"v": 1}}, {"id": "two", "parameters": {"v": 1}}]}]}
            path = root / "cases.json"
            path.write_text(json.dumps(spec))
            manifest = write_parameter_workload(path, root, root / "generated")
            self.assertEqual(len(manifest["queries"]), 2)  # Never collapse similar/equal SQL instances.
            self.assertEqual(manifest["queries"][0]["query_crc32"], manifest["queries"][1]["query_crc32"])
            self.assertEqual((root / "generated/tpch/schema.sql").read_text(), (root / "tpch/schema.sql").read_text())
            with self.assertRaises(FileExistsError):
                write_parameter_workload(path, root, root / "generated")
            spec["templates"][0]["cases"][1]["parameters"] = {}
            path.write_text(json.dumps(spec))
            with self.assertRaises(ValueError):
                write_parameter_workload(path, root, root / "invalid")
            self.assertFalse((root / "invalid").exists())

    def test_parameter_report_keeps_siblings_and_missing_or_failed_instances(self) -> None:
        manifest = {"sampling_unit": "declared_parameter_instances", "parameter_design": {"method": "test"},
                    "queries": [{"query": f"tpch/{name}", "query_crc32": name, "template": "same",
                                 "case": name, "parameters": {"x": i}}
                                for i, name in enumerate(("one", "two", "failed", "missing"))]}
        rejected = {"rule_hash": "r", "status": "match_rejected", "direct_insertions": None, "memo_outcome": None}
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for i, query in enumerate(manifest["queries"][:3]):
                run = {"optimizer": "pg_orca", "plan_rc": 0, "rows_rc": 0,
                       "experiment_outcomes": [{"stats_targets": []}], "candidate_events": [],
                       "dsl_observability": {"memo": {"group_expressions": i + 10}}, "fake_count": i + 1}
                if query["case"] == "failed":
                    run.update(plan_rc=124, error="timeout", experiment_outcomes=[])
                comparison = {"query_crc32": query["query_crc32"], "modes": {"replacement": run},
                              "rule_profile": {"rule_hash": "r"},
                              "stats_experiments": [{"path": "/tmp/observe.yaml", "modes": {"replacement": run},
                                  "comparisons": {"replacement": {"rows_equal": True, "plan_comparison": "identical"}}}]}
                path = root / query["query"] / "comparison.json"
                path.parent.mkdir(parents=True)
                path.write_text(json.dumps(comparison))
            with patch("profile_rule_candidates.candidate_evidence", side_effect=lambda r: {
                    "complete": r["plan_rc"] == 0, "exclusions": [] if r["plan_rc"] == 0 else ["plan_timeout"],
                    "attempts": r["fake_count"], "rows": [rejected] * r["fake_count"]}):
                rows = parameter_candidate_evidence(manifest, root)["runs"]
                contribution = {"delta": None, "target_states": [{"predicate_structure": "unknown"}]}
                with patch("profile_rule_candidates.cbo_contribution", return_value=[contribution]) as contrast:
                    profiled = parameter_candidate_evidence(manifest, root, "r")
                    self.assertEqual(profiled["runs"][0]["contribution"], contribution)
                    self.assertNotIn("contribution", profiled["runs"][3])
                    self.assertEqual(contrast.call_args.args[0]["points"][0]["factors"], [])
                    self.assertEqual(profiled["runs"][0]["target_states"][0]["predicate_structure"], "unknown")
                with self.assertRaisesRegex(ValueError, "target rule mismatch"):
                    parameter_candidate_evidence(manifest, root, "another")
            self.assertEqual([r["status"] for r in rows], ["ok", "ok", "plan_timeout", "missing"])
            self.assertEqual([r["attempts"] for r in rows], [1, 2, None, None])
            self.assertEqual([r["memo_expressions"] for r in rows], [10, 11, None, None])
            self.assertIsNone(rows[2]["state_coverage"])
            self.assertIsNone(rows[3]["rule_state_coverage"])
            self.assertEqual(rows[0]["rule_state_coverage"]["r"]["attempts"], 1)
            manifest["queries"].append(manifest["queries"][0])
            with self.assertRaisesRegex(ValueError, "duplicate"):
                parameter_candidate_evidence(manifest, root)

    def test_copy_multiset_diagnosis_preserves_null_empty_duplicates_and_newlines(self) -> None:
        def fingerprint(text):
            return copy_result_summary(text)["rows_bag_hash"]
        self.assertEqual(fingerprint('"a\nb",1\nc,2\n'), fingerprint('c,2\n"a\nb",1\n'))
        self.assertNotEqual(fingerprint('\n'), fingerprint('""\n'))  # NULL versus empty text.
        self.assertNotEqual(fingerprint('a\na\nb\n'), fingerprint('a\nb\nb\n'))
        self.assertNotEqual(fingerprint('"a\nb"\n'), fingerprint('a\nb\n'))
        self.assertNotEqual(fingerprint('a,b\n'), fingerprint('"a,b"\n'))
        self.assertEqual(copy_result_summary('"a\nb"\n')["rows_count"], 1)

    def test_frozen_input_reference_checks_identity_and_never_fills_cached_rows(self) -> None:
        import zlib
        fingerprint = "0123456789abcdef"
        event = {"kind": "stats_observation", "experiment": "discovery", "fingerprint": fingerprint,
                 "operator": "CLogicalGet", "native_rows": 42}
        run = {"plan_rc": 0, "rows_rc": 0, "rows_hash": "same", "optimizer": "pg_orca",
               "experiment_outcomes": [{"experiment": "discovery", "stats_targets": []}], "stats_events": [event]}
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            files = [root / name for name in ("query.sql", "schema.sql", "setup.sql")]
            for path, content in zip(files, ("SELECT 1", "schema", "setup")):
                path.write_text(content)
            query, schema, setup = files
            crc = lambda p: f"{zlib.crc32(p.read_bytes()):08x}"
            source = root / "reference/tpch/q1/comparison.json"
            source.parent.mkdir(parents=True)
            (root / "reference/summary.json").write_text(json.dumps({"fixture": {
                "workload": "tpch", "schema_crc32": crc(schema), "setup": {"crc32": crc(setup)}}}))
            comparison = {"workload": "tpch", "query_crc32": crc(query),
                          "stats_experiments": [{"modes": {"native": run}}]}
            source.write_text(json.dumps(comparison))
            reference = load_input_stats_reference(source, "tpch", query, schema, setup)
            self.assertEqual(reference["targets"][0]["rows"], 42)
            self.assertFalse(reference["optimizer_consumed"])
            for path in files:
                content = path.read_text()
                path.write_text("changed")
                with self.assertRaisesRegex(ValueError, "identity mismatch"):
                    load_input_stats_reference(source, "tpch", query, schema, setup)
                path.write_text(content)
            for field, value in (("plan_rc", 124), ("optimizer", "postgres"),
                                 ("stats_events", [event, {"kind": "stats_injection"}])):
                old = run[field]
                run[field] = value
                source.write_text(json.dumps(comparison))
                with self.assertRaisesRegex(ValueError, "observation-only"):
                    load_input_stats_reference(source, "tpch", query, schema, setup)
                run[field] = old
            run["stats_events"].append({**event, "native_rows": 43})
            source.write_text(json.dumps(comparison))
            ambiguous = load_input_stats_reference(source, "tpch", query, schema, setup)
            self.assertEqual(ambiguous["targets"][0]["status"], "ambiguous")
            node = {"operator": "CLogicalGet", "reference_key": fingerprint, "rows": None, "stats_source": "missing"}
            unknown = {**node, "reference_key": "different"}
            current = {"modes": {"replacement": {"rows_rc": 0, "rows_hash": "same",
                "rule_input_observations": [{"input_context": {"root": node,
                    "children": [{"node": unknown}]}}]}}, "stats_experiments": [], "rule_profile": None}
            with patch.object(Path, "read_bytes", side_effect=AssertionError("reference read after query")), \
                 patch.object(Path, "read_text", side_effect=AssertionError("reference read after query")):
                annotate_input_stats_reference(current, reference)
                self.assertEqual(node["external_reference"], {"status": "available", "rows": 42})
                self.assertEqual(unknown["external_reference"]["status"], "unmatched")
                self.assertIsNone(node["rows"])
                self.assertEqual(node["stats_source"], "missing")
                annotate_input_stats_reference(current, ambiguous)
                self.assertEqual(node["external_reference"], {"status": "ambiguous", "rows": None})
                current["modes"]["replacement"]["rows_hash"] = "different"
                annotate_input_stats_reference(current, reference)
                self.assertEqual(node["external_reference"]["status"], "result_not_verified")

    def test_placement_pairs_use_cbo_and_keep_missing_failed_or_duplicate_blocks(self) -> None:
        run = {"plan_rc": 0, "rows_rc": 0, "rows_hash": "same", "optimizer": "pg_orca",
               "dsl_observability": {"memo": {"group_expressions": 20}}}
        samples = [{"phase": "measurement", "block": block, "scenario": 0, "arm": arm,
                    "status": "ok", "stats_experiment": None, "diagnostic_plan_matches": True,
                    "diagnostic_rows_hash": "same", "planning_ms": planning, "execution_ms": execution}
                   for block in range(2) for arm, planning, execution in (("rbo", 8, 5), ("cbo", 10, 4))]
        comparison = {"rule_profile": {"rule_hash": "target", "scenarios": [{
            "stats_experiment": None, "arms": {"rbo": run, "cbo": run,
                                                "off": {"plan_rc": 124}}}],
            "timing": {"source": "untraced_explain_analyze", "design": "randomized_complete_blocks",
                       "repeats": 2, "samples": samples}}}
        evidence = placement_evidence(comparison)
        row = evidence["scenarios"][0]
        self.assertEqual(evidence["baseline"], "cbo")
        self.assertFalse(evidence["automatic_migration"])
        self.assertEqual(row["valid_pairs"], 2)  # An OFF timeout does not invalidate RBO-versus-CBO.
        self.assertEqual(row["pairs"][0]["delta"], {"planning_ms": -2, "execution_ms": 1, "total_ms": -1})
        for field, value in (("status", "plan_timeout"), ("diagnostic_plan_matches", False),
                             ("planning_ms", float("nan")), ("execution_ms", -1),
                             ("diagnostic_rows_hash", "different"), ("stats_experiment", "other")):
            saved = samples[0][field]
            samples[0][field] = value
            row = placement_evidence(comparison)["scenarios"][0]
            self.assertEqual(row["valid_pairs"], 1)
            self.assertIsNone(row["pairs"][0]["delta"])
            samples[0][field] = saved
        samples.append(samples[0])
        with self.assertRaisesRegex(ValueError, "duplicate"):
            placement_evidence(comparison)
        samples.pop()
        samples.pop()
        row = placement_evidence(comparison)["scenarios"][0]
        self.assertEqual(row["attempted_pairs"], 2)
        self.assertEqual(row["valid_pairs"], 1)
        self.assertIn("cbo:missing_sample", row["pairs"][1]["exclusions"])
        comparison["rule_profile"]["scenarios"][0]["arms"]["cbo"] = {**run, "rows_hash": "different"}
        self.assertEqual(placement_evidence(comparison)["scenarios"][0]["valid_pairs"], 0)

    def test_local_inputs_keep_missing_stats_and_filter_only_by_rule_identity(self) -> None:
        observation = {"rule_hash": "target", "status": "match_rejected",
                       "input_sampling": "first_rule_status", "input_context": {
                           "capture": "before_evaluation", "root": {"rows": None}}}
        run = {"plan_rc": 124, "rows_rc": 124, "error": "timeout",
               "rule_input_observations": [observation, {**observation, "rule_hash": "another"}]}
        comparison = {"rule_profile": {"rule_hash": "target", "scenarios": [
            {"stats_experiment": "config", "arms": {"cbo": run}}]}}
        records = local_input_records(comparison)
        self.assertEqual(len(records), 1)
        self.assertIsNone(records[0]["input_context"]["root"]["rows"])
        self.assertEqual(records[0]["run_status"], "plan_timeout")
        observation["input_context"]["capture"] = "after_evaluation"
        with self.assertRaisesRegex(ValueError, "before evaluation"):
            local_input_records(comparison)

    def test_rule_distribution_ignores_query_identity_and_separates_anchors_and_failures(self) -> None:
        rows = [{"query": "example", "rule_hash": "r", "split": "discovery", "arm": "cbo",
                 "status": "ok", "target_applied": 0, "result_empty": False, "delta_memo_expressions": 0}]
        rows += [{**rows[0], "query": "failed", "status": "plan_timeout", "target_applied": None,
                  "result_empty": None, "delta_memo_expressions": None},
                 {**rows[0], "query": "anchor", "split": "anchor", "target_applied": 1,
                  "delta_memo_expressions": 210}]
        profile = rule_distribution(rows)
        self.assertEqual(profile, rule_distribution([{**r, "query": f"renamed-{i}"} for i, r in enumerate(rows)]))
        group = next(g for g in profile["groups"] if g["split"] == "discovery" and g["arm"] == "cbo")
        self.assertEqual(group["attempts"], 2)
        self.assertEqual(group["comparable"], 1)
        self.assertEqual(group["applied_samples"], 0)
        self.assertEqual(group["memo_delta"]["count"], 1)
        self.assertEqual(group["memo_delta"]["max"], 0)
        self.assertFalse(profile["generalization_validated"])

    def test_query_cohort_groups_literals_and_keeps_holdout_families_separate(self) -> None:
        self.assertEqual(query_features("SELECT * FROM t WHERE x=1 -- one")[0],
                         query_features("SELECT * FROM t WHERE x=9")[0])
        self.assertNotEqual(query_features("SELECT * FROM t WHERE x=1")[0],
                            query_features("SELECT * FROM t WHERE y=1")[0])
        with self.assertRaises(ValueError):
            query_features("SELECT 1; SELECT 2")
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            queries = root / "tpch/sql"
            queries.mkdir(parents=True)
            (root / "tpch/manifest.json").write_text("{}")
            for index in range(10):
                (queries / f"q{index}.sql").write_text(f"SELECT * FROM t WHERE x{index // 2}={index}")
            (queries / "bad.sql").write_text("SELECT 1; SELECT 2")
            cohort = build_cohort(root, ["tpch"], 2, 7, ["tpch/q0"])
            self.assertEqual(cohort, build_cohort(root, ["tpch"], 2, 7, ["tpch/q0"]))
            rows = cohort["queries"]
            self.assertEqual(sum(r["selected"] for r in rows), 3)
            self.assertEqual(sum(r["status"] == "parse_error" for r in rows), 1)
            families = {}
            for row in rows:
                if row["status"] != "ok":
                    continue
                families.setdefault(row["family"], set()).add(row["split"])
                if row["split"] == "holdout":
                    self.assertFalse(row["selected"])
            self.assertTrue(all(len(splits) == 1 for splits in families.values()))
            self.assertTrue(any(splits == {"holdout"} for splits in families.values()))
            expanded = build_cohort(root, ["tpch"], 3, 7, ["tpch/q0"])
            batch = incremental_cohort(expanded, cohort)
            self.assertEqual(sum(q["selected"] for q in batch["queries"]), 1)
            self.assertTrue(all(q["split"] == "discovery" for q in batch["queries"] if q["selected"]))
            with self.assertRaisesRegex(ValueError, "cumulative"):
                incremental_cohort(expanded, batch)
            expanded["queries"][0]["query_crc32"] = "changed"
            with self.assertRaisesRegex(ValueError, "preserve"):
                incremental_cohort(expanded, cohort)

    def test_stratified_cohort_preserves_frame_and_family_probabilities(self) -> None:
        cohort = {"seed": 7, "queries": [
            {"query": f"tpch/{family}{member}", "family": family, "workload": "tpch",
             "split": "holdout" if family == "holdout" else "anchor" if family == "anchor" else "discovery",
             "selected": family in ("prior", "anchor") and member == 0,
             "features": {"select": 1, "group": int(family == "rare")}}
            for family in ("a", "b", "c", "rare", "prior", "holdout", "anchor") for member in range(2)]}
        batch = stratified_cohort(cohort, 1)
        self.assertEqual(batch, stratified_cohort(cohort, 1))
        self.assertEqual(batch["eligible_families"], 4)
        selected = [q for q in batch["queries"] if q["selected"]]
        self.assertEqual(len(selected), 2)
        self.assertTrue(all(q["family"] in ("a", "b", "c", "rare") for q in selected))
        weights = family_design_weights(batch)
        for q in selected:
            pi = 1 if q["family"] == "rare" else 1 / 3
            self.assertEqual(q["family_inclusion_probability"], pi)
            self.assertEqual(q["sql_inclusion_probability"], pi / 2)
            self.assertEqual(weights[q["query"]], 1 / pi)  # Family mean, not SQL-total weight.
        self.assertEqual(sum(weights.values()), 4)
        with self.assertRaisesRegex(ValueError, "unknown"):
            family_design_weights({**batch, "sampling_unit": "unknown"})
        census = stratified_cohort(cohort, 100)
        self.assertEqual(len(family_design_weights(census)), 4)
        with self.assertRaisesRegex(ValueError, "cumulative"):
            stratified_cohort(batch, 1)
        for bad in (0, -1, True, 1.5):
            with self.assertRaises(ValueError):
                stratified_cohort(cohort, bad)
        for bad in (0, float("nan"), 0.8):
            damaged = json.loads(json.dumps(batch))
            next(q for q in damaged["queries"] if q["selected"])["family_inclusion_probability"] = bad
            with self.assertRaisesRegex(ValueError, "probability"):
                family_design_weights(damaged)
        damaged = json.loads(json.dumps(batch))
        damaged["eligible_families"] += 1
        with self.assertRaisesRegex(ValueError, "frame size"):
            family_design_weights(damaged)
        damaged = json.loads(json.dumps(batch))
        damaged["strata"][0]["sampled_families"] += 1
        with self.assertRaisesRegex(ValueError, "sample count"):
            family_design_weights(damaged)
        # Literal variants may not leak across strata or cause a second member
        # of an already observed family to re-enter the sampling frame.
        cohort["queries"][0]["features"]["window"] = 1
        with self.assertRaisesRegex(ValueError, "crosses"):
            stratified_cohort(cohort, 1)

    def test_candidate_cohort_equal_family_weights_and_failed_denominators(self) -> None:
        names = ["a", "b", "timeout", "missing", "partial", "anchor"]
        cohort = {"queries": [{"query": f"tpch/{name}", "family": name, "selected": True,
                               "split": "anchor" if name == "anchor" else "discovery", "query_crc32": name}
                              for name in names]}
        def row(ready, elapsed):
            return {"rule_hash": "r", "status": "ready_cbo" if ready else "match_rejected",
                    "input_context": {"root": {"operator": "CLogicalSelect"},
                                      "source_shape": {"complete": True, "pattern_nodes": 0, "nodes": 17}},
                    "shape_features": {"source_subqueries": "no_subquery"}, "match_us": elapsed,
                    "constraint_us": 0, "instantiate_us": 0, "direct_insertions": 4 if ready else None,
                    "memo_outcome": {"status": "memo_inserted"} if ready else None}
        baseline = {"plan_rc": 0, "rows_rc": 0, "optimizer": "pg_orca"}
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for name in names:
                if name == "missing":
                    continue
                run = {**baseline, "rows_count": 2, "experiment_outcomes": [{"stats_targets": []}], "candidate_events": [],
                       "fake_rows": [row(True, 10)] if name != "b" else [row(False, 1)] * 9,
                       "partial": name == "partial"}
                if name == "timeout":
                    run["plan_rc"] = 124
                point = {"path": "same", "modes": {"replacement": run},
                         "comparisons": {"replacement": {"rows_equal": True, "plan_comparison": "identical"}}}
                path = root / f"tpch/{name}/comparison.json"
                path.parent.mkdir(parents=True)
                path.write_text(json.dumps({"query_crc32": name, "stats_experiments": [point],
                                            "rows_equal": False, "rows_bag_equal": True,
                                            "forbidden_native_origins": ["native_origin"],
                                            "modes": {"replacement": baseline}}))
            with patch("profile_rule_candidates.candidate_evidence", side_effect=lambda r: {
                "complete": not r["partial"], "exclusions": ["incomplete"] if r["partial"] else [],
                "rows": r["fake_rows"], "attempts": len(r["fake_rows"])}):
                report = cohort_candidate_evidence(cohort, root)
                self.assertEqual(report["selected_families"], 6)
                self.assertEqual(report["complete_families"], 3)
                self.assertFalse(report["runs"][0]["native_rows_equal"])
                self.assertTrue(report["runs"][0]["native_rows_bag_equal"])
                self.assertEqual(report["runs"][0]["forbidden_native_origins"], ["native_origin"])
                self.assertEqual(report["runs"][0]["result_rows"], 2)
                self.assertEqual([r["status"] for r in report["runs"]],
                                 ["ok", "ok", "plan_timeout", "missing", "incomplete_trace", "ok"])
                group = next(g for g in report["groups"] if g["split"] == "discovery")
                self.assertEqual(group["families"], 2)
                self.assertEqual(group["attempts"], 10)
                self.assertEqual(group["mean_family_ready_rate"], 0.5)  # Not pooled 1/10.
                self.assertEqual(group["mean_family_evaluation_us"], 5.5)
                self.assertEqual(group["source_nodes_band"], 16)
                self.assertTrue(all(r["attempts"] is None for r in report["runs"] if r["status"] != "ok"))
                weighted = {"sampling_unit": "stratified_discovery_family_then_uniform_member",
                            "eligible_families": 7, "strata": [], "queries": []}
                for q in cohort["queries"]:
                    if q["split"] == "anchor":
                        continue
                    size = 3 if q["family"] == "a" else 1
                    weighted["queries"].append({**q, "stratum": q["family"], "family_inclusion_probability": 1 / size})
                    weighted["strata"].append({"stratum": q["family"], "families": size, "sampled_families": 1})
                report = cohort_candidate_evidence(weighted, root)
                group = report["groups"][0]
                self.assertEqual(group["mean_family_ready_rate"], 0.75)
                self.assertEqual(group["mean_family_evaluation_us"], 7.75)
                self.assertEqual(group["mean_family_status_rates"], {"ready_cbo": 0.75, "match_rejected": 0.25})
                self.assertEqual(group["mean_family_direct_insertions_per_attempt"], 3)
                self.assertEqual(group["mean_family_root_inserted_per_attempt"], 0.75)
                self.assertEqual(report["weighted_status_share"]["ok"], 4 / 7)
                self.assertEqual(report["weighted_status_share"]["plan_timeout"], 1 / 7)
            cohort["queries"][0]["split"] = "holdout"
            with self.assertRaisesRegex(ValueError, "holdout"):
                cohort_candidate_evidence(cohort, root)

    def test_cohort_report_separates_nonapplication_from_failure_and_identity_mismatch(self) -> None:
        cohort = {"queries": [{"query": "tpch/q1", "family": "f", "split": "discovery",
                               "selected": True, "query_crc32": "abcd"}]}
        run = {"plan_rc": 0, "rows_rc": 0, "rows_hash": "same", "optimizer": "pg_orca",
               "dsl_observability": {"memo": {"group_expressions": 10}}, "profile_rule_statuses": []}
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.assertTrue(all(r["status"] == "missing" for r in cohort_results(cohort, root)))
            path = root / "tpch/q1/comparison.json"
            path.parent.mkdir(parents=True)
            result = {"query_crc32": "abcd", "rule_profile": {"rule_hash": "r", "scenarios": [{
                "stats_experiment": None, "arms": {"off": run, "rbo": run,
                    "cbo": {**run, "plan_rc": 124, "error": "timeout"}}}]}}
            path.write_text(json.dumps(result))
            rows = cohort_results(cohort, root)
            self.assertEqual(rows[1]["target_applied"], 0)
            self.assertEqual(rows[1]["delta_memo_expressions"], 0)
            self.assertEqual(rows[2]["status"], "plan_timeout")
            self.assertIsNone(rows[2]["target_applied"])
            result["query_crc32"] = "changed"
            path.write_text(json.dumps(result))
            self.assertTrue(all(r["status"] == "query_identity_mismatch" for r in cohort_results(cohort, root)))

    def test_timing_randomized_blocks_and_plan_measurement_filter(self) -> None:
        schedule = list(timing_schedule(2, 3, 1, 7))
        self.assertEqual(schedule, list(timing_schedule(2, 3, 1, 7)))
        self.assertNotEqual(schedule, list(timing_schedule(2, 3, 1, 8)))
        self.assertEqual(len(schedule), 24)
        binary_schedule = list(timing_schedule(2, 3, 1, 7, ("off", "cbo")))
        self.assertEqual(len(binary_schedule), 16)
        self.assertEqual({arm for _, _, _, arm in binary_schedule}, {"off", "cbo"})
        for block in range(3):
            self.assertEqual({(i, arm) for phase, b, i, arm in binary_schedule if phase == "measurement" and b == block},
                             {(i, arm) for i in range(2) for arm in ("off", "cbo")})
        with self.assertRaises(ValueError):
            list(timing_schedule(1, 1, 0, 7, ("cbo",)))
        for phase, rounds in (("warmup", 1), ("measurement", 3)):
            for block in range(rounds):
                self.assertEqual({(i, arm) for p, b, i, arm in schedule if (p, b) == (phase, block)},
                                 {(i, arm) for i in range(2) for arm in ("off", "rbo", "cbo")})
        plan = {"Node Type": "Sort", "Sort Key": ["x"], "Actual Rows": 4, "Shared Hit Blocks": 5,
                "Plans": [{"Node Type": "Seq Scan", "Filter": "x > 0", "Actual Loops": 1}]}
        self.assertEqual(timing_plan(plan), {"Node Type": "Sort", "Sort Key": ["x"],
                         "Plans": [{"Node Type": "Seq Scan", "Filter": "x > 0"}]})

    def test_untraced_timing_preserves_failures_and_separate_validation(self) -> None:
        args = SimpleNamespace(timing_repeats=2, timing_warmups=1, timing_seed=7,
                               profile_policies={arm: None for arm in ("off", "rbo", "cbo")},
                               port=1, timeout=60)
        plan = {"Node Type": "Result", "Actual Rows": 1}
        diagnostic = {"plan": plan, "plan_rc": 0, "rows_rc": 0, "rows_hash": "same",
                      "optimizer": "pg_orca"}
        scenarios = [{"stats_experiment": None,
                      "arms": {arm: diagnostic for arm in ("off", "rbo", "cbo")}}]
        stdout = json.dumps([{"Plan": plan, "Optimizer": "pg_orca", "Planning Time": 2,
                              "Execution Time": 3}])
        with tempfile.TemporaryDirectory() as temporary, patch("run_workload_comparison.psql") as execute:
            execute.side_effect = [("", "TIMEOUT", 124, 60000), *[(stdout, "", 0, 8)] * 8]
            result = run_profile_timing(args, Path("psql"), Path("/tmp"), "db", "SELECT 1",
                                        Path(temporary), [], scenarios)
            self.assertEqual(len(result["samples"]), 9)
            self.assertEqual(result["samples"][0]["status"], "plan_timeout")
            self.assertIsNone(result["samples"][0]["execution_ms"])
            self.assertEqual(result["samples"][1]["comparison_exclusions"], [])
            self.assertNotIn("rows_rc", result["samples"][1])
            self.assertEqual(len((Path(temporary) / "timing-samples.jsonl").read_text().splitlines()), 9)
            for call in execute.call_args_list:
                self.assertFalse(call.kwargs["retry_on_server_failure"])
                self.assertIn("SET pg_orca.trace_dsl_rule=off", call.args[4])
                self.assertIn("SET optimizer_print_optimization_stats=off", call.args[4])
            execute.reset_mock()
            execute.side_effect = [(stdout, "", 0, 8)] * 6
            args.profile_policies = {arm: None for arm in ("off", "cbo")}
            scenarios[0]["arms"].pop("rbo")
            result = run_profile_timing(args, Path("psql"), Path("/tmp"), "db", "SELECT 1", Path(temporary), [], scenarios)
            self.assertEqual(result["arms"], ["off", "cbo"])
            self.assertEqual(len(result["samples"]), 6)
            self.assertEqual({s["arm"] for s in result["samples"]}, {"off", "cbo"})

    def test_observed_edges_preserve_missing_empty_and_raw_evidence(self) -> None:
        self.assertIsNone(observed_rule_edges({}, None))
        self.assertEqual(observed_rule_edges({"rule_edges": []}, None), [])
        edge = {"kind": "rule_edge", "src_rule": "r", "dst_rule": "s", "target_path": "r/1"}
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "old.trace"
            path.write_text('DSL_TRACE {"kind":"application"}\nDSL_TRACE ' + json.dumps(edge) + '\n')
            self.assertEqual(observed_rule_edges({}, path), [edge])
            self.assertEqual(observed_rule_edges({"rule_edges": []}, path), [])

    def test_search_contribution_requires_both_complete_comparable_arms(self) -> None:
        arms = {"off": {"n": 2}, "cbo": {"n": 5}}
        lifecycle = {"complete": True, "exclusions": [], "status_counts": {"best_updated": 1},
                     "materialized_candidates": 1, "ever_best_candidates": 1, "selected_distinct_candidates": 1}
        checks = {"complete": True, "exclusions": [], "status_counts": {"prune": {"pruned": 3}}}
        with patch("profile_rule_candidates.cost_evidence", side_effect=lambda run: {
                "complete": True, "exclusions": [], "status_counts": {"costed": run["n"]}}), \
             patch("profile_rule_candidates.cost_lifecycle_evidence", return_value=lifecycle), \
             patch("profile_rule_candidates.search_check_evidence", return_value=checks):
            report = search_contribution(arms, [])
            self.assertTrue(report["complete"])
            self.assertEqual(report["delta"]["cost:costed"], 3)
            self.assertEqual(report["delta"]["prune:pruned"], 0)
            self.assertEqual(report["arms"]["off"]["metrics"]["cost:costed"], 2)
            self.assertIsNone(search_contribution(arms, ["rows_differ"])["delta"])
            checks.update(complete=False, exclusions=["search_check_count_mismatch"])
            report = search_contribution(arms, [])
            self.assertFalse(report["complete"])
            self.assertIsNone(report["delta"])
            self.assertIsNone(report["arms"]["cbo"]["metrics"])
            self.assertIn("off:search_check_count_mismatch", report["exclusions"])
        old = search_contribution({"off": {}, "cbo": {}}, [])
        self.assertIsNone(old["delta"])  # An absent trace is not zero work.

    def test_target_cost_lineage_requires_exact_new_root_origin(self) -> None:
        target = [{"memo_outcome": {"status": "memo_inserted", "group": 3, "group_expression": 1}},
                  {"memo_outcome": {"status": "memo_deduplicated", "group": 3, "group_expression": 2}}]
        run = {"cost_events": [
            {"sequence": 1, "origin_group": 3, "origin_expression": 1, "status": "costed", "cost": 12,
             "context_best_cost_at_event": 10, "operator": "CPhysicalHashAgg", "required_columns": 2,
             "required_order_columns": 1},
            {"sequence": 2, "origin_group": 3, "origin_expression": 2, "status": "costed"},
            {"sequence": 3, "origin_group": 4, "origin_expression": 1, "status": "costed"}],
            "cost_lifecycle_events": [{"candidate_sequence": 1, "status": "retained_new"},
                                      {"candidate_sequence": 2, "status": "selected_plan"}]}
        result = target_root_costs(run, target)
        self.assertEqual(result["cost_sequences"], [1])
        self.assertEqual(result["comparisons"][0]["cost_gap"], 2)
        self.assertEqual(result["comparisons"][0]["operator"], "CPhysicalHashAgg")
        self.assertEqual(result["comparisons"][0]["required_order_columns"], 1)
        run["cost_events"][0]["context_best_cost_at_event"] = 0
        self.assertEqual(target_root_costs(run, target)["comparisons"][0]["cost_gap"], 12)
        run["cost_events"][0].pop("context_best_cost_at_event")
        self.assertIsNone(target_root_costs(run, target)["comparisons"][0]["cost_gap"])
        self.assertEqual(result["lifecycle_status_counts"], {"retained_new": 1})
        self.assertEqual(target_root_costs(run, [])["cost_sequences"], [])

    def test_cost_origin_chain_crosses_lowering_without_group_cooccurrence(self) -> None:
        event = {'experiment': 'e', 'sequence': 1, 'group': 3, 'group_expression': 9,
                 'optimization_context': 0, 'optimization_request': 0, 'search_stage': 0,
                 'memo_version': 10, 'preceding_rule_candidates': 1, 'status': 'costed',
                 'cost_kind': 'computed', 'cost': 12, 'context_best_cost_at_event': 10,
                 'origin_group': 3, 'origin_expression': 2,
                 'origin_chain': [{'group': 3, 'group_expression': 2}, {'group': 3, 'group_expression': 1}]}
        run = {'plan_rc': 0, 'rows_rc': 0, 'optimizer': 'pg_orca', 'cost_events': [event],
               'cost_lifecycle_events': [], 'experiment_outcomes': [{'experiment': 'e', 'stats_targets': [],
               'cost_trace_version': 1, 'cost_candidates': 1, 'rule_candidates': 1}]}
        target = [{'memo_outcome': {'status': 'memo_inserted', 'group': 3, 'group_expression': 1}}]
        self.assertTrue(cost_evidence(run)['complete'])
        self.assertEqual(target_root_costs(run, target)['cost_sequences'], [])
        self.assertEqual(target_root_costs(run, target, True)['cost_sequences'], [1])
        target.append({'memo_outcome': {'status': 'memo_inserted', 'group': 3, 'group_expression': 2}})
        self.assertEqual(target_root_costs(run, target, True)['cost_sequences'], [1])  # Not double credit.
        target[0]['memo_outcome']['group'] = 4
        self.assertEqual(target_root_costs(run, target[:1], True)['cost_sequences'], [])
        chain = event['origin_chain']
        for invalid, reason in [(None, 'invalid_origin_chain'), (chain[::-1], 'origin_chain_head_mismatch'),
                                (chain + chain[:1], 'cyclic_origin_chain'), ([], 'origin_chain_head_mismatch')]:
            event['origin_chain'] = invalid
            self.assertIn(reason, cost_evidence(run)['exclusions'])
        del event['origin_chain']
        self.assertTrue(cost_evidence(run)['complete'])  # Old direct-only logs remain usable.
        with self.assertRaisesRegex(ValueError, 'missing_origin_chain'):
            target_root_costs(run, target, True)

    def test_cbo_contribution_separates_fallback_from_performance(self) -> None:
        run = {"optimizer": "pg_orca", "plan_rc": 0, "rows_rc": 0, "rows_hash": "same",
               "experiment_outcomes": [{"stats_targets": []}]}
        off, cbo = dict(run), dict(run)
        comparison = {"rule_profile": {"rule_hash": "r", "arms": ["off", "cbo"],
                       "policy_snapshots": {"off": "off", "cbo": "cbo"}, "scenarios": [
                           {"stats_experiment": "/tmp/card.yaml", "arms": {"off": off, "cbo": cbo},
                            "comparisons": {"cbo": {"plan_comparison": "identical"}}}]}}
        comparison.update(join_enumeration_replaced=False,
                          forbidden_native_origins=["CXformInnerJoinCommutativity"])
        off["native_memo_origins"] = cbo["native_memo_origins"] = ["CXformInnerJoinCommutativity"]
        diagnostics = {"off": [{"status": "ok", "memo_expressions": 10, "optimizer_cost": 5}],
                       "cbo": [{"status": "ok", "memo_expressions": 14, "optimizer_cost": 3}]}
        manifest = {"points": [{"file": "card.yaml", "factors": [1]}]}
        target = {"rule_hash": "r", "status": "ready_cbo", "direct_insertions": 3,
                  "memo_outcome": {"status": "memo_inserted"}}
        other = {"rule_hash": "other", "status": "constraint_rejected", "failed_constraint": "Unique",
                 "direct_insertions": None, "memo_outcome": None}
        cbo_rows, off_rows = [target, other], [other]
        samples = [{"file": "card.yaml", "arm": "cbo", "exclusions": [], "planning_ms": 2,
                    "execution_ms": 3, "delta_planning_ms": 1, "delta_execution_ms": -1}]
        comparison["rule_profile"]["timing"] = {}  # Parsed timing is mocked below.
        with patch("profile_rule_candidates.profile_points", return_value=diagnostics), \
             patch("profile_rule_candidates.timing_points", return_value=samples), \
             patch("profile_rule_candidates.candidate_evidence", side_effect=lambda r: {
                 "complete": True, "exclusions": [], "attempts": len(cbo_rows if r is cbo else off_rows),
                 "rows": cbo_rows if r is cbo else off_rows}):
            row = cbo_contribution(manifest, comparison, Path("/tmp"))[0]
            self.assertEqual(row["delta"], {"attempts": 1, "memo_expressions": 4, "optimizer_cost": -2})
            self.assertEqual(row["target_root_inserted"], 1)
            self.assertFalse(row["replacement_audit"]["join_enumeration_replaced"])
            self.assertEqual(row["native_origins"]["cbo"], ["CXformInnerJoinCommutativity"])
            self.assertEqual(row["rule_deltas"]["r"]["direct_insertions"], 3)
            self.assertEqual(row["rule_deltas"]["other"]["constraint:Unique"], 0)
            self.assertEqual(row["other_rule_attempts_delta"], 0)
            self.assertEqual(row["pairs"][0]["delta_execution_ms"], -1)
            comparison["artifact_provenance"] = {"endpoints_equal": False}
            changed = cbo_contribution(manifest, comparison, Path("/tmp"))[0]
            self.assertIn("experiment_artifacts_changed", changed["exclusions"])
            self.assertIsNone(changed["delta"])
            self.assertIsNone(changed["pairs"][0]["delta_execution_ms"])
            del comparison["artifact_provenance"]
            del comparison["rule_profile"]["timing"]
            untimed = cbo_contribution(manifest, comparison, Path("/tmp"))[0]
            self.assertEqual(untimed["pairs"], [])
            self.assertEqual(untimed["delta"], row["delta"])
            comparison["rule_profile"]["timing"] = {}
            cbo_rows.append(other)
            row = cbo_contribution(manifest, comparison, Path("/tmp"))[0]
            self.assertEqual(row["other_rule_attempts_delta"], 1)
            self.assertEqual(row["rule_deltas"]["other"]["constraint:Unique"], 1)
            cbo_rows.pop()
            off_rows.extend([other, other])
            row = cbo_contribution(manifest, comparison, Path("/tmp"))[0]
            self.assertEqual(row["other_rule_attempts_delta"], -2)
            self.assertEqual(row["rule_deltas"]["other"]["attempts"], -2)
            off_rows[:] = [other]
            off["optimizer"] = "postgres"
            diagnostics["off"][0]["status"] = "fallback"
            row = cbo_contribution(manifest, comparison, Path("/tmp"))[0]
            self.assertTrue(row["off_fallback_cbo_ok"])
            self.assertIsNone(row["delta"])
            self.assertIsNone(row["rule_deltas"])
            self.assertIsNone(row["other_rule_attempts_delta"])
            self.assertIsNone(row["pairs"][0]["delta_execution_ms"])
            self.assertEqual(row["target_ready"], 1)  # Preserve CBO evidence even when OFF is unusable.
        comparison["rule_profile"]["arms"].insert(1, "rbo")
        with self.assertRaisesRegex(ValueError, "OFF/CBO"):
            cbo_contribution(manifest, comparison, Path("/tmp"))

    @patch("run_workload_comparison.wait_ready")
    @patch("run_workload_comparison.run")
    def test_timing_psql_never_retries(self, run, ready) -> None:
        run.return_value = subprocess.CompletedProcess([], 2, "", "database system is in recovery mode")
        result = psql(Path("psql"), Path("/tmp"), 1, "db", "SELECT 1", 1,
                      retry_on_server_failure=False)
        self.assertEqual(result[2], 2)
        self.assertEqual(run.call_count, 1)
        ready.assert_not_called()

    @patch("plot_stats_sweep.profile_points", return_value={
        arm: [{"status": "ok"}] for arm in ("off", "rbo", "cbo")})
    def test_timing_plot_retains_failed_pairs_and_rejects_missing_samples(self, diagnostics) -> None:
        path = str(Path("/tmp/test-card.yaml"))
        manifest = {"points": [{"file": "test-card.yaml", "factors": [1]}]}
        samples = [{"phase": phase, "block": block, "scenario": index, "arm": arm,
                    "sequence": seq, "planning_ms": 3, "execution_ms": 4,
                    "comparison_exclusions": ["plan_timeout"] if arm == "off" and block == 0 else []}
                   for seq, (phase, block, index, arm) in enumerate(timing_schedule(1, 2, 0, 7))]
        comparison = {"rule_profile": {"scenarios": [{"stats_experiment": path}],
                      "timing": {"repeats": 2, "warmups": 0, "seed": 7, "samples": samples}}}
        points = timing_points(manifest, comparison, Path("/tmp"))
        self.assertEqual(len(points), 6)
        for point in points:
            if point["block"] == 0:
                self.assertIsNone(point["delta_execution_ms"])
                self.assertIn("off_not_comparable", point["exclusions"])
            else:
                self.assertEqual(point["delta_execution_ms"], 0)
        diagnostics.return_value["cbo"][0]["status"] = "manifest_target_mismatch"
        for point in timing_points(manifest, comparison, Path("/tmp")):
            if point["arm"] == "cbo":
                self.assertIsNone(point["execution_ms"])
                self.assertIn("cbo:manifest_target_mismatch", point["exclusions"])
        samples.pop()
        with self.assertRaisesRegex(ValueError, "complete randomized schedule"):
            timing_points(manifest, comparison, Path("/tmp"))

    def test_lero_geometric_grid_covers_declared_ratio_interval(self) -> None:
        self.assertEqual(geometric_factors(2, 4), [0.25, 0.5, 1, 2, 4])
        self.assertEqual(geometric_factors(2, 1), [1])
        for alpha, delta in ((2, 100), (3, 8), (1.5, 10)):
            grid = geometric_factors(alpha, delta)
            for i in range(101):
                ratio = math.exp(-math.log(delta) + 2 * math.log(delta) * i / 100)
                self.assertLessEqual(min(max(f / ratio, ratio / f) for f in grid), alpha)
        for alpha, delta in ((1, 4), (2, 0), (float("inf"), 4), (2, float("nan"))):
            with self.assertRaises(ValueError):
                geometric_factors(alpha, delta)

    def test_cardinality_probes_use_count_and_retain_failures(self) -> None:
        probes = [{"fingerprint": "0123456789abcdef", "operator": "CLogicalGet", "sql": "SELECT * FROM t;"}]
        with patch("run_workload_comparison.psql", return_value=("17\n", "", 0, 2.0)) as execute:
            result = collect_cardinalities(Path("psql"), Path("/tmp"), 1, "db", probes, 60)
            self.assertEqual(result[0]["actual_rows"], 17)
            sql = execute.call_args.args[4]
            self.assertIn("READ ONLY", sql)
            self.assertIn("SELECT COUNT(*) FROM (SELECT * FROM t)", sql)
        for stdout, rc in (("", 124), ("1\n2\n", 0), ("-1", 0)):
            with patch("run_workload_comparison.psql", return_value=(stdout, "failure", rc, 2.0)):
                result = collect_cardinalities(Path("psql"), Path("/tmp"), 1, "db", probes, 60)
                self.assertEqual(result[0]["status"], "error")
                self.assertIsNone(result[0]["actual_rows"])
        with self.assertRaises(ValueError):
            collect_cardinalities(Path("psql"), Path("/tmp"), 1, "db", probes * 2, 60)

    def test_sweep_plot_does_not_turn_invalid_samples_into_zero(self) -> None:
        manifest = {"targets": [{"fingerprint": "f", "operator": "CLogicalGet"}], "points": [
            {"file": "one.yaml", "factors": [1], "requested_rows": [10], "design": "control"},
            {"file": "missing.yaml", "factors": [2], "requested_rows": [20], "design": "one_at_a_time"},
        ]}
        run = {"optimizer": "pg_orca", "plan_rc": 0, "rows_rc": 0, "rows_hash": "same",
               "experiment_outcomes": [{"optimizer_cost": 2, "memo_group_expressions": 3,
                                        "rule_applications": 0, "stats_targets": [
                   {"fingerprint": "f", "operator": "CLogicalGet", "selector_kind": "expression",
                    "selector": "f", "requested_rows": 10, "consumed": True}]}],
               "dsl_observability": {"rules": {"rule": {"binding_attempts": 4}}}}
        comparison = {"modes": {"replacement": run}, "stats_experiments": [
            {"path": "/tmp/one.yaml", "modes": {"replacement": run},
             "comparisons": {"replacement": {"plan_comparison": "identical"}}}]}
        points = measured_points(manifest, comparison, Path("/tmp"), "replacement")
        self.assertEqual(points[0]["binding_attempts"], 4)
        self.assertEqual(points[1]["status"], "missing")
        self.assertIsNone(points[1]["binding_attempts"])
        manifest["points"][0]["requested_rows"] = [11]
        point = measured_points(manifest, comparison, Path("/tmp"), "replacement")[0]
        self.assertEqual(point["status"], "manifest_target_mismatch")
        self.assertIsNone(point["optimizer_cost"])
        run["plan_rc"] = 124
        self.assertEqual(measured_points(manifest, comparison, Path("/tmp"), "replacement")[0]["status"],
                         "plan_timeout")

    def test_stats_sweep_design_is_bounded_and_reproducible(self) -> None:
        points = sweep_points(2, [0.25, 1, 4], 4, 7)
        self.assertEqual(len(points), 11)  # control + 4 local + 2 common + 4 joint
        self.assertEqual(points, sweep_points(2, [4, 0.25, 1, 1], 4, 7))
        self.assertNotEqual(points, sweep_points(2, [0.25, 1, 4], 4, 8))
        for point in points[1:5]:
            self.assertEqual(sum(f != 1 for f in point["factors"]), 1)
        for dimension in range(2):
            strata = [int((math.log(p["factors"][dimension]) - math.log(0.25))
                          / math.log(16) * 4) for p in points[-4:]]
            self.assertEqual(sorted(strata), list(range(4)))
        self.assertEqual(len(sweep_points(20, [0.25, 1, 4], 8, 0)), 51)
        self.assertEqual(len(sweep_points(1, [1], 8, 0)), 1)
        for factors in ([], [0], [-1], [float("nan")], [float("inf")]):
            with self.assertRaises(ValueError):
                sweep_points(2, factors, 4, 0)
        with self.assertRaises(ValueError):
            sweep_points(2, [0.25, 4], -1, 0)

    def test_stats_sweep_freezes_trace_rows_and_refuses_ambiguous_baselines(self) -> None:
        fingerprint = "0123456789abcdef"
        event = {"kind": "stats_observation", "experiment": "discovery",
                 "operator": "CLogicalGet", "fingerprint": fingerprint, "native_rows": 10}
        outcome = {"kind": "experiment_outcome", "experiment": "discovery", "stats_targets": []}
        records = [event, outcome]
        targets = discovery_targets([event, event, outcome], [fingerprint])
        self.assertEqual(targets[0]["baseline_rows"], 10)
        for invalid in ([event], [event, outcome, outcome],
                        [*records, {**event, "native_rows": 11}],
                        [*records, {**event, "experiment": "different"}],
                        [*records, {"kind": "stats_injection"}],
                        [{**event, "native_rows": 0}, outcome]):
            with self.assertRaises(ValueError):
                discovery_targets(invalid, [fingerprint])
        with self.assertRaises(ValueError):
            discovery_targets(records, ["fedcba9876543210"])
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            trace = root / "source.trace"
            trace.write_text("\n".join("DSL_TRACE " + json.dumps(r) for r in records))
            output = root / "sweep"
            manifest = write_sweep(trace, output, [fingerprint], [0.25, 4], 0, 7)
            import gzip
            compressed = trace.with_suffix('.trace.gz')
            compressed.write_bytes(gzip.compress(trace.read_bytes()))
            archived = write_sweep(compressed, root / 'compressed', [fingerprint], [0.25, 4], 0, 7)
            self.assertEqual(archived['targets'], manifest['targets'])
            self.assertEqual(archived['points'], manifest['points'])
            self.assertEqual([p["requested_rows"] for p in manifest["points"]], [[10], [2.5], [40]])
            self.assertIn("    rows: 2.5\n", (output / manifest["points"][1]["file"]).read_text())
            self.assertEqual(json.loads((output / "manifest.json").read_text()), manifest)
            self.assertEqual(manifest["sampling_measure"]["factor_bounds"], [0.25, 4])
            self.assertEqual(manifest["sampling_measure"]["estimation_subset"], "log_latin_hypercube")
            with self.assertRaises(FileExistsError):
                write_sweep(trace, output, [fingerprint], [0.25, 4], 0, 7)
            with self.assertRaises(ValueError):
                write_sweep(trace, root / "overflow", [fingerprint], [1e250], 0, 7)
            self.assertFalse((root / "overflow").exists())
            with self.assertRaisesRegex(ValueError, "instead of clamping"):
                write_sweep(trace, root / "subunit", [fingerprint], [0.05], 0, 7)
            self.assertFalse((root / "subunit").exists())
            actual = root / "actual.json"
            measured = {"baseline_kind": "measured_actual_rows", "targets": [
                {"fingerprint": fingerprint, "operator": "CLogicalGet", "actual_rows": 40,
                 "sql": "SELECT * FROM t", "status": "ok"}]}
            actual.write_text(json.dumps(measured))
            truth = write_sweep(trace, root / "truth", [fingerprint], [0.25, 4], 0, 7, actual)
            self.assertEqual(truth["baseline_kind"], "measured_actual_rows")
            self.assertEqual(truth["targets"][0]["native_rows"], 10)
            self.assertEqual([p["requested_rows"] for p in truth["points"]], [[40], [10], [160]])
            measured["targets"][0]["actual_rows"] = 0
            actual.write_text(json.dumps(measured))
            with self.assertRaises(ValueError):
                write_sweep(trace, root / "zero", [fingerprint], [1], 0, 7, actual)

    def test_coupled_stats_coordinates_preserve_nested_input_ratios(self):
        parent, child = '0123456789abcdef', 'fedcba9876543210'
        records = [{'kind': 'stats_observation', 'experiment': 'discovery',
                    'operator': operator, 'fingerprint': fingerprint, 'native_rows': rows}
                   for operator, fingerprint, rows in [('CLogicalSelect', parent, 6), ('CLogicalGet', child, 8)]]
        records.append({'kind': 'experiment_outcome', 'experiment': 'discovery', 'stats_targets': []})
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            trace = root / 'source.trace'
            trace.write_text('\n'.join('DSL_TRACE ' + json.dumps(r) for r in records))
            grouped = write_sweep(trace, root / 'coupled', [parent, child], [.25, 4], 0, 7,
                                  coordinate_groups=[[child, parent]])
            self.assertEqual([p['requested_rows'] for p in grouped['points']], [[6, 8], [1.5, 2], [24, 32]])
            self.assertEqual([p['coordinate_factors'] for p in grouped['points']], [[1], [.25], [4]])
            self.assertEqual(grouped['sampling_measure']['space'], 'declared_coordinate_groups_not_individual_targets')
            for groups in ([], [[parent]], [[parent, child], [child]], [[parent, 'bad']], [[], [parent, child]]):
                with self.assertRaisesRegex(ValueError, 'partition'):
                    write_sweep(trace, root / 'invalid', [parent, child], [1], 0, 7, coordinate_groups=groups)
                self.assertFalse((root / 'invalid').exists())

    def test_cohort_sweep_samples_observed_inputs_without_rule_outcomes(self) -> None:
        event = {"kind": "stats_observation", "experiment": "discovery", "operator": "CLogicalGet",
                 "fingerprint": "0123456789abcdef", "native_rows": 10}
        other = {**event, "operator": "CLogicalCTEConsumer", "fingerprint": "fedcba9876543210", "native_rows": 2}
        records = [event, other, {**event, "operator": "CLogicalInnerJoin", "fingerprint": "0000000000000000"},
                   {"kind": "experiment_outcome", "experiment": "discovery", "stats_targets": []}]
        target, size = sampled_input_target(records, "seed")
        self.assertEqual(size, 2)
        self.assertEqual((target, size), sampled_input_target(list(reversed(records)), "seed"))
        self.assertEqual((target, size), sampled_input_target([*records, event], "seed"))
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "tpch/q"
            source.mkdir(parents=True)
            (source / "stats-1.replacement.trace").write_text("\n".join("DSL_TRACE " + json.dumps(r) for r in records))
            (source / "comparison.json").write_text(json.dumps({"query_crc32": "abc"}))
            cohort = {"queries": [{"query": "tpch/q", "family": "f", "query_crc32": "abc", "selected": True, "split": "discovery"}]}
            path = root / "cohort.json"
            path.write_text(json.dumps(cohort))
            suite = write_cohort_sweeps(path, root, root / "configs", [0.5, 1, 2], 7, None)
            self.assertEqual(suite["queries"][0]["input_probability"], 0.5)
            manifest = json.loads(Path(suite["queries"][0]["manifest"]).read_text())
            self.assertEqual(len(manifest["points"]), 3)
            cohort["queries"][0]["split"] = "holdout"
            path.write_text(json.dumps(cohort))
            with self.assertRaisesRegex(ValueError, "discovery"):
                write_cohort_sweeps(path, root, root / "bad", [1], 7, None)
            self.assertFalse((root / "bad").exists())

    def test_injection_order_and_paired_rule_sweep_fail_closed(self) -> None:
        row = {"kind": "rule_candidate", "placement": "cbo", "rule_hash": "r", "sequence": 1,
               "status": "match_rejected", "evaluated": True, "memo_outcome": None, "direct_insertions": None,
               "input_context": {"root": {"stats_source": "missing"}}}
        injection = {"kind": "stats_injection", "experiment": "exp", "fingerprint": "f",
                     "operator": "CLogicalGet", "rows": 10}
        audit = {"complete": True, "rows": [row], "exclusions": []}
        self.assertEqual(injection_order([row, injection], audit)["attempts_after_first_injection"], 0)
        self.assertEqual(injection_order([injection, row], audit)["attempts_after_first_injection"], 1)
        self.assertIsNone(injection_order([row], audit)["attempts_after_first_injection"])
        with self.assertRaisesRegex(ValueError, "sequences"):
            injection_order([{**row, "rule_hash": "other"}], audit)
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for index, factor in enumerate((1, 2), 1):
                events = [row, {**injection, "rows": factor * 10}]
                (root / f"stats-{index}.replacement.trace").write_text("\n".join("DSL_TRACE " + json.dumps(r) for r in events))
                (root / f"stats-{index}.replacement.plan.json").write_text('[{"Plan":{"Node Type":"Result"}}]')
            manifest = {"targets": [{"fingerprint": "f", "operator": "CLogicalGet"}], "points": [
                {"file": f"{factor}.yaml", "factors": [factor], "requested_rows": [factor * 10],
                 "design": "control" if factor == 1 else "one_at_a_time"} for factor in (1, 2, 4)]}
            experiments = []
            for factor in (1, 2):
                run = {"plan_rc": 0, "rows_rc": 0, "rows_hash": "same", "optimizer": "pg_orca",
                       "candidate_events": [row], "dsl_observability": {"rules": {}},
                       "experiment_outcomes": [{"experiment": "exp", "memo_group_expressions": 8,
                           "stats_targets": [{"operator": "CLogicalGet", "fingerprint": "f", "selector": "f",
                           "selector_kind": "expression", "requested_rows": factor * 10, "consumed": True}]}]}
                experiments.append({"path": str(root / f"{factor}.yaml"), "modes": {"replacement": run},
                                    "comparisons": {"replacement": {"plan_comparison": "identical"}}})
            comparison = {"stats_experiments": experiments, "modes": {"replacement": run}}
            with patch("profile_rule_candidates.candidate_evidence", return_value=audit) as evaluator:
                points = sweep_candidate_evidence(manifest, comparison, root, root)
                self.assertEqual([p["status"] for p in points], ["ok", "ok", "missing"])
                self.assertEqual(points[1]["rule_deltas"]["r"]["attempts"], 0)
                self.assertTrue(points[1]["stage_path_equal_to_control"])
                self.assertIsNone(points[2]["rules"])
                self.assertFalse(points[2]["paired"])
                changed = {**row, "status": "constraint_rejected", "failed_constraint": "AttrsSub"}
                experiments[1]["modes"]["replacement"]["candidate_events"] = [changed]
                (root / "stats-2.replacement.trace").write_text("\n".join("DSL_TRACE " + json.dumps(r)
                    for r in [changed, {**injection, "rows": 20}]))
                evaluator.side_effect = lambda run: {**audit, "rows": run["candidate_events"]}
                points = sweep_candidate_evidence(manifest, comparison, root, root)
                self.assertFalse(points[1]["stage_path_equal_to_control"])
                self.assertEqual(points[1]["rule_deltas"]["r"]["match_rejected"], -1)
                self.assertEqual(points[1]["rule_deltas"]["r"]["constraint:AttrsSub"], 1)
                # Joint sweeps must consume every coordinate, including unchanged ones.
                manifest["targets"].append({"fingerprint": "g", "operator": "CLogicalGet"})
                for point in manifest["points"]:
                    point["factors"].append(1)
                    point["requested_rows"].append(5)
                for index, experiment in enumerate(experiments, 1):
                    mode = experiment["modes"]["replacement"]
                    mode["experiment_outcomes"][0]["stats_targets"].append({
                        "operator": "CLogicalGet", "fingerprint": "g", "selector": "g",
                        "selector_kind": "expression", "requested_rows": 5, "consumed": True})
                    trace = root / f"stats-{index}.replacement.trace"
                    trace.write_text(trace.read_text() + "\nDSL_TRACE " + json.dumps(
                        {**injection, "fingerprint": "g", "rows": 5}))
                points = sweep_candidate_evidence(manifest, comparison, root, root)
                self.assertEqual([p["status"] for p in points], ["ok", "ok", "missing"])
                trace = root / "stats-2.replacement.trace"
                trace.write_text(trace.read_text().rsplit("\n", 1)[0])
                points = sweep_candidate_evidence(manifest, comparison, root, root)
                self.assertEqual(points[1]["status"], "invalid_trace")
                self.assertFalse(points[1]["paired"])
                # A failed control excludes every paired response, not just 1x.
                experiments[0]["modes"]["replacement"]["plan_rc"] = 124
                points = sweep_candidate_evidence(manifest, comparison, root, root)
                self.assertFalse(any(p["paired"] for p in points))

    def test_binding_origin_edges_retain_attempts_and_instance_paths(self) -> None:
        candidate = {"sequence": 1, "rule_hash": "b", "status": "match_rejected", "evaluated": True}
        edge = {"kind": "rule_edge", "engine": "pgorca", "scheduler": "cbo", "src_rule": "a", "dst_rule": "b",
                "target_path": "r", "src_target_path": "r", "dst_source_path": None, "dst_binding_path": "r/0",
                "path_kind": "source_binding_expression", "candidate_status": "match_rejected",
                "dst_candidate_sequence": 1, "binding_edge_sequence": 1, "binding_group": 2,
                "binding_group_expression": 1, "evidence": "runtime_observed", "relation": "binding_observed",
                "producer_relation": "memo_consumes"}
        final = {"binding_edge_trace_version": 1, "binding_origin_edges": 1}
        run = {"experiment_outcomes": [final], "rule_edges": [edge]}
        with patch("profile_rule_candidates.candidate_evidence", return_value={
                "complete": True, "exclusions": [], "rows": [candidate]}):
            self.assertTrue(binding_origin_evidence(run)["complete"])
            for change, error in (({"dst_candidate_sequence": 2}, "unresolved_binding_edge_candidate"),
                                  ({"binding_edge_sequence": 2}, "binding_edge_sequence_gap_or_duplicate"),
                                  ({"dst_source_path": "r/0"}, "binding_path_mislabeled_as_template"),
                                  ({"relation": "memo_consumes"}, "invalid_binding_edge_relation")):
                evidence = binding_origin_evidence({**run, "rule_edges": [{**edge, **change}]})
                self.assertFalse(evidence["complete"])
                self.assertIn(error, evidence["exclusions"])
            self.assertFalse(binding_origin_evidence({**run, "rule_edges": []})["complete"])
            self.assertFalse(binding_origin_evidence({**run, "experiment_outcomes": [{}]})["complete"])
        base = {"nodes": [{"rule_hash": "a"}, {"rule_hash": "b"}], "edges": []}
        graph = merge_graph(base, [edge, edge, {**edge, "dst_binding_path": "r/1"}])
        self.assertEqual(len(graph["edges"]), 2)
        self.assertEqual(graph["edges"][0]["candidate_status_counts"], {"match_rejected": 2})
        self.assertIsNone(graph["edges"][0]["dst_source_path"])
        self.assertIn("r->r/0 x2", render_dot(graph))

    def test_runtime_rule_edges_merge_as_multigraph_evidence(self) -> None:
        base = {
            "schema_version": 1,
            "nodes": [
                {
                    "rule_id": 1,
                    "rule_hash": "a",
                    "source_root": "Filter",
                    "target_root": "Input",
                },
                {
                    "rule_id": 2,
                    "rule_hash": "b",
                    "source_root": "Proj",
                    "target_root": "Proj",
                },
            ],
            "edges": [
                {
                    "src_rule": "a",
                    "dst_rule": "b",
                    "target_path": "r",
                    "evidence": "static_template",
                }
            ],
            "unresolved_inputs": [],
        }
        event = {
            "kind": "rule_edge",
            "engine": "pgorca",
            "scheduler": "rbo",
            "src_rule": "a",
            "dst_rule": "b",
            "target_path": "r/0",
            "path_kind": "instantiated_expression",
            "binding_path": "r/1",
            "evidence": "runtime_observed",
            "relation": "followed_by",
        }

        graph = merge_graph(base, [event, event])

        self.assertEqual(graph["schema_version"], 2)
        self.assertEqual(len(graph["edges"]), 2)
        self.assertEqual(graph["edges"][1]["observations"], 2)
        self.assertEqual(graph["edges"][1]["src_target_path"], "r/0")
        self.assertEqual(graph["edges"][1]["dst_source_path"], "r")
        self.assertEqual(
            graph["edges"][1]["binding_path_counts"], {"r/1": 2}
        )
        dot = render_dot(graph)
        self.assertIn("color=blue", dot)
        self.assertIn(r"followed_by\\nrbo:r/0->r x2", dot)
        with self.assertRaisesRegex(ValueError, "unknown rule"):
            merge_graph(base, [{**event, "dst_rule": "missing"}])

        with tempfile.TemporaryDirectory() as temporary:
            artifact = Path(temporary) / "query" / "replacement.plan"
            artifact.parent.mkdir()
            artifact.write_text(
                'LOG: DSL_TRACE {"kind":"rule_edge","engine":"pgorca"}\n',
                encoding="utf-8",
            )
            ignored = artifact.with_suffix(".json")
            ignored.write_text('{"kind":"rule_edge"}\n', encoding="utf-8")
            self.assertEqual(
                list(read_trace_inputs([Path(temporary)])),
                [{"kind": "rule_edge", "engine": "pgorca"}],
            )

    def test_rbo_candidate_set_adds_directed_order_edge(self) -> None:
        base = {
            "nodes": [
                {"rule_hash": "a", "source_root": "Filter"},
                {"rule_hash": "b", "source_root": "Filter"},
            ],
            "edges": [],
        }
        shared = {
            "kind": "rule_candidate",
            "engine": "pgorca",
            "experiment": "trial",
            "state_fingerprint": "state",
            "binding_fingerprint": "binding",
            "binding_path": "r/0",
            "placement": "rbo",
        }
        graph = merge_graph(
            base,
            [
                {**shared, "rule_hash": "a", "status": "applied_rbo"},
                {**shared, "rule_hash": "b", "status": "applicable_rbo"},
                {
                    "kind": "experiment_outcome",
                    "engine": "pgorca",
                    "experiment": "trial",
                },
            ],
        )

        self.assertEqual(graph["nodes"][0]["candidate_observations"], 1)
        self.assertEqual(
            graph["nodes"][1]["candidate_status_counts"],
            {"applicable_rbo": 1},
        )
        self.assertEqual(len(graph["edges"]), 1)
        self.assertEqual(graph["edges"][0]["relation"], "ordered_before")
        self.assertEqual(graph["edges"][0]["src_rule"], "a")
        self.assertEqual(graph["edges"][0]["dst_rule"], "b")

    def test_cbo_candidate_outcome_records_memo_acceptance(self) -> None:
        base = {
            "nodes": [{"rule_hash": "a", "source_root": "Filter"}],
            "edges": [],
        }
        candidate = {
            "kind": "rule_candidate",
            "engine": "pgorca",
            "experiment": "trial",
            "sequence": 3,
            "rule_hash": "a",
            "placement": "cbo",
            "status": "ready_cbo",
        }
        outcome = {
            "kind": "rule_candidate_outcome",
            "engine": "pgorca",
            "experiment": "trial",
            "candidate_sequence": 3,
            "rule_hash": "a",
            "status": "memo_inserted",
            "memo_version_before": 8,
            "memo_version_after": 10,
        }
        experiment_outcome = {
            "kind": "experiment_outcome",
            "engine": "pgorca",
            "experiment": "trial",
            "selected_plan_cbo_dsl_rules": {"a": 2},
        }

        graph = merge_graph(base, [candidate, outcome, experiment_outcome])

        self.assertEqual(
            graph["nodes"][0]["candidate_outcome_counts"],
            {"memo_inserted": 1},
        )
        self.assertEqual(graph["nodes"][0]["memo_version_delta_total"], 2)
        self.assertEqual(graph["nodes"][0]["selected_plan_observations"], 2)
        self.assertEqual(graph["nodes"][0]["selected_plan_queries"], 1)
        with self.assertRaisesRegex(ValueError, "no matching candidate"):
            merge_graph(base, [outcome])

    @patch("run_workload_comparison.wait_ready", return_value=True)
    @patch("run_workload_comparison.run")
    def test_workload_psql_retries_after_server_recovery(self, run, _ready) -> None:
        run.side_effect = [
            subprocess.CompletedProcess([], 2, "", "database system is in recovery mode"),
            subprocess.CompletedProcess([], 0, "ok", ""),
        ]

        stdout, _, returncode, _ = psql(Path("psql"), Path("/tmp"), 1, "db", "SELECT 1", 1)

        self.assertEqual((stdout, returncode), ("ok", 0))
        self.assertEqual(run.call_count, 2)

    def test_workload_comparison_preserves_xform_order_and_plan_reason(self) -> None:
        trace = (
            'TRACE,"Xform: CXformFirst\nAlternatives:\n0:\n"\n'
            'TRACE,"Xform: CXformSecond\nAlternatives:\n"\n'
            'TRACE,"Xform: CXformThird\nAlternatives:\n0:\n"\n'
        )
        left = {
            "Node Type": "Hash Join",
            "Plans": [
                {"Node Type": "Seq Scan", "Relation Name": "a"},
                {"Node Type": "Seq Scan", "Relation Name": "b"},
            ],
        }
        right = {
            "Node Type": "Hash Join",
            "Plans": [
                {"Node Type": "Seq Scan", "Relation Name": "b"},
                {"Node Type": "Seq Scan", "Relation Name": "a"},
            ],
        }

        self.assertEqual(produced_xforms(trace), ["CXformFirst", "CXformThird"])
        self.assertEqual(plan_difference(left, right), "join_order")
        right["Node Type"] = "Merge Join"
        self.assertEqual(plan_difference(left, right), "join_order")
        self.assertEqual(
            error_summary('TRACE,"large trace"\nERROR: timed out\n'),
            "ERROR: timed out",
        )
        self.assertIn(
            "Failed assertion",
            error_summary(
                "INFO:  pg_orca: falling back to standard planner\n"
                "DETAIL:  file.cpp:1: Failed assertion: value\n"
            ),
        )
        self.assertEqual(
            optimizer_name('[{"Plan": {}, "Optimizer": "pg_orca"}]'),
            "pg_orca",
        )
        self.assertEqual(optimizer_name('[{"Plan": {}}]'), "postgres")
        self.assertEqual(
            explain_times('[{"Plan": {}, "Planning Time": 1.25, "Execution Time": 2.5}]'),
            (1.25, 2.5),
        )
        self.assertEqual(
            optimization_time("[OPT]: Total Optimization Time: 17ms"), 17
        )
        self.assertTrue(server_failure("server closed the connection unexpectedly"))
        timings = timing_summary(
            [{"modes": {"native": {"value": value}}} for value in range(1, 101)],
            "native",
            "value",
        )
        self.assertEqual((timings["p50_ms"], timings["p95_ms"]), (50, 95))
        self.assertIn("SET optimizer_print_xform=off;", trace_settings("replacement", [], None))

    def test_workload_stats_experiment_is_query_local_and_recorded(self) -> None:
        self.assertIn(
            "RESET pg_orca.dsl_stats_experiment_path;",
            settings("native", [], None),
        )
        configured = settings("replacement", [], None, Path("/tmp/a'b.yml"))
        self.assertIn(
            "SET pg_orca.dsl_stats_experiment_path='/tmp/a''b.yml';",
            configured,
        )
        records = trace_records(
            'LOG: DSL_TRACE {"kind":"stats_injection","relations":"a",'
            '"native_rows":10,"rows":20}\n'
            'LOG: DSL_TRACE {"kind":"experiment_outcome",'
            '"experiment":"trial","optimizer_cost":1.5}\n'
        )
        self.assertEqual(records[0]["rows"], 20)
        self.assertEqual(records[1]["experiment"], "trial")

    def test_stats_experiment_observability_keeps_trigger_and_search_distributions(self) -> None:
        records = [
            {"kind": "memo_summary", "groups": 4, "group_expressions": 9},
            {"kind": "pipeline_summary", "bindings_built": 7},
            {
                "kind": "rule_summary",
                "rule_hash": "a",
                "binding_attempts": 3,
                "generated_alternatives": 1,
            },
            {
                "kind": "rule_candidate",
                "rule_hash": "a",
                "status": "applied_rbo",
                "match_us": 4,
                "constraint_us": 2,
                "instantiate_us": 3,
            },
            {
                "kind": "rule_candidate_outcome",
                "rule_hash": "a",
                "status": "memo_inserted",
                "memo_version_before": 5,
                "memo_version_after": 7,
            },
        ]
        observed = dsl_observability(records)
        run = {
            "workload": "tpch",
            "query": "q01.sql",
            "modes": {},
            "rule_profile": None,
            "stats_experiments": [{
                "path": "/tmp/card.yaml",
                "modes": {"replacement": {
                    "plan_rc": 0,
                    "rows_rc": 0,
                    "optimizer": "pg_orca",
                    "stats_events": [{
                        "kind": "stats_injection",
                        "fingerprint": "f",
                        "native_rows": 10,
                        "rows": 20,
                    }],
                    "experiment_outcomes": [{
                        "experiment": "card-2x",
                        "memo_groups": 4,
                        "stats_targets": [{"selector_kind": "expression", "selector": "g",
                                           "operator": "CLogicalGet", "fingerprint": "g",
                                           "requested_rows": 30, "consumed": True}],
                    }],
                    "optimization_ms": 2,
                    "planning_ms": 3.0,
                    "execution_ms": 1.0,
                    "dsl_observability": observed,
                }},
            }],
        }

        observations = experiment_observations([run])
        distributions = experiment_distribution_summary(observations)
        summary = distributions["groups"][0]

        self.assertEqual(distribution([1, 2, 9])["p50"], 2)
        self.assertEqual(summary["statistics"]["f"]["scale"]["mean"], 2.0)
        self.assertEqual(summary["search_space"]["group_expressions"]["max"], 9)
        self.assertEqual(
            summary["rule_triggers"]["a"]["generated_alternatives"]["mean"],
            1.0,
        )
        self.assertEqual(
            summary["candidate_triggers"]["a"]["statuses"]["applied_rbo"]["max"],
            1,
        )
        self.assertEqual(
            summary["candidate_effects"]["a"]["memo_version_delta"]["max"],
            2,
        )
        point = distributions["factor_curves"][0]["points"][0]
        self.assertTrue(point["intervened"])
        self.assertEqual(point["scale"], 2.0)
        self.assertEqual(point["scale_basis"], "stats_at_application")
        self.assertEqual(point["intervention_targets"][0]["fingerprint"], "g")
        self.assertEqual(point["search"]["memo_groups"], 4)
        self.assertEqual(
            point["rule_triggers"],
            {"a": {"binding_attempts": 3, "generated_alternatives": 1}},
        )
        self.assertEqual(point["candidate_triggers"]["a"]["statuses"], {"applied_rbo": 1})

    def test_experiment_status_does_not_accept_failed_or_fallback_arms(self) -> None:
        success = {
            "plan_rc": 0, "rows_rc": 0, "optimizer": "pg_orca",
            "rows_hash": "same", "experiment_outcomes": [{"experiment": "card", "stats_targets": []}],
        }
        cases = [
            ({}, "ok"),
            ({"plan_rc": 124}, "plan_timeout"),
            ({"plan_rc": 3, "error": "ERROR: canceling statement due to statement timeout"}, "plan_timeout"),
            ({"rows_rc": 124}, "rows_timeout"),
            ({"plan_rc": 3}, "plan_error"),
            ({"rows_rc": 3}, "rows_error"),
            ({"server_failure": True}, "server_failure"),
            ({"optimizer": "postgres"}, "fallback"),
            ({"optimizer": None}, "unknown_optimizer"),
            ({"experiment_outcomes": []}, "incomplete_trace"),
            ({"experiment_outcomes": [{}, {}]}, "incomplete_trace"),
            ({"experiment_outcomes": [{"experiment": "legacy"}]}, "incomplete_trace"),
        ]
        for changes, expected in cases:
            with self.subTest(expected=expected, changes=changes):
                result = {**success, **changes}
                self.assertEqual(experiment_run_status(result, True), expected)
                self.assertEqual(
                    profile_comparability(success, result, True)["result_comparable"],
                    expected == "ok",
                )
        self.assertEqual(experiment_run_status({}, False), "unknown")
        self.assertTrue(profile_comparability(
            success, {**success, "experiment_outcomes": []}, False,
        )["result_comparable"])
        self.assertEqual(profile_comparability(
            success, {**success, "rows_hash": "different"}, True,
        )["exclusions"], ["rows_mismatch"])
        self.assertFalse(profile_comparability(
            {**success, "plan_rc": 3}, {**success, "plan_rc": 3}, True,
        )["result_comparable"])

    def test_profile_exports_baseline_and_failed_samples_without_zero_filling(self) -> None:
        success = {
            "plan_rc": 0, "rows_rc": 0, "optimizer": "pg_orca", "rows_hash": "same",
            "stats_events": [{"kind": "stats_injection", "fingerprint": "f",
                              "native_rows": 10, "rows": 20}],
            "experiment_outcomes": [{"experiment": "card", "memo_groups": 4, "stats_targets": []}],
            "optimization_ms": 2, "planning_ms": 3, "execution_ms": 1,
            "dsl_observability": dsl_observability([
                {"kind": "rule_summary", "rule_hash": "a", "binding_attempts": 3},
            ]),
        }
        baseline = {**success, "experiment_outcomes": [], "stats_events": []}
        timeout = {**success, "plan_rc": 124, "rows_rc": 124,
                   "experiment_outcomes": [], "dsl_observability": dsl_observability([])}
        run = {
            "workload": "tpch", "query": "q04.sql", "stats_experiments": [],
            "rule_profile": {"rule_hash": "target", "scenarios": [
                {"stats_experiment": None, "arms": dict.fromkeys(("off", "rbo", "cbo"), baseline)},
                {"stats_experiment": "/tmp/card.yaml", "arms": {
                    "off": success, "rbo": timeout,
                    "cbo": {**success, "optimizer": "postgres", "experiment_outcomes": []},
                }},
                {"stats_experiment": "/tmp/card.yaml", "arms": dict.fromkeys(("off", "rbo", "cbo"), success)},
            ]},
        }
        observations = experiment_observations([run])
        self.assertEqual(len(observations), 9)
        self.assertEqual([x["status"] for x in observations[:3]], ["ok"] * 3)
        self.assertEqual(observations[4]["status"], "plan_timeout")
        self.assertEqual(observations[5]["status"], "fallback")
        self.assertFalse(observations[4]["comparison"]["result_comparable"])
        self.assertEqual(observations[4]["stats"], success["stats_events"])
        summary = experiment_distribution_summary(observations)
        group = next(g for g in summary["groups"]
                     if g["experiment_path"] and g["arm"] == "rbo")
        self.assertEqual(group["runs"], 2)
        self.assertEqual(group["complete_runs"], 1)
        self.assertEqual(group["result_comparable_runs"], 1)
        self.assertEqual(group["status_counts"], {"ok": 1, "plan_timeout": 1})
        self.assertEqual(group["experiment"], "card")
        self.assertEqual(group["profile_rule_hash"], "target")
        self.assertEqual(group["rule_triggers"]["a"]["binding_attempts"]["mean"], 3)
        self.assertEqual(group["search_space"]["memo_groups"]["count"], 1)
        curve = next(c for c in summary["factor_curves"] if c["arm"] == "rbo")
        self.assertEqual([p["complete"] for p in curve["points"]], [False, True])

    def test_profile_requires_consumed_and_matching_stats_targets(self) -> None:
        target = {"selector_kind": "expression", "selector": "f", "operator": "CLogicalGet",
                  "fingerprint": "f", "requested_rows": 10, "consumed": True}

        def result(targets):
            return {"plan_rc": 0, "rows_rc": 0, "optimizer": "pg_orca", "rows_hash": "same",
                    "experiment_outcomes": [{"experiment": "card", "stats_targets": targets}],
                    "stats_events": [{"kind": "stats_injection", "changed": False}]}

        reference = result([target])
        self.assertTrue(profile_comparability(reference, reference, True)["result_comparable"])
        self.assertEqual(experiment_run_status(result([{**target, "consumed": False}]), True),
                         "injection_unconsumed")
        self.assertFalse(profile_comparability(
            reference, result([{**target, "consumed": False}]), True,
        )["result_comparable"])
        self.assertEqual(experiment_run_status(result([{}]), True), "incomplete_trace")
        for field, value in (("selector", "g"), ("fingerprint", "g"), ("requested_rows", 20),
                             ("operator", "CLogicalSelect"), ("selector_kind", "relations")):
            with self.subTest(field=field):
                self.assertEqual(profile_comparability(
                    reference, result([{**target, field: value}]), True,
                )["exclusions"], ["injection_target_mismatch"])
        second = {**target, "selector": "g", "fingerprint": "g"}
        self.assertTrue(profile_comparability(
            result([target, second]), result([second, target]), True,
        )["result_comparable"])

    def test_workload_rule_profile_changes_only_target_policy(self) -> None:
        args = SimpleNamespace(
            profile_rule="0123456789abcdef",
            profile_rbo_phase="pre_join",
            profile_effect="preserves_join_graph",
        )
        with tempfile.TemporaryDirectory() as temporary:
            policies = write_profile_policies(Path(temporary), args)
            off = policies["off"].read_text(encoding="utf-8")
            rbo = policies["rbo"].read_text(encoding="utf-8")
            cbo = policies["cbo"].read_text(encoding="utf-8")
            args.profile_cbo_only = True
            binary_dir = Path(temporary) / "binary"
            binary_dir.mkdir()
            binary = write_profile_policies(binary_dir, args)
            self.assertEqual(set(binary), {"off", "cbo"})
            self.assertFalse((binary_dir / "profile-rbo.policy").exists())
            self.assertEqual(binary["off"].read_text(), off)
            self.assertEqual(binary["cbo"].read_text(), cbo)

        self.assertIn("- rule: 0123456789abcdef\n", off)
        self.assertIn("enabled: false", off)
        self.assertIn("placement: rbo", rbo)
        self.assertIn("phase: pre_join", rbo)
        self.assertIn("placement: cbo", cbo)

    def test_replacement_rule_identities_are_explicitly_classified(self) -> None:
        rule_file = SCRIPT_DIR / "rules" / "orca_replacements.rules"
        identities, errors = audit_rule_file(rule_file)

        self.assertEqual(errors, [])
        self.assertEqual(
            sum(item.classification.startswith("BRIDGE(") for item in identities),
            0,
        )
        self.assertEqual(
            sum(item.classification.startswith("DIRECT(") for item in identities),
            2,
        )

    def test_replacement_rule_identity_requires_an_immediate_tag(self) -> None:
        rule = "Input<t0>|Input<t1>|TableEq(t1,t0)\n"
        _, errors = audit_rule_text(rule)

        self.assertRegex(errors[0], "requires exactly one immediate")

    def test_replacement_inventory_merges_runtime_and_causal_evidence(self) -> None:
        runtime = {
            "xforms": [
                {"name": "CXformSimplifyGbAgg", "category": "semantic_rewrite"},
                {
                    "name": "CXformJoinAssociativity",
                    "category": "join_enumeration",
                    "replacement_owner": "dphyper",
                },
                {
                    "name": "CXformImplementFullOuterMergeJoin",
                    "category": "implementation_property",
                },
            ]
        }
        matrices = [
            {
                "case": "simplify",
                "xforms": ["CXformSimplifyGbAgg"],
                "scope": "key-backed pure dedup",
                "status": "verified_partial_replacement",
                "dsl_rules": [101],
                "dsl_rule_hashes": ["0123456789abcdef"],
                "excluded_native_domains": ["unsafe error reordering"],
            }
        ]

        inventory = merge_inventory(runtime, matrices)
        graph = {'nodes': [{'rule_hash': '0123456789abcdef'}]}
        self.assertEqual(merge_inventory(runtime, matrices, graph), inventory)
        with self.assertRaisesRegex(ValueError, 'simplify:.*absent rules'):
            merge_inventory(runtime, matrices, {'nodes': []})

        self.assertEqual(inventory["summary"]["verified_partial_xforms"], 1)
        self.assertEqual(
            inventory["xforms"][0]["replacement_status"],
            "verified_partial_replacement",
        )
        self.assertEqual(
            inventory["xforms"][0]["evidence"][0]["dsl_rule_hashes"],
            ["0123456789abcdef"],
        )
        self.assertEqual(
            inventory["xforms"][0]["evidence"][0]["excluded_native_domains"],
            ["unsafe error reordering"],
        )
        self.assertEqual(
            inventory["xforms"][1]["replacement_status"],
            "replaced_by_dphyper",
        )
        self.assertEqual(
            inventory["xforms"][2]["replacement_status"],
            "retained_in_cascades",
        )

    def test_provenance_audit_separates_retained_and_native_logic(self) -> None:
        runtime = {
            "xforms": [
                {"name": "CXformRewrite", "category": "semantic_rewrite"},
                {"name": "CXformSplit", "category": "implementation_property"},
            ]
        }
        records = [
            {"kind": "memo_alternative", "source": "input", "origin": "input"},
            {
                "kind": "memo_alternative",
                "source": "dsl",
                "origin": "CXformDSLRule_Select",
            },
            {
                "kind": "memo_alternative",
                "source": "native",
                "origin": "CXformSplit",
            },
            {
                "kind": "memo_alternative",
                "source": "native",
                "origin": "CXformRewrite",
            },
        ]

        audit = audit_memo_provenance(runtime, records)

        self.assertEqual(audit["origin_categories"]["retained_cascades"], 1)
        self.assertEqual(audit["unexpected_native_origins"], {"CXformRewrite": 1})

    def test_replacement_matrix_requires_causal_four_states(self) -> None:
        matrix = {
            "replacement": {"xforms": ["CXformSimplifyGbAgg"]},
            "plans": [
                {"name": "native", "dsl": False},
                {"name": "shadow"},
                {
                    "name": "negative",
                    "dsl": False,
                    "disable_xforms": ["CXformSimplifyGbAgg"],
                },
                {
                    "name": "replacement",
                    "disable_xforms": ["CXformSimplifyGbAgg"],
                    "provenance": {
                        "required_sources": ["dsl"],
                        "forbidden_origins": ["CXformSimplifyGbAgg"],
                    },
                },
            ],
            "rows": {"disable_xforms": ["CXformSimplifyGbAgg"]},
        }

        self.assertIsNone(validate_replacement_matrix(matrix))
        matrix["replacement"]["excluded_native_domains"] = [""]
        with self.assertRaisesRegex(ValueError, "unique descriptions"):
            validate_replacement_matrix(matrix)
        del matrix["replacement"]["excluded_native_domains"]
        del matrix["plans"][3]["provenance"]
        with self.assertRaisesRegex(ValueError, "must require memo provenance"):
            validate_replacement_matrix(matrix)

    def test_replacement_matrix_rejects_noncausal_controls(self) -> None:
        matrix = {
            "replacement": {"xforms": ["CXformSimplifyGbAgg"]},
            "plans": [
                {"name": "native", "dsl": False},
                {"name": "shadow"},
                {"name": "negative", "dsl": False},
                {
                    "name": "replacement",
                    "disable_xforms": ["CXformSimplifyGbAgg"],
                },
            ],
            "rows": {"disable_xforms": ["CXformSimplifyGbAgg"]},
        }

        with self.assertRaisesRegex(ValueError, "negative must disable"):
            validate_replacement_matrix(matrix)

    def test_e2e_can_preserve_extension_guc_defaults(self) -> None:
        self.assertEqual(
            bool_guc_setting("pg_orca.enable_dphyper", "default", False),
            "RESET pg_orca.enable_dphyper;",
        )

    def test_e2e_alternative_matching_rejects_xform_name_prefixes(self) -> None:
        output = (
            'TRACE,"Xform: CXformPushGbBelowUnionAll\n'
            "Input:\nAlternatives:\n0:\n"
            'TRACE,"Xform: CXformPushGbBelowUnion\n'
            "Input:\nAlternatives:\n"
        )

        self.assertTrue(
            produced_alternative(output, "CXformPushGbBelowUnionAll")
        )
        self.assertFalse(
            produced_alternative(output, "CXformPushGbBelowUnion")
        )

    def test_e2e_audits_transitive_memo_provenance(self) -> None:
        output = (
            'TRACE,"DSL_TRACE {"kind":"memo_alternative","source":"input",'
            '"origin":"input"}\n'
            'TRACE,"DSL_TRACE {"kind":"memo_alternative","source":"dsl",'
            '"origin":"CXformSelect2Filter"}\n'
        )
        contract = {
            "required_sources": ["dsl"],
            "required_origins": ["CXformSelect2Filter"],
            "forbidden_origins": ["CXformSelect2Apply"],
        }

        self.assertEqual(len(memo_provenance(output)), 2)
        self.assertEqual(
            actual_plan({"provenance": contract}, output)["provenance"], contract
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
                "priority_conflicts": [],
                "application_status_distribution": {
                    "applied_rbo": 1,
                    "duplicate": 1,
                },
            },
        )

    def test_alignment_summary_reports_priority_conflicts(self) -> None:
        records = [
            {
                "kind": "application",
                "rule_id": 32,
                "status": "applicable_rbo",
                "selected_rule_id": 21,
            },
            {
                "kind": "application",
                "rule_id": 32,
                "status": "applicable_rbo",
                "selected_rule_id": 21,
            },
        ]

        summary = alignment_summary([], records)

        self.assertEqual(
            summary["priority_conflicts"],
            [{"selected_rule_id": 21, "applicable_rule_id": 32, "count": 2}],
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
        self.assertEqual(
            disabled_xform_settings(
                {"disable_xforms": ["CXformRewrite"]},
                ["CXformRewrite", "CXformOther"],
            ).count("disable_xform("),
            2,
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


class RuleExampleExportTest(unittest.TestCase):
    def test_wide_example_keeps_union_row_and_projects_a_proper_subset(self):
        import sqlglot
        from sqlglot import exp
        example = render_example('CREATE TABLE t(a INT, b INT); CREATE TABLE u(x INT, y INT);',
            'SELECT DISTINCT a FROM (SELECT a,b FROM t UNION SELECT x,y FROM u) v',
            'SELECT a FROM t UNION SELECT x FROM u', 'wide_')
        source = sqlglot.parse_one(example['source'], read='postgres')
        target = sqlglot.parse_one(example['target'], read='postgres')
        self.assertEqual(len(source.selects), 1)
        self.assertEqual(len(source.find(exp.Union).left.selects), 2)
        self.assertEqual(len(target.left.selects), 1)
        for create in sqlglot.parse(example['schema'], read='postgres'):
            self.assertEqual(len(create.this.expressions), 2)

    def test_relation_instantiation_is_simultaneous_and_preserves_aliases(self):
        import sqlglot
        from sqlglot import exp
        sql = 'SELECT a.k, b.k, "Odd Name".k FROM t a JOIN t b ON a.k=b.k CROSS JOIN "Odd Name"'
        replacement = {'t': 'SELECT * FROM t WHERE k IS NOT NULL',
                       'Odd Name': 'SELECT * FROM "Odd Name" WHERE k > 0'}
        tree = sqlglot.parse_one(instantiate_relations(sql, replacement), read='postgres')
        subqueries = list(tree.find_all(exp.Subquery))
        self.assertEqual([s.alias for s in subqueries], ['a', 'b', 'Odd Name'])
        self.assertEqual(len(list(tree.find_all(exp.Table))), 3)
        self.assertEqual([c.sql() for c in tree.selects], ['a.k', 'b.k', '"Odd Name".k'])
        for original, mapping in [('WITH t AS (SELECT 1) SELECT * FROM t', replacement),
                                  ('SELECT * FROM public.t', replacement),
                                  ('SELECT * FROM missing', replacement),
                                  ('SELECT * FROM t', {'t': 'DELETE FROM t'}),
                                  ('SELECT * FROM t', {'t': 'SELECT k INTO copy FROM t'}),
                                  ('SELECT * FROM t', {'t': 'SELECT 1; SELECT 2'}),
                                  ('SELECT 1', replacement)]:
            with self.assertRaises(ValueError):
                instantiate_relations(original, mapping)

    def test_setop_examples_align_by_position_and_reject_missing_columns(self):
        ddl = 'CREATE TABLE l (a INT); CREATE TABLE r (b INT)'
        source = 'SELECT DISTINCT a FROM (SELECT a FROM l UNION ALL SELECT b FROM r) u'
        target = 'SELECT DISTINCT a FROM l UNION SELECT DISTINCT b FROM r'
        pair = render_example(ddl, source, target, 'union_')
        self.assertIn('UNION ALL', pair['source'])
        self.assertIn('"union_l"', pair['source'])
        self.assertIn('"union_r"', pair['target'])
        self.assertIn('NULL', pair['setup'])
        invalid = 'SELECT a, a FROM l UNION ALL SELECT b FROM r'
        with self.assertRaisesRegex(ValueError, 'set-op input arity'):
            render_example(ddl, invalid, invalid, 'union_')

    def test_postgres_oracle_rejects_common_mode_error_and_missing_result(self):
        args = SimpleNamespace(port=1, timeout=60)
        # Both ORCA modes agree on an incorrect duplicate NULL. PG deduplicates it.
        wrong = {'rows_rc': 0, **copy_result_summary('\n\n1\n')}
        with tempfile.TemporaryDirectory() as temporary, patch(
                'run_workload_comparison.psql', return_value=('\n1\n', '', 0, 1.0)) as run:
            result = collect_postgres_oracle(args, Path('psql'), Path('/tmp'), 'db',
                'SELECT DISTINCT a FROM t', Path(temporary), {'native': wrong, 'replacement': wrong})
            self.assertFalse(result['valid'])
            self.assertEqual(result['mode_bag_equal'], {'native': False, 'replacement': False})
            self.assertIn('pg_orca.enable_orca=off', run.call_args.args[4])
            good = {'rows_rc': 0, **copy_result_summary('\n1\n')}
            result = collect_postgres_oracle(args, Path('psql'), Path('/tmp'), 'db',
                'SELECT DISTINCT a FROM t', Path(temporary), {'native': good, 'replacement': good})
            self.assertTrue(result['valid'])
            # Matching bags cannot excuse a wrong ORDER BY result.
            result = collect_postgres_oracle(args, Path('psql'), Path('/tmp'), 'db',
                'SELECT DISTINCT a FROM t ORDER BY a', Path(temporary),
                {'native': {**good, 'rows_hash': 'incorrect-order'}})
            self.assertTrue(result['mode_bag_equal']['native'])
            self.assertFalse(result['valid'])
            run.return_value = ('', 'ERROR: failed', 1, 1.0)
            result = collect_postgres_oracle(args, Path('psql'), Path('/tmp'), 'db',
                'SELECT DISTINCT a FROM t', Path(temporary), {'native': good, 'replacement': good})
            self.assertFalse(result['valid'])
            self.assertIsNone(result['rows_bag_hash'])

    def test_report_retains_native_interference_and_missing_memo_counts(self):
        design = {'schema_crc32': 'schema', 'setup_crc32': 'setup', 'counts': {'ok': 1},
                  'limitations': [], 'examples': [{'line': 1, 'status': 'ok',
                      'queries': {s: {'crc32': s} for s in ('source', 'target')}}]}
        row = {'rule_id': 1, 'rule_hash': '0123456789abcdef', 'status': 'ready_cbo',
               'direct_insertions': None}
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary)
            (output / 'manifest.json').write_text(json.dumps(design))
            (output / 'summary.json').write_text(json.dumps({'fixture': {
                'schema_crc32': 'schema', 'setup': {'crc32': 'setup'}}}))
            for side in ('source', 'target'):
                folder = output / 'rule_examples' / f'r1_{side}'
                folder.mkdir(parents=True)
                case = {'query_crc32': side, 'forbidden_native_origins': ['native-rule'],
                        'outcome_equal': True, 'rows_bag_equal': True,
                        'join_enumeration_replaced': False,
                        'stats_experiments': [{'modes': {'replacement': {'stats_events': []}}}],
                        'modes': {m: {'rows_rc': 0, 'rows_bag_hash': 'same'} for m in ('native', 'replacement')}}
                (folder / 'comparison.json').write_text(json.dumps(case))
            plotting = MagicMock()
            plotting.subplots.return_value = (MagicMock(), [MagicMock(), MagicMock()])
            with patch('plot_stats_sweep.chinese_plotting', return_value=plotting), patch(
                    'profile_rule_candidates.candidate_evidence', return_value={
                        'complete': True, 'exclusions': [], 'rows': [row]}):
                destination = output / 'new-audit'
                report = summarize_examples(output, output, None, destination)
            self.assertTrue((destination / '生成场景验证.json').is_file())
            self.assertFalse((output / '生成场景验证.json').exists())
            source = report['examples'][0]['sides']['source']
            self.assertEqual(source['forbidden_native_origins'], ['native-rule'])
            self.assertIsNone(source['direct_insertions'])
            self.assertFalse(report['valid'])
            for side in ('source', 'target'):
                path = output / 'rule_examples' / f'r1_{side}' / 'comparison.json'
                case = json.loads(path.read_text())
                case.update(forbidden_native_origins=[], join_enumeration_replaced=True,
                            artifact_provenance={'endpoints_equal': False})
                path.write_text(json.dumps(case))
            with patch('plot_stats_sweep.chinese_plotting', return_value=plotting), patch(
                    'profile_rule_candidates.candidate_evidence', return_value={
                        'complete': True, 'exclusions': [], 'rows': [row]}):
                report = summarize_examples(output, output, None, destination)
            self.assertFalse(report['valid'])
            self.assertIn('experiment_artifacts_changed', report['examples'][0]['sides']['source']['exclusions'])

    def test_typed_pair_and_output_arity_guard(self):
        import sqlglot
        from sqlglot import exp

        ddl = 'CREATE TABLE t (a INT UNIQUE NOT NULL, b INT)'
        pair = render_example(ddl, 'SELECT DISTINCT a FROM t', 'SELECT a FROM t', 'seed_')
        self.assertIn('"seed_t"', pair['schema'])
        self.assertIn('UNIQUE NOT NULL', pair['schema'])
        self.assertIn('NULL', pair['setup'])
        inserts = sqlglot.parse_one(pair['setup'], read='postgres')
        rows = inserts.expression.expressions
        self.assertEqual(len(rows), 8)
        keys = [r.expressions[0].this for r in rows]
        self.assertEqual(len(set(keys)), 8)
        self.assertTrue(all(not isinstance(r.expressions[0], exp.Null) for r in rows))
        comparison = sqlglot.parse_one(pair['equivalence'], read='postgres')
        differences = list(comparison.find_all(exp.Except))
        self.assertEqual(len(differences), 2)
        self.assertTrue(all(d.args['distinct'] is False for d in differences))
        with self.assertRaisesRegex(ValueError, 'output arity'):
            render_example(ddl, 'SELECT a FROM t', 'SELECT * FROM t', 'seed_')
        with self.assertRaisesRegex(ValueError, 'integer'):
            render_example('CREATE TABLE t (a TEXT)', 'SELECT a FROM t', 'SELECT a FROM t', 'seed_')

    def test_adapter_keeps_every_failure_in_denominator(self):
        import base64

        message = base64.b64encode(b'unsupported constraint').decode()
        record = f'7\tsupported_domain\t{message}\n'
        self.assertEqual(decode_rule_examples(record, {7: 'rule'})[0]['payload'],
                         ['unsupported constraint'])
        for invalid in ('', record + record, record.replace('7\t', '8\t'),
                        record.replace('supported_domain', 'ok')):
            with self.assertRaises(ValueError):
                decode_rule_examples(invalid, {7: 'rule'})

    def test_example_unique_is_a_strict_key_not_nullable_sql_unique(self):
        import sqlglot
        from sqlglot import exp

        pair = render_example('CREATE TABLE t (a INT UNIQUE)',
                              'SELECT DISTINCT a FROM t', 'SELECT * FROM t', 'key_')
        self.assertIn('UNIQUE NOT NULL', pair['schema'])
        rows = sqlglot.parse_one(pair['setup'], read='postgres').expression.expressions
        values = [row.expressions[0] for row in rows]
        self.assertTrue(all(not isinstance(v, exp.Null) for v in values))
        self.assertEqual(len({v.sql() for v in values}), len(rows))


if __name__ == "__main__":
    unittest.main()
