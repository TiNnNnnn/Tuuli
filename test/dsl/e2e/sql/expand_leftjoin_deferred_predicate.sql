SELECT l.k, r.k
FROM dsl_eq_left l LEFT JOIN dsl_eq_right r ON 10 / (2 - l.k) < r.k
LIMIT 1;
