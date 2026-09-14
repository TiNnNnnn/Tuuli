SELECT count(*) AS expanded_rows, count(DISTINCT copied_token) AS evaluations
FROM (
    SELECT q.token + 0 AS copied_token, generate_series(1, 2) AS expanded
    FROM (
        SELECT nextval('dsl_compute_sequence') AS token
        FROM dsl_insub_outer
    ) AS q
) AS expanded;
