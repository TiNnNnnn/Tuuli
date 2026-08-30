SELECT o.g
FROM dsl_agg_outer AS o
WHERE EXISTS (
    SELECT count(*)
    FROM dsl_agg_outer AS i
    WHERE i.g = o.g
    GROUP BY i.v
    HAVING i.v = i.v)
ORDER BY o.g;
