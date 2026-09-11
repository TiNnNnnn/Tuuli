SELECT o.k FROM dsl_eq_left o WHERE o.k IN (SELECT i.k FROM dsl_eq_right i) ORDER BY o.k;
