SELECT o.id
FROM dsl_insub_outer AS o
WHERE (o.id #=# 0) IS NULL
  AND o.v = (
    SELECT i.payload FROM dsl_correlated_exists AS i
    WHERE i.k = o.id OR i.k IS NULL)
ORDER BY o.id;
