SELECT DISTINCT c.parent_id, p.payload
FROM dsl_fk_child AS c
INNER JOIN dsl_fk_parent AS p ON c.parent_id = p.id
ORDER BY c.parent_id, p.payload;
