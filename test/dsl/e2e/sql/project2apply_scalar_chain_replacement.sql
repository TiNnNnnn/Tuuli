SELECT o.id,
       (SELECT i.payload FROM dsl_correlated_exists AS i WHERE i.k = o.id) AS first_value,
       (SELECT j.payload FROM dsl_correlated_exists AS j WHERE j.k = o.id + 1) AS second_value
FROM dsl_insub_outer AS o
ORDER BY o.id;
