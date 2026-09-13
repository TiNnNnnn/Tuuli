from copy import deepcopy
import math
from pathlib import Path
import tempfile
import unittest

try:
    import torch
except ModuleNotFoundError as error:
    if error.name != 'torch':
        raise
    raise unittest.SkipTest('install tools/learnable/requirements-training.txt for network tests')

from tools.learnable.encoding.rule_policy_encoding import encode_sequence, fit_vocabulary
from tools.learnable.tests.test_rule_history_encoding import history_fixture
from tools.learnable.training.train_observed_search import (ObservedSearchPredictor, internal_split, observed_features,
                                   observed_target, restore_checkpoint, training_contract, validate_resume_contract)


class ObservedSearchTest(unittest.TestCase):
    def test_resume_rejects_other_network_or_target(self):
        contract = training_contract(32, 'graph')
        validate_resume_contract({'training_contract': contract}, 32, 'graph')
        validate_resume_contract({'width': 32}, 32, 'graph')  # Legacy checkpoint.
        for bad in ({'training_contract': training_contract(16, 'graph')},
                    {'training_contract': training_contract(32, 'context')},
                    {'training_contract': contract | {'objective': {'name': 'policy_value'}}},
                    {'width': 16}):
            with self.assertRaises(ValueError):
                validate_resume_contract(bad, 32, 'graph')

    def test_resume_preserves_optimizer_and_does_not_invent_old_loss(self):
        torch.manual_seed(929)
        model = torch.nn.Linear(2, 1)
        optimizer = torch.optim.Adam(model.parameters(), lr=.003)
        def update(net, opt):
            opt.zero_grad()
            net(torch.tensor([[1., 2.]])).square().mean().backward()
            opt.step()
        update(model, optimizer)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'latest.pt'
            checkpoint = {'model': model.state_dict(), 'optimizer': optimizer.state_dict(), 'steps': 1, 'epoch': 1}
            torch.save(checkpoint, path)
            clone = torch.nn.Linear(2, 1)
            other = torch.optim.Adam(clone.parameters(), lr=.1)
            self.assertEqual(restore_checkpoint(path, clone, other, 3), (1, 0, 1, 0., 0))
            update(model, optimizer)
            update(clone, other)
            for actual, expected in zip(model.parameters(), clone.parameters()):
                torch.testing.assert_close(actual, expected, rtol=0, atol=0)
            checkpoint.update(steps=3, epoch_loss_sum=2., epoch_loss_count=3)
            torch.save(checkpoint, path)
            self.assertEqual(restore_checkpoint(path, clone, other, 3), (3, 0, 3, 2., 3))
            for bad in ({'steps': -1}, {'epoch': 2}, {'epoch_loss_count': 4}):
                torch.save(checkpoint | bad, path)
                with self.assertRaises(ValueError):
                    restore_checkpoint(path, clone, other, 3)

    def test_targets_are_existing_microseconds_and_never_imputed(self):
        summary = {'complete': True, 'returncode': 0, 'attempts': 2,
                   'groups': [{'attempts': 2, 'match_us': 2000, 'constraint_us': 3000,
                               'instantiate_us': 7000}]}
        self.assertEqual(observed_target(summary), [math.log1p(5), math.log1p(7)])
        for patch in ({'complete': False}, {'groups': None}, {'attempts': 3}):
            with self.assertRaises(ValueError):
                observed_target(summary | patch)
        summary['groups'][0]['match_us'] = float('nan')
        with self.assertRaises(ValueError):
            observed_target(summary)
        self.assertEqual(internal_split('same_family', 929), internal_split('same_family', 929))

    def test_real_tree_gnn_updates_without_sql_and_excludes_outcome_tokens(self):
        base, history = history_fixture()
        graph = deepcopy(history['runs'][0])
        graph['trees'] = graph['trees'][:1]
        graph['contexts'] = graph['contexts'][:1]
        binding = {**base['query_binding'], 'sequence': base['sequences']['query'][0]}
        feature = observed_features(base, graph, binding, {'0': 0, '1': 1})
        other = deepcopy(graph)
        other['edges'][0]['producer_outcome'] = 'different_outcome'
        self.assertEqual(feature, observed_features(base, other, binding, {'0': 0, '1': 1}))
        self.assertNotIn('history_admission', feature)
        vocab = fit_vocabulary([feature['sequences']])
        feature['sequences'] = {k: [encode_sequence(s, vocab) for s in v]
                                for k, v in feature['sequences'].items()}
        torch.manual_seed(929)
        torch.set_num_threads(1)
        model = ObservedSearchPredictor(len(vocab), 4, 'static', 3, history=True)
        optimizer = torch.optim.Adam(model.parameters())
        before = model.readout[-1].weight.detach().clone()
        predicted = model(feature)
        self.assertEqual(predicted.shape, (2,))
        loss = (predicted - torch.tensor([1., 2.])).square().mean()
        loss.backward()
        self.assertTrue(all(torch.isfinite(p.grad).all() for p in model.parameters() if p.grad is not None))
        optimizer.step()
        self.assertFalse(torch.equal(before, model.readout[-1].weight))
        with self.assertRaisesRegex(ValueError, 'retrospective'):
            model({**feature, 'observation_scope': 'prospective'})


if __name__ == '__main__':
    unittest.main()
