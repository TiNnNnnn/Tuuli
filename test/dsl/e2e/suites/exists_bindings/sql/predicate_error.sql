SELECT i FROM exists_outer
WHERE EXISTS (SELECT 1 FROM exists_inner WHERE 1 / (j - j) > 0)
ORDER BY i NULLS FIRST;
