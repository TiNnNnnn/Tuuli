#!/usr/bin/env python3
"""Check production-IR template metrics on the existing audit fixture."""
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


def check_learning_ir(node):
    ir = node['learning_ir']
    assert ir['schema_version'] == 1
    seen = []

    def symbols(refs):
        for ref in refs:
            assert type(ref) is int and 0 <= ref < len(ir['symbols'])
            if ref not in seen:
                seen.append(ref)

    def tree(op):
        assert set(op) == {'op', 'symbols', 'children'}
        symbols(op['symbols'])
        return 1 + sum(tree(child) for child in op['children'])

    for side in ('source', 'target'):
        assert ir[side]['op'] == node[side + '_root']
        assert tree(ir[side]) == node['template_features'][side + '_nodes']
    for constraint in ir['constraints']:
        assert set(constraint) == {'kind', 'symbols'}
        symbols(constraint['symbols'])
    assert len(ir['constraints']) == node['template_features']['constraint_count']
    assert {c['kind'] for c in ir['constraints']} == set(node['constraints'])
    assert seen == list(range(len(ir['symbols'])))
    assert all(set(s) == {'kind', 'side'} and s['side'] in ('source', 'target')
               and len(s['kind']) == 1 for s in ir['symbols'])


def check_alpha_and_binding(binary, directory):
    # Structural fixtures, not newly proved or registered rewrite rules.
    base = ('Filter<p0 a0>(Filter<p1 a1>(Input<t0>))|'
            'Filter<p2 a2>(Filter<p3 a3>(Input<t1>))|'
            'TableEq(t1,t0);AttrsEq(a2,a0);AttrsEq(a3,a1);'
            'PredicateEq(p2,p0);PredicateEq(p3,p1)')
    renamed = re.sub(r'\b([a-z])(\d+)\b', lambda m: m[1] + str(100 + int(m[2])), base)
    rebound = base.replace('PredicateEq(p2,p0);PredicateEq(p3,p1)',
                           'PredicateEq(p2,p1);PredicateEq(p3,p0)')
    shared = base.replace('PredicateEq(p3,p1)', 'PredicateEq(p3,p0)')
    join = ('InnerJoin<a0 a1>(Input<t0>,Filter<p0 a2>(Input<t1>))|'
            'InnerJoin<a3 a4>(Input<t2>,Filter<p1 a5>(Input<t3>))|'
            'TableEq(t2,t0);TableEq(t3,t1)')
    reordered = join.replace('Input<t0>,Filter<p0 a2>(Input<t1>)',
                             'Filter<p0 a2>(Input<t1>),Input<t0>')
    # a5 is introduced by a constructive constraint, not an operator slot.
    derived = ('Filter<p0 a0>(Input<t0>)|Filter<p1 a1>('
               'LeftApply<p2 a2 a3 a4>(Input<t1>,Input<t2>))|'
               'TableEq(t1,t0);ExprListScalarSubquery(p0,p1,p2,a2,a3,a4,a5,t2)')
    cases = [base, renamed, rebound, shared, join, reordered, derived,
             re.sub(r'\b([a-z])(\d+)\b', lambda m: m[1] + str(300 + int(m[2])), derived)]
    inputs = directory / 'learning-input'
    inputs.mkdir()
    (inputs / 'cases.rules').write_text('\n'.join(cases) + '\n')
    output = directory / 'learning-output'
    subprocess.run([binary, str(inputs), str(output)], check=True)
    nodes = json.loads((output / 'rule_graph.json').read_text())['nodes']
    assert len(nodes) == len(cases)
    for node in nodes:
        check_learning_ir(node)
    ir = [node['learning_ir'] for node in nodes]
    assert ir[0] == ir[1]
    assert nodes[0]['rule_hash'] != nodes[1]['rule_hash']  # Do not change policy identity.
    assert ir[0] != ir[2]  # Same operators/constraint kinds, different bindings.
    assert ir[0] != ir[3]  # Repeated symbol is not two independent symbols.
    assert ir[4] != ir[5]  # Ordered children, including a non-scan Input placeholder.
    assert ir[6] == ir[7]  # Constraint-only LET symbols are normalized too.


