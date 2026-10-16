SELECT q.id, q.inner_a, q.inner_b, 1 / (q.v - q.v) AS upper_value
FROM (
    SELECT id, v, v > 5 AS inner_a, v > 25 AS inner_b
    FROM dsl_insub_outer
) AS q
ORDER BY q.id;
