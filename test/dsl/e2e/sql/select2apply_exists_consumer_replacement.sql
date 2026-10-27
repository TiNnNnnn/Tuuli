SELECT o.id
FROM dsl_insub_outer AS o
WHERE EXISTS (
  SELECT i.k FROM dsl_correlated_exists AS i
  WHERE i.k = o.id AND i.payload > o.v)
ORDER BY o.id;
