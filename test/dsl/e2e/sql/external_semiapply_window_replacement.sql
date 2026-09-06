SELECT p.id
FROM dsl_fk_parent AS p
WHERE EXISTS (
  SELECT 1
  FROM dsl_fk_child AS b
  WHERE EXISTS (
    SELECT 1
    FROM (
      SELECT c.id, c.parent_id,
             max(c.id) OVER (PARTITION BY c.id, c.parent_id) AS max_id
      FROM dsl_fk_child AS c
      WHERE c.id = b.id
        AND c.parent_id = p.id) AS q
    WHERE q.max_id > 0))
ORDER BY p.id;
