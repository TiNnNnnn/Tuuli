SELECT c.parent_id + 0
FROM dsl_fk_nullable_child AS c
WHERE c.parent_id IN (SELECT p.id FROM dsl_fk_parent AS p)
ORDER BY 1;
