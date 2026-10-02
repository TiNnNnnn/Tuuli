SELECT q.id, q.inner_a, q.inner_b,
       q.v > 15 AS outer_a, q.v IS NULL AS outer_b
FROM (
    SELECT id, v, v > 5 AS inner_a, v > 25 AS inner_b
    FROM dsl_insub_outer
) AS q
ORDER BY q.id;
