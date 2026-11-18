SELECT o.id, l.k
FROM dsl_insub_outer AS o
JOIN dsl_eq_left AS l ON l.k = o.id
WHERE o.id NOT IN (
    SELECT i.k
    FROM dsl_correlated_exists AS i)
ORDER BY o.id, l.k;
