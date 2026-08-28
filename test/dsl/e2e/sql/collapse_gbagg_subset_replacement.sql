SELECT q.g
FROM (
    SELECT g, v
    FROM dsl_agg_outer
    GROUP BY g, v
) AS q
GROUP BY q.g
ORDER BY q.g;
