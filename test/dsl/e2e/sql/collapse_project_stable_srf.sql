SELECT count(*) AS expanded_rows, count(DISTINCT token) AS distinct_values
FROM (
    SELECT q.token, generate_series(1, 2) AS expanded
    FROM (
        SELECT pg_backend_pid() AS token
        FROM dsl_insub_outer
    ) AS q
) AS expanded;
