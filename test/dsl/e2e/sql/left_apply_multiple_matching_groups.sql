SELECT o.k, q.v, q.m
FROM dsl_eq_left AS o
LEFT JOIN LATERAL (
    SELECT i.v, max(i.g::bigint) AS m
    FROM dsl_agg_outer AS i
    WHERE i.g = o.k
    GROUP BY i.v
    HAVING max(i.g::bigint) < 3::bigint
) AS q ON true
ORDER BY o.k, q.v NULLS LAST;
