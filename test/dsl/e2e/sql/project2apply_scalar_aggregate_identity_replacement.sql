SELECT o.id,
       (SELECT max(i.payload)
        FROM dsl_correlated_exists AS i) AS max_payload
FROM dsl_insub_outer AS o
ORDER BY o.id;
