SELECT j, count(*),
       bool_or(DISTINCT (i > 1) IS NOT TRUE),
       bool_and((i > 1) IS NOT TRUE)
FROM context_input
GROUP BY j
HAVING count(*) > 0
ORDER BY j NULLS FIRST;
