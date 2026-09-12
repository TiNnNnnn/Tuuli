SELECT o.k, (SELECT i.k FROM dsl_eq_right i) FROM dsl_eq_left o;
