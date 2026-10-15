SELECT DISTINCT l.k
FROM dsl_eq_left AS l
JOIN dsl_eq_right AS r ON l.k = r.k
ORDER BY l.k;
