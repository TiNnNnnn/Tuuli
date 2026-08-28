SELECT q.id, q.inner_value, generate_series(1, 2) AS expanded_value
FROM (
    SELECT id, v + 1 AS inner_value
    FROM dsl_insub_outer
) AS q
ORDER BY q.id, expanded_value;
