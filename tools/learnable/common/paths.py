"""Repository assets and source provenance; no imports from the test harness."""
from pathlib import Path

PGORCA_ROOT = Path(__file__).resolve().parents[3]
LEARNABLE_ROOT = PGORCA_ROOT / 'tools/learnable'
TEST_ASSETS = PGORCA_ROOT / 'test/dsl'


def source_file(filename):
    matches = [p for p in LEARNABLE_ROOT.rglob(filename) if 'tests' not in p.relative_to(LEARNABLE_ROOT).parts]
    if len(matches) != 1:
        raise ValueError('require one learnable source file: ' + filename)
    return matches[0]


def package_sources():
    return {'learnable:' + str(p.relative_to(LEARNABLE_ROOT)): p
            for p in sorted(LEARNABLE_ROOT.rglob('*.py'))
            if 'tests' not in p.relative_to(LEARNABLE_ROOT).parts}
