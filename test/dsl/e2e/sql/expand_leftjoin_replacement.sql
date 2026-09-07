SELECT l.k AS left_k, r.k AS right_k, s.k AS second_k
FROM dsl_loj_outer AS l
LEFT JOIN dsl_loj_inner AS r ON l.k = r.k
LEFT JOIN dsl_loj_inner AS s ON l.k = s.k
ORDER BY left_k, right_k NULLS LAST, second_k NULLS LAST;
