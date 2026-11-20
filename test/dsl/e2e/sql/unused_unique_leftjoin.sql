SELECT p.id
FROM dsl_fk_parent AS p
LEFT JOIN (
    SELECT parent_id
    FROM dsl_fk_child
    WHERE parent_id = 10
    GROUP BY parent_id
) AS c ON p.id = c.parent_id;

