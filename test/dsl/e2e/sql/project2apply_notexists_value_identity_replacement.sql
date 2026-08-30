SELECT o.id,
       NOT EXISTS (
         SELECT 1
         FROM dsl_correlated_exists AS i
         WHERE i.k = o.id) AS has_no_match
FROM dsl_insub_outer AS o
ORDER BY o.id;
