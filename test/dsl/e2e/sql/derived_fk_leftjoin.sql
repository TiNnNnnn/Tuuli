SELECT c.parent_id, p.id
FROM (
    SELECT parent_id
    FROM dsl_fk_child
    WHERE id > 0
    GROUP BY parent_id
) AS c
LEFT JOIN dsl_fk_parent AS p ON c.parent_id = p.id
ORDER BY c.parent_id;

