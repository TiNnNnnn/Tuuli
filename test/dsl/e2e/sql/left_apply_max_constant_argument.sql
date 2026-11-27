SELECT o.k, q.m
FROM dsl_eq_left o
LEFT JOIN LATERAL (
    SELECT max(9) AS m FROM dsl_agg_outer i
    WHERE i.g = o.k AND i.v < 10 HAVING max(9) > 0
) q ON true
ORDER BY o.k;
