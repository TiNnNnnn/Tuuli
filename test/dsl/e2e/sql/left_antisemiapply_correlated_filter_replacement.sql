SELECT o.payload
FROM dsl_correlated_exists AS o
WHERE NOT EXISTS (
    SELECT 1
    FROM dsl_correlated_exists AS i
    WHERE i.k = o.k)
ORDER BY o.payload;
