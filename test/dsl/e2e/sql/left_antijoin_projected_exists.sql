SELECT l.a, l.b
FROM (
    SELECT orig.a, orig.b, EXISTS (
        SELECT 1 FROM dsl_eq_right AS witness
        WHERE witness.k = orig.b AND witness.k = 2
    ) AS hit
    FROM dsl_eq_pair_left AS orig
    OFFSET 0
) AS l
WHERE NOT EXISTS (
    SELECT 1 FROM dsl_eq_right AS r WHERE l.a > 1 OR l.hit
)
ORDER BY l.a NULLS LAST, l.b NULLS LAST;
