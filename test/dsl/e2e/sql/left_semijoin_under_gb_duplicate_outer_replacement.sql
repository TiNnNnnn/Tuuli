SELECT l.k
FROM dsl_eq_left AS l
WHERE EXISTS (
  SELECT 1
  FROM dsl_eq_right AS r
  WHERE r.k = l.k AND r.k >= l.k)
ORDER BY l.k;
