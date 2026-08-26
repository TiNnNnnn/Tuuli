SELECT l.k, r.k, o.id
FROM dsl_eq_left AS l
JOIN dsl_eq_right AS r ON true
JOIN dsl_insub_outer AS o ON l.k + r.k = o.id
ORDER BY l.k, r.k, o.id;
