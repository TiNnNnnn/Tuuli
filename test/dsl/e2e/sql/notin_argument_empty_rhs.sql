SELECT o.case_id
FROM dsl_notin_outer AS o
WHERE o.case_id = 2
  AND o.x / (o.set_id - o.x) NOT IN (
      SELECT i.y FROM dsl_notin_inner AS i WHERE i.set_id = 1);
