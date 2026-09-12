#!/usr/bin/env python3
"""Shared CBO graph ablations across catalogs under one audited execution configuration."""

import argparse
from collections import Counter
import json
import math
from pathlib import Path
import random
import re
from statistics import median
import time

from export_policy_learning_samples import export_comparison
from export_policy_learning_samples import read_snapshot
from rule_policy_encoding import input_sequences, encode_sequence, fit_vocabulary
from run_workload_comparison import artifact_snapshot


def baseline_exclusions(record):
    errors = list(record['admission']['feature_exclusions'])
    if not record['admission']['feature_integrity_verified']:
        errors.append('input_integrity_unverified')
    if record['inputs']['stats_experiment_document'] is not None:
        errors.append('intervention_channel_not_encoded')
    if any(r.get('placement') != 'cbo' for r in record['inputs']['candidate_policy'] or []):
        errors.append('outside_fixed_cbo_action_domain')
    if record['response']['status'] != 'complete':
        errors.extend(record['response']['exclusions'])
        errors.append('incomplete_response')
    for key in ('planning_ms_median', 'execution_ms_median'):
        value = record['response'][key]
        if type(value) not in (int, float) or not math.isfinite(value) or value < 0:
            errors.append('invalid_response_' + key)
    return sorted(set(errors))


def measurement_environment(context, comparison, arm):
    """Catalog contents are encoded per query; unencoded execution settings must agree.

    Names, OIDs, capture timestamps and schema sizes are not environment IDs.
    Policy paths are replaced only because their native resolved contents are
    already model inputs; every other generated runtime statement is retained.
    """
    artifacts = comparison['artifact_provenance']['before_server_start']
    binaries = {}
    for key in ('postgres', 'pg_orca', 'rule_audit', 'rules', 'runner'):
        snapshot = artifacts[key]
        if (type(snapshot.get('size')) is not int or snapshot['size'] < 1
                or not re.fullmatch(r'[0-9a-f]{8}', snapshot.get('crc32', ''))):
            raise ValueError('invalid measurement environment artifact: ' + key)
        binaries[key] = {k: snapshot[k] for k in ('size', 'crc32')}
    settings = context['catalog']['settings']
    runtime = context['settings_sql'][arm]
    if not isinstance(settings, dict) or not settings or not isinstance(runtime, str) or not runtime:
        raise ValueError('missing captured measurement settings')
    runtime, replaced = re.subn(r"(?m)^SET pg_orca\.dsl_rule_policy_path='(?:''|[^'])*';$",
                               "SET pg_orca.dsl_rule_policy_path='<encoded_policy>';", runtime)
    if replaced != 1:
        raise ValueError('expected one explicitly resolved policy setting')
    return {'artifacts': binaries, 'catalog_settings': settings, 'runtime_settings_sql': runtime,
            'scope': 'captured_software_and_settings_only_not_hardware_or_load_equivalence'}


