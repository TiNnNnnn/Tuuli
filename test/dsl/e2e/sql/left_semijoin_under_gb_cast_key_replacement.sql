SELECT p.id
FROM dsl_fk_parent AS p
WHERE EXISTS (
  SELECT 1
  FROM dsl_fk_child AS c
  WHERE c.parent_id::oid = p.id::oid)
ORDER BY p.id;
