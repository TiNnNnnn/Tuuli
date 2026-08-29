SELECT DISTINCT c.id, c.parent_id
FROM dsl_fk_nullable_child AS c
INNER JOIN dsl_fk_parent AS p ON c.parent_id = p.id
ORDER BY c.id, c.parent_id;
