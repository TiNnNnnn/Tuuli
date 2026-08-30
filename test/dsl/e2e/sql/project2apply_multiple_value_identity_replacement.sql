SELECT o.id,
       (SELECT max(i.payload)
        FROM dsl_correlated_exists AS i) AS max_payload,
       EXISTS (
         SELECT 1
         FROM dsl_correlated_exists AS i
         WHERE i.k = o.id) AS has_match
FROM dsl_insub_outer AS o
ORDER BY o.id;
