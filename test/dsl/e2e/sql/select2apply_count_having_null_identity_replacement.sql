SELECT o.id
FROM dsl_insub_outer AS o
WHERE (
  SELECT count(*)
  FROM dsl_correlated_exists AS i
  HAVING o.id = 1) IS NULL
ORDER BY o.id;
