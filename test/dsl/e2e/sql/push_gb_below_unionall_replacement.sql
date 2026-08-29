SELECT k
FROM (
    SELECT k, k % 2 AS v FROM dsl_eq_left
    UNION ALL
    SELECT k, k % 2 AS v FROM dsl_eq_right
) AS union_rows
GROUP BY k
ORDER BY k;
