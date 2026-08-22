SELECT l.k
FROM dsl_eq_left AS l
INNER JOIN dsl_eq_right AS r ON l.k = r.k
GROUP BY l.k
ORDER BY l.k;

