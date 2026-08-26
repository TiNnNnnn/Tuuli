SELECT l.k, r.k, o.id
FROM dsl_eq_left AS l
LEFT JOIN (
    dsl_eq_right AS r
    JOIN dsl_insub_outer AS o ON o.id = r.k)
ON l.k = r.k
ORDER BY l.k, r.k NULLS LAST, o.id NULLS LAST;
