SELECT q.id, q.inner_value, q.v + 100 AS independent_value
FROM (
    SELECT id, v, v + 1 AS inner_value
    FROM dsl_insub_outer
) AS q
ORDER BY q.id;
