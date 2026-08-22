SELECT l.k, r.k
FROM dsl_eq_left AS l
INNER JOIN dsl_eq_left AS r ON l.k = r.k
WHERE l.k > 1 AND r.k > 2
ORDER BY l.k, r.k;

