SELECT o.id,
       o.id < 0 AND o.v = (SELECT i.payload FROM dsl_correlated_exists AS i) AS value
FROM dsl_insub_outer AS o
ORDER BY o.id;
