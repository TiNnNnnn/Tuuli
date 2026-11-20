SELECT l.k, r.k
FROM dsl_eq_left AS l
LEFT JOIN dsl_eq_left AS r ON r.k = l.k
ORDER BY l.k, r.k;
