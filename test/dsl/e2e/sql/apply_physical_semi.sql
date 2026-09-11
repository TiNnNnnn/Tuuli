SELECT o.k FROM dsl_eq_left o WHERE EXISTS (SELECT 1 FROM dsl_eq_right i WHERE i.k > 0) ORDER BY o.k;
