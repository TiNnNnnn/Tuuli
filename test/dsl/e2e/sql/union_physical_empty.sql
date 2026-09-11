SELECT count(*) FROM (SELECT FROM dsl_eq_pair_left WHERE a < 0 UNION SELECT FROM dsl_eq_pair_right WHERE a < 0) u;
