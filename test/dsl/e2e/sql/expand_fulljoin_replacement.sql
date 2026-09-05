SELECT l.k AS left_k, r.k AS right_k
FROM dsl_eq_left AS l
FULL OUTER JOIN dsl_eq_right AS r ON l.k = r.k
ORDER BY left_k NULLS LAST, right_k NULLS LAST;
