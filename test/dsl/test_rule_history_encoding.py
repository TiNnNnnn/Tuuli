"""History time/population gates, concrete root routing, and trainable context fusion."""

from copy import deepcopy
import json
import unittest
from unittest.mock import patch

from group_expression_encoding import group_expression_tree
from profile_rule_candidates import candidate_state
from rule_history_encoding import attach_history
from rule_policy_encoding import encode_sequence, fit_vocabulary
from test_group_expression_encoding import attempt_fixture
from test_rule_policy_encoding import ir_fixture
from test_rule_tree_model import tree_features, torch


def actual_tree():
    row = attempt_fixture()
    row['input_context']['children'][0]['node'].update(operator='CLogicalSelect', arity=2)
    return group_expression_tree(candidate_state(row))


def history_fixture():
    with patch('test_rule_tree_model.encode_sequence', side_effect=lambda s, _: s):
        feature, _ = tree_features([ir_fixture(), ir_fixture()])
    tree = actual_tree()
    history = {'schema_version': 1, 'capture': 'audited_history_frozen', 'frozen_at_utc': '2026-09-12T10:00:00+00:00',
        'runs': [{'source': {'path': 'source'}, 'unit': {'workload': 'dev', 'query': '1'},
            'inputs': {'query_sql': 'select * from a', 'catalog_snapshot': {'path': 'catalog'},
                       'graph_snapshot': {'path': 'graph'}}, 'trees': [tree, None],
            'contexts': [{'rule_hash': '1', 'tree': 0, 'attempts': 2}, {'rule_hash': '1', 'tree': 1, 'attempts': 1}],
            'edges': [{'src_rule': '0', 'dst_rule': '1', 'src_target_path': 'r/0/0/5',
                       'dst_binding_path': 'r/0', 'producer_relation': 'input_exposes',
                       'producer_outcome': 'memo_duplicate', 'tree': 0,
                       'root': next(i for i, n in enumerate(tree['nodes']) if n['path'] == 'r/0')}],
            'denominators': {'attempts': 3, 'observed_edges': 1, 'admitted_edges': 1, 'exclusions': {'context_unavailable': 1}}}]}
    return feature, history


def attach(feature, bundle, timestamp='2026-09-12T11:00:00+00:00', allowed=None):
    documents = {'source': {}, 'catalog': {'catalog': {'captured_at': '2026-09-12T09:00:00+00:00'}},
                 'graph': {'nodes': [{'rule_hash': '0'}, {'rule_hash': '1'}]}}
    with patch('rule_history_encoding.read_snapshot', side_effect=lambda s: json.dumps(documents[s['path']]).encode()), \
         patch('rule_history_encoding.input_sequences', return_value=deepcopy(feature)):
        return attach_history(feature, bundle, {}, timestamp, {'dev:1': 'select * from a'} if allowed is None else allowed)


class RuleHistoryEncodingTest(unittest.TestCase):
    def test_ordered_actual_tree_preserves_statistics_and_missing_deep_nodes(self):
        tree = actual_tree()
        self.assertEqual(tree['nodes'][tree['root']]['path'], 'r')
        indices = {n['path']: i for i, n in enumerate(tree['nodes'])}
        self.assertEqual(tree['nodes'][indices['r']]['children'], [indices['r/0'], indices['r/1'], indices['r/2']])
        self.assertIn(('ge:rows', None), tree['sequences'][indices['r/0/0']])
        self.assertNotIn(('ge:rows', None), tree['sequences'][indices['r/0']])
        row = attempt_fixture()
        with self.assertRaisesRegex(ValueError, 'disagree'):
            group_expression_tree(candidate_state(row))
        row['input_context']['children'][0]['node'].update(operator='CLogicalSelect', arity=2)
        row['input_context']['source_tree']['nodes'] = row['input_context']['source_tree']['nodes'][:3]
        row['input_context']['source_tree']['complete'] = False
        partial = group_expression_tree(candidate_state(row))
        self.assertFalse(partial['complete'])
        self.assertEqual(len(partial['nodes']), 3)
        self.assertNotIn('r/1', [n['path'] for n in partial['nodes']])

    def test_admission_refuses_future_and_unassigned_queries(self):
        feature, bundle = history_fixture()
        for timestamp in ('2026-09-12T09:59:59+00:00', bundle['frozen_at_utc'], '2026-09-12T11:00:00'):
            with self.assertRaises(ValueError):
                attach(deepcopy(feature), bundle, timestamp)
        with self.assertRaisesRegex(ValueError, 'training population'):
            attach(deepcopy(feature), bundle, allowed={})
        bundle['runs'][0]['unit']['query'] = '1.sql'
        ready = attach(feature, bundle)
        self.assertEqual(ready['history_contexts'][1]['count'], 1)
        self.assertEqual(ready['history_edges'][0]['root'], bundle['runs'][0]['edges'][0]['root'])
        # Instantiated producer paths may extend beyond a template's Input.
        self.assertEqual(ready['edge_roots'], [[5, 2]])  # Static ports remain separate.

    def test_wrong_binding_path_cannot_be_replaced_by_a_template_root(self):
        feature, bundle = history_fixture()
        bundle['runs'][0]['edges'][0]['dst_binding_path'] = 'r/0/99'
        with self.assertRaisesRegex(ValueError, 'binding root'):
            attach(feature, bundle)

    def test_duplicate_runs_bad_indices_and_lost_denominators_rejected(self):
        feature, history = history_fixture()
        for mutate in (lambda b: b['runs'].append(deepcopy(b['runs'][0])),
                       lambda b: b['runs'][0]['edges'][0].update(tree=-1),
                       lambda b: b['runs'][0]['contexts'][0].update(attempts=-1),
                       lambda b: b['runs'][0]['denominators'].update(attempts=99)):
            bad = deepcopy(history)
            mutate(bad)
            with self.assertRaises(ValueError):
                attach(deepcopy(feature), bad)


