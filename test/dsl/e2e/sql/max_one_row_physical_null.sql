SELECT o.case_id,(SELECT i.y FROM dsl_notin_inner i WHERE i.set_id=3 AND i.y IS NULL) FROM dsl_notin_outer o ORDER BY o.case_id;
