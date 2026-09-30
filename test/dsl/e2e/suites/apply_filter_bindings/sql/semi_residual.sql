SELECT o.payload FROM dsl_correlated_exists o
WHERE EXISTS (
  SELECT 1 FROM (
    SELECT i.k, i.payload FROM dsl_correlated_exists i
    WHERE i.k = o.k ORDER BY i.payload LIMIT 1
  ) limited WHERE limited.payload > o.k
)
ORDER BY o.payload;
