SELECT shape, id
FROM (
  SELECT 'compute' AS shape, p.id
  FROM dsl_fk_parent AS p
  WHERE EXISTS (
    SELECT 1
    FROM dsl_fk_child AS b
    WHERE b.id IN (
      SELECT q.computed_id
      FROM (
        SELECT c.id + 0 AS computed_id, c.parent_id
        FROM dsl_fk_child AS c) AS q
      WHERE q.parent_id = p.id))
  UNION ALL
  SELECT 'join' AS shape, p.id
  FROM dsl_fk_parent AS p
  WHERE EXISTS (
    SELECT 1
    FROM dsl_fk_child AS b
    WHERE b.id IN (
      SELECT c.id
      FROM dsl_fk_child AS c
      JOIN dsl_notin_tag AS tag ON tag.id = c.id
      WHERE c.parent_id = p.id))
) AS cases
ORDER BY shape, id;
