SELECT i, j, k, (CASE WHEN i = 1 AND j = 1 THEN k ELSE j END) = 2 AS matched
FROM comparison_input
ORDER BY i NULLS FIRST, j NULLS FIRST, k NULLS FIRST;
