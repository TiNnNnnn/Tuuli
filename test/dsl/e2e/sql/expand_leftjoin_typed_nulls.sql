SELECT l.k, l.label, l.amount, l.flag, r.k, r.label, r.amount, r.flag
FROM dsl_typed_join_left AS l
LEFT OUTER JOIN dsl_typed_join_right AS r ON l.k = r.k
ORDER BY l.k NULLS LAST, r.k NULLS LAST, l.label NULLS LAST, r.label NULLS LAST;
