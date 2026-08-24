SELECT case_id
FROM dsl_notin_outer o
WHERE x NOT IN (
    SELECT y
    FROM dsl_notin_inner i
    WHERE i.set_id = o.set_id
)
ORDER BY case_id;
