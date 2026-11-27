SELECT o.k, q.m
FROM (
    SELECT k FROM dsl_eq_left
    UNION ALL
    SELECT v FROM dsl_agg_outer WHERE v IS NULL
) o
LEFT JOIN LATERAL (
    SELECT max(i.v::bigint) AS m
    FROM dsl_agg_outer i
    WHERE i.g = o.k
    HAVING max(i.v::bigint) > 5::bigint
) q ON true
ORDER BY o.k NULLS LAST;
