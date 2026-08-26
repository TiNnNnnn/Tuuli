SELECT l.k, r.k, o.id
FROM dsl_eq_left AS l
FULL JOIN dsl_eq_right AS r ON l.k = r.k
JOIN dsl_insub_outer AS o ON o.id = COALESCE(l.k, r.k)
ORDER BY COALESCE(l.k, r.k), l.k NULLS LAST, r.k NULLS LAST, o.id;
