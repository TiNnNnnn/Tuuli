SELECT l.k, r.k
FROM (SELECT k FROM dsl_eq_left WHERE 10 / (2 - k) > 0 OFFSET 0) l
LEFT JOIN dsl_eq_right r ON l.k = r.k
ORDER BY l.k, r.k;
