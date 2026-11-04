SELECT l.k, r.k
FROM dsl_eq_left l FULL JOIN dsl_eq_right r ON l.k = r.k
WHERE r.k >= 2
ORDER BY l.k, r.k;
