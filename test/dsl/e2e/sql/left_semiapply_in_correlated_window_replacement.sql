SELECT o.payload
FROM dsl_correlated_exists AS o
WHERE o.payload IN (
    SELECT ranked.max_payload
    FROM (
        SELECT i.k,
               max(i.payload) OVER (PARTITION BY i.k) AS max_payload
        FROM dsl_correlated_exists AS i
        WHERE i.k = o.k) AS ranked)
ORDER BY o.payload;
