SELECT sum(CASE WHEN q.token + q.token = 2 * q.token THEN 1 ELSE 0 END) AS reused_value
FROM (
    SELECT (
        SELECT nextval('dsl_compute_sequence')
        FROM dsl_insub_inner AS i
        WHERE i.id = o.id
    ) AS token
    FROM dsl_insub_outer AS o
) AS q;
