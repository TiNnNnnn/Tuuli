SELECT l.k, o.id, x.k
FROM dsl_eq_left AS l
JOIN dsl_insub_outer AS o ON o.id = l.k
CROSS JOIN LATERAL (
    SELECT r.k
    FROM dsl_eq_right AS r
    WHERE r.k = l.k AND r.k = o.id
    ORDER BY r.k
    LIMIT 1) AS x
ORDER BY l.k, o.id, x.k;
