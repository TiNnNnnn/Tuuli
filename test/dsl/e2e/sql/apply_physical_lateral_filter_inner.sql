SELECT o.case_id,q.y,q.set_id FROM dsl_notin_outer o CROSS JOIN LATERAL (SELECT i.y,i.set_id FROM dsl_notin_inner i WHERE i.set_id=o.set_id AND i.y>=o.x) q ORDER BY o.case_id,q.y;
