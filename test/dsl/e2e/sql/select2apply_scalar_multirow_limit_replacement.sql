SELECT o.id
FROM dsl_insub_outer AS o
WHERE o.v = (SELECT i.payload FROM dsl_correlated_exists AS i)
LIMIT 1;
