SELECT o.id,
       o.v < ALL (
         SELECT i.payload
         FROM dsl_correlated_exists AS i
         WHERE i.k = o.id) AS all_match
FROM dsl_insub_outer AS o
ORDER BY o.id;
