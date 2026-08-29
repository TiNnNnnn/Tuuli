SELECT o.payload
FROM dsl_correlated_exists AS o
WHERE EXISTS (
    SELECT *
    FROM dsl_correlated_exists AS i
    WHERE o.k IS NULL OR i.k IS NULL)
ORDER BY o.payload;
