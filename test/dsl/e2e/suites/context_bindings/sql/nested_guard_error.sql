SELECT i, j,
       CASE WHEN i = 1 THEN (i / 0 > 1) IS NOT TRUE ELSE false END AS first_value,
       (j > 1) IS NOT TRUE AS second_value
FROM context_input
ORDER BY i NULLS FIRST, j;
