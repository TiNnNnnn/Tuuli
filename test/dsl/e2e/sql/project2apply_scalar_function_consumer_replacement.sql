SELECT o.id, int4div((SELECT i.payload FROM dsl_correlated_exists AS i WHERE i.k >= o.id), 0) AS subquery_value
FROM dsl_insub_outer AS o ORDER BY o.id;
