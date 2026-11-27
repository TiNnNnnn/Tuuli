SELECT o.k, q.m
FROM dsl_eq_right o
LEFT JOIN LATERAL (
    SELECT max(100 / (i.v - 5)) AS m
    FROM dsl_agg_outer i
    WHERE i.g = o.k
    HAVING max(100 / (i.v - 5)) > 0
) q ON true
ORDER BY o.k;
