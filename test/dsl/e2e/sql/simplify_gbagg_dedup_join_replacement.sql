SELECT DISTINCT p.id
FROM dsl_fk_parent AS p
JOIN (SELECT DISTINCT parent_id FROM dsl_fk_child) AS c
  ON p.id = c.parent_id
ORDER BY p.id;
