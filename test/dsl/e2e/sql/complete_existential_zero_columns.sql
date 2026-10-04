SELECT scenario, a, b FROM (
    SELECT 'semi' AS scenario, l.a, l.b FROM dsl_eq_pair_left l
    WHERE EXISTS (SELECT WHERE l.a = l.b OFFSET 0 LIMIT 1)
    UNION ALL
    SELECT 'anti', l.a, l.b FROM dsl_eq_pair_left l
    WHERE NOT EXISTS (SELECT WHERE l.a = l.b OFFSET 0 LIMIT 1)
    UNION ALL
    SELECT 'empty', l.a, l.b FROM dsl_eq_pair_left l
    WHERE EXISTS (SELECT WHERE l.a = l.b OFFSET 1 LIMIT 1)
) cases
ORDER BY scenario, a NULLS FIRST, b NULLS FIRST;
