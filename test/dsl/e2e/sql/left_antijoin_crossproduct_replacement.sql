SELECT scenario, a, b
FROM (
    SELECT 'nonempty' AS scenario, l.a, l.b
    FROM dsl_eq_pair_left AS l
    WHERE NOT EXISTS (
        SELECT 1 FROM dsl_eq_right AS r WHERE l.a > 1)
    UNION ALL
    SELECT 'empty' AS scenario, l.a, l.b
    FROM dsl_eq_pair_left AS l
    WHERE NOT EXISTS (
        SELECT 1
        FROM (SELECT k FROM dsl_eq_right WHERE k > 100) AS r
        WHERE l.a > 1)
) AS cases
ORDER BY scenario, a NULLS LAST, b NULLS LAST;
