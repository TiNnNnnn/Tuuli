SELECT q.id, q.inner_value, generate_series(10, 11) AS outer_value
FROM (
    SELECT id, generate_series(1, 2) AS inner_value
    FROM dsl_insub_outer
) AS q
ORDER BY q.id, q.inner_value, outer_value;
