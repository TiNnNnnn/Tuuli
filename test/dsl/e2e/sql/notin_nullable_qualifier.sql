SELECT o.case_id
FROM dsl_notin_outer AS o
WHERE o.set_id NOT IN (
    SELECT i.y FROM dsl_notin_inner AS i WHERE i.set_id = o.x)
ORDER BY o.case_id;
