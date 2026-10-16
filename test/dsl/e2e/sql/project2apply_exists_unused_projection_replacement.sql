SELECT o.id,
       EXISTS (
         SELECT 1 / (i.payload - i.payload)
         FROM dsl_correlated_exists AS i
         WHERE i.k = o.id) AS has_match,
       NOT EXISTS (
         SELECT 1 / (i.payload - i.payload)
         FROM dsl_correlated_exists AS i
         WHERE i.k = o.id) AS no_match
FROM dsl_insub_outer AS o
ORDER BY o.id;
