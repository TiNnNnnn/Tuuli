SELECT p.id
FROM dsl_fk_parent AS p
WHERE EXISTS (
  SELECT 1
  FROM dsl_fk_child AS c
  WHERE c.parent_id = p.id
    AND c.id + p.id > 0)
ORDER BY p.id;
