SELECT bool_and(min_flag = max_flag) AS same_value_per_input
FROM (
    SELECT id, min(flag::int) AS min_flag, max(flag::int) AS max_flag
    FROM (
        SELECT q.id, q.flag, generate_series(1, 2) AS expanded
        FROM (
            SELECT id, id #=# v AS flag
            FROM dsl_insub_outer
        ) AS q
    ) AS expanded
    GROUP BY id
) AS grouped;
