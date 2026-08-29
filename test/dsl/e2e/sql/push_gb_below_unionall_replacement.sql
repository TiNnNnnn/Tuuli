SELECT DISTINCT k
FROM (
    SELECT k FROM dsl_eq_left
    UNION ALL
    SELECT k FROM dsl_eq_right
) AS union_rows
ORDER BY k;
