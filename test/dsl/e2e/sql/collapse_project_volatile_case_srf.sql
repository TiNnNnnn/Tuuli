SELECT count(*) AS expanded_rows,
       count(copied_token) AS nonnull_tokens,
       count(DISTINCT copied_token) AS evaluations,
       count(short_series) AS short_rows,
       count(long_series) AS long_rows
FROM (
    SELECT q.token + 0 AS copied_token,
           generate_series(1, 2) AS short_series,
           generate_series(1, 3) AS long_series
    FROM (
        SELECT CASE WHEN id = 2 THEN NULL
                    WHEN v > 0 THEN nextval('dsl_compute_sequence')
                    ELSE (1 / (v - v))::bigint END AS token
        FROM dsl_insub_outer
    ) AS q
) AS expanded;
