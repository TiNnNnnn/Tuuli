"""Fixed-checkpoint CPU profiling on complete existing query graphs; never executes SQL."""
import argparse
import cProfile
from copy import deepcopy
import gzip
import importlib.util
import json
from pathlib import Path
import pstats
from statistics import median
import time

from tools.learnable.common.artifacts import read_snapshot, artifact_snapshot
from tools.learnable.encoding.observed_search import observed_features
from tools.learnable.encoding.query_policy_encoding import query_context
from tools.learnable.encoding.rule_policy_encoding import input_sequences, encode_sequence


def load_sample(manifest, vocabulary, case_id):
    item = next(i for i in manifest['queries'] if i['case']['case_id'] == case_id)
    graph = json.loads(gzip.decompress(read_snapshot(item['graph_snapshot'])))
    if graph['case'] != item['case']:
        raise ValueError('graph/query identity mismatch')
    snapshot = graph['source_files']['context']
    catalog = json.loads(read_snapshot(snapshot))
    if catalog['status'] != 'ok' or not catalog['capture_input_endpoints_equal']:
        raise ValueError('invalid catalog capture')
    static = manifest['input_snapshots']['graph']
    native = json.loads(read_snapshot(static))
    base = input_sequences({'query_sql': item['case']['query'], 'graph_snapshot': static,
        'catalog_snapshot': snapshot,
        'candidate_policy': catalog['resolved_policies']['behavior']['snapshot']['rules']}, static, tree_rules=True)
    features = observed_features(base, graph, query_context(item['case']['query'], catalog['catalog']),
                                {n['rule_hash']: i for i, n in enumerate(native['nodes'])})
    features['sequences'] = {k: [encode_sequence(s, vocabulary) for s in rows]
                             for k, rows in features['sequences'].items()}
    return features, item['target_log1p_ms']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--checkpoint', type=Path, required=True)
    parser.add_argument('--case', action='append', required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--threads', type=int, default=1)
    parser.add_argument('--repeats', type=int, default=3)
    parser.add_argument('--profile', action='store_true', help='additional instrumented step, excluded from timings')
    parser.add_argument('--reference-source', type=Path, help='trusted archived rule_tree_model.py to compare')
    args = parser.parse_args()
    if args.threads < 1 or args.repeats < 1:
        parser.error('positive threads and repeats required')
    if args.output.exists():
        parser.error('output already exists')
    import torch
    from tools.learnable.models.rule_tree_model import ObservedSearchPredictor
    if args.reference_source:
        spec = importlib.util.spec_from_file_location('reference_tree_model', args.reference_source)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        ObservedSearchPredictor = module.ObservedSearchPredictor
    torch.set_num_threads(args.threads)
    artifacts = artifact_snapshot({'manifest': args.manifest, 'checkpoint': args.checkpoint,
        'vocabulary': args.manifest.parent / 'vocabulary.json'})
    manifest = json.loads(read_snapshot(artifacts['manifest']))
    vocabulary = json.loads(read_snapshot(artifacts['vocabulary']))
    checkpoint = torch.load(args.checkpoint, weights_only=True, map_location='cpu')
    model = ObservedSearchPredictor(len(vocabulary), manifest.get('width', 32), 'static', 3,
                                   history=True, history_mode=manifest['mode'])
    optimizer = torch.optim.Adam(model.parameters(), lr=.003)
    report = {'scope': 'fixed_checkpoint_complete_graph_benchmark_not_training', 'threads': args.threads,
              'checkpoint_step': checkpoint['steps'], 'artifacts': artifacts, 'cases': []}
    for case_id in args.case:
        start = time.perf_counter()
        features, labels = load_sample(manifest, vocabulary, case_id)
        target = torch.tensor(labels)
        row = {'case_id': case_id, 'load_seconds': time.perf_counter() - start,
               'rules': sum(features['loaded_rules']), 'trees': len(features['history_trees']),
               'nodes': sum(len(t['nodes']) for t in features['history_trees']),
               'contexts': len(features['history_contexts']), 'edges': len(features['history_edges']), 'steps': []}
        for repeat in range(-1, args.repeats + int(args.profile)):
            model.load_state_dict(checkpoint['model'], strict=True)
            # Adam's load_state_dict may share CPU tensors; clone before a benchmark update.
            optimizer.load_state_dict(deepcopy(checkpoint['optimizer']))
            optimizer.zero_grad()
            profile = cProfile.Profile() if args.profile and repeat == args.repeats else None
            if profile:
                profile.enable()
            start = time.perf_counter()
            prediction = model(features)
            loss = torch.nn.functional.smooth_l1_loss(prediction, target)
            forward = time.perf_counter()
            loss.backward()
            backward = time.perf_counter()
            torch.nn.utils.clip_grad_norm_(model.parameters(), 5., error_if_nonfinite=True)
            optimizer.step()
            end = time.perf_counter()
            if profile:
                profile.disable()
                stats = pstats.Stats(profile).stats
                row['profile'] = [{'file': file, 'line': line, 'function': name, 'calls': s[1],
                                   'self_seconds': s[2], 'inclusive_seconds': s[3]}
                                  for (file, line, name), s in sorted(stats.items(), key=lambda p: p[1][3], reverse=True)[:60]]
            elif repeat >= 0:
                row['steps'].append({'forward_seconds': forward - start, 'backward_seconds': backward - forward,
                                     'update_seconds': end - backward, 'seconds': end - start,
                                     'loss': loss.item(), 'prediction': prediction.detach().tolist()})
            del loss, prediction
        row['median_seconds'] = median(s['seconds'] for s in row['steps'])
        report['cases'].append(row)
        print(json.dumps({k: v for k, v in row.items() if k not in ('steps', 'profile')}), flush=True)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open('x') as stream:
        json.dump(report, stream, indent=2, allow_nan=False)


if __name__ == '__main__':
    main()
