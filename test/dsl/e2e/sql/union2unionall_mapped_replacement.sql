SELECT k, v, n
FROM (
    SELECT id AS k, v, NULL::integer AS n FROM dsl_insub_outer WHERE id <= 2
    UNION
    SELECT id, v, NULL::integer FROM dsl_insub_outer WHERE id >= 2
    UNION
    SELECT 1, id, id FROM dsl_insub_outer WHERE id = 1
) AS union_rows
ORDER BY k, v, n NULLS FIRST;
