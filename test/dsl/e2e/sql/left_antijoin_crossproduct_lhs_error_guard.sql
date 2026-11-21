SELECT l.a
FROM (SELECT a FROM dsl_eq_pair_left WHERE 10 / (2 - a) > 0 OFFSET 0) l
WHERE NOT EXISTS (SELECT 1 FROM dsl_eq_right r WHERE l.a > 1)
ORDER BY l.a;
