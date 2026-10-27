SELECT l.k, r.k
FROM dsl_eq_left AS l
INNER JOIN dsl_eq_right AS r ON l.k = r.k
WHERE l.k + 1 = 3
ORDER BY l.k, r.k;
