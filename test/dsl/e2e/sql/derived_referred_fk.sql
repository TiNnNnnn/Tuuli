SELECT c.id
FROM dsl_fk_child AS c
INNER JOIN (
    SELECT parent_id
    FROM dsl_fk_child
    GROUP BY parent_id
) AS covered_keys ON c.parent_id = covered_keys.parent_id;

