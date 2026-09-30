SELECT o.k, o.payload, q.payload
FROM dsl_correlated_exists AS o
LEFT JOIN LATERAL (
    SELECT i.payload
    FROM dsl_correlated_exists AS i
    WHERE i.k = o.k) AS q ON true
ORDER BY o.payload, q.payload;
