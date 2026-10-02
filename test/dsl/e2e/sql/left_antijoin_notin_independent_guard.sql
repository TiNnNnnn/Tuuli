SELECT scenario, a, b
FROM (
    SELECT 'nonempty' AS scenario, l.a, l.b
    FROM dsl_eq_pair_left l
    WHERE l.a <> ALL (SELECT 1 FROM dsl_eq_right r)
    UNION ALL
    SELECT 'empty' AS scenario, l.a, l.b
    FROM dsl_eq_pair_left l
    WHERE l.a <> ALL (SELECT 1 FROM dsl_eq_right r WHERE k > 100)
    UNION ALL
    SELECT 'unknown' AS scenario, l.a, l.b
    FROM dsl_eq_pair_left l
    WHERE l.a <> ALL (SELECT NULL::int FROM dsl_eq_right r)
) cases
ORDER BY scenario, a NULLS LAST, b NULLS LAST;
