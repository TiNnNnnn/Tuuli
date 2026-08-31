SELECT o.payload
FROM dsl_correlated_exists AS o
WHERE NOT EXISTS (
    SELECT 1
    FROM (
        SELECT i.k,
               max(i.payload) OVER (PARTITION BY i.k) AS max_payload
        FROM dsl_correlated_exists AS i
        WHERE i.k = o.k) AS ranked
    WHERE ranked.max_payload > 0)
ORDER BY o.payload;
