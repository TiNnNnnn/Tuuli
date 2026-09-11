SELECT o.k FROM dsl_eq_left o WHERE NOT EXISTS (SELECT 1 FROM dsl_eq_right i WHERE i.k > 1000) ORDER BY o.k;
