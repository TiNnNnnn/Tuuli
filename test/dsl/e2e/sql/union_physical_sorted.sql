SELECT COALESCE(a::text, 'NULL') FROM (SELECT a::bit(32) AS a FROM dsl_eq_pair_left UNION SELECT a::bit(32) FROM dsl_eq_pair_right) u ORDER BY 1;
