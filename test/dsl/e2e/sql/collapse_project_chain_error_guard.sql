SELECT q.id, q.inner_value, q.dependent_value, q.independent_value,
       q.v + 1000 AS outer_value
FROM (
    SELECT p.id, p.v, p.inner_value,
           p.inner_value + 1 AS dependent_value,
           p.v + 100 AS independent_value
    FROM (
        SELECT id, v, v + 1 AS inner_value
        FROM dsl_insub_outer
    ) AS p
) AS q
ORDER BY q.id;
