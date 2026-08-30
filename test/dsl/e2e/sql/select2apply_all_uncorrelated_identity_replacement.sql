SELECT o.id
FROM dsl_insub_outer AS o
WHERE o.v > ALL (
  SELECT i.payload
  FROM dsl_correlated_exists AS i
  WHERE i.k = 1)
ORDER BY o.id;
