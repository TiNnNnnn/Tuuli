SELECT i FROM correlation_outer
WHERE EXISTS (SELECT 1 FROM correlation_inner WHERE j > 0)
ORDER BY i NULLS FIRST;
