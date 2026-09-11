SELECT o.case_id,NOT EXISTS (SELECT 1 FROM dsl_notin_inner i WHERE i.set_id=o.set_id AND i.y=o.x) FROM dsl_notin_outer o ORDER BY o.case_id;
