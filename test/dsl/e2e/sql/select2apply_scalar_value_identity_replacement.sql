SELECT o.id
FROM dsl_insub_outer AS o
WHERE o.v = (
  SELECT i.payload
  FROM dsl_correlated_exists AS i
  WHERE i.k = o.id)
ORDER BY o.id;
