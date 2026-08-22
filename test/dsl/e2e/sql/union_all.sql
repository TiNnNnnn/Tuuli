SELECT id
FROM (
    SELECT id FROM dsl_insub_outer WHERE id <= 2
    UNION ALL
    SELECT id FROM dsl_insub_outer WHERE id >= 2
) AS union_rows
ORDER BY id;

