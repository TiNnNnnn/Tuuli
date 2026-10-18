SELECT p.id
FROM dsl_fk_parent AS p
WHERE EXISTS (
  SELECT 1
  FROM (
    SELECT c.parent_id
    FROM dsl_fk_child AS c
    WHERE c.id * 2147483647 > 0
    LIMIT 10
  ) AS c
  WHERE c.parent_id = p.id)
ORDER BY p.id;
