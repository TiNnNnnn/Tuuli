SELECT DISTINCT c.parent_id
FROM dsl_fk_child AS c
JOIN dsl_fk_parent AS p
  ON c.parent_id = p.id AND c.id < p.payload
ORDER BY c.parent_id;
