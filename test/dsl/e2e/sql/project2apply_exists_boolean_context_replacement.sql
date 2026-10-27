SELECT o.id,
       (o.v > 15) AND EXISTS (
         SELECT 1
         FROM dsl_correlated_exists AS i
         WHERE i.k = o.id AND i.payload > o.v) AS has_match
FROM dsl_insub_outer AS o
ORDER BY o.id;
