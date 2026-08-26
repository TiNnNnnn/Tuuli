SELECT l.k, r.k, o.id, c.id
FROM dsl_eq_left AS l
JOIN dsl_eq_right AS r ON l.k = r.k
JOIN dsl_insub_outer AS o ON r.k = o.id
JOIN dsl_fk_child AS c ON o.id = c.id
ORDER BY l.k, r.k, o.id, c.id;
