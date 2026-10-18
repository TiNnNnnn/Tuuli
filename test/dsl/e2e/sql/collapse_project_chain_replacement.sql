SELECT q.id, q.inner_value, q.middle_value, q.outer_value,
       q.v IS NULL AS last_value
FROM (
    SELECT p.id, p.v, p.inner_value,
           p.v > 15 AS middle_value,
           p.v > 25 AS outer_value
    FROM (
        SELECT id, v, v > 5 AS inner_value
        FROM dsl_insub_outer
    ) AS p
) AS q
ORDER BY q.id;
