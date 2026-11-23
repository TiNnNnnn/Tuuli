SELECT p.b
FROM dsl_bag_pair_right AS p
WHERE EXISTS (
  SELECT 1
  FROM dsl_fk_child AS b
  WHERE EXISTS (
    SELECT max(c.b)
    FROM dsl_bag_pair_left AS c
    WHERE c.a = b.id AND c.b = p.b
    GROUP BY c.a
    HAVING max(c.b) = 2))
ORDER BY p.b;
