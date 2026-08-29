SELECT a
FROM (
    SELECT a, b FROM dsl_eq_pair_left WHERE a IS NOT NULL
    UNION
    SELECT a, b FROM dsl_eq_pair_right WHERE a IS NOT NULL
) AS union_rows
GROUP BY a
ORDER BY a;
