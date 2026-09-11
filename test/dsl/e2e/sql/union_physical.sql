-- The parent discards a dedup key; UNION must still deduplicate the full row.
SELECT COALESCE(a, -1) AS a
FROM (
    SELECT a, b FROM dsl_eq_pair_left
    UNION
    SELECT b AS a, a AS b FROM dsl_eq_pair_right
    UNION
    SELECT a, b FROM dsl_eq_pair_left WHERE a = 1
) u
ORDER BY 1;
