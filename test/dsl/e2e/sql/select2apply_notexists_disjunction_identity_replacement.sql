SELECT o.id
FROM dsl_insub_outer AS o
WHERE o.id = 1
   OR NOT EXISTS (
        SELECT 1
        FROM dsl_correlated_exists AS i
        WHERE i.k = o.id
          AND i.payload <= 20)
ORDER BY o.id;
