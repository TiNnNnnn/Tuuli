SELECT q.id, q.inner_value, q.v > 15 AS independent_value
FROM (
    SELECT id, v, v > 5 AS inner_value
    FROM dsl_insub_outer
) AS q
ORDER BY q.id;
