SELECT o.case_id FROM dsl_notin_outer o WHERE o.x NOT IN (SELECT i.y FROM dsl_notin_inner i WHERE i.set_id=o.set_id) ORDER BY o.case_id;
