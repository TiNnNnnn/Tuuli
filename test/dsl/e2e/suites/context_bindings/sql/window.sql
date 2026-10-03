SELECT i, j,
       max(CASE WHEN (i > 1) IS NOT TRUE THEN j ELSE i END)
         OVER (PARTITION BY j ORDER BY i NULLS FIRST
               ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW) AS running_max
FROM context_input
ORDER BY i NULLS FIRST, j, running_max;
