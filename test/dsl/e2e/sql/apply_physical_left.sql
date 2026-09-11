SELECT o.k, (SELECT max(i.k) FROM dsl_eq_right i) FROM dsl_eq_left o ORDER BY o.k;