def selection_metrics(predictions, records, split):
    """Score one fixed policy for the whole split, never choose using response labels.

    Descriptive equal-query pilot only: failed/missing arms exclude the whole
    query from paired scoring, not just the inconvenient policy.
    """
    def key(unit):
        return json.dumps(unit, sort_keys=True)

    predicted = {}
    for item in predictions:
        if item['split'] == split:
            identity = key(item['unit'])
            if identity in predicted:
                raise ValueError('duplicate policy prediction')
            predicted[identity] = item
    groups, seen = {}, set()
    for record in records:
        if record['pilot']['split'] != split:
            continue
        unit = record['unit']
        identity = key(unit)
        if identity in seen:
            raise ValueError('duplicate policy response')
        seen.add(identity)
        group = key({k: v for k, v in unit.items() if k != 'policy'})
        groups.setdefault(group, []).append(record)
    if predicted.keys() - seen:
        raise ValueError('prediction without assigned response')
    totals, rows, excluded = {}, [], []
    channels = {'model': 'predicted_log1p_ms', 'training_policy_constant': 'training_policy_constant'}
    for group, cells in groups.items():
        if (len(cells) < 2 or any(r['pilot']['exclusions'] or key(r['unit']) not in predicted
                                or r['response']['status'] != 'complete' for r in cells)):
            excluded.append({'unit': json.loads(group), 'reason': 'incomplete_policy_comparison'})
            continue
        arms = {r['unit']['policy'] for r in cells}
        if totals and arms != set(totals):
            raise ValueError('workload selection requires identical candidate policy sets')
        costs = {}
        for record in cells:
            arm = record['unit']['policy']
            prediction = predicted[key(record['unit'])]
            samples = [s for s in record['response']['timing_samples'] if s['phase'] == 'measurement']
            if not samples or any(s['status'] != 'ok' or s['comparison_exclusions']
                    or s.get('optimizer') != 'pg_orca'
                    or any(type(s.get(k)) not in (int, float) or not math.isfinite(s[k]) or s[k] < 0
                           for k in ('planning_ms', 'execution_ms')) for s in samples):
                raise ValueError('complete response has invalid measured timings')
            costs[arm] = {'observed_ms': median(s['planning_ms'] + s['execution_ms'] for s in samples)}
            for channel, field in channels.items():
                values = prediction[field]
                if len(values) != 2 or any(type(v) not in (int, float) or not math.isfinite(v) for v in values):
                    raise ValueError('invalid time prediction')
                # Nonnegative time domain; a sum of predicted marginal medians
                # is only a proxy for the median of joint planning+execution.
                costs[arm][channel + '_ms'] = sum(math.expm1(max(0., v)) for v in values)
            total = totals.setdefault(arm, {k: 0. for k in costs[arm]})
            for channel, value in costs[arm].items():
                total[channel] += value
                if not math.isfinite(total[channel]):
                    raise ValueError('nonfinite policy total')
        rows.append({'unit': json.loads(group), 'policies': costs})
    result = {'assigned_queries': len(groups), 'paired_queries': len(rows), 'excluded': excluded,
              'objective': 'equal_query_sum_of_median_plan_plus_execution_ms',
              'scope': 'descriptive_complete_pairs_not_population_or_safety_certificate',
              'includes_inference_or_maintenance_cost': False, 'queries': rows, 'policy_totals': totals}
    if totals:
        oracle = min(totals, key=lambda arm: (totals[arm]['observed_ms'], arm))
        result['best_measured_fixed_policy'] = oracle
        result['selectors'] = {}
        for channel in channels:
            chosen = min(totals, key=lambda arm: (totals[arm][channel + '_ms'], arm))
            result['selectors'][channel] = {'policy': chosen,
                'observed_ms': totals[chosen]['observed_ms'],
                'empirical_regret_ms': totals[chosen]['observed_ms'] - totals[oracle]['observed_ms']}
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, required=True, help='pre-collection family assignments')
    parser.add_argument('--run', type=Path, required=True, help='comparison collection with a common audited execution configuration')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--epochs', type=int, default=100)
    parser.add_argument('--seed', type=int, default=929)
    parser.add_argument('--width', type=int, default=32)
    parser.add_argument('--graph-mode', choices=('none', 'static', 'self'), default='static')
    parser.add_argument('--rule-encoding', choices=('tree', 'sequence'), default='tree',
                        help='tree is the primary model; sequence reproduces the earlier ablation only')
    parser.add_argument('--message-rounds', type=int, help='tree default 3; legacy sequence supports 1')
    parser.add_argument('--static-graph', type=Path, help='native template-only graph export; also aligns vocabulary in K=0')
    parser.add_argument('--history', type=Path, help='audited history frozen before collection; training query population only')
    parser.add_argument('--history-mode', choices=('none', 'context', 'graph'), default='graph',
                        help='with --history, same vocabulary/initialization; none masks history, context masks dynamic edges')
    parser.add_argument('--history-pooling', choices=('mean', 'mean_variance'), default='mean',
                        help='attempt-weighted encoded context moments; mean preserves the original baseline')
    parser.add_argument('--evaluate-test', action='store_true', help='only after freezing the model; default evaluates development splits only')
    args = parser.parse_args()
    if args.epochs < 1 or args.width < 1:
        parser.error('positive fixed epoch budget and hidden width required')
    if args.graph_mode != 'none' and args.static_graph is None:
        parser.error('graph ablations require --static-graph')
    if args.rule_encoding == 'tree' and args.static_graph is None:
        parser.error('tree encoding requires a verified --static-graph for endpoint binding')
    if args.history is not None and args.rule_encoding != 'tree':
        parser.error('historical actual trees require --rule-encoding tree')
    if args.history_pooling != 'mean' and (args.history is None or args.history_mode == 'none'):
        parser.error('context pooling requires enabled --history')
    if args.message_rounds is None:
        args.message_rounds = 3 if args.rule_encoding == 'tree' else 1
    if args.message_rounds < 1 or (args.rule_encoding == 'sequence' and args.message_rounds != 1):
        parser.error('positive message rounds required; legacy sequence ablation supports one round')
    manifest = json.loads(args.manifest.read_bytes())
    families, units, records, examples = {}, set(), [], []
    catalogs, environments, assets = set(), set(), {'manifest': args.manifest, 'trainer': Path(__file__)}
    history = None
    if args.history is not None:
        from rule_history_encoding import attach_history
        assets['history'] = args.history
        history_snapshot = artifact_snapshot({'history': args.history})['history']
        history = json.loads(read_snapshot(history_snapshot))
        for i, run in enumerate(history['runs']):
            assets['history_source:' + str(i)] = Path(run['source']['path'])
            for key in ('catalog_snapshot', 'graph_snapshot'):
                assets['history_' + key + ':' + str(i)] = Path(run['inputs'][key]['path'])
    allowed_history = {a['case_id']: a['query'] for a in manifest['queries'] if a['split'] == 'train'}
    static_snapshot = None
    if args.static_graph is not None:
        assets['static_graph'] = args.static_graph
        static_snapshot = artifact_snapshot({'static_graph': args.static_graph})['static_graph']
    for assignment in manifest['queries']:
        family, split = assignment['family'], assignment['split']
        if split not in ('train', 'validation', 'test') or families.get(family, split) != split:
            raise ValueError('invalid or leaking family partition')
        families[family] = split
        workload, query = assignment['case_id'].split(':')
        if (workload, query) in units:
            raise ValueError('duplicate assigned query')
        units.add((workload, query))
        path = args.run / workload / query / 'comparison.json'
        assets['comparison:' + assignment['case_id']] = path
        comparison = json.loads(path.read_bytes())
        for record in export_comparison(path):
            if record['inputs']['query_sql'] != assignment['query']:
                raise ValueError('query changed since split assignment; require a new revision')
            record['pilot'] = {'split': split, 'family': family, 'exclusions': baseline_exclusions(record)}
            records.append(record)
            snapshot = record['inputs']['catalog_snapshot'] or {}
            catalogs.add((snapshot.get('size'), snapshot.get('crc32')))
            if record['pilot']['exclusions']:
                continue
            context = json.loads(read_snapshot(record['inputs']['catalog_snapshot']))
            environment = measurement_environment(context, comparison, record['unit']['policy'])
            environments.add(json.dumps(environment, sort_keys=True))
            feature = input_sequences(record['inputs'], static_snapshot, tree_rules=args.rule_encoding == 'tree')
            if not feature['query_binding']['complete']:
                record['pilot']['exclusions'].append('query_binding_unavailable')
                continue
            if history is not None:
                catalog = json.loads(read_snapshot(record['inputs']['catalog_snapshot']))
                attach_history(feature, history, static_snapshot, catalog['catalog']['captured_at'], allowed_history)
            # Legacy/static controls share native vocabulary. Runtime evidence
            # enters only through the explicit time/population-audited history.
            rule_channels = ('rule_node', 'rule_symbol', 'rule_constraint') if args.rule_encoding == 'tree' else ('rule',)
            feature['sequences'] = {key: value for key, value in feature['sequences'].items()
                                    if key in (*rule_channels, 'policy', 'query', 'relation')
                                    or (key == 'edge' and static_snapshot is not None)
                                    or (history is not None and key.startswith('history_'))}
            examples.append((record, feature))
    if len(environments) != 1:
        raise ValueError('unencoded software/runtime settings differ; cannot pool these measurements')
    required_splits = ('train', 'validation', 'test') if args.evaluate_test else ('train', 'validation')
    if not all(any(r['pilot']['split'] == split for r, _ in examples) for split in required_splits):
        raise ValueError('not enough usable split coverage; do not reassign failed units')
    vocabulary = fit_vocabulary([f['sequences'] for r, f in examples if r['pilot']['split'] == 'train'])
    for record, feature in examples:
        feature['sequences'] = {key: [encode_sequence(s, vocabulary) for s in value]
                                for key, value in feature['sequences'].items()}
    args.output.mkdir(parents=True, exist_ok=False)
    with (args.output / 'samples.jsonl').open('x') as stream:
        for record in records:
            stream.write(json.dumps(record, allow_nan=False) + '\n')
    with (args.output / 'vocabulary.json').open('x') as stream:
        json.dump(vocabulary, stream)
    for name in ('rule_policy_model.py', 'rule_tree_model.py', 'rule_policy_encoding.py', 'query_policy_encoding.py',
                 'rule_history_encoding.py', 'group_expression_encoding.py', 'profile_rule_candidates.py'):
        assets[name] = Path(__file__).with_name(name)
    before_fit = artifact_snapshot(assets)
    if static_snapshot is not None and before_fit['static_graph'] != static_snapshot:
        raise ValueError('static graph changed during input preparation')
    if history is not None and before_fit['history'] != history_snapshot:
        raise ValueError('history changed during input preparation')
    import torch
    from rule_policy_model import WholePolicyPredictor
    from rule_tree_model import TreePolicyPredictor
    torch.set_num_threads(1)
    torch.manual_seed(args.seed)
    model = (TreePolicyPredictor(len(vocabulary), args.width, args.graph_mode, args.message_rounds,
                                history=history is not None, history_mode=args.history_mode,
                                history_pooling=args.history_pooling)
             if args.rule_encoding == 'tree' else WholePolicyPredictor(len(vocabulary), args.width, args.graph_mode))
    optimizer = torch.optim.Adam(model.parameters(), lr=.003)
    train = [(r, f) for r, f in examples if r['pilot']['split'] == 'train']
    validation = [(r, f) for r, f in examples if r['pilot']['split'] == 'validation']

    def target(record):
        return [math.log1p(record['response'][key]) for key in ('planning_ms_median', 'execution_ms_median')]

    losses, curve = [], []
    best_validation, best_epoch = math.inf, None
    fit_wall, fit_cpu = time.perf_counter(), time.process_time()
    for epoch in range(args.epochs):
        model.train()
        order = list(range(len(train)))
        random.Random(args.seed + epoch).shuffle(order)
        total = 0.
        for index in order:
            record, feature = train[index]
            optimizer.zero_grad()
            loss = torch.nn.functional.smooth_l1_loss(model(feature), torch.tensor(target(record)))
            if not torch.isfinite(loss):
                raise ValueError('nonfinite training loss')
            loss.backward()
            torch.nn.utils.clip_grad_norm_(model.parameters(), 5., error_if_nonfinite=True)
            optimizer.step()
            total += loss.item()
        losses.append(total / len(train))
        model.eval()
        with torch.inference_mode():
            validation_loss = sum(torch.nn.functional.smooth_l1_loss(
                model(feature), torch.tensor(target(record))).item()
                for record, feature in validation) / len(validation)
        if not math.isfinite(validation_loss):
            raise ValueError('nonfinite validation loss')
        improved = validation_loss < best_validation
        if improved:
            best_validation, best_epoch = validation_loss, epoch + 1
            torch.save(model.state_dict(), args.output / 'best-validation.pt')
        row = {'epoch': epoch + 1, 'training_online_smooth_l1': losses[-1],
               'validation_smooth_l1': validation_loss, 'checkpoint_improved': improved,
               'fit_wall_seconds': time.perf_counter() - fit_wall}
        curve.append(row)
        with (args.output / 'learning-curve.jsonl').open('a') as stream:
            stream.write(json.dumps(row, allow_nan=False) + '\n')
        print(json.dumps(row), flush=True)
    # Test responses never participate in epoch selection. Preserve the final
    # epoch too so continued fitting is not confused with the chosen checkpoint.
    torch.save(model.state_dict(), args.output / 'last-epoch.pt')
    model.load_state_dict(torch.load(args.output / 'best-validation.pt', weights_only=True))
    fit_seconds = {'wall': time.perf_counter() - fit_wall, 'cpu': time.process_time() - fit_cpu}
    constants = {}
    for policy in {r['unit']['policy'] for r, _ in train}:
        labels = [target(r) for r, _ in train if r['unit']['policy'] == policy]
        constants[policy] = [sum(y[j] for y in labels) / len(labels) for j in range(2)]
    predictions, metrics, inference_seconds = [], {}, []
    model.eval()
    with torch.inference_mode():
        for record, feature in examples:
            if record['pilot']['split'] == 'test' and not args.evaluate_test:
                continue
            start = time.perf_counter()
            predicted = model(feature).tolist()
            inference_seconds.append(time.perf_counter() - start)
            if not all(math.isfinite(v) for v in predicted):
                raise ValueError('nonfinite prediction')
            predictions.append({'unit': record['unit'], 'split': record['pilot']['split'],
                'observed_log1p_ms': target(record), 'predicted_log1p_ms': predicted,
                'training_policy_constant': constants[record['unit']['policy']],
                'unknown_tokens': sum(s['unknown_tokens'] for seqs in feature['sequences'].values() for s in seqs)})
    for split in ('train', 'validation', 'test'):
        subset = [p for p in predictions if p['split'] == split]
        if not subset:
            metrics[split] = {'evaluated': False}
            continue
        metrics[split] = {'records': len(subset), 'unknown_tokens': sum(p['unknown_tokens'] for p in subset)}
        for key in ('predicted_log1p_ms', 'training_policy_constant'):
            metrics[split][key + '_mae'] = [sum(abs(p[key][j] - p['observed_log1p_ms'][j])
                                                 for p in subset) / len(subset) for j in range(2)]
    after_fit = artifact_snapshot(assets)
    if after_fit != before_fit:
        raise ValueError('input or code changed during model fitting')
    report = {'scope': 'fixed_cbo_graph_ablation_completed_response_pilot_not_generalization_or_dro_certificate',
        'model_trained': True, 'epochs': args.epochs, 'width': args.width, 'seed': args.seed,
        'catalog_snapshots': len(catalogs), 'measurement_environment': json.loads(next(iter(environments))),
        'architecture': ('rooted_tree_v1' if args.rule_encoding == 'tree' else 'mean_count_sequence_v3'),
        'graph_mode': args.graph_mode, 'rule_encoding': args.rule_encoding,
        'message_rounds': args.message_rounds if args.graph_mode != 'none' else 0,
        'history_enabled': history is not None,
        'history_mode': args.history_mode if history is not None else 'none',
        'history_pooling': args.history_pooling if history is not None and args.history_mode != 'none' else 'none',
        'history_admission': None if history is None else {'frozen_at_utc': history['frozen_at_utc'],
            'scope': 'training_queries_only_before_collection',
            'runs': [r['denominators'] for r in history['runs']]},
        'parameter_count': sum(p.numel() for p in model.parameters()), 'fit_seconds': fit_seconds,
        'inference_wall_seconds': inference_seconds, 'inference_includes_input_preparation': False,
        'test_evaluated': args.evaluate_test,
        'validation_used_for_model_selection': True, 'selected_epoch': best_epoch,
        'checkpoint_objective': 'validation_mean_smooth_l1_log1p_plan_execution',
        'learning_curve': curve, 'records': len(records),
        'exclusions': dict(Counter(e for r in records for e in r['pilot']['exclusions'])),
        'response_statuses': dict(Counter(r['response']['status'] for r in records)),
        'global_training_admission_overridden': False, 'training_losses': losses,
        'metrics': metrics, 'predictions': predictions, 'input_snapshots': before_fit,
        'selection': {split: selection_metrics(predictions, records, split)
                      for split in ('train', 'validation', 'test')
                      if split != 'test' or args.evaluate_test},
        'fit_artifact_endpoints_equal': after_fit == before_fit,
        'not_claimed': ['template_or_application_independence', 'rbo_or_intervention_generalization',
                        'policy_recommendation_safety', 'gnn_incremental_value', 'causal_rule_effect']}
    with (args.output / 'report.json').open('x') as stream:
        json.dump(report, stream, indent=2, allow_nan=False)
    torch.save(model.state_dict(), args.output / 'model.pt')
    print(json.dumps({'records': len(records), 'metrics': metrics, 'exclusions': report['exclusions']}))


if __name__ == '__main__':
    main()
