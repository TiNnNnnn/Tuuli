SELECT o.id, q.g, q.n
FROM dsl_insub_outer AS o
CROSS JOIN LATERAL (
    SELECT i.g, count(*) AS n
    FROM dsl_agg_outer AS i
    GROUP BY i.g
    HAVING i.g = o.id) AS q
ORDER BY o.id;
