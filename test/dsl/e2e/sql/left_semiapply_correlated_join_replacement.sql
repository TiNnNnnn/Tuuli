SELECT o.payload
FROM dsl_correlated_exists AS o
WHERE EXISTS (
    SELECT 1
    FROM dsl_correlated_exists AS i
    JOIN dsl_correlated_exists AS j
      ON i.payload = j.payload
     AND i.k = o.k)
ORDER BY o.payload;
