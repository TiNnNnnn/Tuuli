SELECT o.g
FROM dsl_agg_outer AS o
WHERE NOT EXISTS (
    SELECT max(i.v)
    FROM dsl_agg_outer AS i
    WHERE i.g = o.g
    GROUP BY i.v
    HAVING max(i.v) = (
        SELECT min(j.v) FROM dsl_agg_outer AS j
        WHERE j.v = i.v AND j.g = 3))
ORDER BY o.g;
