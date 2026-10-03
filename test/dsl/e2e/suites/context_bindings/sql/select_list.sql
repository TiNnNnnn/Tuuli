SELECT i, j, (i > 1) IS NOT TRUE AS first_value,
       (j > 1) IS NOT TRUE AS second_value
FROM context_input
ORDER BY i NULLS FIRST, j;
