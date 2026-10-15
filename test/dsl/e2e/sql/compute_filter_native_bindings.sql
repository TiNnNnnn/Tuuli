SELECT id, v > 20 AS large_value
FROM dsl_insub_outer
WHERE v > 10
ORDER BY id;
