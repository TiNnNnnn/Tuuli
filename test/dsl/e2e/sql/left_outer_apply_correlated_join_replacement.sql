SELECT o.k, o.payload, q.payload
FROM dsl_correlated_exists AS o
LEFT JOIN LATERAL (
    SELECT i.payload
    FROM dsl_correlated_exists AS i
    JOIN dsl_eq_right AS r ON r.k = i.k
    WHERE i.k = o.k) AS q ON true
ORDER BY o.payload, q.payload;
