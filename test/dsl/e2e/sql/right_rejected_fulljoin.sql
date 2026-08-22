SELECT c.id, p.id
FROM dsl_fk_parent AS p
FULL JOIN dsl_fk_child AS c ON p.id = c.parent_id
WHERE c.id > 0
ORDER BY c.id;

