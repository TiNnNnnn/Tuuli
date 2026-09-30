SELECT o.k, q.payload
FROM dsl_correlated_exists AS o
CROSS JOIN LATERAL (
    SELECT i.payload
    FROM dsl_correlated_exists AS i
    WHERE i.k = o.k) AS q
ORDER BY o.k, q.payload;
