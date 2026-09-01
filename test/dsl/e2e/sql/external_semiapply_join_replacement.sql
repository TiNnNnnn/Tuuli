SELECT p.id
FROM dsl_fk_parent AS p
WHERE EXISTS (
  SELECT 1
  FROM dsl_fk_child AS b
  WHERE EXISTS (
    SELECT 1
    FROM dsl_fk_child AS c
    JOIN dsl_notin_tag AS tag ON tag.id = c.id
    WHERE tag.id = b.id
      AND c.parent_id = p.id))
ORDER BY p.id;
