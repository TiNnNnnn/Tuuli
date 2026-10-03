SELECT l.a * 10
FROM dsl_eq_pair_left AS l
WHERE l.b <> 1
  AND EXISTS (
      SELECT r.b FROM dsl_eq_pair_right AS r
      WHERE r.a = l.a AND r.b <> 2)
ORDER BY 1;