def check_policy_snapshot(binary, directory):
    library = directory / 'learning-input/cases.rules'
    policy = directory / 'schedule.policy'

    def resolve(document=None):
        args = [binary, '--policy-snapshot', str(library)]
        if document is not None:
            policy.write_text(document)
            args.append(str(policy))
        result = subprocess.run(args, capture_output=True, text=True, check=True)
        return json.loads(result.stdout)

    default = resolve()
    assert default['load'] == {'admitted': 8, 'skipped_non_eq': 0, 'failed': 0}
    assert not default['explicit_policy']
    rows = default['rules']
    hashes = [r['rule_hash'] for r in rows]
    for i, row in enumerate(rows):
        assert row['enabled'] and row['placement'] == 'cbo' and row['phase'] == 'explore'
        assert row['candidate_list_position'] == i and row['source_line'] == i + 1
        assert row['budget'] == dict(per_node=0, per_rule=0, per_query=0)
    wildcard = resolve('- rule: "*"\n  enabled: false\n  priority: 100\n'
                       f'- rule: {hashes[0]}\n  placement: cbo\n')
    assert wildcard['explicit_policy']
    assert wildcard['rules'][0]['enabled'] and wildcard['rules'][0]['priority'] == 0
    assert all(not r['enabled'] and r['candidate_list'] is None for r in wildcard['rules'][1:])
    auto = resolve(f'- rule: {hashes[0]}\n  placement: auto\n  effect: changes_join_graph\n'
                   '  priority: 7\n  fixpoint: true\n  budget:\n    per_node: 2\n    per_rule: 3\n    per_query: 4\n'
                   f'- rule: {hashes[1]}\n  placement: auto\n  effect: preserves_join_graph\n'
                   f'- rule: {hashes[2]}\n  placement: auto\n  effect: join_reordering\n')
    assert (auto['rules'][0]['placement'], auto['rules'][0]['candidate_list']) == ('rbo', 'pre_join')
    assert auto['rules'][0]['budget'] == dict(per_node=2, per_rule=3, per_query=4)
    assert auto['rules'][0]['fixpoint'] and auto['rules'][0]['order'] == 'bottom_up'
    assert auto['rules'][1]['placement'] == 'cbo'
    assert auto['rules'][2]['placement'] == 'rejected' and auto['rules'][2]['candidate_list'] is None
    ordered = resolve('- rule: "*"\n  placement: rbo\n  phase: cleanup\n'
                      f'- rule: {hashes[-1]}\n  placement: rbo\n  phase: cleanup\n  priority: 1\n')
    assert [r['candidate_list_position'] for r in ordered['rules']] == [1, 2, 3, 4, 5, 6, 7, 0]
    for document in ('- rule: 0000000000000000\n', '- rule: "*"\n  priority: broken\n',
                     f'- rule: {hashes[0]}\n- rule: {hashes[0]}\n'):
        policy.write_text(document)
        failed = subprocess.run([binary, '--policy-snapshot', str(library), str(policy)],
                                capture_output=True, text=True)
        assert failed.returncode != 0 and failed.stderr and not failed.stdout
    with library.open('a') as stream:
        stream.write('not a rule\nnot admitted\tNEQ\n')
    partial = resolve()
    assert partial['load'] == {'admitted': 8, 'skipped_non_eq': 1, 'failed': 1}
    assert partial['load_errors'] and partial['rules'] == rows


def main():
    with tempfile.TemporaryDirectory(prefix='pgorca-template-features.') as directory:
        subprocess.run([sys.argv[1], str(Path(__file__).resolve().parents[2] / 'test/dsl/audit'), directory], check=True)
        nodes = json.loads((Path(directory) / 'rule_graph.json').read_text())['nodes']
        runtime = json.loads((Path(directory) / 'coverage.json').read_text())
        categories = {x['name']: x['category'] for x in runtime['xforms']}
        assert categories['CXformCTEAnchor2Sequence'] == 'implementation_property'
        assert categories['CXformSubquery2CorrelatedApply'] == 'implementation_property'
        for name in ('CXformSelect2Apply', 'CXformProject2Apply', 'CXformGbAgg2Apply', 'CXformSequenceProject2Apply'):
            assert categories[name] == 'semantic_rewrite'
        for name in ('CXformCTEAnchor2TrivialSelect', 'CXformInlineCTEConsumer',
                     'CXformInlineCTEConsumerUnderSelect'):
            assert categories[name] == 'semantic_rewrite'
        assert len(nodes) == 3
        for n in nodes:
            check_learning_ir(n)
            f = n['template_features']
            nested = n['source_pattern'].count('Filter') == 2
            count = 3 if nested else 2
            for side in ('source', 'target'):
                assert f[side + '_nodes'] == count
                assert f[side + '_depth'] == count
                assert f[side + '_inputs'] == 1
                assert f[side + '_symbol_slots'] == (5 if nested else 3 if n['source_root'] == 'Filter' else 2)
            assert f['constraint_count'] == (5 if nested else 3 if n['source_root'] == 'Filter' else 2)
            assert f['constraint_kinds'] == len(n['constraints'])
        check_alpha_and_binding(sys.argv[1], Path(directory))
        check_policy_snapshot(sys.argv[1], Path(directory))
    print('production RuleIR features, alpha-normalized bindings and native policy snapshots: OK')


if __name__ == '__main__':
    main()
