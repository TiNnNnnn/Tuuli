SELECT o.k
FROM dsl_eq_left AS o
WHERE EXISTS (
    SELECT 1
    FROM dsl_eq_right AS i
    WHERE i.k > 0)
ORDER BY o.k;
