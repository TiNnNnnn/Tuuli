SELECT l.k, r.k
FROM dsl_eq_left AS l
FULL JOIN dsl_eq_right AS r ON l.k = r.k
WHERE l.k = 3 OR r.k = 4
ORDER BY l.k, r.k;
