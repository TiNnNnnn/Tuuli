SELECT o.g
FROM dsl_agg_outer AS o
WHERE o.v IN (
    SELECT max(q.v) OVER (PARTITION BY q.g)
    FROM (SELECT i.g, i.v
          FROM dsl_agg_outer AS i
          WHERE 100 / (i.v + 1) > 0
          ORDER BY i.g, i.v LIMIT 100) AS q
    WHERE q.g = o.g)
ORDER BY o.g;
