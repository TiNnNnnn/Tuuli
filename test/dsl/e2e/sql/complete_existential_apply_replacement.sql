SELECT scenario, a, b FROM (
    SELECT 'semi' AS scenario, l.a, l.b FROM dsl_eq_pair_left l
    WHERE EXISTS (SELECT * FROM dsl_eq_pair_right r)
    UNION ALL
    SELECT 'anti_empty', l.a, l.b FROM dsl_eq_pair_left l
    WHERE NOT EXISTS (SELECT * FROM dsl_eq_pair_right r WHERE r.b < 0)
    UNION ALL
    SELECT 'semi_correlated', l.a, l.b FROM dsl_eq_pair_left l
    WHERE EXISTS (SELECT r.b + 7 FROM dsl_eq_pair_right r WHERE r.a <> l.a AND r.b > 0)
    UNION ALL
    SELECT 'anti_correlated', l.a, l.b FROM dsl_eq_pair_left l
    WHERE NOT EXISTS (SELECT r.b + 7 FROM dsl_eq_pair_right r WHERE r.a = l.a AND r.b > 0)
    UNION ALL
    SELECT 'semi_on', l.a, l.b FROM dsl_eq_pair_left l
    WHERE EXISTS (SELECT * FROM dsl_eq_pair_right r WHERE r.a = l.a LIMIT 1)
    UNION ALL
    SELECT 'anti_on', l.a, l.b FROM dsl_eq_pair_left l
    WHERE NOT EXISTS (SELECT * FROM dsl_eq_pair_right r WHERE r.a = l.a LIMIT 1)
) cases
ORDER BY scenario, a NULLS FIRST, b NULLS FIRST;
