#!/usr/bin/env python3
"""Check production-IR template metrics on the existing audit fixture."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    with tempfile.TemporaryDirectory(prefix='pgorca-template-features.') as directory:
        subprocess.run([sys.argv[1], str(Path(__file__).resolve().parents[2] / 'test/dsl/audit'), directory], check=True)
        nodes = json.loads((Path(directory) / 'rule_graph.json').read_text())['nodes']
        assert len(nodes) == 3
        for n in nodes:
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
    print('production RuleIR template features: OK')


if __name__ == '__main__':
    main()
