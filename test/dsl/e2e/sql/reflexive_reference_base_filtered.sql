SELECT l.k, r.k
FROM dsl_eq_left AS l
LEFT JOIN (SELECT k FROM dsl_eq_left WHERE k > 2) AS r ON r.k = l.k
ORDER BY l.k, r.k;
