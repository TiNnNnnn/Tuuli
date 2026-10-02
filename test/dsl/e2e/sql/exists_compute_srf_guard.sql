SELECT scenario, a, b FROM (
    SELECT 'semi' AS scenario, l.a, l.b FROM dsl_eq_pair_left l
    WHERE EXISTS (SELECT generate_series(1, r.a-r.a) FROM dsl_eq_pair_right r)
    UNION ALL
    SELECT 'anti', l.a, l.b FROM dsl_eq_pair_left l
    WHERE NOT EXISTS (SELECT generate_series(1, r.a-r.a) FROM dsl_eq_pair_right r)
) cases
ORDER BY scenario, a NULLS FIRST, b NULLS FIRST;
