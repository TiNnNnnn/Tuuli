SELECT o.k FROM dsl_eq_left o WHERE EXISTS (SELECT DISTINCT i.k FROM dsl_eq_right i) ORDER BY o.k;
