SELECT o.id,
       (SELECT i.payload
        FROM dsl_correlated_exists AS i
        WHERE i.k = o.id) AS subquery_value
FROM dsl_insub_outer AS o
ORDER BY o.id;
