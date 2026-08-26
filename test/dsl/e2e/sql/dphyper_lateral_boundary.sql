SELECT l.k, x.k, o.id
FROM dsl_eq_left AS l
CROSS JOIN LATERAL (
    SELECT r.k
    FROM dsl_eq_right AS r
    WHERE r.k = l.k
    ORDER BY r.k
    LIMIT 1) AS x
JOIN dsl_insub_outer AS o ON o.id = x.k
ORDER BY l.k, x.k, o.id;
