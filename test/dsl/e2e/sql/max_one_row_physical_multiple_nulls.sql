SELECT o.case_id,(SELECT i.y FROM dsl_notin_inner i WHERE i.y IS NULL UNION ALL SELECT j.y FROM dsl_notin_inner j WHERE j.y IS NULL) FROM dsl_notin_outer o ORDER BY o.case_id;
