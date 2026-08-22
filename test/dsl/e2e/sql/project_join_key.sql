SELECT l.k + 0 AS projected_k
FROM dsl_eq_left AS l
INNER JOIN dsl_eq_right AS r ON l.k = r.k
ORDER BY projected_k;

