SELECT o.k
FROM dsl_eq_left AS o
WHERE o.k < ANY (
    SELECT DISTINCT i.k
    FROM dsl_eq_right AS i)
ORDER BY o.k;
