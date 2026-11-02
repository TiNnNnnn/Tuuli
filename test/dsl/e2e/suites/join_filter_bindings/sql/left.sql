SELECT a.i, b.i
FROM binding_left a LEFT JOIN (SELECT i FROM binding_right WHERE i > 2) b
  ON a.i < b.i
ORDER BY a.i NULLS FIRST, b.i NULLS FIRST;
