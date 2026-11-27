SELECT o.k, q.m
FROM dsl_eq_left o
LEFT JOIN LATERAL (
    SELECT max(i.v::bigint) AS m FROM dsl_agg_outer i
    WHERE i.g = o.k AND i.v < 10 HAVING max(i.v::bigint) > 0::bigint
) q ON true
ORDER BY o.k;
