SELECT c.id, c.k1, c.k2
FROM dsl_composite_fk_child AS c
WHERE EXISTS (
  SELECT 1
  FROM dsl_composite_fk_parent AS p
  WHERE c.k1 = p.k1
    AND c.k2 = p.k2
    AND c.id + p.payload > 0)
ORDER BY c.id, c.k1, c.k2;
