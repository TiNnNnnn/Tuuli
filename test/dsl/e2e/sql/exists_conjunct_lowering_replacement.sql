SELECT scenario, a, b FROM (
    SELECT 'semi' AS scenario, l.a, l.b FROM dsl_eq_pair_left l
    WHERE l.a > 0 AND EXISTS (
        SELECT 1 FROM dsl_eq_pair_right r WHERE r.a <> l.a AND r.b > 0)
    UNION ALL
    SELECT 'anti', l.a, l.b FROM dsl_eq_pair_left l
    WHERE l.a > 0 AND NOT EXISTS (
        SELECT 1 FROM dsl_eq_pair_right r WHERE r.a = l.a AND r.b < 0)
    UNION ALL
    SELECT 'mixed', l.a, l.b FROM dsl_eq_pair_left l
    WHERE EXISTS (SELECT 1 FROM dsl_eq_pair_right r WHERE r.a <> l.a AND r.b > 0)
      AND NOT EXISTS (SELECT 1 FROM dsl_eq_pair_right r WHERE r.a = l.a AND r.b < 0)
) cases
ORDER BY scenario, a NULLS FIRST, b NULLS FIRST;
