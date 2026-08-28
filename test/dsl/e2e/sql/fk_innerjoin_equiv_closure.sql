SELECT c.id
FROM dsl_fk_child AS c
INNER JOIN dsl_fk_parent AS p0 ON c.parent_id = p0.id
INNER JOIN dsl_fk_parent AS p1 ON c.parent_id = p1.id;
