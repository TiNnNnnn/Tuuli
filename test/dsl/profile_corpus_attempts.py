#!/usr/bin/env python3
"""Sample imported SQL and observe all dispatched DSL attempts, not just winners."""

import argparse
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
import gzip
import json
import os
from pathlib import Path
import random
import subprocess
import tempfile
import zlib

from plot_stats_sweep import chinese_plotting
from profile_rule_candidates import candidate_evidence, STAGES
from run_dphyper_stability import imported_cases
from run_trace_corpus import render_trace_query, orca_fallback_reason
from run_workload_comparison import dsl_observability, inventory, trace_records

SCRIPT_DIR = Path(__file__).resolve().parent


def select_cases(root, count, seed, offset=0):
    datasets = []
    for path in sorted(root.glob('*/cases.sql')):
        cases = imported_cases(path.parent.name, path)
        rng = random.Random(f'{seed}:{path.parent.name}')
        rng.shuffle(cases)
        datasets.append({'dataset': path.parent.name, 'population': len(cases),
                         'schema_crc32': f'{zlib.crc32((path.parent / "schema.sql").read_bytes()):08x}',
                         'cases': [{'case_id': key, 'query': sql,
                                    'query_crc32': f'{zlib.crc32(sql.encode()):08x}'}
                                   for key, sql in (cases[offset:offset + count] if count else cases[offset:])]})
    return datasets


def coverage(runs):
    """Coverage is observed evidence, never filtered by final-plan provenance."""
    rules, ready, edges = set(), set(), set()
    statuses = Counter()
    for run in runs:
        rules.update(run.get('attempted_rules', []))
        ready.update(run.get('ready_rules', []))
        statuses.update(run.get('statuses', {}))
        for edge in run.get('edges', []):
            edges.add(tuple(edge.get(k) for k in ('src_rule', 'dst_rule', 'src_target_path', 'dst_source_path',
                                                'dst_binding_path', 'relation')))
    return {'queries': len(runs), 'attempts': sum(r.get('attempts', 0) for r in runs),
            'attempted_rules': sorted(rules), 'ready_rules': sorted(ready),
            'statuses': dict(statuses), 'unique_consumption_edges': sorted(edges, key=str),
            'edge_events': sum(len(r.get('edges', [])) for r in runs)}


def export_workload(items, corpus, output, schema_domain='wetune'):
    """Export unchanged SQL with either the aligned or original migrated DDL domain."""
    if schema_domain not in {'wetune', 'base'}:
        raise ValueError('unknown schema domain')
    for item in items:
        directory = output / item['dataset']
        (directory / 'sql').mkdir(parents=True, exist_ok=False)
        schema = (corpus / item['dataset'] / 'schema.sql').read_bytes()
        if schema_domain == 'base':
            # This exact boundary is emitted by import_wetune_workloads. Keep
            # original DDL constraints; only the appended WeTune patch is omitted.
            schema = schema.split(b'\n-- WeTune schema patches\n', 1)[0]
        (directory / 'schema.sql').write_bytes(schema)
        item['exported_schema_crc32'] = f'{zlib.crc32(schema):08x}'
        for case in item['cases']:
            (directory / 'sql' / (case['case_id'].split(':')[1] + '.sql')).write_text(case['query'])


def summarize_trace(text, rc, fallback):
    records = trace_records(text)
    events = [r for r in records if r['kind'] in {'rule_candidate', 'rule_candidate_outcome'}]
    # This is a planning-only audit. No row-equivalence or runtime claim is made.
    run = {'plan_rc': rc, 'rows_rc': 0, 'error': text[-4000:], 'fallback': bool(fallback),
           'optimizer': 'postgres' if fallback else 'pg_orca', 'candidate_events': events,
           'experiment_outcomes': [r for r in records if r['kind'] == 'experiment_outcome'],
           'dsl_observability': dsl_observability(records)}
    audit = candidate_evidence(run)
    rows = audit['rows']
    attempted = sorted({r['rule_hash'] for r in rows if r.get('evaluated') is True})
    ready = sorted({r['rule_hash'] for r in rows if r['status'] == 'ready_cbo'})
    return {'complete': audit['complete'], 'exclusions': audit['exclusions'],
            'returncode': rc, 'fallback': fallback, 'attempts': len(rows),
            'evaluations': audit['evaluations'], 'attempted_rules': attempted,
            'ready_rules': ready, 'statuses': dict(Counter(r['status'] for r in rows)),
            'groups': audit['groups'],
            'memo_outcomes': dict(Counter(r['memo_outcome']['status'] for r in rows if r['memo_outcome'])),
            'edges': [r for r in records if r['kind'] == 'rule_edge'],
            'memo': run['dsl_observability']['memo']}


