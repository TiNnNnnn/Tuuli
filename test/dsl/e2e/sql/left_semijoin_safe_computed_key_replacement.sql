SELECT l.k
FROM dsl_eq_left AS l
WHERE l.k IN (SELECT COALESCE(r.a, 3) FROM dsl_nullable_unique AS r)
ORDER BY 1;
