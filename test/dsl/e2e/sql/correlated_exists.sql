SELECT payload
FROM dsl_correlated_exists o
WHERE EXISTS (
    SELECT 1
    FROM dsl_correlated_exists i
    WHERE i.k = o.k)
ORDER BY payload;

