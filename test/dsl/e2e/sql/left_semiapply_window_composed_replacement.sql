SELECT o.g
FROM dsl_agg_outer AS o
WHERE o.v IN (
    SELECT max(q.v) OVER (PARTITION BY q.g)
    FROM dsl_agg_outer AS q
    WHERE q.g = o.g)
ORDER BY o.g;
