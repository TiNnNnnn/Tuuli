SELECT c.id, p.id
FROM dsl_fk_nullable_child AS c
LEFT JOIN dsl_fk_parent AS p ON c.parent_id = p.id
ORDER BY c.id;