def run_dataset(args, item, index, disabled):
    output = args.output / item['dataset']
    output.mkdir()
    if not item['cases']:
        return {'dataset': item['dataset'], 'runner_returncode': None, 'runs': []}
    with tempfile.TemporaryDirectory(prefix='pgorca-profile-sql.') as temp:
        queries = Path(temp)
        for case in item['cases']:
            name = case['case_id'].split(':')[1]
            observation = str((SCRIPT_DIR / 'rules/stats_input_observation.yaml').resolve()).replace("'", "''")
            (queries / f'{name}.sql').write_text(
                f"SET pg_orca.dsl_stats_experiment_path='{observation}';\n"
                "SET pg_orca.trace_fallback=on;\n"
                "SET pg_orca.enable_assert_maxonerow=off;\n"
                "SET pg_orca.dsl_only_xforms='';\n" + render_trace_query(case['query']))
        env = dict(os.environ, PG_CONFIG=str(args.pg_config.resolve()),
                   DSL_TRACE_PORT=str(args.base_port + index), DSL_TRACE_STATEMENT_TIMEOUT='60000',
                   DSL_TRACE_DSL_ENABLED='on', DSL_TRACE_DPHYPER_ENABLED='on',
                   DSL_TRACE_DPHYPER_SHADOW='off', DSL_TRACE_POLICY_PATH='',
                   DSL_TRACE_VERBOSE='0', DSL_TRACE_XFORMS='0', DSL_TRACE_OPT_STATS='1',
                   DSL_TRACE_DISABLE_XFORMS=','.join(disabled),
                   DSL_TRACE_MAX_ALTERNATIVES='0', DSL_TRACE_MAX_ALTERNATIVES_PER_RULE='0')
        with (output / 'runner.log').open('w') as log:
            process = subprocess.run([str(SCRIPT_DIR / 'trace_corpus.sh'), str(args.rules.resolve()),
                                      str((args.corpus_dir / item['dataset'] / 'schema.sql').resolve()),
                                      str(queries), str(output.resolve())], env=env, stdout=log, stderr=log)
    status_path = output / 'status.tsv'
    statuses = dict(line.split('\t') for line in status_path.read_text().splitlines()) if status_path.exists() else {}
    runs = []
    for case in item['cases']:
        stem = case['case_id'].split(':')[1]
        path = output / 'logs' / f'{stem}.log'
        result = {k: v for k, v in case.items() if k != 'query'}
        result.update(dataset=item['dataset'], complete=False, exclusions=['not_run'])
        if path.exists():
            text = path.read_text(errors='replace')
            try:
                result.update(summarize_trace(text, int(statuses.get(stem, 1)), orca_fallback_reason(path)))
            except ValueError as error:
                result.update(exclusions=[f'invalid_trace:{error}'])
            # Raw evidence is kept losslessly, without storing expanded context copies.
            with gzip.open(path.with_suffix('.log.gz'), 'wb') as archive:
                archive.write(path.read_bytes())
            path.unlink()
        runs.append(result)
    report = {'dataset': item['dataset'], 'runner_returncode': process.returncode, 'runs': runs}
    (output / 'summary.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
    print(f"{item['dataset']}: {sum(r['complete'] for r in runs)}/{len(runs)} complete", flush=True)
    return report


def render(report, output, font):
    plt = chinese_plotting(font)
    fig, axes = plt.subplots(2, 2, figsize=(16, 10))
    datasets = report['datasets']
    labels = [d['dataset'] for d in datasets]
    complete = [[r for r in d['runs'] if r['complete']] for d in datasets]
    partial = [[r for r in d['runs'] if not r['complete']] for d in datasets]
    for shift, key, label, color in [(-.2, 'attempted_rules', '参与求值的规则', '#4477aa'),
                                    (.2, 'ready_rules', '生成候选的规则', '#228833')]:
        axes[0, 0].bar([i + shift for i in range(len(labels))],
                       [len({h for r in d['runs'] for h in r.get(key, [])}) for d in datasets],
                       width=.4, label=label, color=color)
    axes[0, 0].set_xticks(range(len(labels)), labels)
    axes[0, 0].set_title('已观测规则覆盖（含失败前缀；不限最终计划）')
    axes[0, 0].legend()
    totals = coverage([r for rows in complete for r in rows])['statuses']
    prefixes = coverage([r for rows in partial for r in rows])['statuses']
    values = [totals.get(s, 0) for s in STAGES]
    axes[0, 1].bar([STAGES[s][1] for s in STAGES], values, label='完整查询')
    axes[0, 1].bar([STAGES[s][1] for s in STAGES], [prefixes.get(s, 0) for s in STAGES],
                   bottom=values, label='不完整查询的已观测前缀', color='#aaaaaa')
    axes[0, 1].set_yscale('symlog')
    axes[0, 1].set_title('全部已发出候选状态（对称对数轴）')
    axes[0, 1].legend()
    axes[1, 0].bar(labels, [len(d['runs']) for d in datasets], label='预先选定查询', color='#cccccc')
    axes[1, 0].bar(labels, [len(rows) for rows in complete], label='完整观测', color='#228833')
    axes[1, 0].set_title('保留失败分母；不要求执行或入选最终计划')
    axes[1, 0].legend()
    counts = [sum(len(r.get('edges', [])) for r in rows) for rows in complete]
    axes[1, 1].bar(labels, counts, label='完整查询')
    axes[1, 1].bar(labels, [sum(len(r.get('edges', [])) for r in rows) for rows in partial],
                   bottom=counts, color='#aaaaaa', label='不完整查询的已观测前缀')
    axes[1, 1].legend()
    axes[1, 1].set_title('实际来源消费边事件（不是共同出现次数）')
    for ax in axes.flat:
        ax.tick_params(axis='x', labelrotation=75)
    fig.suptitle('跨应用规则尝试画像：空表、通用参数计划、全部规则默认成本搜索')
    fig.tight_layout()
    fig.savefig(output / '规则尝试覆盖.png', dpi=150)
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--corpus-dir', type=Path, default=SCRIPT_DIR / 'corpus')
    parser.add_argument('--rules', type=Path, default=SCRIPT_DIR / 'rules/orca_replacements.rules')
    parser.add_argument('--pg-config', type=Path, required=True)
    parser.add_argument('--audit-bin', required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--per-dataset', type=int, default=4, help='uniform without replacement; 0 selects all')
    parser.add_argument('--offset', type=int, default=0, help='skip this many entries of each seeded permutation')
    parser.add_argument('--case', action='append', default=[], help='explicit dataset:line diagnostic case; may repeat')
    parser.add_argument('--export-only', action='store_true', help='export selected SQL/schema for the workload comparator; do not start PostgreSQL')
    parser.add_argument('--schema-domain', choices=('wetune', 'base'), default='wetune',
                        help='export-only: base omits appended WeTune schema patches, never original DDL constraints')
    parser.add_argument('--seed', type=int, default=7)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--base-port', type=int, default=60600)
    parser.add_argument('--font', type=Path, default=Path('output/fonts/NotoSansCJKsc-Regular.otf'))
    args = parser.parse_args()
    if args.schema_domain != 'wetune' and not args.export_only:
        parser.error('--schema-domain base requires --export-only')
    if args.per_dataset < 0 or args.offset < 0 or args.jobs < 1:
        parser.error('invalid sample size or jobs')
    items = select_cases(args.corpus_dir, 0 if args.case else args.per_dataset,
                         args.seed, 0 if args.case else args.offset)
    if args.case:
        wanted = set(args.case)
        available = {r['case_id'] for d in items for r in d['cases']}
        if wanted - available:
            parser.error(f'unknown cases: {sorted(wanted - available)}')
        for item in items:
            item['cases'] = [r for r in item['cases'] if r['case_id'] in wanted]
        items = [item for item in items if item['cases']]
    if not items or not 1024 <= args.base_port <= 65535 - len(items):
        parser.error('no corpus or invalid port range')
    args.output.mkdir(parents=True, exist_ok=False)
    disabled, _, _, _ = inventory(args)
    manifest = {'sampling': 'uniform_without_replacement_within_application_no_sql_dedup',
                'seed': args.seed, 'per_dataset': args.per_dataset, 'offset': args.offset, 'datasets': items,
                'rule_crc32': f'{zlib.crc32(args.rules.read_bytes()):08x}',
                'rules': str(args.rules.resolve()), 'disabled_xforms': disabled,
                'scope': 'all_dispatched_cbo_attempts_not_final_plan; planning_only_empty_tables',
                'statement_timeout_ms': 60000}
    if args.case:
        manifest.update(sampling='explicit_diagnostic_cases_not_random_sample', case_ids=args.case)
    if args.export_only:
        manifest['scope'] = 'selected_migrated_sql_schema_export_no_execution'
        manifest['schema_domain'] = args.schema_domain
        export_workload(items, args.corpus_dir, args.output, args.schema_domain)
    (args.output / 'manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n')
    if args.export_only:
        return
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        reports = list(pool.map(lambda p: run_dataset(args, p[1], p[0], disabled), enumerate(items)))
    runs = [r for d in reports for r in d['runs']]
    report = {'scope': manifest['scope'], 'datasets': reports,
              'complete_queries': coverage([r for r in runs if r['complete']]),
              'incomplete_query_observations_lower_bound': coverage([r for r in runs if not r['complete']])}
    (args.output / 'summary.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
    render(report, args.output, args.font)


if __name__ == '__main__':
    main()
