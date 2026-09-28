SELECT i FROM apply_outer WHERE EXISTS
  (SELECT * FROM apply_inner WHERE 1 / (j - j) > 0)
ORDER BY i NULLS FIRST;
