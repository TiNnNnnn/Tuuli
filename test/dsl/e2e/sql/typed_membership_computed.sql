SELECT l.k * 10
FROM dsl_eq_left AS l
WHERE l.k IN (SELECT coalesce(r.a, 3) FROM dsl_eq_pair_right AS r)
ORDER BY 1;
