SELECT count(*) FROM (SELECT FROM dsl_eq_pair_left UNION SELECT FROM dsl_eq_pair_right) u;
