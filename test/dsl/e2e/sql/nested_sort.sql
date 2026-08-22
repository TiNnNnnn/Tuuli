SELECT id, v
FROM (
    SELECT id, v
    FROM dsl_insub_outer
    ORDER BY v
) AS ordered_inner
ORDER BY id;

