SELECT o.g
FROM dsl_agg_outer AS o
WHERE NOT EXISTS (
    SELECT min(q.v)
    FROM (SELECT i.g, i.v FROM dsl_agg_outer AS i
          WHERE 100 / (i.v + 1) > 0
          ORDER BY i.g, i.v LIMIT 100) AS q
    WHERE q.g = o.g
    GROUP BY q.v
    HAVING q.v = q.v)
ORDER BY o.g;
