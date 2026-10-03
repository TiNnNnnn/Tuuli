SELECT l.k
FROM dsl_eq_left AS l
WHERE EXISTS (
    SELECT 1
    FROM (SELECT DISTINCT coalesce(r.a, 3) AS a FROM dsl_eq_pair_right AS r) AS d
    WHERE d.a = l.k)
ORDER BY 1;
