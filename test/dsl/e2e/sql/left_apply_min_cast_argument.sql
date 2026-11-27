SELECT o.k, q.m
FROM dsl_eq_left o
LEFT JOIN LATERAL (
    SELECT min(i.v::bigint) AS m FROM dsl_agg_outer i
    WHERE i.g = o.k AND i.v < 30 HAVING min(i.v::bigint) > 5::bigint
) q ON true
ORDER BY o.k;
