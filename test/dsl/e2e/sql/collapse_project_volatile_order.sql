SELECT bool_and(outer_token > inner_token) AS lower_evaluated_first
FROM (
    SELECT q.inner_token, nextval('dsl_compute_sequence') AS outer_token
    FROM (
        SELECT nextval('dsl_compute_sequence') AS inner_token
        FROM dsl_insub_outer
    ) AS q
) AS evaluations;
