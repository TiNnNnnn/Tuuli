SELECT case_id
FROM dsl_notin_outer AS o
WHERE x NOT IN (
  SELECT y
  FROM dsl_notin_inner AS i
  WHERE i.set_id = o.set_id)
ORDER BY case_id;
