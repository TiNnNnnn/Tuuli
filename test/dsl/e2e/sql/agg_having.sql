SELECT g, max(v) AS max_v
FROM dsl_agg_outer
GROUP BY g
HAVING max(v) > 5
ORDER BY g;

