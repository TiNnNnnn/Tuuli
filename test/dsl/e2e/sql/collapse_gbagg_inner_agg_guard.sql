SELECT q.g
FROM (
    SELECT g, max(v) AS max_v
    FROM dsl_agg_outer
    GROUP BY g
) AS q
GROUP BY q.g
ORDER BY q.g;
