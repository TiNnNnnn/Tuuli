SELECT q.id, q.expanded, 1 / (q.v - q.v) AS reachable_error
FROM (
    SELECT id, v, generate_series(1, v - v + 1) AS expanded
    FROM dsl_insub_outer
) AS q
ORDER BY q.id;
