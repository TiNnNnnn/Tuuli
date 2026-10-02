SELECT scenario, a, b FROM (
    SELECT 'semi' AS scenario, l.a, l.b FROM dsl_eq_pair_left l
    WHERE EXISTS (SELECT 1 / (r.a - r.a) FROM dsl_eq_pair_right r
                  WHERE r.a <> l.a AND r.b > 0)
    UNION ALL
    SELECT 'anti', l.a, l.b FROM dsl_eq_pair_left l
    WHERE NOT EXISTS (SELECT 1 / (r.a - r.a) FROM dsl_eq_pair_right r
                      WHERE r.a = l.a AND r.b > 0)
) cases
ORDER BY scenario, a NULLS FIRST, b NULLS FIRST;
