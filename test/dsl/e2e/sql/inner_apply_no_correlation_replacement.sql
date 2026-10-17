SELECT o.k
FROM dsl_eq_left AS o
WHERE EXISTS (
    SELECT DISTINCT i.k
    FROM (VALUES (1), (1), (2), (4)) AS i(k))
ORDER BY o.k;
