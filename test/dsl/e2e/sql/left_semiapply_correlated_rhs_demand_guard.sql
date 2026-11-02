SELECT o.k
FROM dsl_eq_right AS o
WHERE EXISTS (
    SELECT 1
    FROM (SELECT i.k FROM dsl_eq_right AS i
          WHERE 100 / (i.k + 10) > 0
          ORDER BY i.k LIMIT 2) AS q
    WHERE q.k = o.k)
ORDER BY o.k;
