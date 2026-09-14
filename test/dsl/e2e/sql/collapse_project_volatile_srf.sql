SELECT count(*) AS expanded_rows, count(DISTINCT token) AS evaluations
FROM (
    SELECT q.token, generate_series(1, 2) AS expanded
    FROM (
        SELECT nextval('dsl_compute_sequence') AS token
        FROM dsl_insub_outer
    ) AS q
) AS expanded;
