SELECT c.id
FROM dsl_fk_parent AS p
INNER JOIN (
    SELECT * FROM dsl_fk_child WHERE id > 0
) AS c ON p.id = c.parent_id
ORDER BY c.id;
