SELECT l.k
FROM (
    SELECT k
    FROM dsl_eq_left
    ORDER BY k
    LIMIT 3) AS l
WHERE EXISTS (
    SELECT 1
    FROM dsl_eq_right AS r
    WHERE l.k > 0)
ORDER BY l.k;
