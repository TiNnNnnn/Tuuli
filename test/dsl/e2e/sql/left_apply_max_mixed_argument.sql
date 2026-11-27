SELECT o.k, q.m
FROM dsl_eq_left o
LEFT JOIN LATERAL (
    SELECT max(coalesce(i.v, o.k)) AS m
    FROM dsl_agg_outer i
    WHERE i.g = o.k
    HAVING max(coalesce(i.v, o.k)) > 0
) q ON true
ORDER BY o.k;
