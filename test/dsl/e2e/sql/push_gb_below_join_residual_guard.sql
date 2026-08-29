SELECT DISTINCT c.parent_id
FROM dsl_fk_nullable_child AS c
INNER JOIN dsl_fk_parent AS p
  ON c.parent_id = p.id AND c.id + p.id > 0
ORDER BY c.parent_id;
