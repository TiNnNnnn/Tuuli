SELECT p.id
FROM dsl_fk_parent AS p
WHERE EXISTS (
  SELECT 1
  FROM dsl_fk_child AS b
  WHERE EXISTS (
    SELECT max(c.id)
    FROM dsl_fk_child AS c
    WHERE c.id = b.id
      AND c.parent_id = p.id
    GROUP BY c.id, c.parent_id
    HAVING max(c.id) > 0))
ORDER BY p.id;
