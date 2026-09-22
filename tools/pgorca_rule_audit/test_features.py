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
    assert ir['schema_version'] in (1, 2)
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
    for binding in ir.get('bindings', []):
        assert set(binding) == {'kind', 'mode', 'symbols'}
        assert binding['kind'] in ('Not', 'NotTrue', 'And', 'Or', 'Ref', 'NullSafeEq')
        assert binding['mode'] in ('match', 'build')
        symbols(binding['symbols'])
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


def check_expression_bindings(binary, directory):
    inline = ('Filter<p0 a0>(Input<t0>)|Filter<Not(Not(p0)) a1>(Input<t1>)|'
              'TableEq(t1,t0);AttrsEq(a1,a0)')
    named = ('Filter<p0 a0>(Input<t0>)|Filter<p7 a1>(Input<t1>)|'
             'TableEq(t1,t0);AttrsEq(a1,a0);p8 := Not(p0);p7 := Not(p8)')
    capture = ('Filter<Not(Not(p0)) a0>(Input<t0>)|Filter<p1 a1>(Input<t1>)|'
               'TableEq(t1,t0);AttrsEq(a1,a0);p1 := p0')
    disjunction = ('Filter<Or(Not(Not(p0)),p1) a0>(Input<t0>)|Filter<Or(p2,p3) a1>(Input<t1>)|'
                   'TableEq(t1,t0);AttrsEq(a1,a0);p2 := p0;p3 := p1')
    source = directory / 'expression-input'
    source.mkdir()
    compact = inline.replace('TableEq(', 'Eq(').replace('AttrsEq(', 'Eq(')
    not_true = inline.replace('Not(', 'NotTrue(')
    comparison = ('Filter<NullSafeEq(a2,a3) a0>(Input<t0>)|'
                  'Filter<NullSafeEq(a8,a9) a1>(Input<t1>)|'
                  'Eq(t1,t0);Eq(a1,a0);a8 := a2;a9 := a3')
    (source / 'cases.rules').write_text('\n'.join([inline, named, capture, disjunction, compact, not_true, comparison]) + '\n')
    output = directory / 'expression-output'
    subprocess.run([binary, str(source), str(output)], check=True)
    nodes = json.loads((output / 'rule_graph.json').read_text())['nodes']
    assert len(nodes) == 6  # Surface Eq must not add a duplicate rule-graph node.
    for node in nodes:
        check_learning_ir(node)
        assert node['learning_ir']['schema_version'] == 2
    assert nodes[0]['learning_ir'] == nodes[1]['learning_ir']
    compact_source = directory / 'compact-expression-input'
    compact_source.mkdir()
    (compact_source / 'cases.rules').write_text(compact + '\n')
    compact_output = directory / 'compact-expression-output'
    subprocess.run([binary, str(compact_source), str(compact_output)], check=True)
    compact_nodes = json.loads((compact_output / 'rule_graph.json').read_text())['nodes']
    assert len(compact_nodes) == 1
    for key in ('learning_ir', 'rule_hash', 'template_features'):
        assert nodes[0][key] == compact_nodes[0][key], key
    assert [b['mode'] for b in nodes[2]['learning_ir']['bindings']] == ['match', 'match', 'build']
    assert sum(b['kind'] == 'Or' for b in nodes[3]['learning_ir']['bindings']) == 2
    assert all(b['kind'] == 'NotTrue' for b in nodes[4]['learning_ir']['bindings'])
    assert nodes[0]['learning_ir'] != nodes[4]['learning_ir']
    mixed = nodes[5]['learning_ir']
    assert sum(b['kind'] == 'NullSafeEq' for b in mixed['bindings']) == 2
    for binding in mixed['bindings']:
        types = [mixed['symbols'][ref]['kind'] for ref in binding['symbols']]
        assert types == (['a', 'a'] if binding['kind'] == 'Ref' else ['p', 'a', 'a'])
    # Exercise the actual exported IR, not just hand-written Python fixtures.
    sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'ml-orca'))
    from ml_orca.encoding.rule_policy_encoding import rule_sequence, rule_structure, rule_root_index
    for node in nodes:
        ir = node['learning_ir']
        rule_sequence(ir)
        structure = rule_structure(ir)
        assert any('Expression:' in token for seq in structure['sequences']['rule_node'] for token, _ in seq)
        for side in ('source', 'target'):
            rule_root_index(structure, side, 'r/0')
            try:
                rule_root_index(structure, side, 'r/1')
            except ValueError:
                pass
            else:
                raise AssertionError('scalar child admitted as a relational dependency root')


def check_surface_equalities(binary, directory):
    """The full public replacement bank keeps its policy keys and schedule."""
    rules = Path(__file__).resolve().parents[2] / 'test/dsl/rules'
    original = rules / 'orca_replacements.rules'
    compact = directory / 'surface-equality.rules'
    compact.write_text(re.sub(
        r'\b(?:Table|Attrs|Predicate|Schema|Func|Scalar|ExprList|Order|Window|Frame|FrameBound|Rank)Eq\(',
        'Eq(', original.read_text()))
    snapshots = []
    for library in (original, compact):
        result = subprocess.run([binary, '--policy-snapshot', str(library),
                                 str(rules / 'empty_workload_cbo.policy')],
                                capture_output=True, text=True, check=True)
        snapshots.append(json.loads(result.stdout))
    assert snapshots[0]['load']['failed'] == 0
    assert snapshots[0]['load']['admitted'] > 0
    assert snapshots[0] == snapshots[1]


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


def check_e2e_policies(binary, directory):
    dsl = Path(__file__).resolve().parents[2] / 'test/dsl'
    library = directory / 'e2e.rules'
    library.write_text('\n'.join((dsl / 'rules' / name).read_text()
                                for name in ('framework.rules', 'orca_replacements.rules')))
    policies = set()
    for path in (dsl / 'e2e/expect').glob('*.expect'):
        case = json.loads(path.read_text())
        for state in [*case.get('plans', []), case.get('rows', {})]:
            if state.get('policy'):
                policies.add(state['policy'])
    for policy in sorted(policies):
        result = subprocess.run([binary, '--policy-snapshot', str(library),
                                 str(dsl / 'rules' / policy)], capture_output=True, text=True)
        assert result.returncode == 0, f'{policy}: {result.stderr}'
        snapshot = json.loads(result.stdout)
        assert snapshot['load']['failed'] == 0, snapshot.get('load_errors')


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
        check_expression_bindings(sys.argv[1], Path(directory))
        check_surface_equalities(sys.argv[1], Path(directory))
        check_policy_snapshot(sys.argv[1], Path(directory))
        check_e2e_policies(sys.argv[1], Path(directory))
    print('production RuleIR features, alpha-normalized bindings and native policy snapshots: OK')


if __name__ == '__main__':
    main()
