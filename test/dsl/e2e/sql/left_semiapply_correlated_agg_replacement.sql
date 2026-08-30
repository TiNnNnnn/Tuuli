SELECT o.g
FROM dsl_agg_outer AS o
WHERE 1::bigint IN (
    SELECT count(*)
    FROM dsl_agg_outer AS i
    WHERE i.g = o.g
    GROUP BY i.g)
ORDER BY o.g;