@unittest.skipIf(torch is None, 'PyTorch is optional')
class RuleHistoryModelTest(unittest.TestCase):
    def test_context_moments_distinguish_spread_and_preserve_counts_and_gradients(self):
        from rule_tree_model import context_summary
        # Equal hidden means/counts, different empirical distributions.
        spread = torch.tensor([[-.75, -.25], [.75, .25]], dtype=torch.float64, requires_grad=True)
        flat = torch.zeros_like(spread)
        destinations = torch.tensor([0, 0])
        weights = torch.ones((2, 1), dtype=torch.float64)
        torch.testing.assert_close(context_summary(spread, destinations, weights, 2),
                                   context_summary(flat, destinations, weights, 2))
        first = context_summary(spread, destinations, weights, 2, variance=True)
        second = context_summary(flat, destinations, weights, 2, variance=True)
        self.assertFalse(torch.equal(first, second))
        torch.testing.assert_close(first[0, 2:4], torch.tensor([.5625, .0625], dtype=torch.float64))
        self.assertTrue(torch.equal(first[1], torch.zeros_like(first[1])))  # Unobserved rule.
        # Compression is exact for identical encoded vectors; order is irrelevant.
        weights = torch.tensor([[3.], [2.]], dtype=torch.float64)
        compressed = context_summary(spread, destinations, weights, 2, variance=True)
        expanded = context_summary(spread.repeat_interleave(torch.tensor([3, 2]), dim=0),
                                   torch.zeros(5, dtype=torch.long), torch.ones((5, 1), dtype=torch.float64),
                                   2, variance=True)
        torch.testing.assert_close(compressed, expanded)
        torch.testing.assert_close(torch.autograd.grad(compressed.square().sum(), spread)[0],
                                   torch.autograd.grad(expanded.square().sum(), spread)[0])
        torch.testing.assert_close(compressed, context_summary(spread.flip(0), destinations,
                                                               weights.flip(0), 2, variance=True))
        empty = context_summary(spread[:0], destinations[:0], weights[:0], 2, variance=True)
        self.assertTrue(torch.equal(empty, torch.zeros_like(empty)))
        # Do not imply that two moments uniquely identify a distribution.
        another = torch.tensor([[-1.], [0.], [0.], [1.]], dtype=torch.float64)
        same_moments = torch.tensor([[-2**-.5], [-2**-.5], [2**-.5], [2**-.5]], dtype=torch.float64)
        args = (torch.zeros(4, dtype=torch.long), torch.ones((4, 1), dtype=torch.float64), 1)
        torch.testing.assert_close(context_summary(another, *args, variance=True),
                                   context_summary(same_moments, *args, variance=True))

    def test_variance_pooling_integrates_without_expanding_rule_nodes_or_edges(self):
        from rule_tree_model import TreePolicyPredictor
        threads = torch.get_num_threads()
        torch.set_num_threads(1)
        self.addCleanup(torch.set_num_threads, threads)
        feature, bundle = history_fixture()
        features = attach(feature, bundle)
        vocabulary = fit_vocabulary([features['sequences']])
        features['sequences'] = {k: [encode_sequence(s, vocabulary) for s in seqs]
                                for k, seqs in features['sequences'].items()}
        original = deepcopy(features)
        model = TreePolicyPredictor(len(vocabulary), 8, history=True, history_pooling='mean_variance')
        before = model(features)
        before.sum().backward()
        self.assertEqual(features, original)
        self.assertEqual(model.history_update.in_features, 17)
        self.assertTrue(torch.isfinite(model.history_update.weight.grad).all())
        self.assertGreater(model.history_update.weight.grad[:, 8:16].abs().sum().item(), 0)
        clone = TreePolicyPredictor(len(vocabulary), 8, history=True, history_pooling='mean_variance')
        clone.load_state_dict(model.state_dict())
        torch.testing.assert_close(before, clone(features))
        for count in (0, -1, True, float('nan')):
            bad = deepcopy(features)
            bad['history_contexts'][0]['count'] = count
            with self.assertRaisesRegex(ValueError, 'attempt count'):
                model(bad)

    def test_batched_history_projections_match_scalar_values_and_gradients(self):
        from rule_tree_model import TreePolicyPredictor
        threads = torch.get_num_threads()
        torch.set_num_threads(1)
        self.addCleanup(torch.set_num_threads, threads)
        feature, bundle = history_fixture()
        features = attach(feature, bundle)
        # Repeated edges remain repeated observations, not sampled unique pairs.
        features['history_edges'] *= 3
        features['sequences']['history_edge'] *= 3
        vocabulary = fit_vocabulary([features['sequences']])
        features['sequences'] = {k: [encode_sequence(s, vocabulary) for s in seqs] for k, seqs in features['sequences'].items()}
        torch.manual_seed(973)
        model = TreePolicyPredictor(len(vocabulary), 8, history=True)
        prediction = model(features)
        prediction.sum().backward()
        gradients = {n: p.grad.clone() for n, p in model.named_parameters() if p.grad is not None}
        model.zero_grad()
        attempt, ports = model.history_attempt.forward, model.history_ports.forward
        with patch.object(model.history_attempt, 'forward', side_effect=lambda values: torch.stack([attempt(v) for v in values])), \
             patch.object(model.history_ports, 'forward', side_effect=lambda values: torch.stack([ports(v) for v in values])):
            reference = model(features)
            reference.sum().backward()
        torch.testing.assert_close(prediction, reference)
        for name, parameter in model.named_parameters():
            if name in gradients:
                torch.testing.assert_close(parameter.grad, gradients[name], rtol=2e-5, atol=2e-7)
        # Controls share weights/vocabulary. Each masks only its declared channel.
        for mode in ('none', 'context'):
            model.history_mode = mode
            before = model(features)
            poisoned = deepcopy(features)
            poisoned['history_edges'] = []
            poisoned['sequences']['history_edge'] = []
            if mode == 'none':
                poisoned['history_contexts'] = []
                poisoned['history_trees'] = []
            torch.testing.assert_close(before, model(poisoned), rtol=0, atol=0)

    def test_history_tree_policy_and_actual_roots_train_through_all_rounds(self):
        from rule_tree_model import TreePolicyPredictor
        threads = torch.get_num_threads()
        torch.set_num_threads(1)
        self.addCleanup(torch.set_num_threads, threads)
        torch.manual_seed(963)
        feature, bundle = history_fixture()
        features = attach(feature, bundle)
        vocabulary = fit_vocabulary([features['sequences']])
        features['sequences'] = {k: [encode_sequence(s, vocabulary) for s in seqs] for k, seqs in features['sequences'].items()}
        model = TreePolicyPredictor(len(vocabulary), 8, history=True)
        before = model(features)
        for kind in ('root', 'tree', 'behavior_policy', 'query_statistics', 'attempt_count'):
            changed = deepcopy(features)
            if kind == 'root':
                changed['history_edges'][0]['root'] = changed['history_trees'][0]['root']
            elif kind in ('tree', 'behavior_policy', 'query_statistics'):
                channel = {'tree': 'history_node', 'behavior_policy': 'history_policy', 'query_statistics': 'history_relation'}[kind]
                for seq in changed['sequences'][channel]:
                    seq['numbers'] = [v + 10 if known else v for v, known in zip(seq['numbers'], seq['known_number'])]
            else:
                changed['history_contexts'][0]['count'] += 10
            self.assertFalse(torch.equal(before, model(changed)), kind)
        changed = deepcopy(features)
        changed['response'] = {'execution_ms': 1e20}
        changed['current_memo'] = {'rows': 1e20}
        torch.testing.assert_close(before, model(changed), rtol=0, atol=0)
        optimizer = torch.optim.Adam(model.parameters())
        before.square().sum().backward()
        for name, parameter in model.named_parameters():
            if name.startswith(('history_', 'rounds.')):
                self.assertIsNotNone(parameter.grad, name)
                self.assertTrue(torch.isfinite(parameter.grad).all(), name)
        optimizer.step()
        self.assertFalse(torch.equal(before, model(features)))
        with self.assertRaisesRegex(ValueError, 'explicitly enable'):
            TreePolicyPredictor(len(vocabulary), 8)(features)


if __name__ == '__main__':
    unittest.main()
