SELECT l.a * 10 + l.b
FROM dsl_eq_pair_left AS l
WHERE EXISTS (
    SELECT 1
    FROM dsl_eq_pair_right AS r
    WHERE r.a = l.a AND r.b = l.b)
ORDER BY 1;
