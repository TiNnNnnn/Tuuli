SELECT l.k
FROM dsl_eq_left AS l
INNER JOIN dsl_insub_inner AS r ON l.k = r.id
GROUP BY l.k
ORDER BY l.k;
