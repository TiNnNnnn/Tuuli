SELECT d.k
FROM (SELECT DISTINCT k FROM dsl_eq_left) AS d
JOIN dsl_insub_outer AS o ON d.k = o.id
ORDER BY d.k;
