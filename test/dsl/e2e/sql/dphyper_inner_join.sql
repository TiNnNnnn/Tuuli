SELECT l.k, r.k, o.id
FROM dsl_eq_left AS l
JOIN dsl_eq_right AS r ON l.k = r.k
JOIN dsl_insub_outer AS o ON o.id = l.k
ORDER BY l.k, r.k, o.id;
