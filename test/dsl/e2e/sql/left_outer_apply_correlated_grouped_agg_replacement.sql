SELECT o.id, q.g, q.max_v
FROM dsl_insub_outer AS o
LEFT JOIN LATERAL (
    SELECT i.g, max(i.v) AS max_v
    FROM dsl_agg_outer AS i
    GROUP BY i.g
    HAVING i.g = o.id) AS q ON true
ORDER BY o.id;
