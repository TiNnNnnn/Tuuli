"""Retrospective trace work labels; not policy value or total planning latency."""
import math

TARGETS = ('traced_match_constraint_ms', 'traced_instantiate_ms')

def observed_target(summary):
    if not summary.get('complete') or summary.get('returncode') != 0:
        raise ValueError('incomplete_query')
    groups = summary.get('groups')
    if not isinstance(groups, list) or not groups:
        raise ValueError('timing_groups_unavailable')
    if sum(g['attempts'] for g in groups) != summary['attempts']:
        raise ValueError('timing_attempt_denominator_mismatch')
    values = []
    for fields in (('match_us', 'constraint_us'), ('instantiate_us',)):
        total = 0
        for group in groups:
            for field in fields:
                value = group.get(field)
                if type(value) not in (int, float) or not math.isfinite(value) or value < 0:
                    raise ValueError('invalid_timing_' + field)
                total += value
        values.append(math.log1p(total / 1000.))
    return values
