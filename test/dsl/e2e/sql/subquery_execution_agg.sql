SELECT sum((SELECT max(i.k) FROM dsl_eq_right i WHERE i.k<=o.k)) FROM dsl_eq_left o;
