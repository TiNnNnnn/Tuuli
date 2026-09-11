SELECT o.case_id,q.y FROM dsl_notin_outer o CROSS JOIN LATERAL (SELECT i.y FROM dsl_notin_inner i WHERE i.set_id=o.set_id ORDER BY i.y LIMIT 1 OFFSET 1) q ORDER BY o.case_id;
