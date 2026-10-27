SELECT p.id
FROM dsl_fk_parent AS p
WHERE EXISTS (
  SELECT 1
  FROM dsl_fk_child AS b
  WHERE EXISTS (
    SELECT 1
    FROM (
      SELECT c.id / c.parent_id AS computed_id, c.parent_id
      FROM dsl_fk_child AS c) AS q
    WHERE q.computed_id = b.id
      AND q.parent_id = p.id))
ORDER BY p.id;
