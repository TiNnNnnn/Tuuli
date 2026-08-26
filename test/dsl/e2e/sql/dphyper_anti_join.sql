SELECT o.id, l.k
FROM dsl_insub_outer AS o
JOIN dsl_eq_left AS l ON l.k = o.id
WHERE NOT EXISTS (
    SELECT 1
    FROM dsl_insub_inner AS i
    WHERE i.id = o.id)
ORDER BY o.id, l.k;
