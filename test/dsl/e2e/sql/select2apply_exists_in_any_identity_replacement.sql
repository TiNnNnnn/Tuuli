SELECT o.id
FROM dsl_insub_outer AS o
WHERE EXISTS (
  SELECT 1
  FROM dsl_correlated_exists AS e
  WHERE e.payload = 10) = ANY (
    SELECT i.payload > 20
    FROM dsl_correlated_exists AS i
    WHERE i.k = o.id)
ORDER BY o.id;
