SELECT o.case_id
FROM dsl_notin_outer AS o
WHERE true = ANY (
  SELECT i.y = 1 FROM dsl_notin_inner AS i
  WHERE i.set_id = o.set_id)
ORDER BY o.case_id;
