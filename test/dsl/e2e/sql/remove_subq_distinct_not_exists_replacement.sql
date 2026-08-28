SELECT o.k
FROM dsl_eq_left AS o
WHERE NOT EXISTS (
    SELECT DISTINCT i.k
    FROM dsl_eq_right AS i
    WHERE i.k = o.k)
ORDER BY o.k;
