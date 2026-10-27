SELECT o.g
FROM dsl_agg_outer AS o
WHERE EXISTS (
  SELECT min(i.x) FROM dsl_exists_inner AS i
  WHERE i.x = o.v
  HAVING min(i.x) IS NULL)
ORDER BY o.g;
