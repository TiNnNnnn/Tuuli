SELECT l.k * 10 + s.k
FROM dsl_eq_left AS l
JOIN dsl_eq_right AS s ON l.k = s.k
WHERE l.k IN (SELECT coalesce(r.a, 3) FROM dsl_eq_pair_right AS r)
ORDER BY 1;
