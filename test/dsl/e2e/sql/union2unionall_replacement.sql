SELECT id
FROM (
    SELECT id FROM dsl_insub_outer WHERE id = 1
    UNION
    SELECT id FROM dsl_insub_outer WHERE id = 2
    UNION
    SELECT id FROM dsl_insub_outer WHERE id = 3
) AS union_rows
ORDER BY id;
