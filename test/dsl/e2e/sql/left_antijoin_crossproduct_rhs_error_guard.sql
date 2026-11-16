SELECT l.a, l.b
FROM dsl_eq_empty_pair_left l
WHERE NOT EXISTS (
    SELECT 1 FROM dsl_eq_right r
    WHERE 10 / (2 - r.k) > 0 AND l.a > 1)
ORDER BY l.a, l.b;
