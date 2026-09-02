SELECT o.id
FROM dsl_insub_outer AS o
WHERE (
  SELECT max(i2.payload)
  FROM dsl_correlated_exists AS i2) < ALL (
    SELECT i.payload
    FROM dsl_correlated_exists AS i
    WHERE i.k = o.id)
ORDER BY o.id;
