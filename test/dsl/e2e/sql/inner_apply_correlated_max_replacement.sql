SELECT o.k, q.g, q.max_v
FROM dsl_eq_right AS o
CROSS JOIN LATERAL (
    SELECT i.g, max(i.v) AS max_v
    FROM dsl_agg_outer AS i
    GROUP BY i.g
    HAVING i.g = o.k) AS q
ORDER BY o.k;
