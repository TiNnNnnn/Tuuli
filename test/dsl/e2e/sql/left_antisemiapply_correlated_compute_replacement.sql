SELECT o.payload
FROM dsl_correlated_exists AS o
WHERE NOT EXISTS (
    SELECT 1
    FROM dsl_correlated_exists AS i
    WHERE o.k IS NULL OR i.k IS NULL)
ORDER BY o.payload;
