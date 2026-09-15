SELECT count(*) AS expanded_rows, count(DISTINCT token) AS tokens
FROM (
    SELECT q.token, generate_series(1,2) AS expanded
    FROM (SELECT source_stable::text AS token FROM dsl_io_values) AS q
) AS expanded;
