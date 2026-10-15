SELECT DISTINCT l.b
FROM dsl_eq_pair_left AS l
JOIN dsl_insub_inner AS r ON l.a = r.id
ORDER BY l.b;
