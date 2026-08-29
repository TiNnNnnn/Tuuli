SELECT c.id, c.parent_id
FROM dsl_fk_nullable_child AS c
WHERE EXISTS (
  SELECT 1
  FROM dsl_fk_parent AS p
  WHERE c.parent_id = p.id
    AND c.id + p.id > 0)
ORDER BY c.id, c.parent_id;
