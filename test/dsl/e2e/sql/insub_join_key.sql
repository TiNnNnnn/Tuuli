SELECT l.k
FROM dsl_eq_left AS l
JOIN dsl_eq_right AS r ON l.k = r.k
WHERE l.k IN (
    SELECT k FROM dsl_eq_left
    UNION ALL
    SELECT k FROM dsl_eq_left)
ORDER BY l.k;
