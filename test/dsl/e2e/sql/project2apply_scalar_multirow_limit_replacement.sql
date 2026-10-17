SELECT o.id, (SELECT i.payload FROM dsl_correlated_exists AS i)
FROM dsl_insub_outer AS o
LIMIT 1;
