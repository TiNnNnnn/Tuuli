SELECT o.id
FROM dsl_insub_outer AS o
WHERE (SELECT i.payload FROM dsl_correlated_exists AS i WHERE i.k = o.id) IS NULL
ORDER BY o.id;
