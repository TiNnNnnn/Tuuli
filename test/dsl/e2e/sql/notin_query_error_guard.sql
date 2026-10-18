SELECT case_id
FROM dsl_notin_outer
WHERE case_id = 1 AND x NOT IN (
    SELECT y / (y - 3)
    FROM dsl_notin_inner
    WHERE set_id = 4);
