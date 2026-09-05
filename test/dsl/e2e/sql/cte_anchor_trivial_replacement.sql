WITH c AS MATERIALIZED (
    SELECT id, v FROM dsl_insub_outer
)
SELECT id, v
FROM c
WHERE v >= 10
ORDER BY id;
