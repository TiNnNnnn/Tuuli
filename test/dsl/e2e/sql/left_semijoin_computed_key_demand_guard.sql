SELECT l.k
FROM dsl_eq_left AS l
WHERE l.k IN (SELECT r.k * 2 FROM dsl_eq_right AS r)
ORDER BY 1;
