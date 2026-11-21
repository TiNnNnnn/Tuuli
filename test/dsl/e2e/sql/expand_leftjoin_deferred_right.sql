SELECT l.k, r.k
FROM dsl_eq_left l LEFT JOIN
    (SELECT k FROM dsl_eq_right WHERE 10 / (2 - k) > 0 OFFSET 0) r
ON l.k = r.k
ORDER BY l.k, r.k;
