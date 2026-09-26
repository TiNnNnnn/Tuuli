SELECT i FROM exists_outer
WHERE EXISTS (SELECT 1 / (j - j) FROM exists_inner WHERE j > 0)
ORDER BY i NULLS FIRST;
