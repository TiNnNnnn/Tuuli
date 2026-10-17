SELECT o.k
FROM dsl_eq_left AS o
WHERE EXISTS (
    SELECT DISTINCT i.k
    FROM dsl_eq_right AS i)
ORDER BY o.k;
