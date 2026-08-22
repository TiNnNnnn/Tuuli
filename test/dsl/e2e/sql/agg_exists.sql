SELECT g, max(v) AS max_v
FROM dsl_agg_outer
GROUP BY g
HAVING max(v) > 5
   AND EXISTS (
       SELECT x + 1
       FROM dsl_exists_inner
       WHERE x = max(v))
ORDER BY g;

