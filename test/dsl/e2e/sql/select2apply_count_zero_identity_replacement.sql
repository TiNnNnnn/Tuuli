SELECT o.id
FROM dsl_insub_outer AS o
WHERE (
  SELECT count(*)
  FROM dsl_correlated_exists AS i
  WHERE i.k = o.id
    AND i.payload > 20) = 0
ORDER BY o.id;
