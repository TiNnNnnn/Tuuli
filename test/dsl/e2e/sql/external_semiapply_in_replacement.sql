SELECT p.id
FROM dsl_fk_parent AS p
WHERE EXISTS (
  SELECT 1
  FROM dsl_fk_child AS b
  WHERE b.id IN (
    SELECT c.id
    FROM dsl_fk_child AS c
    WHERE c.parent_id = p.id))
ORDER BY p.id;
